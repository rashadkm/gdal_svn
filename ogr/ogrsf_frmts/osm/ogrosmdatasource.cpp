/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements OGROSMDataSource class.
 * Author:   Even Rouault, <even dot rouault at mines dash paris dot org>
 *
 ******************************************************************************
 * Copyright (c) 2012, Even Rouault
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "ogr_osm.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "ogr_api.h"
#include "swq.h"
#include "gpb.h"

#define LIMIT_IDS_PER_REQUEST 200

#define MAX_NODES_PER_WAY 2000

#define IDX_LYR_POINTS           0
#define IDX_LYR_LINES            1
#define IDX_LYR_POLYGONS         2
#define IDX_LYR_MULTILINESTRINGS 3
#define IDX_LYR_MULTIPOLYGONS    4
#define IDX_LYR_OTHER_RELATIONS  5

#define DBL_TO_INT(x)            (int)floor((x) * 1e7 + 0.5)
#define INT_TO_DBL(x)            ((x) / 1e7)

#define MAX_COUNT_FOR_TAGS_IN_WAY   255  /* must fit on 1 byte */
#define MAX_SIZE_FOR_TAGS_IN_WAY    1024

/* 5 bytes for encoding a int : really the worst case scenario ! */
#define WAY_BUFFER_SIZE (1 + MAX_NODES_PER_WAY * 2 * 5 + MAX_SIZE_FOR_TAGS_IN_WAY)

/* 65536 * 65536 so we can adress node ids between 0 and 4.2 billion */
#define BUCKET_SIZE         65536
#define BUCKET_COUNT        65536

/* Minimum size of data written on disk */
#define SECTOR_SIZE         512
/* Which represents, 64 nodes */
/* #define NODE_PER_SECTOR     SECTOR_SIZE / (2 * 4) */
#define NODE_PER_SECTOR     64
#define NODE_PER_SECTOR_SHIFT   6

/* Per bucket, we keep track of the absence/presence of sectors */
/* only, to reduce memory usage */
/* #define BUCKET_BITMAP_SIZE  BUCKET_SIZE / (8 * NODE_PER_SECTOR) */
#define BUCKET_BITMAP_SIZE  128

/* Max number of features that are accumulated in pasWayFeaturePairs */
#define MAX_DELAYED_FEATURES        75000
/* Max number of tags that are accumulated in pasAccumulatedTags */
#define MAX_ACCUMULATED_TAGS        MAX_DELAYED_FEATURES * 5
/* Max size of the string with tag values that are accumulated in pabyNonRedundantValues */
#define MAX_NON_REDUNDANT_VALUES    MAX_DELAYED_FEATURES * 10
/* Max number of features that are accumulated in panUnsortedReqIds */
#define MAX_ACCUMULATED_NODES       1000000

//#define FAKE_LOOKUP_NODES

//#define DEBUG_MEM_USAGE
#ifdef DEBUG_MEM_USAGE
size_t GetMaxTotalAllocs();
#endif

CPL_CVSID("$Id$");

/************************************************************************/
/*                        OGROSMDataSource()                            */
/************************************************************************/

OGROSMDataSource::OGROSMDataSource()

{
    nLayers = 0;
    papoLayers = NULL;
    pszName = NULL;
    bInterleavedReading = -1;
    poCurrentLayer = NULL;
    psParser = NULL;
    bHasParsedFirstChunk = FALSE;
    bStopParsing = FALSE;
#ifdef HAVE_SQLITE_VFS
    pMyVFS = NULL;
#endif
    hDB = NULL;
    hInsertNodeStmt = NULL;
    hInsertWayStmt = NULL;
    nNodesInTransaction = 0;
    bInTransaction = FALSE;
    pahSelectNodeStmt = NULL;
    pahSelectWayStmt = NULL;
    panLonLatCache = NULL;
    bInMemoryTmpDB = FALSE;
    bMustUnlink = TRUE;
    nMaxSizeForInMemoryDBInMB = 0;
    bReportAllNodes = FALSE;
    bReportAllWays = FALSE;
    bFeatureAdded = FALSE;

    /* The following 4 config options are only usefull for debugging */
    bIndexPoints = CSLTestBoolean(CPLGetConfigOption("OSM_INDEX_POINTS", "YES"));
    bUsePointsIndex = CSLTestBoolean(CPLGetConfigOption("OSM_USE_POINTS_INDEX", "YES"));
    bIndexWays = CSLTestBoolean(CPLGetConfigOption("OSM_INDEX_WAYS", "YES"));
    bUseWaysIndex = CSLTestBoolean(CPLGetConfigOption("OSM_USE_WAYS_INDEX", "YES"));

    poResultSetLayer = NULL;
    bIndexPointsBackup = FALSE;
    bUsePointsIndexBackup = FALSE;
    bIndexWaysBackup = FALSE;
    bUseWaysIndexBackup = FALSE;

    bIsFeatureCountEnabled = FALSE;

    bAttributeNameLaundering = TRUE;

    pabyWayBuffer = NULL;

    nWaysProcessed = 0;
    nRelationsProcessed = 0;

    bCustomIndexing = CSLTestBoolean(CPLGetConfigOption("OSM_USE_CUSTOM_INDEXING", "YES"));

    bInMemoryNodesFile = FALSE;
    bMustUnlinkNodesFile = TRUE;
    fpNodes = NULL;
    nNodesFileSize = 0;

    nPrevNodeId = -INT_MAX;
    nBucketOld = -1;
    nOffInBucketReducedOld = -1;
    pabySector = NULL;
    papsBuckets = NULL;

    nReqIds = 0;
    panReqIds = NULL;
    pasLonLatArray = NULL;
    nUnsortedReqIds = 0;
    panUnsortedReqIds = NULL;
    nWayFeaturePairs = 0;
    pasWayFeaturePairs = NULL;
    nAccumulatedTags = 0;
    pasAccumulatedTags = NULL;
    nNonRedundantValuesLen = 0;
    pabyNonRedundantValues = NULL;
    nNextKeyIndex = 0;
}

/************************************************************************/
/*                          ~OGROSMDataSource()                         */
/************************************************************************/

OGROSMDataSource::~OGROSMDataSource()

{
    int i;
    for(i=0;i<nLayers;i++)
        delete papoLayers[i];
    CPLFree(papoLayers);

    CPLFree(pszName);

    if( psParser != NULL )
        CPLDebug("OSM", "Number of bytes read in file : " CPL_FRMT_GUIB, OSM_GetBytesRead(psParser));
    OSM_Close(psParser);

    CPLFree(panLonLatCache);
    CPLFree(pabyWayBuffer);

    if( hDB != NULL )
        CloseDB();

#ifdef HAVE_SQLITE_VFS
    if (pMyVFS)
    {
        sqlite3_vfs_unregister(pMyVFS);
        CPLFree(pMyVFS->pAppData);
        CPLFree(pMyVFS);
    }
#endif

    if( osTmpDBName.size() && bMustUnlink )
    {
        const char* pszVal = CPLGetConfigOption("OSM_UNLINK_TMPFILE", "YES");
        if( !EQUAL(pszVal, "NOT_EVEN_AT_END") )
            VSIUnlink(osTmpDBName);
    }

    CPLFree(panReqIds);
    CPLFree(pasLonLatArray);
    CPLFree(panUnsortedReqIds);

    for( i = 0; i < nWayFeaturePairs; i++)
    {
        delete pasWayFeaturePairs[i].poFeature;
    }
    CPLFree(pasWayFeaturePairs);
    CPLFree(pasAccumulatedTags);
    CPLFree(pabyNonRedundantValues);

#ifdef OSM_DEBUG
    FILE* f;
    f = fopen("keys.txt", "wt");
    std::map<CPLString, KeyDesc>::iterator oIter;
    for( oIter = aoMapIndexedKeys.begin(); oIter != aoMapIndexedKeys.end(); ++oIter )
    {
        fprintf(f, "%08d idx=%d %s\n",
                oIter->second.nOccurences,
                oIter->second.nKeyIndex,
                oIter->first.c_str());
    }
    fclose(f);
#endif

    for(i=0;i<(int)asKeys.size();i++)
        CPLFree(asKeys[i]);
    std::map<CPLString, KeyDesc>::iterator oIter = aoMapIndexedKeys.begin();
    for(; oIter != aoMapIndexedKeys.end(); ++oIter)
    {
        KeyDesc& sKD = oIter->second;
        for(i=0;i<(int)sKD.asValues.size();i++)
            CPLFree(sKD.asValues[i]);
    }

    if( fpNodes )
        VSIFCloseL(fpNodes);
    if( osNodesFilename.size() && bMustUnlinkNodesFile )
    {
        const char* pszVal = CPLGetConfigOption("OSM_UNLINK_TMPFILE", "YES");
        if( !EQUAL(pszVal, "NOT_EVEN_AT_END") )
            VSIUnlink(osNodesFilename);
    }

    CPLFree(pabySector);
    if( papsBuckets )
    {
        for( i = 0; i < BUCKET_COUNT; i++)
        {
            CPLFree(papsBuckets[i].pabyBitmap);
        }
        CPLFree(papsBuckets);
    }
}

/************************************************************************/
/*                             CloseDB()                               */
/************************************************************************/

void OGROSMDataSource::CloseDB()
{
    int i;

    if( hInsertNodeStmt != NULL )
        sqlite3_finalize( hInsertNodeStmt );
    hInsertNodeStmt = NULL;

    if( hInsertWayStmt != NULL )
        sqlite3_finalize( hInsertWayStmt );
    hInsertWayStmt = NULL;

    if( pahSelectNodeStmt != NULL )
    {
        for(i = 0; i < LIMIT_IDS_PER_REQUEST; i++)
        {
            if( pahSelectNodeStmt[i] != NULL )
                sqlite3_finalize( pahSelectNodeStmt[i] );
        }
        CPLFree(pahSelectNodeStmt);
        pahSelectNodeStmt = NULL;
    }

    if( pahSelectWayStmt != NULL )
    {
        for(i = 0; i < LIMIT_IDS_PER_REQUEST; i++)
        {
            if( pahSelectWayStmt[i] != NULL )
                sqlite3_finalize( pahSelectWayStmt[i] );
        }
        CPLFree(pahSelectWayStmt);
        pahSelectWayStmt = NULL;
    }

    if( bInTransaction )
        CommitTransaction();

    sqlite3_close(hDB);
    hDB = NULL;
}

/************************************************************************/
/*                             IndexPoint()                             */
/************************************************************************/

static const GByte abyBitsCount[] = {
0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

int OGROSMDataSource::IndexPoint(OSMNode* psNode)
{
    if( !bIndexPoints )
        return TRUE;

    if( bCustomIndexing)
        return IndexPointCustom(psNode);
    else
        return IndexPointSQLite(psNode);
}

int OGROSMDataSource::IndexPointSQLite(OSMNode* psNode)
{
    sqlite3_bind_int64( hInsertNodeStmt, 1, psNode->nID );

    int anLonLat[2];
    anLonLat[0] = DBL_TO_INT(psNode->dfLon);
    anLonLat[1] = DBL_TO_INT(psNode->dfLat);

    sqlite3_bind_blob( hInsertNodeStmt, 2, anLonLat, 2 * sizeof(int), SQLITE_STATIC );

    int rc = sqlite3_step( hInsertNodeStmt );
    sqlite3_reset( hInsertNodeStmt );
    if( !(rc == SQLITE_OK || rc == SQLITE_DONE) )
    {
        CPLError(CE_Failure, CPLE_AppDefined, "Failed inserting node " CPL_FRMT_GIB ": %s",
            psNode->nID, sqlite3_errmsg(hDB));
    }

    return TRUE;
}

int OGROSMDataSource::IndexPointCustom(OSMNode* psNode)
{
    if( psNode->nID <= nPrevNodeId)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "Non increasing node id. Use OSM_USE_CUSTOM_INDEXING=NO");
        bStopParsing = TRUE;
        return FALSE;
    }
    if( psNode->nID < 0 || psNode->nID >= (GIntBig)BUCKET_SIZE * BUCKET_SIZE)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "Unsupported node id value (" CPL_FRMT_GIB "). Use OSM_USE_CUSTOM_INDEXING=NO",
                 psNode->nID);
        bStopParsing = TRUE;
        return FALSE;
    }

    int nBucket = (int)(psNode->nID / BUCKET_SIZE);
    int nOffInBucket = psNode->nID % BUCKET_SIZE;
    int nOffInBucketReduced = nOffInBucket >> NODE_PER_SECTOR_SHIFT;
    int nOffInBucketReducedRemainer = nOffInBucket & ((1 << NODE_PER_SECTOR_SHIFT) - 1);
    Bucket* psBucket = &papsBuckets[nBucket];

    int nBitmapIndex = nOffInBucketReduced / 8;
    int nBitmapRemainer = nOffInBucketReduced % 8;
    psBucket->pabyBitmap[nBitmapIndex] |= (1 << nBitmapRemainer);

    if( nBucket != nBucketOld )
    {
        CPLAssert(nBucket > nBucketOld);
        if( nBucketOld >= 0 )
        {
#ifndef FAKE_LOOKUP_NODES
            if( VSIFWriteL(pabySector, 1, SECTOR_SIZE, fpNodes) != SECTOR_SIZE )
            {
                bStopParsing = TRUE;
                return FALSE;
            }
#endif
            nNodesFileSize += SECTOR_SIZE;
        }
        memset(pabySector, 0, SECTOR_SIZE);
        nBucketOld = nBucket;
        nOffInBucketReducedOld = nOffInBucketReduced;
        CPLAssert(psBucket->nOff == -1);
        psBucket->nOff = VSIFTellL(fpNodes);
    }
    else if( nOffInBucketReduced != nOffInBucketReducedOld )
    {
        CPLAssert(nOffInBucketReduced > nOffInBucketReducedOld);
#ifndef FAKE_LOOKUP_NODES
        if( VSIFWriteL(pabySector, 1, SECTOR_SIZE, fpNodes) != SECTOR_SIZE )
        {
            bStopParsing = TRUE;
            return FALSE;
        }
#endif
        nNodesFileSize += SECTOR_SIZE;
        memset(pabySector, 0, SECTOR_SIZE);
        nOffInBucketReducedOld = nOffInBucketReduced;
    }

    int* panLonLat = (int*)(pabySector + 8 * nOffInBucketReducedRemainer);
    panLonLat[0] = DBL_TO_INT(psNode->dfLon);
    panLonLat[1] = DBL_TO_INT(psNode->dfLat);

    nPrevNodeId = psNode->nID;

    return TRUE;
}

/************************************************************************/
/*                             NotifyNodes()                            */
/************************************************************************/

void OGROSMDataSource::NotifyNodes(unsigned int nNodes, OSMNode* pasNodes)
{
    unsigned int i;

    const OGREnvelope* psEnvelope =
        papoLayers[IDX_LYR_POINTS]->GetSpatialFilterEnvelope();

    for(i = 0; i < nNodes; i++)
    {
        /* If the point doesn't fit into the envelope of the spatial filter */
        /* then skip it */
        if( psEnvelope != NULL &&
            !(pasNodes[i].dfLon >= psEnvelope->MinX &&
              pasNodes[i].dfLon <= psEnvelope->MaxX &&
              pasNodes[i].dfLat >= psEnvelope->MinY &&
              pasNodes[i].dfLat <= psEnvelope->MaxY) )
            continue;

        if( !IndexPoint(&pasNodes[i]) )
            break;

        if( !papoLayers[IDX_LYR_POINTS]->IsUserInterested() )
            continue;

        unsigned int j;
        int bInterestingTag = bReportAllNodes;
        OSMTag* pasTags = pasNodes[i].pasTags;

        if( !bReportAllNodes )
        {
            for(j = 0; j < pasNodes[i].nTags; j++)
            {
                const char* pszK = pasTags[j].pszK;
                if( papoLayers[IDX_LYR_POINTS]->aoSetUnsignificantKeys.find(pszK) ==
                    papoLayers[IDX_LYR_POINTS]->aoSetUnsignificantKeys.end() )
                {
                    bInterestingTag = TRUE;
                    break;
                }
            }
        }

        if( bInterestingTag )
        {
            OGRFeature* poFeature = new OGRFeature(
                        papoLayers[IDX_LYR_POINTS]->GetLayerDefn());

            poFeature->SetGeometryDirectly(
                new OGRPoint(pasNodes[i].dfLon, pasNodes[i].dfLat));

            papoLayers[IDX_LYR_POINTS]->SetFieldsFromTags(
                poFeature, pasNodes[i].nID, pasNodes[i].nTags, pasTags, &pasNodes[i].sInfo );

            int bFilteredOut = FALSE;
            if( !papoLayers[IDX_LYR_POINTS]->AddFeature(poFeature, FALSE, &bFilteredOut) )
            {
                bStopParsing = TRUE;
                break;
            }
            else if (!bFilteredOut)
                bFeatureAdded = TRUE;
        }
    }
}

static void OGROSMNotifyNodes (unsigned int nNodes, OSMNode* pasNodes,
                               OSMContext* psOSMContext, void* user_data)
{
    ((OGROSMDataSource*) user_data)->NotifyNodes(nNodes, pasNodes);
}

/************************************************************************/
/*                          SortReqIdsArray()                           */
/************************************************************************/

static int SortReqIdsArray(const void *a, const void *b)
{
    GIntBig* pa = (GIntBig*)a;
    GIntBig* pb = (GIntBig*)b;
    if (*pa < *pb)
        return -1;
    else if (*pa > *pb)
        return 1;
    else
        return 0;
}

/************************************************************************/
/*                            LookupNodes()                             */
/************************************************************************/

void OGROSMDataSource::LookupNodes( )
{
    if( bCustomIndexing )
        LookupNodesCustom();
    else
        LookupNodesSQLite();
}

void OGROSMDataSource::LookupNodesSQLite( )
{
    unsigned int iCur;
    unsigned int i;

    CPLAssert(nUnsortedReqIds <= MAX_ACCUMULATED_NODES);

    nReqIds = 0;
    for(i = 0; i < nUnsortedReqIds; i++)
    {
        GIntBig id = panUnsortedReqIds[i];
        panReqIds[nReqIds++] = id;
    }

    qsort(panReqIds, nReqIds, sizeof(GIntBig), SortReqIdsArray);

    /* Remove duplicates */
    unsigned int j = 0;
    for(i = 0; i < nReqIds; i++)
    {
        if (!(i > 0 && panReqIds[i] == panReqIds[i-1]))
            panReqIds[j++] = panReqIds[i];
    }
    nReqIds = j;

    iCur = 0;
    j = 0;
    while( iCur < nReqIds )
    {
        unsigned int nToQuery = nReqIds - iCur;
        if( nToQuery > LIMIT_IDS_PER_REQUEST )
            nToQuery = LIMIT_IDS_PER_REQUEST;

        sqlite3_stmt* hStmt = pahSelectNodeStmt[nToQuery-1];
        for(i=iCur;i<iCur + nToQuery;i++)
        {
             sqlite3_bind_int64( hStmt, i - iCur +1, panReqIds[i] );
        }
        iCur += nToQuery;

        while( sqlite3_step(hStmt) == SQLITE_ROW )
        {
            GIntBig id = sqlite3_column_int(hStmt, 0);
            int* panLonLat = (int*)sqlite3_column_blob(hStmt, 1);

            panReqIds[j] = id;
            pasLonLatArray[j].nLon = panLonLat[0];
            pasLonLatArray[j].nLat = panLonLat[1];
            j++;
        }

        sqlite3_reset(hStmt);
    }
    nReqIds = j;
}

void OGROSMDataSource::LookupNodesCustom( )
{
    nReqIds = 0;

    if( nBucketOld >= 0 )
    {
#ifndef FAKE_LOOKUP_NODES
        if( VSIFWriteL(pabySector, 1, SECTOR_SIZE, fpNodes) != SECTOR_SIZE )
        {
            bStopParsing = TRUE;
            return;
        }
#endif
        nNodesFileSize += SECTOR_SIZE;
        nBucketOld = -1;
    }

    unsigned int i;

    CPLAssert(nUnsortedReqIds <= MAX_ACCUMULATED_NODES);

    for(i = 0; i < nUnsortedReqIds; i++)
    {
        GIntBig id = panUnsortedReqIds[i];

        int nBucket = (int)(id / BUCKET_SIZE);
        int nOffInBucket = id % BUCKET_SIZE;
        int nOffInBucketReduced = nOffInBucket >> NODE_PER_SECTOR_SHIFT;
        int nBitmapIndex = nOffInBucketReduced / 8;
        int nBitmapRemainer = nOffInBucketReduced % 8;
        Bucket* psBucket = &papsBuckets[nBucket];

        if( !(psBucket->pabyBitmap[nBitmapIndex] & (1 << nBitmapRemainer)) )
            continue;

        panReqIds[nReqIds++] = id;
    }

    qsort(panReqIds, nReqIds, sizeof(GIntBig), SortReqIdsArray);

    /* Remove duplicates */
    unsigned int j = 0;
    for(i = 0; i < nReqIds; i++)
    {
        if (!(i > 0 && panReqIds[i] == panReqIds[i-1]))
            panReqIds[j++] = panReqIds[i];
    }
    nReqIds = j;
    
    for(i = 0; i < nReqIds; i++)
    {
#ifdef FAKE_LOOKUP_NODES
        pasLonLatArray[i].nLon = 0;
        pasLonLatArray[i].nLat = 0;
#else
        GIntBig id = panReqIds[i];

        int nBucket = (int)(id / BUCKET_SIZE);
        int nOffInBucket = id % BUCKET_SIZE;
        int nOffInBucketReduced = nOffInBucket >> NODE_PER_SECTOR_SHIFT;
        int nOffInBucketReducedRemainer = nOffInBucket & ((1 << NODE_PER_SECTOR_SHIFT) - 1);

        int nBitmapIndex = nOffInBucketReduced / 8;
        int nBitmapRemainer = nOffInBucketReduced % 8;
        Bucket* psBucket = &papsBuckets[nBucket];

        int j;
        int nSector = 0;
        for(j = 0; j < nBitmapIndex; j++)
            nSector += abyBitsCount[psBucket->pabyBitmap[j]];
        if (nBitmapRemainer)
            nSector += abyBitsCount[psBucket->pabyBitmap[nBitmapIndex] & ((1 << nBitmapRemainer) - 1)];

        VSIFSeekL(fpNodes, psBucket->nOff + nSector * SECTOR_SIZE + nOffInBucketReducedRemainer * 8, SEEK_SET);
        if( VSIFReadL(pasLonLatArray + i, 1, 8, fpNodes) != 8 )
        {
            CPLError(CE_Failure,  CPLE_AppDefined,
                     "Cannot read node " CPL_FRMT_GIB, id);
            // FIXME ?
        }
#endif
    }
}

/************************************************************************/
/*                            WriteVarInt()                             */
/************************************************************************/

static void WriteVarInt(int nVal, GByte** ppabyData)
{
    GByte* pabyData = *ppabyData;
    while(TRUE)
    {
        if( (nVal & (~0x7f)) == 0 )
        {
            *pabyData = (GByte)nVal;
            *ppabyData = pabyData + 1;
            return;
        }

        *pabyData = 0x80 | (GByte)(nVal & 0x7f);
        nVal >>= 7;
        pabyData ++;
    }
}

/************************************************************************/
/*                            WriteVarSInt()                            */
/************************************************************************/

static void WriteVarSInt(int nSVal, GByte** ppabyData)
{
    int nVal;
    if( nSVal >= 0 )
        nVal = nSVal << 1;
    else
        nVal = ((-1-nSVal) << 1) + 1;

    WriteVarInt(nVal, ppabyData);
}
/************************************************************************/
/*                           WriteVarSInt64()                           */
/************************************************************************/

static void WriteVarSInt64(GIntBig nSVal, GByte** ppabyData)
{
    GIntBig nVal;
    if( nSVal >= 0 )
        nVal = nSVal << 1;
    else
        nVal = ((-1-nSVal) << 1) + 1;

    GByte* pabyData = *ppabyData;
    while(TRUE)
    {
        if( (nVal & (~0x7f)) == 0 )
        {
            *pabyData = (GByte)nVal;
            *ppabyData = pabyData + 1;
            return;
        }

        *pabyData = 0x80 | (GByte)(nVal & 0x7f);
        nVal >>= 7;
        pabyData ++;
    }
}

/************************************************************************/
/*                             CompressWay()                            */
/************************************************************************/

int OGROSMDataSource::CompressWay ( unsigned int nTags, OSMTag* pasTags,
                                    int nPoints, int* panLonLatPairs,
                                    GByte* pabyCompressedWay )
{
    GByte* pabyPtr = pabyCompressedWay;
    pabyPtr ++;

    int nTagCount = 0;
    CPLAssert(nTags < MAX_COUNT_FOR_TAGS_IN_WAY);
    for(unsigned int iTag = 0; iTag < nTags; iTag++)
    {
        const char* pszK = pasTags[iTag].pszK;
        const char* pszV = pasTags[iTag].pszV;

        if ((int)(pabyPtr - pabyCompressedWay) + 2 >= MAX_SIZE_FOR_TAGS_IN_WAY)
        {
            break;
        }

        std::map<CPLString, KeyDesc>::iterator oIterK;
        oIterK = aoMapIndexedKeys.find(pszK);
        CPLAssert (oIterK != aoMapIndexedKeys.end());
        KeyDesc& sKD = oIterK->second;
        WriteVarInt(sKD.nKeyIndex, &pabyPtr);

        /* to fit in 2 bytes, the theoretical limit would be 127 * 128 + 127 */
        if( sKD.nNextValueIndex < 1024 )
        {
            if ((int)(pabyPtr - pabyCompressedWay) + 2 >= MAX_SIZE_FOR_TAGS_IN_WAY)
            {
                break;
            }

            std::map<CPLString, int>::iterator oIterV;
            oIterV = sKD.anMapV.find(pszV);
            CPLAssert(oIterV != sKD.anMapV.end());
            WriteVarInt(oIterV->second, &pabyPtr);
        }
        else
        {
            int nLenV = strlen(pszV) + 1;
            if ((int)(pabyPtr - pabyCompressedWay) + 2 + nLenV >= MAX_SIZE_FOR_TAGS_IN_WAY)
            {
                break;
            }

            if( !sKD.bHasWarnedManyValues )
            {
                sKD.bHasWarnedManyValues = TRUE;
                CPLDebug("OSM", "More than %d different values for tag %s",
                         1024, pszK);
            }

            WriteVarInt(0, &pabyPtr);

            memcpy(pabyPtr, pszV, nLenV);
            pabyPtr += nLenV;
        }

        nTagCount ++;
    }

    pabyCompressedWay[0] = (GByte) nTagCount;

    memcpy(pabyPtr, panLonLatPairs + 0, sizeof(int));
    memcpy(pabyPtr + sizeof(int), panLonLatPairs + 1, sizeof(int));
    pabyPtr += 2 * sizeof(int);
    for(int i=1;i<nPoints;i++)
    {
        GIntBig nDiff64;
        int nDiff;

        nDiff64 = (GIntBig)panLonLatPairs[2 * i + 0] - (GIntBig)panLonLatPairs[2 * (i-1) + 0];
        WriteVarSInt64(nDiff64, &pabyPtr);

        nDiff = panLonLatPairs[2 * i + 1] - panLonLatPairs[2 * (i-1) + 1];
        WriteVarSInt(nDiff, &pabyPtr);
    }
    int nBufferSize = (int)(pabyPtr - pabyCompressedWay);
    return nBufferSize;
}

/************************************************************************/
/*                             UncompressWay()                          */
/************************************************************************/

int OGROSMDataSource::UncompressWay( int nBytes, GByte* pabyCompressedWay,
                                     int* panCoords,
                                     unsigned int* pnTags, OSMTag* pasTags )
{
    GByte* pabyPtr = pabyCompressedWay;
    unsigned int nTags = *pabyPtr;
    pabyPtr ++;

    if (pnTags)
        *pnTags = nTags;

    for(unsigned int iTag = 0; iTag < nTags; iTag++)
    {
        int nK = ReadVarInt32(&pabyPtr);
        int nV = ReadVarInt32(&pabyPtr);
        GByte* pszV = NULL;
        if( nV == 0 )
        {
            pszV = pabyPtr;
            while(*pabyPtr != '\0')
                pabyPtr ++;
            pabyPtr ++;
        }

        if( pasTags )
        {
            pasTags[iTag].pszK = asKeys[nK];
            pasTags[iTag].pszV = nV ? aoMapIndexedKeys[pasTags[iTag].pszK].asValues[nV] : (const char*) pszV;
        }
    }

    memcpy(&panCoords[0], pabyPtr, sizeof(int));
    memcpy(&panCoords[1], pabyPtr + sizeof(int), sizeof(int));
    pabyPtr += 2 * sizeof(int);
    int nPoints = 1;
    do
    {
        GIntBig nSVal64 = ReadVarInt64(&pabyPtr);
        GIntBig nDiff64 = ((nSVal64 & 1) == 0) ? (((GUIntBig)nSVal64) >> 1) : -(((GUIntBig)nSVal64) >> 1)-1;
        panCoords[2 * nPoints + 0] = panCoords[2 * (nPoints - 1) + 0] + nDiff64;

        int nSVal = ReadVarInt32(&pabyPtr);
        int nDiff = ((nSVal & 1) == 0) ? (((unsigned int)nSVal) >> 1) : -(((unsigned int)nSVal) >> 1)-1;
        panCoords[2 * nPoints + 1] = panCoords[2 * (nPoints - 1) + 1] + nDiff;

        nPoints ++;
    } while (pabyPtr < pabyCompressedWay + nBytes);

    return nPoints;
}

/************************************************************************/
/*                              IndexWay()                              */
/************************************************************************/

void OGROSMDataSource::IndexWay(GIntBig nWayID,
                                unsigned int nTags, OSMTag* pasTags,
                                int* panLonLatPairs, int nPairs)
{
    if( !bIndexWays )
        return;

    sqlite3_bind_int64( hInsertWayStmt, 1, nWayID );

    int nBufferSize = CompressWay (nTags, pasTags, nPairs, panLonLatPairs, pabyWayBuffer);
    CPLAssert(nBufferSize <= WAY_BUFFER_SIZE);
    sqlite3_bind_blob( hInsertWayStmt, 2, pabyWayBuffer,
                       nBufferSize, SQLITE_STATIC );

    int rc = sqlite3_step( hInsertWayStmt );
    sqlite3_reset( hInsertWayStmt );
    if( !(rc == SQLITE_OK || rc == SQLITE_DONE) )
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                "Failed inserting way " CPL_FRMT_GIB ": %s",
                nWayID, sqlite3_errmsg(hDB));
    }
}

/************************************************************************/
/*                              FindNode()                              */
/************************************************************************/

int OGROSMDataSource::FindNode(GIntBig nID)
{
    int iFirst = 0;
    int iLast = nReqIds - 1;
    while(iFirst <= iLast)
    {
        int iMid = (iFirst + iLast) / 2;
        if( nID < panReqIds[iMid])
            iLast = iMid - 1;
        else if( nID > panReqIds[iMid])
            iFirst = iMid + 1;
        else
            return iMid;
    }
    return -1;
}

/************************************************************************/
/*                         ProcessWaysBatch()                           */
/************************************************************************/

void OGROSMDataSource::ProcessWaysBatch()
{
    if( nWayFeaturePairs == 0 ) return;

    //printf("nodes = %d, features = %d\n", nUnsortedReqIds, nWayFeaturePairs);
    LookupNodes();

    for(int iPair = 0; iPair < nWayFeaturePairs; iPair ++)
    {
        int iLocalCurLayer = pasWayFeaturePairs[iPair].iCurLayer;

        unsigned int nFound = 0;
        unsigned int i;
        for(i=0;i<pasWayFeaturePairs[iPair].nRefs;i++)
        {
            int nIdx = FindNode( pasWayFeaturePairs[iPair].panNodeRefs[i] );
            if (nIdx >= 0)
            {
                panLonLatCache[nFound * 2 + 0] = pasLonLatArray[nIdx].nLon;
                panLonLatCache[nFound * 2 + 1] = pasLonLatArray[nIdx].nLat;
                nFound ++;
            }
        }

        if( nFound > 0 && iLocalCurLayer == IDX_LYR_POLYGONS )
        {
            panLonLatCache[nFound * 2 + 0] = panLonLatCache[0 * 2 + 0];
            panLonLatCache[nFound * 2 + 1] = panLonLatCache[0 * 2 + 1];
            nFound ++;
        }

        if( nFound < 2 )
        {
            CPLDebug("OSM", "Way " CPL_FRMT_GIB " with %d nodes that could be found. Discarding it",
                    pasWayFeaturePairs[iPair].nWayID, nFound);
            delete pasWayFeaturePairs[iPair].poFeature;
            continue;
        }

        if( iLocalCurLayer == IDX_LYR_POLYGONS && papoLayers[IDX_LYR_MULTIPOLYGONS]->IsUserInterested() )
            IndexWay(pasWayFeaturePairs[iPair].nWayID,
                     pasWayFeaturePairs[iPair].nTags,
                     pasWayFeaturePairs[iPair].pasTags,
                     panLonLatCache, (int)nFound);
        else
            IndexWay(pasWayFeaturePairs[iPair].nWayID, 0, NULL,
                     panLonLatCache, (int)nFound);

        if( pasWayFeaturePairs[iPair].poFeature == NULL )
        {
            continue;
        }

        OGRLineString* poLS;
        OGRGeometry* poGeom;
        if( iLocalCurLayer == IDX_LYR_POLYGONS )
        {
            OGRPolygon* poPoly = new OGRPolygon();
            OGRLinearRing* poRing = new OGRLinearRing();
            poPoly->addRingDirectly(poRing);
            poLS = poRing;

            poGeom = poPoly;
        }
        else
        {
            poLS = new OGRLineString();
            poGeom = poLS;
        }

        poLS->setNumPoints((int)nFound);
        for(i=0;i<nFound;i++)
        {
            poLS->setPoint(i,
                        INT_TO_DBL(panLonLatCache[i * 2 + 0]),
                        INT_TO_DBL(panLonLatCache[i * 2 + 1]));
        }

        pasWayFeaturePairs[iPair].poFeature->SetGeometryDirectly(poGeom);

        if( nFound != pasWayFeaturePairs[iPair].nRefs + (iLocalCurLayer == IDX_LYR_POLYGONS) )
            CPLDebug("OSM", "For way " CPL_FRMT_GIB ", got only %d nodes instead of %d",
                   pasWayFeaturePairs[iPair].nWayID, nFound,
                   pasWayFeaturePairs[iPair].nRefs + (iLocalCurLayer == IDX_LYR_POLYGONS));

        int bFilteredOut = FALSE;
        if( !papoLayers[iLocalCurLayer]->AddFeature(pasWayFeaturePairs[iPair].poFeature,
                                                    pasWayFeaturePairs[iPair].bAttrFilterAlreadyEvaluated,
                                                    &bFilteredOut) )
            bStopParsing = TRUE;
        else if (!bFilteredOut)
            bFeatureAdded = TRUE;
    }

    nWayFeaturePairs = 0;
    nUnsortedReqIds = 0;

    nAccumulatedTags = 0;
    nNonRedundantValuesLen = 0;
}

/************************************************************************/
/*                              NotifyWay()                             */
/************************************************************************/

void OGROSMDataSource::NotifyWay (OSMWay* psWay)
{
    unsigned int i;

    nWaysProcessed++;
    if( (nWaysProcessed % 10000) == 0 )
    {
        CPLDebug("OSM", "Ways processed : %d", nWaysProcessed);
#ifdef DEBUG_MEM_USAGE
        CPLDebug("OSM", "GetMaxTotalAllocs() = " CPL_FRMT_GUIB, (GUIntBig)GetMaxTotalAllocs());
#endif
    }

    if( !bUsePointsIndex )
        return;

    //printf("way %d : %d nodes\n", (int)psWay->nID, (int)psWay->nRefs);
    if( psWay->nRefs > MAX_NODES_PER_WAY )
    {
        CPLError(CE_Failure, CPLE_NotSupported,
                 "Ways with more than %d nodes are not supported",
                 MAX_NODES_PER_WAY);
        return;
    }

    if( psWay->nRefs < 2 )
    {
        CPLDebug("OSM", "Way " CPL_FRMT_GIB " with %d nodes. Discarding it",
                 psWay->nID, psWay->nRefs);
        return;
    }

    /* Is a closed way a polygon ? */
    int bIsArea = FALSE;
    if( psWay->panNodeRefs[0] == psWay->panNodeRefs[psWay->nRefs - 1] )
    {
        for(i=0;i<psWay->nTags;i++)
        {
            const char* pszK = psWay->pasTags[i].pszK;
            if( strcmp(pszK, "area") == 0 )
            {
                if( strcmp(psWay->pasTags[i].pszV, "yes") == 0 )
                {
                    bIsArea = TRUE;
                }
                else if( strcmp(psWay->pasTags[i].pszV, "no") == 0 )
                {
                    bIsArea = FALSE;
                    break;
                }
            }
            else if( aoSetClosedWaysArePolygons.find(pszK) !=
                     aoSetClosedWaysArePolygons.end() )
            {
                bIsArea = TRUE;
            }
        }
    }
    int iCurLayer = (bIsArea) ? IDX_LYR_POLYGONS : IDX_LYR_LINES ;

    OGRFeature* poFeature = NULL;

    int bInterestingTag = bReportAllWays;
    if( !bReportAllWays )
    {
        for(i=0;i<psWay->nTags;i++)
        {
            const char* pszK = psWay->pasTags[i].pszK;
            if( papoLayers[iCurLayer]->aoSetUnsignificantKeys.find(pszK) ==
                papoLayers[iCurLayer]->aoSetUnsignificantKeys.end() )
            {
                bInterestingTag = TRUE;
                break;
            }
        }
    }

    int bAttrFilterAlreadyEvaluated = FALSE;
    if( papoLayers[iCurLayer]->IsUserInterested() && bInterestingTag )
    {
        poFeature = new OGRFeature(papoLayers[iCurLayer]->GetLayerDefn());

        papoLayers[iCurLayer]->SetFieldsFromTags(
            poFeature, psWay->nID, psWay->nTags, psWay->pasTags, &psWay->sInfo );

        /* Optimization : if we have an attribute filter, that does not require geometry, */
        /* and if we don't need to index ways, then we can just evaluate the attribute */
        /* filter without the geometry */
        if( papoLayers[iCurLayer]->HasAttributeFilter() &&
            !papoLayers[iCurLayer]->AttributeFilterEvaluationNeedsGeometry() &&
            !bIndexWays )
        {
            if( !papoLayers[iCurLayer]->EvaluateAttributeFilter(poFeature) )
            {
                delete poFeature;
                return;
            }
            bAttrFilterAlreadyEvaluated = TRUE;
        }
    }
    else if( !bIndexWays )
    {
        return;
    }

    if( nUnsortedReqIds + psWay->nRefs > MAX_ACCUMULATED_NODES ||
        nWayFeaturePairs == MAX_DELAYED_FEATURES ||
        nAccumulatedTags + psWay->nTags > MAX_ACCUMULATED_TAGS ||
        nNonRedundantValuesLen + 1024 > MAX_NON_REDUNDANT_VALUES )
    {
        ProcessWaysBatch();
    }

    pasWayFeaturePairs[nWayFeaturePairs].nWayID = psWay->nID;
    pasWayFeaturePairs[nWayFeaturePairs].nRefs = psWay->nRefs - bIsArea;
    pasWayFeaturePairs[nWayFeaturePairs].panNodeRefs = panUnsortedReqIds + nUnsortedReqIds;
    pasWayFeaturePairs[nWayFeaturePairs].poFeature = poFeature;
    pasWayFeaturePairs[nWayFeaturePairs].iCurLayer = iCurLayer;
    pasWayFeaturePairs[nWayFeaturePairs].bInterestingTag = bInterestingTag;
    pasWayFeaturePairs[nWayFeaturePairs].bAttrFilterAlreadyEvaluated = bAttrFilterAlreadyEvaluated;

    if( bIsArea && papoLayers[IDX_LYR_MULTIPOLYGONS]->IsUserInterested() )
    {
        int nTagCount = 0;

        pasWayFeaturePairs[nWayFeaturePairs].pasTags = pasAccumulatedTags + nAccumulatedTags;

        for(unsigned int iTag = 0; iTag < psWay->nTags; iTag++)
        {
            const char* pszK = psWay->pasTags[iTag].pszK;
            const char* pszV = psWay->pasTags[iTag].pszV;

            if (strcmp(pszK, "area") == 0)
                continue;
            if (strcmp(pszK, "created_by") == 0)
                continue;
            if (strcmp(pszK, "converted_by") == 0)
                continue;
            if (strcmp(pszK, "note") == 0)
                continue;
            if (strcmp(pszK, "todo") == 0)
                continue;
            if (strcmp(pszK, "fixme") == 0)
                continue;
            if (strcmp(pszK, "FIXME") == 0)
                continue;

            std::map<CPLString, KeyDesc>::iterator oIterK;
            oIterK = aoMapIndexedKeys.find(pszK);
            if (oIterK == aoMapIndexedKeys.end())
            {
                KeyDesc sKD;
                sKD.nKeyIndex = nNextKeyIndex ++;
                //CPLDebug("OSM", "nNextKeyIndex=%d", nNextKeyIndex);
                sKD.nNextValueIndex = 1;
                sKD.bHasWarnedManyValues = FALSE;
                sKD.nOccurences = 0;
                sKD.asValues.push_back(CPLStrdup(""));
                aoMapIndexedKeys[pszK] = sKD;
                asKeys.push_back(CPLStrdup(pszK));
                oIterK = aoMapIndexedKeys.find(pszK);
            }
            KeyDesc& sKD = oIterK->second;
            sKD.nOccurences ++;

            pasAccumulatedTags[nAccumulatedTags].pszK = asKeys[sKD.nKeyIndex];

            /* to fit in 2 bytes, the theoretical limit would be 127 * 128 + 127 */
            if( sKD.nNextValueIndex < 1024 )
            {
                std::map<CPLString, int>::iterator oIterV;
                oIterV = sKD.anMapV.find(pszV);
                int nVIndex;
                if (oIterV == sKD.anMapV.end())
                {
                    nVIndex = sKD.nNextValueIndex;
                    sKD.anMapV[pszV] = sKD.nNextValueIndex ++;
                    sKD.asValues.push_back(CPLStrdup(pszV));
                }
                else
                    nVIndex = oIterV->second;

                pasAccumulatedTags[nAccumulatedTags].pszV = sKD.asValues[nVIndex];
            }
            else
            {
                int nLenV = strlen(pszV) + 1;

                if( !sKD.bHasWarnedManyValues )
                {
                    sKD.bHasWarnedManyValues = TRUE;
                    CPLDebug("OSM", "More than %d different values for tag %s",
                            1024, pszK);
                }

                CPLAssert( nNonRedundantValuesLen + nLenV <= MAX_NON_REDUNDANT_VALUES );
                memcpy(pabyNonRedundantValues + nNonRedundantValuesLen, pszV, nLenV);
                pasAccumulatedTags[nAccumulatedTags].pszV = (const char*) pabyNonRedundantValues + nNonRedundantValuesLen;
                nNonRedundantValuesLen += nLenV;
            }
            nAccumulatedTags ++;

            nTagCount ++;
            if( nTagCount == MAX_COUNT_FOR_TAGS_IN_WAY )
                break;
        }

        pasWayFeaturePairs[nWayFeaturePairs].nTags = nTagCount;
    }
    else
    {
        pasWayFeaturePairs[nWayFeaturePairs].nTags = 0;
        pasWayFeaturePairs[nWayFeaturePairs].pasTags = NULL;
    }

    nWayFeaturePairs++;

    memcpy(panUnsortedReqIds + nUnsortedReqIds, psWay->panNodeRefs, sizeof(GIntBig) * (psWay->nRefs - bIsArea));
    nUnsortedReqIds += (psWay->nRefs - bIsArea);
}

static void OGROSMNotifyWay (OSMWay* psWay, OSMContext* psOSMContext, void* user_data)
{
    ((OGROSMDataSource*) user_data)->NotifyWay(psWay);
}

/************************************************************************/
/*                            LookupWays()                              */
/************************************************************************/

unsigned int OGROSMDataSource::LookupWays( std::map< GIntBig, std::pair<int,void*> >& aoMapWays,
                                           OSMRelation* psRelation )
{
    unsigned int nFound = 0;
    unsigned int iCur = 0;
    unsigned int i;

    while( iCur < psRelation->nMembers )
    {
        unsigned int nToQuery = 0;
        for(i=iCur;i<psRelation->nMembers;i++)
        {
            if( psRelation->pasMembers[i].eType == MEMBER_WAY &&
                strcmp(psRelation->pasMembers[i].pszRole, "subarea") != 0 )
            {
                nToQuery ++;
                if( nToQuery == LIMIT_IDS_PER_REQUEST )
                    break;
            }
        }

        if( nToQuery == 0)
            break;

        unsigned int iLastI = (i == psRelation->nMembers) ? i : i + 1;

        sqlite3_stmt* hStmt = pahSelectWayStmt[nToQuery-1];
        unsigned int nBindIndex = 1;
        for(i=iCur;i<iLastI;i++)
        {
            if( psRelation->pasMembers[i].eType == MEMBER_WAY &&
                strcmp(psRelation->pasMembers[i].pszRole, "subarea") != 0 )
            {
                sqlite3_bind_int64( hStmt, nBindIndex,
                                    psRelation->pasMembers[i].nID );
                nBindIndex ++;
            }
        }
        iCur = iLastI;

        while( sqlite3_step(hStmt) == SQLITE_ROW )
        {
            GIntBig id = sqlite3_column_int(hStmt, 0);
            if( aoMapWays.find(id) == aoMapWays.end() )
            {
                int nBlobSize = sqlite3_column_bytes(hStmt, 1);
                const void* blob = sqlite3_column_blob(hStmt, 1);
                void* blob_dup = CPLMalloc(nBlobSize);
                memcpy(blob_dup, blob, nBlobSize);
                aoMapWays[id] = std::pair<int,void*>(nBlobSize, blob_dup);
            }
            nFound++;
        }

        sqlite3_reset(hStmt);
    }

    return nFound;
}

/************************************************************************/
/*                          BuildMultiPolygon()                         */
/************************************************************************/

OGRGeometry* OGROSMDataSource::BuildMultiPolygon(OSMRelation* psRelation,
                                                 unsigned int* pnTags,
                                                 OSMTag* pasTags)
{

    std::map< GIntBig, std::pair<int,void*> > aoMapWays;
    LookupWays( aoMapWays, psRelation );

    int bMissing = FALSE;
    unsigned int i;
    for(i = 0; i < psRelation->nMembers; i ++ )
    {
        if( psRelation->pasMembers[i].eType == MEMBER_WAY &&
            strcmp(psRelation->pasMembers[i].pszRole, "subarea") != 0 )
        {
            if( aoMapWays.find( psRelation->pasMembers[i].nID ) == aoMapWays.end() )
            {
                CPLDebug("OSM", "Relation " CPL_FRMT_GIB " has missing ways. Ignoring it",
                        psRelation->nID);
                bMissing = TRUE;
                break;
            }
        }
    }

    OGRGeometry* poRet = NULL;
    OGRMultiLineString* poMLS = NULL;
    OGRGeometry** papoPolygons = NULL;
    int nPolys = 0;

    if( bMissing )
        goto cleanup;

    poMLS = new OGRMultiLineString();
    papoPolygons = (OGRGeometry**) CPLMalloc(
        sizeof(OGRGeometry*) *  psRelation->nMembers);
    nPolys = 0;

    if( pnTags != NULL )
        *pnTags = 0;

    for(i = 0; i < psRelation->nMembers; i ++ )
    {
        if( psRelation->pasMembers[i].eType == MEMBER_WAY &&
            strcmp(psRelation->pasMembers[i].pszRole, "subarea") != 0  )
        {
            const std::pair<int, void*>& oGeom = aoMapWays[ psRelation->pasMembers[i].nID ];

            int* panCoords = (int*) panLonLatCache;
            int nPoints;

            if( pnTags != NULL && *pnTags == 0 &&
                strcmp(psRelation->pasMembers[i].pszRole, "outer") == 0 )
            {
                int nCompressedWaySize = oGeom.first;
                GByte* pabyCompressedWay = (GByte*) oGeom.second;

                memcpy(pabyWayBuffer, pabyCompressedWay, nCompressedWaySize);

                nPoints = UncompressWay (nCompressedWaySize, pabyWayBuffer,
                                         panCoords,
                                         pnTags, pasTags );
            }
            else
            {
                nPoints = UncompressWay (oGeom.first, (GByte*) oGeom.second, panCoords,
                                         NULL, NULL);
            }

            OGRLineString* poLS;

            if ( panCoords[0] == panCoords[2 * (nPoints - 1)] &&
                    panCoords[1] == panCoords[2 * (nPoints - 1) + 1] )
            {
                OGRPolygon* poPoly = new OGRPolygon();
                OGRLinearRing* poRing = new OGRLinearRing();
                poPoly->addRingDirectly(poRing);
                papoPolygons[nPolys ++] = poPoly;
                poLS = poRing;
            }
            else
            {
                poLS = new OGRLineString();
                poMLS->addGeometryDirectly(poLS);
            }

            poLS->setNumPoints(nPoints);
            for(int j=0;j<nPoints;j++)
            {
                poLS->setPoint( j,
                                INT_TO_DBL(panCoords[2 * j + 0]),
                                INT_TO_DBL(panCoords[2 * j + 1]) );
            }

        }
    }

    if( poMLS->getNumGeometries() > 0 )
    {
        OGRGeometryH hPoly = OGRBuildPolygonFromEdges( (OGRGeometryH) poMLS,
                                                        TRUE,
                                                        FALSE,
                                                        0,
                                                        NULL );
        if( hPoly != NULL && OGR_G_GetGeometryType(hPoly) == wkbPolygon )
        {
            OGRPolygon* poSuperPoly = (OGRPolygon* ) hPoly;
            for(i = 0; i < 1 + (unsigned int)poSuperPoly->getNumInteriorRings(); i++)
            {
                OGRPolygon* poPoly = new OGRPolygon();
                OGRLinearRing* poRing =  (i == 0) ? poSuperPoly->getExteriorRing() :
                                                    poSuperPoly->getInteriorRing(i - 1);
                if( poRing != NULL && poRing->getNumPoints() >= 4 &&
                    poRing->getX(0) == poRing->getX(poRing->getNumPoints() -1) &&
                    poRing->getY(0) == poRing->getY(poRing->getNumPoints() -1) )
                {
                    poPoly->addRing( poRing );
                    papoPolygons[nPolys ++] = poPoly;
                }
            }
        }

        OGR_G_DestroyGeometry(hPoly);
    }
    delete poMLS;

    if( nPolys > 0 )
    {
        int bIsValidGeometry = FALSE;
        const char* apszOptions[2] = { "METHOD=DEFAULT", NULL };
        OGRGeometry* poGeom = OGRGeometryFactory::organizePolygons(
            papoPolygons, nPolys, &bIsValidGeometry, apszOptions );

        if( poGeom != NULL && poGeom->getGeometryType() == wkbPolygon )
        {
            OGRMultiPolygon* poMulti = new OGRMultiPolygon();
            poMulti->addGeometryDirectly(poGeom);
            poGeom = poMulti;
        }

        if( poGeom != NULL && poGeom->getGeometryType() == wkbMultiPolygon )
        {
            poRet = poGeom;
        }
        else
        {
            CPLDebug("OSM", "Relation " CPL_FRMT_GIB ": Geometry has incompatible type : %s",
                     psRelation->nID,
                     poGeom != NULL ? OGR_G_GetGeometryName((OGRGeometryH)poGeom) : "null" );
            delete poGeom;
        }
    }

    CPLFree(papoPolygons);

cleanup:
    /* Cleanup */
    std::map< GIntBig, std::pair<int,void*> >::iterator oIter;
    for( oIter = aoMapWays.begin(); oIter != aoMapWays.end(); ++oIter )
        CPLFree(oIter->second.second);

    return poRet;
}

/************************************************************************/
/*                          BuildGeometryCollection()                   */
/************************************************************************/

OGRGeometry* OGROSMDataSource::BuildGeometryCollection(OSMRelation* psRelation, int bMultiLineString)
{
    std::map< GIntBig, std::pair<int,void*> > aoMapWays;
    LookupWays( aoMapWays, psRelation );

    unsigned int i;

    OGRGeometryCollection* poColl;
    if( bMultiLineString )
        poColl = new OGRMultiLineString();
    else
        poColl = new OGRGeometryCollection();

    for(i = 0; i < psRelation->nMembers; i ++ )
    {
        if( psRelation->pasMembers[i].eType == MEMBER_NODE && !bMultiLineString )
        {
            nUnsortedReqIds = 1;
            panUnsortedReqIds[0] = psRelation->pasMembers[i].nID;
            LookupNodes();
            if( nReqIds == 1 )
            {
                poColl->addGeometryDirectly(new OGRPoint(
                    INT_TO_DBL(pasLonLatArray[0].nLon),
                    INT_TO_DBL(pasLonLatArray[0].nLat)));
            }
        }
        else if( psRelation->pasMembers[i].eType == MEMBER_WAY &&
                 strcmp(psRelation->pasMembers[i].pszRole, "subarea") != 0  &&
                 aoMapWays.find( psRelation->pasMembers[i].nID ) != aoMapWays.end() )
        {
            const std::pair<int, void*>& oGeom = aoMapWays[ psRelation->pasMembers[i].nID ];

            int* panCoords = (int*) panLonLatCache;
            int nPoints = UncompressWay (oGeom.first, (GByte*) oGeom.second, panCoords, NULL, NULL);

            OGRLineString* poLS;

            poLS = new OGRLineString();
            poColl->addGeometryDirectly(poLS);

            poLS->setNumPoints(nPoints);
            for(int j=0;j<nPoints;j++)
            {
                poLS->setPoint( j,
                                INT_TO_DBL(panCoords[2 * j + 0]),
                                INT_TO_DBL(panCoords[2 * j + 1]) );
            }

        }
    }

    if( poColl->getNumGeometries() == 0 )
    {
        delete poColl;
        poColl = NULL;
    }

    /* Cleanup */
    std::map< GIntBig, std::pair<int,void*> >::iterator oIter;
    for( oIter = aoMapWays.begin(); oIter != aoMapWays.end(); ++oIter )
        CPLFree(oIter->second.second);

    return poColl;
}

/************************************************************************/
/*                            NotifyRelation()                          */
/************************************************************************/

void OGROSMDataSource::NotifyRelation (OSMRelation* psRelation)
{
    unsigned int i;

    if( nWayFeaturePairs != 0 )
        ProcessWaysBatch();

    nRelationsProcessed++;
    if( (nRelationsProcessed % 10000) == 0 )
    {
        CPLDebug("OSM", "Relations processed : %d", nRelationsProcessed);
#ifdef DEBUG_MEM_USAGE
        CPLDebug("OSM", "GetMaxTotalAllocs() = " CPL_FRMT_GUIB, (GUIntBig)GetMaxTotalAllocs());
#endif
    }

    if( !bUseWaysIndex )
        return;

    int bMultiPolygon = FALSE;
    int bMultiLineString = FALSE;
    int bInterestingTagFound = FALSE;
    const char* pszTypeV = NULL;
    for(i = 0; i < psRelation->nTags; i ++ )
    {
        const char* pszK = psRelation->pasTags[i].pszK;
        if( strcmp(pszK, "type") == 0 )
        {
            const char* pszV = psRelation->pasTags[i].pszV;
            pszTypeV = pszV;
            if( strcmp(pszV, "multipolygon") == 0 ||
                strcmp(pszV, "boundary") == 0)
            {
                bMultiPolygon = TRUE;
            }
            else if( strcmp(pszV, "multilinestring") == 0 ||
                     strcmp(pszV, "route") == 0 )
            {
                bMultiLineString = TRUE;
            }
        }
        else if ( strcmp(pszK, "created_by") != 0 )
            bInterestingTagFound = TRUE;
    }

    /* Optimization : if we have an attribute filter, that does not require geometry, */
    /* then we can just evaluate the attribute filter without the geometry */
    int iCurLayer = (bMultiPolygon) ?    IDX_LYR_MULTIPOLYGONS :
                    (bMultiLineString) ? IDX_LYR_MULTILINESTRINGS :
                                         IDX_LYR_OTHER_RELATIONS;
    if( !papoLayers[iCurLayer]->IsUserInterested() )
        return;

    OGRFeature* poFeature = NULL;

    if( papoLayers[iCurLayer]->HasAttributeFilter() &&
        !papoLayers[iCurLayer]->AttributeFilterEvaluationNeedsGeometry() )
    {
        poFeature = new OGRFeature(papoLayers[iCurLayer]->GetLayerDefn());

        papoLayers[iCurLayer]->SetFieldsFromTags( poFeature,
                                                  psRelation->nID,
                                                  psRelation->nTags,
                                                  psRelation->pasTags,
                                                  &psRelation->sInfo);

        if( !papoLayers[iCurLayer]->EvaluateAttributeFilter(poFeature) )
        {
            delete poFeature;
            return;
        }
    }

    OGRGeometry* poGeom;

    unsigned int nExtraTags = 0;
    OSMTag pasExtraTags[1 + MAX_NODES_PER_WAY];

    if( bMultiPolygon )
    {
        if( !bInterestingTagFound )
        {
            poGeom = BuildMultiPolygon(psRelation, &nExtraTags, pasExtraTags);
            pasExtraTags[nExtraTags].pszK = "type";
            pasExtraTags[nExtraTags].pszV = pszTypeV;
            nExtraTags ++;
        }
        else
            poGeom = BuildMultiPolygon(psRelation, NULL, NULL);
    }
    else
        poGeom = BuildGeometryCollection(psRelation, bMultiLineString);

    if( poGeom != NULL )
    {
        int bAttrFilterAlreadyEvaluated;
        if( poFeature == NULL )
        {
            poFeature = new OGRFeature(papoLayers[iCurLayer]->GetLayerDefn());

            papoLayers[iCurLayer]->SetFieldsFromTags( poFeature,
                                                      psRelation->nID,
                                                      nExtraTags ? nExtraTags : psRelation->nTags,
                                                      nExtraTags ? pasExtraTags : psRelation->pasTags,
                                                      &psRelation->sInfo);

            bAttrFilterAlreadyEvaluated = FALSE;
        }
        else
            bAttrFilterAlreadyEvaluated = TRUE;

        poFeature->SetGeometryDirectly(poGeom);

        int bFilteredOut = FALSE;
        if( !papoLayers[iCurLayer]->AddFeature( poFeature,
                                                bAttrFilterAlreadyEvaluated,
                                                &bFilteredOut ) )
            bStopParsing = TRUE;
        else if (!bFilteredOut)
            bFeatureAdded = TRUE;
    }
    else
        delete poFeature;
}

static void OGROSMNotifyRelation (OSMRelation* psRelation,
                                  OSMContext* psOSMContext, void* user_data)
{
    ((OGROSMDataSource*) user_data)->NotifyRelation(psRelation);
}

/************************************************************************/
/*                             NotifyBounds()                           */
/************************************************************************/

void OGROSMDataSource::NotifyBounds (double dfXMin, double dfYMin,
                                     double dfXMax, double dfYMax)
{
    sExtent.MinX = dfXMin;
    sExtent.MinY = dfYMin;
    sExtent.MaxX = dfXMax;
    sExtent.MaxY = dfYMax;
    bExtentValid = TRUE;

    CPLDebug("OSM", "Got bounds : minx=%f, miny=%f, maxx=%f, maxy=%f",
             dfXMin, dfYMin, dfXMax, dfYMax);
}

static void OGROSMNotifyBounds( double dfXMin, double dfYMin,
                                double dfXMax, double dfYMax,
                                OSMContext* psCtxt, void* user_data )
{
    ((OGROSMDataSource*) user_data)->NotifyBounds(dfXMin, dfYMin,
                                                  dfXMax, dfYMax);
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

int OGROSMDataSource::Open( const char * pszFilename, int bUpdateIn)

{
    const char* pszExt = CPLGetExtension(pszFilename);
    if( !EQUAL(pszExt, "pbf") &&
        !EQUAL(pszExt, "osm") &&
        !EQUALN(pszFilename, "/vsicurl_streaming/", strlen("/vsicurl_streaming/")) &&
        strcmp(pszFilename, "/vsistdin/") != 0 &&
        strcmp(pszFilename, "/dev/stdin/") != 0 )
        return FALSE;

    pszName = CPLStrdup( pszFilename );

    psParser = OSM_Open( pszName,
                         OGROSMNotifyNodes,
                         OGROSMNotifyWay,
                         OGROSMNotifyRelation,
                         OGROSMNotifyBounds,
                         this );
    if( psParser == NULL )
        return FALSE;

    if (bUpdateIn)
    {
        CPLError(CE_Failure, CPLE_NotSupported,
                 "OGR/OSM driver does not support opening a file in update mode");
        return FALSE;
    }

    nLayers = 6;
    papoLayers = (OGROSMLayer**) CPLMalloc(nLayers * sizeof(OGROSMLayer*));

    papoLayers[IDX_LYR_POINTS] = new OGROSMLayer(this, "points");
    papoLayers[IDX_LYR_POINTS]->GetLayerDefn()->SetGeomType(wkbPoint);

    papoLayers[IDX_LYR_LINES] = new OGROSMLayer(this, "lines");
    papoLayers[IDX_LYR_LINES]->GetLayerDefn()->SetGeomType(wkbLineString);

    papoLayers[IDX_LYR_POLYGONS] = new OGROSMLayer(this, "polygons");
    papoLayers[IDX_LYR_POLYGONS]->GetLayerDefn()->SetGeomType(wkbPolygon);

    papoLayers[IDX_LYR_MULTILINESTRINGS] = new OGROSMLayer(this, "multilinestrings");
    papoLayers[IDX_LYR_MULTILINESTRINGS]->GetLayerDefn()->SetGeomType(wkbMultiLineString);

    papoLayers[IDX_LYR_MULTIPOLYGONS] = new OGROSMLayer(this, "multipolygons");
    papoLayers[IDX_LYR_MULTIPOLYGONS]->GetLayerDefn()->SetGeomType(wkbMultiPolygon);

    papoLayers[IDX_LYR_OTHER_RELATIONS] = new OGROSMLayer(this, "other_relations");
    papoLayers[IDX_LYR_OTHER_RELATIONS]->GetLayerDefn()->SetGeomType(wkbGeometryCollection);

    if( !ParseConf() )
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "Could not parse configuration file for OSM import");
        return FALSE;
    }

    panLonLatCache = (int*)CPLMalloc(sizeof(int) * 2 * MAX_NODES_PER_WAY);
    pabyWayBuffer = (GByte*)CPLMalloc(WAY_BUFFER_SIZE);

    panReqIds = (GIntBig*)CPLMalloc(MAX_ACCUMULATED_NODES * sizeof(GIntBig));
    pasLonLatArray = (LonLat*)CPLMalloc(MAX_ACCUMULATED_NODES * sizeof(LonLat));
    panUnsortedReqIds = (GIntBig*)CPLMalloc(MAX_ACCUMULATED_NODES * sizeof(GIntBig));
    pasWayFeaturePairs = (WayFeaturePair*)CPLMalloc(MAX_DELAYED_FEATURES * sizeof(WayFeaturePair));
    pasAccumulatedTags = (OSMTag*) CPLMalloc(MAX_ACCUMULATED_TAGS * sizeof(OSMTag));
    pabyNonRedundantValues = (GByte*) CPLMalloc(MAX_NON_REDUNDANT_VALUES);

    nMaxSizeForInMemoryDBInMB = atoi(CPLGetConfigOption("OSM_MAX_TMPFILE_SIZE", "100"));
    GIntBig nSize = (GIntBig)nMaxSizeForInMemoryDBInMB * 1024 * 1024;
    if (nSize < 0 || (GIntBig)(size_t)nSize != nSize)
    {
        CPLError(CE_Failure, CPLE_AppDefined, "Invalid value for OSM_MAX_TMPFILE_SIZE. Using 100 instead.");
        nMaxSizeForInMemoryDBInMB = 100;
        nSize = (GIntBig)nMaxSizeForInMemoryDBInMB * 1024 * 1024;
    }

    if( bCustomIndexing )
    {
        pabySector = (GByte*) CPLCalloc(1, SECTOR_SIZE);
        papsBuckets = (Bucket*) CPLMalloc(sizeof(Bucket) * BUCKET_COUNT);
        size_t i;
        for(i = 0; i < BUCKET_COUNT; i++)
        {
            papsBuckets[i].nOff = -1;
            papsBuckets[i].pabyBitmap = (GByte*)CPLCalloc(1, BUCKET_BITMAP_SIZE);
        }

        bInMemoryNodesFile = TRUE;
        osNodesFilename.Printf("/vsimem/osm_importer/osm_temp_nodes_%p", this);
        fpNodes = VSIFOpenL(osNodesFilename, "wb+");
        if( fpNodes == NULL )
        {
            return FALSE;
        }

        CPLPushErrorHandler(CPLQuietErrorHandler);
        int bSuccess = VSIFSeekL(fpNodes, (vsi_l_offset) (nSize * 3 / 4), SEEK_SET) == 0;
        CPLPopErrorHandler();

        if( bSuccess )
        {
            VSIFSeekL(fpNodes, 0, SEEK_SET);
            VSIFTruncateL(fpNodes, 0);
        }
        else
        {
            CPLDebug("OSM", "Not enough memory for in-memory file. Using disk temporary file instead.");

            VSIFCloseL(fpNodes);
            fpNodes = NULL;
            VSIUnlink(osNodesFilename);

            bInMemoryNodesFile = FALSE;
            osNodesFilename = CPLGenerateTempFilename("osm_tmp_nodes");

            fpNodes = VSIFOpenL(osNodesFilename, "wb+");
            if( fpNodes == NULL )
            {
                return FALSE;
            }

            /* On Unix filesystems, you can remove a file even if it */
            /* opened */
            const char* pszVal = CPLGetConfigOption("OSM_UNLINK_TMPFILE", "YES");
            if( EQUAL(pszVal, "YES") )
            {
                CPLPushErrorHandler(CPLQuietErrorHandler);
                bMustUnlinkNodesFile = VSIUnlink( osNodesFilename ) != 0;
                CPLPopErrorHandler();
            }

            return FALSE;
        }
    }

    return CreateTempDB();
}

/************************************************************************/
/*                             CreateTempDB()                           */
/************************************************************************/

int OGROSMDataSource::CreateTempDB()
{
    char* pszErrMsg = NULL;

    int rc = 0;
    int bIsExisting = FALSE;
    int bSuccess = FALSE;

#ifdef HAVE_SQLITE_VFS
    const char* pszExistingTmpFile = CPLGetConfigOption("OSM_EXISTING_TMPFILE", NULL);
    if ( pszExistingTmpFile != NULL )
    {
        bSuccess = TRUE;
        bIsExisting = TRUE;
        rc = sqlite3_open_v2( pszExistingTmpFile, &hDB,
                              SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX,
                              NULL );
    }
    else
    {
        osTmpDBName.Printf("/vsimem/osm_importer/osm_temp_%p.sqlite", this);

        /* On 32 bit, the virtual memory space is scarce, so we need to reserve it right now */
        /* Won't hurt on 64 bit either. */
        VSILFILE* fp = VSIFOpenL(osTmpDBName, "wb");
        if( fp )
        {
            GIntBig nSize = (GIntBig)nMaxSizeForInMemoryDBInMB * 1024 * 1024;
            if( bCustomIndexing && bInMemoryNodesFile )
                nSize = nSize * 1 / 4;

            CPLPushErrorHandler(CPLQuietErrorHandler);
            bSuccess = VSIFSeekL(fp, (vsi_l_offset) nSize, SEEK_SET) == 0;
            CPLPopErrorHandler();

            if( bSuccess )
                 VSIFTruncateL(fp, 0);

            VSIFCloseL(fp);

            if( !bSuccess )
            {
                CPLDebug("OSM", "Not enough memory for in-memory file. Using disk temporary file instead.");
                VSIUnlink(osTmpDBName);
            }
        }

        if( bSuccess )
        {
            bInMemoryTmpDB = TRUE;
            pMyVFS = OGRSQLiteCreateVFS(NULL, this);
            sqlite3_vfs_register(pMyVFS, 0);
            rc = sqlite3_open_v2( osTmpDBName.c_str(), &hDB,
                                SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
                                pMyVFS->zName );
        }
    }
#endif

    if( !bSuccess )
    {
        osTmpDBName = CPLGenerateTempFilename("osm_tmp");
        rc = sqlite3_open( osTmpDBName.c_str(), &hDB );

        /* On Unix filesystems, you can remove a file even if it */
        /* opened */
        if( rc == SQLITE_OK )
        {
            const char* pszVal = CPLGetConfigOption("OSM_UNLINK_TMPFILE", "YES");
            if( EQUAL(pszVal, "YES") )
            {
                CPLPushErrorHandler(CPLQuietErrorHandler);
                bMustUnlink = VSIUnlink( osTmpDBName ) != 0;
                CPLPopErrorHandler();
            }
        }
    }

    if( rc != SQLITE_OK )
    {
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "sqlite3_open(%s) failed: %s",
                  osTmpDBName.c_str(), sqlite3_errmsg( hDB ) );
        return FALSE;
    }

    if( !SetDBOptions() )
    {
        return FALSE;
    }

    if( !bIsExisting )
    {
        rc = sqlite3_exec( hDB,
                        "CREATE TABLE nodes (id INTEGER PRIMARY KEY, coords BLOB)",
                        NULL, NULL, &pszErrMsg );
        if( rc != SQLITE_OK )
        {
            CPLError( CE_Failure, CPLE_AppDefined,
                    "Unable to create table nodes : %s", pszErrMsg );
            sqlite3_free( pszErrMsg );
            return FALSE;
        }

        rc = sqlite3_exec( hDB,
                        "CREATE TABLE ways (id INTEGER PRIMARY KEY, coords BLOB)",
                        NULL, NULL, &pszErrMsg );
        if( rc != SQLITE_OK )
        {
            CPLError( CE_Failure, CPLE_AppDefined,
                    "Unable to create table ways : %s", pszErrMsg );
            sqlite3_free( pszErrMsg );
            return FALSE;
        }
    }

    return CreatePreparedStatements();
}
/************************************************************************/
/*                            SetDBOptions()                            */
/************************************************************************/

int OGROSMDataSource::SetDBOptions()
{
    char* pszErrMsg = NULL;
    int rc;

    rc = sqlite3_exec( hDB, "PRAGMA synchronous = OFF", NULL, NULL, &pszErrMsg );
    if( rc != SQLITE_OK )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                    "Unable to run PRAGMA synchronous : %s",
                    pszErrMsg );
        sqlite3_free( pszErrMsg );
        return FALSE;
    }

    rc = sqlite3_exec( hDB, "PRAGMA journal_mode = OFF", NULL, NULL, &pszErrMsg );
    if( rc != SQLITE_OK )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                    "Unable to run PRAGMA journal_mode : %s",
                    pszErrMsg );
        sqlite3_free( pszErrMsg );
        return FALSE;
    }

    rc = sqlite3_exec( hDB, "PRAGMA temp_store = MEMORY", NULL, NULL, &pszErrMsg );
    if( rc != SQLITE_OK )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                    "Unable to run PRAGMA temp_store : %s",
                    pszErrMsg );
        sqlite3_free( pszErrMsg );
        return FALSE;
    }

    if( !SetCacheSize() )
        return FALSE;

    if( !StartTransaction() )
        return FALSE;

    return TRUE;
}

/************************************************************************/
/*                              SetCacheSize()                          */
/************************************************************************/

int OGROSMDataSource::SetCacheSize()
{
    int rc;
    const char* pszSqliteCacheMB = CPLGetConfigOption("OSM_SQLITE_CACHE", NULL);
    if (pszSqliteCacheMB != NULL)
    {
        char* pszErrMsg = NULL;
        char **papszResult;
        int nRowCount, nColCount;
        int iSqliteCachePages;
        int iSqlitePageSize = -1;
        int iSqliteCacheBytes = atoi( pszSqliteCacheMB ) * 1024 * 1024;

        /* querying the current PageSize */
        rc = sqlite3_get_table( hDB, "PRAGMA page_size",
                                &papszResult, &nRowCount, &nColCount,
                                &pszErrMsg );
        if( rc == SQLITE_OK )
        {
            int iRow;
            for (iRow = 1; iRow <= nRowCount; iRow++)
            {
                iSqlitePageSize = atoi( papszResult[(iRow * nColCount) + 0] );
            }
            sqlite3_free_table(papszResult);
        }
        if( iSqlitePageSize < 0 )
        {
            CPLError( CE_Failure, CPLE_AppDefined,
                      "Unable to run PRAGMA page_size : %s",
                      pszErrMsg ? pszErrMsg : sqlite3_errmsg(hDB) );
            sqlite3_free( pszErrMsg );
            return TRUE;
        }

        /* computing the CacheSize as #Pages */
        iSqliteCachePages = iSqliteCacheBytes / iSqlitePageSize;
        if( iSqliteCachePages <= 0)
            return TRUE;

        rc = sqlite3_exec( hDB, CPLSPrintf( "PRAGMA cache_size = %d",
                                            iSqliteCachePages ),
                           NULL, NULL, &pszErrMsg );
        if( rc != SQLITE_OK )
        {
            CPLError( CE_Warning, CPLE_AppDefined,
                      "Unrecognized value for PRAGMA cache_size : %s",
                      pszErrMsg );
            sqlite3_free( pszErrMsg );
            rc = SQLITE_OK;
        }
    }
    return TRUE;
}

/************************************************************************/
/*                        CreatePreparedStatements()                    */
/************************************************************************/

int OGROSMDataSource::CreatePreparedStatements()
{
    int rc;

    rc = sqlite3_prepare( hDB, "INSERT INTO nodes (id, coords) VALUES (?,?)", -1,
                          &hInsertNodeStmt, NULL );
    if( rc != SQLITE_OK )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "sqlite3_prepare() failed :  %s", sqlite3_errmsg(hDB) );
        return FALSE;
    }

    pahSelectNodeStmt = (sqlite3_stmt**) CPLCalloc(sizeof(sqlite3_stmt*), LIMIT_IDS_PER_REQUEST);

    char szTmp[LIMIT_IDS_PER_REQUEST*2 + 128];
    strcpy(szTmp, "SELECT id, coords FROM nodes WHERE id IN (");
    int nLen = strlen(szTmp);
    for(int i=0;i<LIMIT_IDS_PER_REQUEST;i++)
    {
        if(i == 0)
        {
            strcpy(szTmp + nLen, "?) ORDER BY id ASC");
            nLen += 2;
        }
        else
        {
            strcpy(szTmp + nLen -1, ",?) ORDER BY id ASC");
            nLen += 2;
        }
        rc = sqlite3_prepare( hDB, szTmp, -1, &pahSelectNodeStmt[i], NULL );
        if( rc != SQLITE_OK )
        {
            CPLError( CE_Failure, CPLE_AppDefined,
                    "sqlite3_prepare() failed :  %s", sqlite3_errmsg(hDB) );
            return FALSE;
        }
    }

    rc = sqlite3_prepare( hDB, "INSERT INTO ways (id, coords) VALUES (?,?)", -1,
                          &hInsertWayStmt, NULL );
    if( rc != SQLITE_OK )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "sqlite3_prepare() failed :  %s", sqlite3_errmsg(hDB) );
        return FALSE;
    }

    pahSelectWayStmt = (sqlite3_stmt**) CPLCalloc(sizeof(sqlite3_stmt*), LIMIT_IDS_PER_REQUEST);

    strcpy(szTmp, "SELECT id, coords FROM ways WHERE id IN (");
    nLen = strlen(szTmp);
    for(int i=0;i<LIMIT_IDS_PER_REQUEST;i++)
    {
        if(i == 0)
        {
            strcpy(szTmp + nLen, "?)");
            nLen += 2;
        }
        else
        {
            strcpy(szTmp + nLen -1, ",?)");
            nLen += 2;
        }
        rc = sqlite3_prepare( hDB, szTmp, -1, &pahSelectWayStmt[i], NULL );
        if( rc != SQLITE_OK )
        {
            CPLError( CE_Failure, CPLE_AppDefined,
                    "sqlite3_prepare() failed :  %s", sqlite3_errmsg(hDB) );
            return FALSE;
        }
    }

    return TRUE;
}

/************************************************************************/
/*                           StartTransaction()                         */
/************************************************************************/

int OGROSMDataSource::StartTransaction()
{
    if( bInTransaction )
        return FALSE;

    char* pszErrMsg = NULL;
    int rc = sqlite3_exec( hDB, "BEGIN", NULL, NULL, &pszErrMsg );
    if( rc != SQLITE_OK )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Unable to start transaction : %s", pszErrMsg );
        sqlite3_free( pszErrMsg );
        return FALSE;
    }

    bInTransaction = TRUE;

    return TRUE;
}

/************************************************************************/
/*                           CommitTransaction()                        */
/************************************************************************/

int OGROSMDataSource::CommitTransaction()
{
    if( !bInTransaction )
        return FALSE;

    bInTransaction = FALSE;

    char* pszErrMsg = NULL;
    int rc = sqlite3_exec( hDB, "COMMIT", NULL, NULL, &pszErrMsg );
    if( rc != SQLITE_OK )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Unable to commit transaction : %s", pszErrMsg );
        sqlite3_free( pszErrMsg );
        return FALSE;
    }

    return TRUE;
}

/************************************************************************/
/*                           ParseConf()                                */
/************************************************************************/

int OGROSMDataSource::ParseConf()
{
    const char *pszFilename = CPLGetConfigOption("OSM_CONFIG_FILE", NULL);
    if( pszFilename == NULL )
        pszFilename = CPLFindFile( "gdal", "osmconf.ini" );
    if( pszFilename == NULL )
    {
        CPLError(CE_Warning, CPLE_AppDefined, "Cannot find osmconf.ini configuration file");
        return FALSE;
    }

    VSILFILE* fpConf = VSIFOpenL(pszFilename, "rb");
    if( fpConf == NULL )
        return FALSE;

    const char* pszLine;
    int iCurLayer = -1;

    int i;

    while((pszLine = CPLReadLine2L(fpConf, -1, NULL)) != NULL)
    {
        if(pszLine[0] == '[' && pszLine[strlen(pszLine)-1] == ']' )
        {
            iCurLayer = -1;
            pszLine ++;
            ((char*)pszLine)[strlen(pszLine)-1] = '\0'; /* Evil but OK */
            for(i = 0; i < nLayers; i++)
            {
                if( strcmp(pszLine, papoLayers[i]->GetName()) == 0 )
                {
                    iCurLayer = i;
                    break;
                }
            }
            if( iCurLayer < 0 )
            {
                CPLError(CE_Warning, CPLE_AppDefined,
                         "Layer '%s' mentionned in %s is unknown to the driver",
                         pszLine, pszFilename);
            }
            continue;
        }

        if( strncmp(pszLine, "closed_ways_are_polygons=",
                    strlen("closed_ways_are_polygons=")) == 0)
        {
            char** papszTokens = CSLTokenizeString2(pszLine, "=", 0);
            if( CSLCount(papszTokens) == 2)
            {
                char** papszTokens2 = CSLTokenizeString2(papszTokens[1], ",", 0);
                for(int i=0;papszTokens2[i] != NULL;i++)
                {
                    aoSetClosedWaysArePolygons.insert(papszTokens2[i]);
                }
                CSLDestroy(papszTokens2);
            }
            CSLDestroy(papszTokens);
        }

        else if(strncmp(pszLine, "report_all_nodes=", strlen("report_all_nodes=")) == 0)
        {
            if( strcmp(pszLine + strlen("report_all_nodes="), "no") == 0 )
            {
                bReportAllNodes = FALSE;
            }
            else if( strcmp(pszLine + strlen("report_all_nodes="), "yes") == 0 )
            {
                bReportAllNodes = TRUE;
            }
        }

        else if(strncmp(pszLine, "report_all_ways=", strlen("report_all_ways=")) == 0)
        {
            if( strcmp(pszLine + strlen("report_all_ways="), "no") == 0 )
            {
                bReportAllWays = FALSE;
            }
            else if( strcmp(pszLine + strlen("report_all_ways="), "yes") == 0 )
            {
                bReportAllWays = TRUE;
            }
        }

        else if(strncmp(pszLine, "attribute_name_laundering=", strlen("attribute_name_laundering=")) == 0)
        {
            if( strcmp(pszLine + strlen("attribute_name_laundering="), "no") == 0 )
            {
                bAttributeNameLaundering = FALSE;
            }
            else if( strcmp(pszLine + strlen("attribute_name_laundering="), "yes") == 0 )
            {
                bAttributeNameLaundering = TRUE;
            }
        }

        else if( iCurLayer >= 0 )
        {
            char** papszTokens = CSLTokenizeString2(pszLine, "=", 0);
            if( CSLCount(papszTokens) == 2 && strcmp(papszTokens[0], "other_tags") == 0 )
            {
                if( strcmp(papszTokens[1], "no") == 0 )
                    papoLayers[iCurLayer]->SetHasOtherTags(FALSE);
                else if( strcmp(papszTokens[1], "yes") == 0 )
                    papoLayers[iCurLayer]->SetHasOtherTags(TRUE);
            }
            else if( CSLCount(papszTokens) == 2 && strcmp(papszTokens[0], "osm_id") == 0 )
            {
                if( strcmp(papszTokens[1], "no") == 0 )
                    papoLayers[iCurLayer]->SetHasOSMId(FALSE);
                else if( strcmp(papszTokens[1], "yes") == 0 )
                {
                    papoLayers[iCurLayer]->SetHasOSMId(TRUE);
                    papoLayers[iCurLayer]->AddField("osm_id", OFTString);
                }
            }
            else if( CSLCount(papszTokens) == 2 && strcmp(papszTokens[0], "osm_version") == 0 )
            {
                if( strcmp(papszTokens[1], "no") == 0 )
                    papoLayers[iCurLayer]->SetHasVersion(FALSE);
                else if( strcmp(papszTokens[1], "yes") == 0 )
                {
                    papoLayers[iCurLayer]->SetHasVersion(TRUE);
                    papoLayers[iCurLayer]->AddField("osm_version", OFTInteger);
                }
            }
            else if( CSLCount(papszTokens) == 2 && strcmp(papszTokens[0], "osm_timestamp") == 0 )
            {
                if( strcmp(papszTokens[1], "no") == 0 )
                    papoLayers[iCurLayer]->SetHasTimestamp(FALSE);
                else if( strcmp(papszTokens[1], "yes") == 0 )
                {
                    papoLayers[iCurLayer]->SetHasTimestamp(TRUE);
                    papoLayers[iCurLayer]->AddField("osm_timestamp", OFTDateTime);
                }
            }
            else if( CSLCount(papszTokens) == 2 && strcmp(papszTokens[0], "osm_uid") == 0 )
            {
                if( strcmp(papszTokens[1], "no") == 0 )
                    papoLayers[iCurLayer]->SetHasUID(FALSE);
                else if( strcmp(papszTokens[1], "yes") == 0 )
                {
                    papoLayers[iCurLayer]->SetHasUID(TRUE);
                    papoLayers[iCurLayer]->AddField("osm_uid", OFTInteger);
                }
            }
            else if( CSLCount(papszTokens) == 2 && strcmp(papszTokens[0], "osm_user") == 0 )
            {
                if( strcmp(papszTokens[1], "no") == 0 )
                    papoLayers[iCurLayer]->SetHasUser(FALSE);
                else if( strcmp(papszTokens[1], "yes") == 0 )
                {
                    papoLayers[iCurLayer]->SetHasUser(TRUE);
                    papoLayers[iCurLayer]->AddField("osm_user", OFTString);
                }
            }
            else if( CSLCount(papszTokens) == 2 && strcmp(papszTokens[0], "osm_changeset") == 0 )
            {
                if( strcmp(papszTokens[1], "no") == 0 )
                    papoLayers[iCurLayer]->SetHasChangeset(FALSE);
                else if( strcmp(papszTokens[1], "yes") == 0 )
                {
                    papoLayers[iCurLayer]->SetHasChangeset(TRUE);
                    papoLayers[iCurLayer]->AddField("osm_changeset", OFTInteger);
                }
            }
            else if( CSLCount(papszTokens) == 2 && strcmp(papszTokens[0], "attributes") == 0 )
            {
                char** papszTokens2 = CSLTokenizeString2(papszTokens[1], ",", 0);
                for(int i=0;papszTokens2[i] != NULL;i++)
                {
                    papoLayers[iCurLayer]->AddField(papszTokens2[i], OFTString);
                }
                CSLDestroy(papszTokens2);
            }
            else if ( CSLCount(papszTokens) == 2 && strcmp(papszTokens[0], "unsignificant") == 0 )
            {
                char** papszTokens2 = CSLTokenizeString2(papszTokens[1], ",", 0);
                for(int i=0;papszTokens2[i] != NULL;i++)
                {
                    papoLayers[iCurLayer]->aoSetUnsignificantKeys.insert(papszTokens2[i]);
                }
                CSLDestroy(papszTokens2);
            }
            else if ( CSLCount(papszTokens) == 2 && strcmp(papszTokens[0], "ignore") == 0 )
            {
                char** papszTokens2 = CSLTokenizeString2(papszTokens[1], ",", 0);
                for(int i=0;papszTokens2[i] != NULL;i++)
                {
                    papoLayers[iCurLayer]->aoSetIgnoreKeys.insert(papszTokens2[i]);
                    papoLayers[iCurLayer]->aoSetWarnKeys.insert(papszTokens2[i]);
                }
                CSLDestroy(papszTokens2);
            }
            CSLDestroy(papszTokens);
        }
    }

    for(i=0;i<nLayers;i++)
    {
        if( papoLayers[i]->HasOtherTags() )
            papoLayers[i]->AddField("other_tags", OFTString);
    }

    VSIFCloseL(fpConf);

    return TRUE;
}

/************************************************************************/
/*                           ResetReading()                            */
/************************************************************************/

int OGROSMDataSource::ResetReading()
{
    if( hDB == NULL )
        return FALSE;
    if( bCustomIndexing && fpNodes == NULL )
        return FALSE;

    OSM_ResetReading(psParser);

    char* pszErrMsg = NULL;
    int rc = sqlite3_exec( hDB, "DELETE FROM nodes", NULL, NULL, &pszErrMsg );
    if( rc != SQLITE_OK )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Unable to DELETE FROM nodes : %s", pszErrMsg );
        sqlite3_free( pszErrMsg );
        return FALSE;
    }

    rc = sqlite3_exec( hDB, "DELETE FROM ways", NULL, NULL, &pszErrMsg );
    if( rc != SQLITE_OK )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Unable to DELETE FROM ways : %s", pszErrMsg );
        sqlite3_free( pszErrMsg );
        return FALSE;
    }

    {
        int i;
        for( i = 0; i < nWayFeaturePairs; i++)
        {
            delete pasWayFeaturePairs[i].poFeature;
        }
        nWayFeaturePairs = 0;
        nUnsortedReqIds = 0;
        nReqIds = 0;
        nAccumulatedTags = 0;
        nNonRedundantValuesLen = 0;

        for(i=0;i<(int)asKeys.size();i++)
            CPLFree(asKeys[i]);
        asKeys.resize(0);
        std::map<CPLString, KeyDesc>::iterator oIter = aoMapIndexedKeys.begin();
        for(; oIter != aoMapIndexedKeys.end(); ++oIter)
        {
            KeyDesc& sKD = oIter->second;
            for(i=0;i<(int)sKD.asValues.size();i++)
                CPLFree(sKD.asValues[i]);
        }
        aoMapIndexedKeys.clear();
        nNextKeyIndex = 0;
    }

    if( bCustomIndexing )
    {
        nPrevNodeId = -1;
        nBucketOld = -1;
        nOffInBucketReducedOld = -1;

        VSIFSeekL(fpNodes, 0, SEEK_SET);
        VSIFTruncateL(fpNodes, 0);
        nNodesFileSize = 0;

        memset(pabySector, 0, SECTOR_SIZE);
        for(int i = 0; i < BUCKET_COUNT; i++)
        {
            papsBuckets[i].nOff = -1;
            memset(papsBuckets[i].pabyBitmap, 0, BUCKET_BITMAP_SIZE);
        }
    }

    for(int i=0;i<nLayers;i++)
    {
        papoLayers[i]->ForceResetReading();
    }

    bStopParsing = FALSE;

    return TRUE;
}

/************************************************************************/
/*                           ParseNextChunk()                           */
/************************************************************************/

int OGROSMDataSource::ParseNextChunk()
{
    if( bStopParsing )
        return FALSE;

    bHasParsedFirstChunk = TRUE;
    bFeatureAdded = FALSE;
    while( TRUE )
    {
#ifdef DEBUG_MEM_USAGE
        static int counter = 0;
        counter ++;
        if ((counter % 1000) == 0)
            CPLDebug("OSM", "GetMaxTotalAllocs() = " CPL_FRMT_GUIB, (GUIntBig)GetMaxTotalAllocs());
#endif

        OSMRetCode eRet = OSM_ProcessBlock(psParser);
        if( eRet == OSM_EOF || eRet == OSM_ERROR )
        {
            if( eRet == OSM_EOF )
            {
                if( nWayFeaturePairs != 0 )
                    ProcessWaysBatch();
            }
            else
            {
                CPLError(CE_Failure, CPLE_AppDefined,
                         "An error occured during the parsing of data around byte " CPL_FRMT_GUIB,
                         OSM_GetBytesRead(psParser));
            }
            bStopParsing = TRUE;
            return FALSE;
        }
        else
        {
            if( bInMemoryTmpDB )
            {
                if( !TransferToDiskIfNecesserary() )
                    return FALSE;
            }

            if( bFeatureAdded )
                break;
        }
    }

    return TRUE;
}

/************************************************************************/
/*                    TransferToDiskIfNecesserary()                     */
/************************************************************************/

int OGROSMDataSource::TransferToDiskIfNecesserary()
{
    if( bInMemoryNodesFile )
    {
        if( nNodesFileSize / 1024 / 1024 > 3 * nMaxSizeForInMemoryDBInMB / 4 )
        {
            bInMemoryNodesFile = FALSE;

            VSIFCloseL(fpNodes);
            fpNodes = NULL;

            CPLString osNewTmpDBName;
            osNewTmpDBName = CPLGenerateTempFilename("osm_tmp_nodes");

            CPLDebug("OSM", "%s too big for RAM. Transfering it onto disk in %s",
                     osNodesFilename.c_str(), osNewTmpDBName.c_str());

            if( CPLCopyFile( osNewTmpDBName, osNodesFilename ) != 0 )
            {
                CPLError(CE_Failure, CPLE_AppDefined,
                         "Cannot copy %s to %s",
                         osNodesFilename.c_str(), osNewTmpDBName.c_str() );
                VSIUnlink(osNewTmpDBName);
                bStopParsing = TRUE;
                return FALSE;
            }

            VSIUnlink(osNodesFilename);

            if( bInMemoryTmpDB )
            {
                /* Try to grow the sqlite in memory-db to the full space now */
                /* it has been freed. */
                VSILFILE* fp = VSIFOpenL(osTmpDBName, "rb+");
                if( fp )
                {
                    VSIFSeekL(fp, 0, SEEK_END);
                    vsi_l_offset nCurSize = VSIFTellL(fp);
                    GIntBig nNewSize = ((GIntBig)nMaxSizeForInMemoryDBInMB) * 1024 * 1024;
                    CPLPushErrorHandler(CPLQuietErrorHandler);
                    int bSuccess = VSIFSeekL(fp, (vsi_l_offset) nNewSize, SEEK_SET) == 0;
                    CPLPopErrorHandler();

                    if( bSuccess )
                        VSIFTruncateL(fp, nCurSize);

                    VSIFCloseL(fp);
                }
            }

            osNodesFilename = osNewTmpDBName;

            fpNodes = VSIFOpenL(osNodesFilename, "rb+");
            if( fpNodes == NULL )
            {
                bStopParsing = TRUE;
                return FALSE;
            }

            VSIFSeekL(fpNodes, 0, SEEK_END);

            /* On Unix filesystems, you can remove a file even if it */
            /* opened */
            const char* pszVal = CPLGetConfigOption("OSM_UNLINK_TMPFILE", "YES");
            if( EQUAL(pszVal, "YES") )
            {
                CPLPushErrorHandler(CPLQuietErrorHandler);
                bMustUnlinkNodesFile = VSIUnlink( osNodesFilename ) != 0;
                CPLPopErrorHandler();
            }
        }
    }

    if( bInMemoryTmpDB )
    {
        VSIStatBufL sStat;

        int nLimitMB = nMaxSizeForInMemoryDBInMB;
        if( bCustomIndexing && bInMemoryNodesFile )
            nLimitMB = nLimitMB * 1 / 4;

        if( VSIStatL( osTmpDBName, &sStat ) == 0 &&
            sStat.st_size / 1024 / 1024 > nLimitMB )
        {
            bInMemoryTmpDB = FALSE;

            CloseDB();

            CPLString osNewTmpDBName;
            int rc;

            osNewTmpDBName = CPLGenerateTempFilename("osm_tmp");

            CPLDebug("OSM", "%s too big for RAM. Transfering it onto disk in %s",
                     osTmpDBName.c_str(), osNewTmpDBName.c_str());

            if( CPLCopyFile( osNewTmpDBName, osTmpDBName ) != 0 )
            {
                CPLError(CE_Failure, CPLE_AppDefined,
                         "Cannot copy %s to %s",
                         osTmpDBName.c_str(), osNewTmpDBName.c_str() );
                VSIUnlink(osNewTmpDBName);
                bStopParsing = TRUE;
                return FALSE;
            }

            VSIUnlink(osTmpDBName);

            osTmpDBName = osNewTmpDBName;

#ifdef HAVE_SQLITE_VFS
            rc = sqlite3_open_v2( osTmpDBName.c_str(), &hDB,
                                  SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX,
                                  NULL );
#else
            rc = sqlite3_open( osTmpDBName.c_str(), &hDB );
#endif
            if( rc != SQLITE_OK )
            {
                CPLError( CE_Failure, CPLE_OpenFailed,
                        "sqlite3_open(%s) failed: %s",
                        osTmpDBName.c_str(), sqlite3_errmsg( hDB ) );
                bStopParsing = TRUE;
                CloseDB();
                return FALSE;
            }

            /* On Unix filesystems, you can remove a file even if it */
            /* opened */
            const char* pszVal = CPLGetConfigOption("OSM_UNLINK_TMPFILE", "YES");
            if( EQUAL(pszVal, "YES") )
            {
                CPLPushErrorHandler(CPLQuietErrorHandler);
                bMustUnlink = VSIUnlink( osTmpDBName ) != 0;
                CPLPopErrorHandler();
            }

            if( !SetDBOptions() || !CreatePreparedStatements() )
            {
                bStopParsing = TRUE;
                CloseDB();
                return FALSE;
            }
        }
    }

    return TRUE;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGROSMDataSource::TestCapability( const char * pszCap )

{
    return FALSE;
}

/************************************************************************/
/*                              GetLayer()                              */
/************************************************************************/

OGRLayer *OGROSMDataSource::GetLayer( int iLayer )

{
    if( iLayer < 0 || iLayer >= nLayers )
        return NULL;
    else
        return papoLayers[iLayer];
}

/************************************************************************/
/*                             GetExtent()                              */
/************************************************************************/

OGRErr OGROSMDataSource::GetExtent( OGREnvelope *psExtent )
{
    if (!bHasParsedFirstChunk)
    {
        bHasParsedFirstChunk = TRUE;
        OSM_ProcessBlock(psParser);
    }

    if (bExtentValid)
    {
        memcpy(psExtent, &sExtent, sizeof(sExtent));
        return OGRERR_NONE;
    }

    return OGRERR_FAILURE;
}


/************************************************************************/
/*                   OGROSMSingleFeatureLayer                           */
/************************************************************************/

class OGROSMSingleFeatureLayer : public OGRLayer
{
  private:
    int                 nVal;
    char               *pszVal;
    OGRFeatureDefn     *poFeatureDefn;
    int                 iNextShapeId;

  public:
                        OGROSMSingleFeatureLayer( const char* pszLayerName,
                                                     int nVal );
                        OGROSMSingleFeatureLayer( const char* pszLayerName,
                                                     const char *pszVal );
                        ~OGROSMSingleFeatureLayer();

    virtual void        ResetReading() { iNextShapeId = 0; }
    virtual OGRFeature *GetNextFeature();
    virtual OGRFeatureDefn *GetLayerDefn() { return poFeatureDefn; }
    virtual int         TestCapability( const char * ) { return FALSE; }
};


/************************************************************************/
/*                    OGROSMSingleFeatureLayer()                        */
/************************************************************************/

OGROSMSingleFeatureLayer::OGROSMSingleFeatureLayer(  const char* pszLayerName,
                                                     int nVal )
{
    poFeatureDefn = new OGRFeatureDefn( "SELECT" );
    poFeatureDefn->Reference();
    OGRFieldDefn oField( pszLayerName, OFTInteger );
    poFeatureDefn->AddFieldDefn( &oField );

    iNextShapeId = 0;
    this->nVal = nVal;
    pszVal = NULL;
}

/************************************************************************/
/*                    OGROSMSingleFeatureLayer()                        */
/************************************************************************/

OGROSMSingleFeatureLayer::OGROSMSingleFeatureLayer(  const char* pszLayerName,
                                                     const char *pszVal )
{
    poFeatureDefn = new OGRFeatureDefn( "SELECT" );
    poFeatureDefn->Reference();
    OGRFieldDefn oField( pszLayerName, OFTString );
    poFeatureDefn->AddFieldDefn( &oField );

    iNextShapeId = 0;
    nVal = 0;
    this->pszVal = CPLStrdup(pszVal);
}

/************************************************************************/
/*                    ~OGROSMSingleFeatureLayer()                       */
/************************************************************************/

OGROSMSingleFeatureLayer::~OGROSMSingleFeatureLayer()
{
    poFeatureDefn->Release();
    CPLFree(pszVal);
}

/************************************************************************/
/*                           GetNextFeature()                           */
/************************************************************************/

OGRFeature * OGROSMSingleFeatureLayer::GetNextFeature()
{
    if (iNextShapeId != 0)
        return NULL;

    OGRFeature* poFeature = new OGRFeature(poFeatureDefn);
    if (pszVal)
        poFeature->SetField(0, pszVal);
    else
        poFeature->SetField(0, nVal);
    poFeature->SetFID(iNextShapeId ++);
    return poFeature;
}

/************************************************************************/
/*                             ExecuteSQL()                             */
/************************************************************************/

OGRLayer * OGROSMDataSource::ExecuteSQL( const char *pszSQLCommand,
                                         OGRGeometry *poSpatialFilter,
                                         const char *pszDialect )

{
/* -------------------------------------------------------------------- */
/*      Special GetBytesRead() command                                  */
/* -------------------------------------------------------------------- */
    if (strcmp(pszSQLCommand, "GetBytesRead()") == 0)
    {
        char szVal[64];
        sprintf(szVal, CPL_FRMT_GUIB, OSM_GetBytesRead(psParser));
        return new OGROSMSingleFeatureLayer( "GetBytesRead", szVal );
    }

    if( poResultSetLayer != NULL )
    {
        CPLError(CE_Failure, CPLE_NotSupported,
                 "A SQL result layer is still in use. Please delete it first");
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Special SET interest_layers = command                           */
/* -------------------------------------------------------------------- */
    if (strncmp(pszSQLCommand, "SET interest_layers =", 21) == 0)
    {
        char** papszTokens = CSLTokenizeString2(pszSQLCommand + 21, ",", CSLT_STRIPLEADSPACES | CSLT_STRIPENDSPACES);
        int i;

        for(i=0; i < nLayers; i++)
        {
            papoLayers[i]->SetDeclareInterest(FALSE);
        }

        for(i=0; papszTokens[i] != NULL; i++)
        {
            OGROSMLayer* poLayer = (OGROSMLayer*) GetLayerByName(papszTokens[i]);
            if( poLayer != NULL )
            {
                poLayer->SetDeclareInterest(TRUE);
            }
        }

        if( papoLayers[IDX_LYR_POINTS]->IsUserInterested() &&
            !papoLayers[IDX_LYR_LINES]->IsUserInterested() &&
            !papoLayers[IDX_LYR_POLYGONS]->IsUserInterested() &&
            !papoLayers[IDX_LYR_MULTILINESTRINGS]->IsUserInterested() &&
            !papoLayers[IDX_LYR_MULTIPOLYGONS]->IsUserInterested() &&
            !papoLayers[IDX_LYR_OTHER_RELATIONS]->IsUserInterested())
        {
            if( CPLGetConfigOption("OSM_INDEX_POINTS", NULL) == NULL )
            {
                CPLDebug("OSM", "Disabling indexing of nodes");
                bIndexPoints = FALSE;
            }
            if( CPLGetConfigOption("OSM_USE_POINTS_INDEX", NULL) == NULL )
            {
                bUsePointsIndex = FALSE;
            }
            if( CPLGetConfigOption("OSM_INDEX_WAYS", NULL) == NULL )
            {
                CPLDebug("OSM", "Disabling indexing of ways");
                bIndexWays = FALSE;
            }
            if( CPLGetConfigOption("OSM_USE_WAYS_INDEX", NULL) == NULL )
            {
                bUseWaysIndex = FALSE;
            }
        }
        else if( (papoLayers[IDX_LYR_LINES]->IsUserInterested() ||
                  papoLayers[IDX_LYR_POLYGONS]->IsUserInterested()) &&
                 !papoLayers[IDX_LYR_MULTILINESTRINGS]->IsUserInterested() &&
                 !papoLayers[IDX_LYR_MULTIPOLYGONS]->IsUserInterested() &&
                 !papoLayers[IDX_LYR_OTHER_RELATIONS]->IsUserInterested() )
        {
            if( CPLGetConfigOption("OSM_INDEX_WAYS", NULL) == NULL )
            {
                CPLDebug("OSM", "Disabling indexing of ways");
                bIndexWays = FALSE;
            }
            if( CPLGetConfigOption("OSM_USE_WAYS_INDEX", NULL) == NULL )
            {
                bUseWaysIndex = FALSE;
            }
        }

        CSLDestroy(papszTokens);

        return NULL;
    }

    while(*pszSQLCommand == ' ')
        pszSQLCommand ++;

    /* Try to analyse the SQL command to get the interest table */
    if( EQUALN(pszSQLCommand, "SELECT", 5) )
    {
        swq_select sSelectInfo;

        CPLPushErrorHandler(CPLQuietErrorHandler);
        CPLErr eErr = sSelectInfo.preparse( pszSQLCommand );
        CPLPopErrorHandler();

        if( eErr == CPLE_None )
        {
            int bOnlyFromThisDataSource = TRUE;

            swq_select* pCurSelect = &sSelectInfo;
            while(pCurSelect != NULL && bOnlyFromThisDataSource)
            {
                for( int iTable = 0; iTable < pCurSelect->table_count; iTable++ )
                {
                    swq_table_def *psTableDef = pCurSelect->table_defs + iTable;
                    if( psTableDef->data_source != NULL )
                    {
                        bOnlyFromThisDataSource = FALSE;
                        break;
                    }
                }

                pCurSelect = pCurSelect->poOtherSelect;
            }

            if( bOnlyFromThisDataSource )
            {
                int bLayerAlreadyAdded = FALSE;
                CPLString osInterestLayers = "SET interest_layers =";

                pCurSelect = &sSelectInfo;
                while(pCurSelect != NULL && bOnlyFromThisDataSource)
                {
                    for( int iTable = 0; iTable < pCurSelect->table_count; iTable++ )
                    {
                        swq_table_def *psTableDef = pCurSelect->table_defs + iTable;
                        if( bLayerAlreadyAdded ) osInterestLayers += ",";
                        bLayerAlreadyAdded = TRUE;
                        osInterestLayers += psTableDef->table_name;
                    }
                    pCurSelect = pCurSelect->poOtherSelect;
                }

                /* Backup current optimization parameters */
                abSavedDeclaredInterest.resize(0);
                for(int i=0; i < nLayers; i++)
                {
                    abSavedDeclaredInterest.push_back(papoLayers[i]->IsUserInterested());
                }
                bIndexPointsBackup = bIndexPoints;
                bUsePointsIndexBackup = bUsePointsIndex;
                bIndexWaysBackup = bIndexWays;
                bUseWaysIndexBackup = bUseWaysIndex;

                /* Update optimization parameters */
                ExecuteSQL(osInterestLayers, NULL, NULL);

                ResetReading();

                /* Run the request */
                OGRLayer* poRet = OGRDataSource::ExecuteSQL( pszSQLCommand,
                                                             poSpatialFilter,
                                                             pszDialect );

                poResultSetLayer = poRet;

                /* If the user explicitely run a COUNT() request, then do it ! */
                if( poResultSetLayer )
                    bIsFeatureCountEnabled = TRUE;

                return poRet;
            }
        }
    }

    return OGRDataSource::ExecuteSQL( pszSQLCommand,
                                      poSpatialFilter,
                                      pszDialect );
}

/************************************************************************/
/*                          ReleaseResultSet()                          */
/************************************************************************/

void OGROSMDataSource::ReleaseResultSet( OGRLayer * poLayer )

{
    if( poLayer != NULL && poLayer == poResultSetLayer )
    {
        poResultSetLayer = NULL;

        bIsFeatureCountEnabled = FALSE;

        /* Restore backup'ed optimization parameters */
        for(int i=0; i < nLayers; i++)
        {
            papoLayers[i]->SetDeclareInterest(abSavedDeclaredInterest[i]);
        }
        if( bIndexPointsBackup && !bIndexPoints )
            CPLDebug("OSM", "Re-enabling indexing of nodes");
        bIndexPoints = bIndexPointsBackup;
        bUsePointsIndex = bUsePointsIndexBackup;
        if( bIndexWaysBackup && !bIndexWays )
            CPLDebug("OSM", "Re-enabling indexing of ways");
        bIndexWays = bIndexWaysBackup;
        bUseWaysIndex = bUseWaysIndexBackup;
        abSavedDeclaredInterest.resize(0);
    }

    delete poLayer;
}

/************************************************************************/
/*                         IsInterleavedReading()                       */
/************************************************************************/

int OGROSMDataSource::IsInterleavedReading()
{
    if( bInterleavedReading < 0 )
    {
        bInterleavedReading = CSLTestBoolean(
                        CPLGetConfigOption("OGR_INTERLEAVED_READING", "NO"));
        CPLDebug("OSM", "OGR_INTERLEAVED_READING = %d", bInterleavedReading);
    }
    return bInterleavedReading;
}