/******************************************************************************
 * $Id$
 *
 * Project:  EarthWatch .TIL Driver
 * Purpose:  Implementation of the TILDataset class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2009, Frank Warmerdam
 * Copyright (c) 2009-2011, Even Rouault <even dot rouault at mines-paris dot org>
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

#include "gdal_pam.h"
#include "gdal_proxy.h"
#include "ogr_spatialref.h"
#include "cpl_string.h"
#include "vrtdataset.h"
#include "cpl_multiproc.h"
#include "cplkeywordparser.h"

#include "rsmd_reader.h"

CPL_CVSID("$Id$");

/************************************************************************/
/* ==================================================================== */
/*				TILDataset				*/
/* ==================================================================== */
/************************************************************************/

class CPL_DLL TILDataset : public GDALPamDataset
{
    VRTDataset *poVRTDS;
    std::vector<GDALDataset *> apoTileDS;

    //CPLString                  osRPBFilename;
    //CPLString                  osIMDFilename;
	
	RSMDReader *pRSMDReader;
  protected:
    virtual int         CloseDependentDatasets();

  public:
    TILDataset();
    ~TILDataset();

    virtual char **GetFileList(void);

    static GDALDataset *Open( GDALOpenInfo * );
    static int Identify( GDALOpenInfo *poOpenInfo );
};

/************************************************************************/
/* ==================================================================== */
/*                            TILRasterBand                             */
/* ==================================================================== */
/************************************************************************/

class TILRasterBand : public GDALPamRasterBand
{
    friend class TILDataset;

    VRTSourcedRasterBand *poVRTBand;

  public:
                   TILRasterBand( TILDataset *, int, VRTSourcedRasterBand * );
    virtual       ~TILRasterBand() {};

    virtual CPLErr IReadBlock( int, int, void * );
    virtual CPLErr IRasterIO( GDALRWFlag, int, int, int, int,
                              void *, int, int, GDALDataType,
                              int, int );
};

/************************************************************************/
/*                           TILRasterBand()                            */
/************************************************************************/

TILRasterBand::TILRasterBand( TILDataset *poTILDS, int nBand, 
                              VRTSourcedRasterBand *poVRTBand )

{
    this->poDS = poTILDS;
    this->poVRTBand = poVRTBand;
    this->nBand = nBand;
    this->eDataType = poVRTBand->GetRasterDataType();

    poVRTBand->GetBlockSize( &nBlockXSize, &nBlockYSize );
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr TILRasterBand::IReadBlock( int iBlockX, int iBlockY, void *pBuffer )

{
    return poVRTBand->ReadBlock( iBlockX, iBlockY, pBuffer );
}

/************************************************************************/
/*                             IRasterIO()                              */
/************************************************************************/

CPLErr TILRasterBand::IRasterIO( GDALRWFlag eRWFlag,
                                 int nXOff, int nYOff, int nXSize, int nYSize,
                                 void * pData, int nBufXSize, int nBufYSize,
                                 GDALDataType eBufType,
                                 int nPixelSpace, int nLineSpace )

{
    if(GetOverviewCount() > 0)
    {
        return GDALPamRasterBand::IRasterIO( eRWFlag, nXOff, nYOff, nXSize, nYSize,
                                 pData, nBufXSize, nBufYSize, eBufType,
                                 nPixelSpace, nLineSpace );
    }
    else //if not exist TIL overviews, try to use band source overviews
    {
        return poVRTBand->IRasterIO( eRWFlag, nXOff, nYOff, nXSize, nYSize,
                                 pData, nBufXSize, nBufYSize, eBufType,
                                 nPixelSpace, nLineSpace );
    }
}

/************************************************************************/
/* ==================================================================== */
/*                             TILDataset                               */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                             TILDataset()                             */
/************************************************************************/

TILDataset::TILDataset()

{
    poVRTDS = NULL;
}

/************************************************************************/
/*                            ~TILDataset()                             */
/************************************************************************/

TILDataset::~TILDataset()

{
	delete pRSMDReader;
    CloseDependentDatasets();
}

/************************************************************************/
/*                        CloseDependentDatasets()                      */
/************************************************************************/

int TILDataset::CloseDependentDatasets()
{
    int bHasDroppedRef = GDALPamDataset::CloseDependentDatasets();

    if( poVRTDS )
    {
        bHasDroppedRef = TRUE;
        delete poVRTDS;
        poVRTDS = NULL;
    }

    while( !apoTileDS.empty() )
    {
        GDALClose( (GDALDatasetH) apoTileDS.back() );
        apoTileDS.pop_back();
    }

    return bHasDroppedRef;
}

/************************************************************************/
/*                              Identify()                              */
/************************************************************************/

int TILDataset::Identify( GDALOpenInfo *poOpenInfo )

{
    if( poOpenInfo->nHeaderBytes < 200 
        || !EQUAL(CPLGetExtension(poOpenInfo->pszFilename),"TIL") )
        return FALSE;

    if( strstr((const char *) poOpenInfo->pabyHeader,"numTiles") == NULL )
        return FALSE;
    else
        return TRUE;
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *TILDataset::Open( GDALOpenInfo * poOpenInfo )

{
    if( !Identify( poOpenInfo ) )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Confirm the requested access is supported.                      */
/* -------------------------------------------------------------------- */
    if( poOpenInfo->eAccess == GA_Update )
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "The TIL driver does not support update access to existing"
                  " datasets.\n" );
        return NULL;
    }
    
    CPLString osDirname = CPLGetDirname(poOpenInfo->pszFilename);
	const std::map<CPLString, CPLStringList>* pmMetadata = NULL;
	CPLStringList slIMD;
	RSMDReader* pRSMDReader = GetRSMDReader(poOpenInfo->pszFilename);
	if(pRSMDReader)
	{
		pmMetadata = pRSMDReader->GetMetadata();
		if(!pmMetadata)
		{
			delete pRSMDReader;
			CPLError( CE_Failure, CPLE_OpenFailed,
					  "Unable to open .TIL dataset due to missing .IMD file." );
			return NULL;
		}
	
/* -------------------------------------------------------------------- */
/*      Try to find the corresponding .IMD file.                        */
/* -------------------------------------------------------------------- */

		std::map<CPLString, CPLStringList>::const_iterator it = pmMetadata->find(MDPrefix_IMD);
		if(it == pmMetadata->end())
		{
			delete pRSMDReader;
			CPLError( CE_Failure, CPLE_OpenFailed,
					  "Unable to open .TIL dataset due to missing .IMD file." );
			return NULL;
		}
		
		slIMD = it->second;
	}

    if( slIMD.List() == NULL )
    {
		if(pRSMDReader)
			delete pRSMDReader;
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "Unable to open .TIL dataset due to missing .IMD file." );
        return NULL;
    }

    if( CSLFetchNameValue( slIMD.List(), "numRows" ) == NULL
        || CSLFetchNameValue( slIMD.List(), "numColumns" ) == NULL
        || CSLFetchNameValue( slIMD.List(), "bitsPerPixel" ) == NULL )
    {
		if(pRSMDReader)
			delete pRSMDReader;
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "Missing a required field in the .IMD file." );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Try to load and parse the .TIL file.                            */
/* -------------------------------------------------------------------- */
    VSILFILE *fp = VSIFOpenL( poOpenInfo->pszFilename, "r" );
    
    if( fp == NULL )
    {
		if(pRSMDReader)
			delete pRSMDReader;
        return NULL;
    }

    CPLKeywordParser oParser;

    if( !oParser.Ingest( fp ) )
    {
		if(pRSMDReader)
			delete pRSMDReader;
        VSIFCloseL( fp );
        return NULL;
    }

    VSIFCloseL( fp );

    char **papszTIL = oParser.GetAllKeywords();

/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    TILDataset 	*poDS;

    poDS = new TILDataset();

    //poDS->osIMDFilename = osIMDFilename; 
    poDS->SetMetadata( slIMD.List(), "IMD" );
    poDS->nRasterXSize = atoi(CSLFetchNameValueDef(slIMD.List(),"numColumns","0"));
    poDS->nRasterYSize = atoi(CSLFetchNameValueDef(slIMD.List(),"numRows","0"));

	poDS->pRSMDReader = pRSMDReader;
	
    if (!GDALCheckDatasetDimensions(poDS->nRasterXSize, poDS->nRasterYSize))
    {
        delete poDS;
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      We need to open one of the images in order to establish         */
/*      details like the band count and types.                          */
/* -------------------------------------------------------------------- */
    GDALDataset *poTemplateDS;
    const char *pszFilename = CSLFetchNameValue( papszTIL, "TILE_1.filename" );
    if( pszFilename == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Missing TILE_1.filename in .TIL file." );
        delete poDS;
        return NULL;
    }

    // trim double quotes. 
    if( pszFilename[0] == '"' )
        pszFilename++;
    if( pszFilename[strlen(pszFilename)-1] == '"' )
        ((char *) pszFilename)[strlen(pszFilename)-1] = '\0';

    CPLString osFilename = CPLFormFilename(osDirname, pszFilename, NULL);
    poTemplateDS = (GDALDataset *) GDALOpen( osFilename, GA_ReadOnly );
    if( poTemplateDS == NULL || poTemplateDS->GetRasterCount() == 0)
    {
        delete poDS;
        if (poTemplateDS != NULL)
            GDALClose( poTemplateDS );
        return NULL;
    }

    GDALRasterBand *poTemplateBand = poTemplateDS->GetRasterBand(1);
    GDALDataType eDT = poTemplateBand->GetRasterDataType();
    int          nBandCount = poTemplateDS->GetRasterCount();

    //we suppose the first tile have the same projection as others (usually so)
    CPLString pszProjection(poTemplateDS->GetProjectionRef());
    if(!pszProjection.empty())
        poDS->SetProjection(pszProjection);

    //we suppose the first tile have the same GeoTransform as others (usually so)
    double      adfGeoTransform[6];
    if( poTemplateDS->GetGeoTransform( adfGeoTransform ) == CE_None )
    {
        adfGeoTransform[0] = CPLAtof(CSLFetchNameValueDef(slIMD.List(),"MAP_PROJECTED_PRODUCT.ULX","0"));
        adfGeoTransform[3] = CPLAtof(CSLFetchNameValueDef(slIMD.List(),"MAP_PROJECTED_PRODUCT.ULY","0"));
        poDS->SetGeoTransform(adfGeoTransform);
    }

    poTemplateBand = NULL;
    GDALClose( poTemplateDS );

/* -------------------------------------------------------------------- */
/*      Create and initialize the corresponding VRT dataset used to     */
/*      manage the tiled data access.                                   */
/* -------------------------------------------------------------------- */
    int iBand;
     
    poDS->poVRTDS = new VRTDataset(poDS->nRasterXSize,poDS->nRasterYSize);

    for( iBand = 0; iBand < nBandCount; iBand++ )
        poDS->poVRTDS->AddBand( eDT, NULL );

    /* Don't try to write a VRT file */
    poDS->poVRTDS->SetWritable(FALSE);

/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    for( iBand = 1; iBand <= nBandCount; iBand++ )
        poDS->SetBand( iBand, 
                       new TILRasterBand( poDS, iBand, 
                (VRTSourcedRasterBand *) poDS->poVRTDS->GetRasterBand(iBand)));

/* -------------------------------------------------------------------- */
/*      Add tiles as sources for each band.                             */
/* -------------------------------------------------------------------- */
    int nTileCount = atoi(CSLFetchNameValueDef(papszTIL,"numTiles","0"));
    int iTile = 0;

    for( iTile = 1; iTile <= nTileCount; iTile++ )
    {
        CPLString osKey;

        osKey.Printf( "TILE_%d.filename", iTile );
        pszFilename = CSLFetchNameValue( papszTIL, osKey );
        if( pszFilename == NULL )
        {
            CPLError( CE_Failure, CPLE_AppDefined,
                      "Missing TILE_%d.filename in .TIL file.", iTile );
            delete poDS;
            return NULL;
        }
        
        // trim double quotes. 
        if( pszFilename[0] == '"' )
            pszFilename++;
        if( pszFilename[strlen(pszFilename)-1] == '"' )
            ((char *) pszFilename)[strlen(pszFilename)-1] = '\0';
        osFilename = CPLFormFilename(osDirname, pszFilename, NULL);

        osKey.Printf( "TILE_%d.ULColOffset", iTile );
        int nULX = atoi(CSLFetchNameValueDef(papszTIL, osKey, "0"));
        
        osKey.Printf( "TILE_%d.ULRowOffset", iTile );
        int nULY = atoi(CSLFetchNameValueDef(papszTIL, osKey, "0"));

        osKey.Printf( "TILE_%d.LRColOffset", iTile );
        int nLRX = atoi(CSLFetchNameValueDef(papszTIL, osKey, "0"));
        
        osKey.Printf( "TILE_%d.LRRowOffset", iTile );
        int nLRY = atoi(CSLFetchNameValueDef(papszTIL, osKey, "0"));

        GDALDataset *poTileDS = 
            new GDALProxyPoolDataset( osFilename, 
                                      nLRX - nULX + 1, nLRY - nULY + 1 );
        if( poTileDS == NULL )
            continue;

        poDS->apoTileDS.push_back( poTileDS );

        for( iBand = 1; iBand <= nBandCount; iBand++ )
        {
            ((GDALProxyPoolDataset *) poTileDS)->
                AddSrcBandDescription( eDT, nLRX - nULX + 1, 1 );

            GDALRasterBand *poSrcBand = poTileDS->GetRasterBand(iBand);

            VRTSourcedRasterBand *poVRTBand = 
                (VRTSourcedRasterBand *) poDS->poVRTDS->GetRasterBand(iBand);
            
            poVRTBand->AddSimpleSource( poSrcBand,
                                        0, 0, 
                                        nLRX - nULX + 1, nLRY - nULY + 1, 
                                        nULX, nULY, 
                                        nLRX - nULX + 1, nLRY - nULY + 1 );
        }
    }

/* -------------------------------------------------------------------- */
/*      Set RPC and Commom IMD metadata.                                */
/* -------------------------------------------------------------------- */
	
	if(pmMetadata)
	{
		CPLStringList slRPCMD = pmMetadata->find(MDPrefix_RPC)->second;
		if( slRPCMD.size() != 0 )
		{
			poDS->SetMetadata( slRPCMD.List(), "RPC" );
		}


		CPLStringList slCommonIMD = pmMetadata->find(MDPrefix_Common_IMD)->second;
		if( slCommonIMD.size() != 0 )
		{
			poDS->SetMetadata( slCommonIMD.List(), "IMAGERY" );
		}
/* -------------------------------------------------------------------- */
/*      Cleanup                                                         */
/* -------------------------------------------------------------------- */

		delete pmMetadata; 
	}

/* -------------------------------------------------------------------- */
/*      Initialize any PAM information.                                 */
/* -------------------------------------------------------------------- */
    poDS->SetDescription( poOpenInfo->pszFilename );
    poDS->TryLoadXML();

/* -------------------------------------------------------------------- */
/*      Check for overviews.                                            */
/* -------------------------------------------------------------------- */
    poDS->oOvManager.Initialize( poDS, poOpenInfo->pszFilename );

    return( poDS );
}

/************************************************************************/
/*                            GetFileList()                             */
/************************************************************************/

char **TILDataset::GetFileList()

{
    unsigned int  i;
    char **papszFileList = GDALPamDataset::GetFileList();

    for( i = 0; i < apoTileDS.size(); i++ )
        papszFileList = CSLAddString( papszFileList,
                                      apoTileDS[i]->GetDescription() );
    
	CPLStringList slMDSources = pRSMDReader->GetSourceFileList();
	for(int i = 0; i < slMDSources.size(); i++)
		papszFileList = CSLAddString( papszFileList, slMDSources[i] );
	/*
    papszFileList = CSLAddString( papszFileList, osIMDFilename );


    if( osRPBFilename != "" )
        papszFileList = CSLAddString( papszFileList, osRPBFilename );
	*/
    return papszFileList;
}

/************************************************************************/
/*                          GDALRegister_TIL()                          */
/************************************************************************/

void GDALRegister_TIL()

{
    GDALDriver	*poDriver;

    if( GDALGetDriverByName( "TIL" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "TIL" );
        poDriver->SetMetadataItem( GDAL_DCAP_RASTER, "YES" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "EarthWatch .TIL" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_til.html" );

        poDriver->SetMetadataItem( GDAL_DCAP_VIRTUALIO, "YES" );

        poDriver->pfnOpen = TILDataset::Open;
        poDriver->pfnIdentify = TILDataset::Identify;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}


