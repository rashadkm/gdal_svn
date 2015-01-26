/******************************************************************************
 * Project:  geography network utility
 * Purpose:  Analyse GNM networks
 * Author:  Mikhail Gusev, gusevmihs at gmail dot com
 *
 ******************************************************************************
 * Copyright (C) 2014 Mikhail Gusev
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

#include "gnm.h"
#include "analysis/gnmstdanalysis.h"
#include "commonutils.h"
#include "ogr_p.h"

#define GNM_MAX_SHORTESTPATHS_COUNT 10

enum operation
{
    op_unknown = 0,
    op_dijkstra,
    op_kpaths,
    op_resource
};

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/
static void Usage(const char* pszAdditionalMsg, int bShort = TRUE)
{
    //OGRSFDriverRegistrar *poR = OGRSFDriverRegistrar::GetRegistrar();

    printf("Usage: gnmanalyse [--help][-q][-quiet][--utility_version][--long-usage]\n"
           "                  [dijkstra start_gfid end_gfid]\n"
           "                  [kpaths start_gfid end_gfid k]\n"
           "                  [resource]\n"
           "                  [-bl gfid][-unbl gfid][-unblall]\n"
           "                  [-ds ds_name][-f ds_format][-l layer_name]\n"
           "                  gnm_name\n");

    if (bShort)
    {
        printf("\nNote: gnmanalyse --long-usage for full help.\n");
        if (pszAdditionalMsg)
            fprintf(stderr, "\nFAILURE: %s\n", pszAdditionalMsg);
        exit(1);
    }

    printf("\n   dijkstra start_gfid end_gfid: calculates the shortest path between two points using Dijkstra algorithm from start_gfid point to end_gfid point\n"
           "   kpaths start_gfid end_gfid k: calculates k (up to 10) shortest paths between two points using Yen\'s algorithm (which internally uses Dijkstra algorithm for single path calculating) from start_gfid point to end_gfid point\n"
           "   resource: calculates the resource destribution according to the blocked/unblocked feature states. If the network contains \'emitter\' features (set via rules) the connected lines to these points returned \n"
           "   -bl gfid: block feature before the main operation. Blocking features are saved in the special layer\n"
           "   -unbl gfid: unblock feature before the main operation\n"
           "   -unblall: unblock all blocked features before the main operation\n"
           "   -ds ds_name: the name&path of the dataset to save the layer with resulting paths. Not need to be existed dataset\n"
           "   -f ds_format: define this to set the format of newly created dataset\n"
           "   -l layer_name: the name of the resulting layer. If the layer exists already - it will be rewritten. For K shortest paths several layers are created in format layer_nameN, where N - is number of the path (0 - is the most shortest one)\n"
           "   gnm_name: the network to work with (path and name)\n"
           );

    if (pszAdditionalMsg)
        fprintf(stderr, "\nFAILURE: %s\n", pszAdditionalMsg);

    exit(1);
}

static void Usage(int bShort = TRUE)
{
    Usage(NULL, bShort);
}


/************************************************************************/
/*                                main()                                */
/************************************************************************/

#define CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(nExtraArg) \
    do { if (iArg + nExtraArg >= nArgc) \
        Usage(CPLSPrintf("%s option requires %d argument(s)", papszArgv[iArg], nExtraArg)); } while(0)

int main( int nArgc, char ** papszArgv )

{
    OGRErr eErr = OGRERR_NONE;
    int bQuiet = FALSE;

    const char *pszDataSource = NULL;

    GNMGFID from = -1;
    GNMGFID to = -1;
    int K = 1;
    const char *dataset = "";
    const char *format = "";
    //const char *layer = "shortestpath";
    std::string layer("shortestpath");

    operation stOper = op_unknown;

    std::vector<GNMGFID> gfidsToBlock;
    std::vector<GNMGFID> gfidsToUnblock;
    bool unblockAll = false;

    /* Check strict compilation and runtime library version as we use C++ API */
    if (! GDAL_CHECK_VERSION(papszArgv[0]))
        exit(1);

    EarlySetConfigOptions(nArgc, papszArgv);

/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
    GDALAllRegister();

/* -------------------------------------------------------------------- */
/*      Processing command line arguments.                              */
/* -------------------------------------------------------------------- */
    nArgc = OGRGeneralCmdLineProcessor( nArgc, &papszArgv, 0 );

    if( nArgc < 1 )
        exit( -nArgc );

    for( int iArg = 1; iArg < nArgc; iArg++ )
    {
        if( EQUAL(papszArgv[iArg], "--utility_version") )
        {
            printf("%s was compiled against GDAL %s and is running against GDAL %s\n",
                   papszArgv[0], GDAL_RELEASE_NAME, GDALVersionInfo("RELEASE_NAME"));
            return 0;
        }

        else if( EQUAL(papszArgv[iArg],"--help") )
        {
            Usage();
        }

        else if ( EQUAL(papszArgv[iArg], "--long-usage") )
        {
            Usage(FALSE);
        }

        else if( EQUAL(papszArgv[iArg],"-q") || EQUAL(papszArgv[iArg],"-quiet") )
        {
            bQuiet = TRUE;
        }

        else if( EQUAL(papszArgv[iArg],"dijkstra") )
        {
            stOper = op_dijkstra;
            from = atoi(papszArgv[++iArg]);
            to = atoi(papszArgv[++iArg]);
        }

        else if( EQUAL(papszArgv[iArg],"kpaths") )
        {
            stOper = op_kpaths;
            from = atoi(papszArgv[++iArg]);
            to = atoi(papszArgv[++iArg]);
            K = atoi(papszArgv[++iArg]);
        }

        else if( EQUAL(papszArgv[iArg],"resource") )
        {
            stOper = op_resource;
        }

        else if( EQUAL(papszArgv[iArg],"-bl") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            gfidsToBlock.push_back(atoi(papszArgv[++iArg]));
        }

        else if( EQUAL(papszArgv[iArg],"-unbl") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            gfidsToUnblock.push_back(atoi(papszArgv[++iArg]));
        }

        else if( EQUAL(papszArgv[iArg],"-unblall") )
        {
            unblockAll = true;
        }

        else if( EQUAL(papszArgv[iArg],"-ds") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            dataset = papszArgv[++iArg];
        }

        else if( EQUAL(papszArgv[iArg],"-f") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            format = papszArgv[++iArg];
        }

        else if( EQUAL(papszArgv[iArg],"-l") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            layer = papszArgv[++iArg];
        }

        else if( papszArgv[iArg][0] == '-' )
        {
            Usage(CPLSPrintf("Unknown option name '%s'", papszArgv[iArg]));
        }

        else if( pszDataSource == NULL )
            pszDataSource = papszArgv[iArg];
    }

    if (stOper == op_dijkstra || stOper == op_resource || stOper == op_kpaths)
    {
        if (pszDataSource == NULL)
            Usage("no dataset name provided");

        GNMGdalNetwork *poNet;
        poNet = GNMManager::GdalOpenNetwork(pszDataSource);

        if (poNet == NULL)
        {
            fprintf(stderr, "\nFAILURE: Failed to open network at %s\n",pszDataSource);
            exit(1);
        }

        bool useTextOutput = false;
        GDALDataset *poTgtDS = NULL;

        if (strcmp(dataset,"") == 0 || strcmp(format,"") == 0)
        {
            printf("\nThe dataset or its format have not been specified. Use text output.\n");
            useTextOutput = true;
        }

        else
        {
            poTgtDS = (GDALDataset*) GDALOpenEx(dataset,
                                                GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                                                NULL, NULL, NULL);
            if(poTgtDS == NULL)
            {
                GDALDriver *poDriver;
                poDriver = GetGDALDriverManager()->GetDriverByName(format);
                if(poDriver == NULL)
                {
                    GNMManager::GdalCloseNetwork(poNet);
                    fprintf(stderr, "\nFAILURE: Error while initializing driver of the defined format");
                    exit( 1 );
                }
                poTgtDS = poDriver->Create(dataset, 0, 0, 0, GDT_Unknown, NULL );
                if(poTgtDS == NULL)
                {
                    GNMManager::GdalCloseNetwork(poNet);
                    fprintf(stderr, "\nFAILURE: Error while creating new dataset for the resulting layer");
                    exit( 1 );
                }
            }

            std::vector<std::string> layers_to_fill;
            if (stOper == op_kpaths)
            {
                if (K <= 0)
                {
                    GNMManager::GdalCloseNetwork(poNet);
                    fprintf(stderr, "\nFAILURE: K is set incorrectly");
                    exit( 1 );
                }

                if (K > GNM_MAX_SHORTESTPATHS_COUNT)
                {
                    K = GNM_MAX_SHORTESTPATHS_COUNT;
                    CPLError(CE_Warning,CPLE_None,"K is too large, only %i paths"
                             " will be found",K);
                }

                for (int i = 0; i < K; i++)
                {
                    // TODO: replace 500 with the calculating char count.
                    char buf[500];
                    sprintf(buf,"%s%i",layer.data(),i);
                    layers_to_fill.push_back(buf);
                }

            }
            else
            {
                layers_to_fill.push_back(layer);
            }

            for (int i = 0; i < layers_to_fill.size(); i++)
            {
                // We try to delete the old layer in order to rewrite it.
                OGRLayer *poLayer = poTgtDS->GetLayerByName(
                            layers_to_fill[i].data());
                if (poLayer != NULL)
                {
                    int cnt = poTgtDS->GetLayerCount();
                    for (int j = 0; j < cnt; j++)
                    {
                        OGRLayer *la = poTgtDS->GetLayer(j);
                        if (la == poLayer)
                        {
                            // We search to delete layer by its index.
                            // Note: in case of K-shortest paths, if K now is less
                            // than in previous time, all other layers will not
                            // be deleted.
                            poTgtDS->DeleteLayer(j);
                            break;
                        }
                    }
                }
            }
        }

// Initializing block:

        GNMGdalStdAnalyser *poAnalyser;

        if (stOper == op_dijkstra || stOper == op_kpaths )
        {
            poAnalyser = new GNMGdalStdRoutingAnalyser();
        }
        else
        {
            poAnalyser = new GNMGdalStdCommutationsAnalyser();
        }

// Preparing block:

        if (poAnalyser->PrepareGraph(poNet) == GNMERR_NONE)
        {
            // Block/unblock features if the according options has been set.
            if (unblockAll)
            {
                poAnalyser->UnblockAllFeatures();
            }
            else
            {
                int j;
                for (j=0; j<gfidsToBlock.size(); j++)
                {
                   poAnalyser->BlockFeature(gfidsToBlock[j]);
                }
                for (j=0; j<gfidsToUnblock.size(); j++)
                {
                   poAnalyser->UnblockFeature(gfidsToUnblock[j]);
                }
            }

// Calculating block:

            std::vector<OGRLayer*> resultLayers;

            if (stOper == op_dijkstra)
            {
                OGRLayer *resLayer = static_cast<GNMGdalStdRoutingAnalyser*>
                        (poAnalyser)->DijkstraShortestPath(from,to);
                resultLayers.push_back(resLayer);
            }
            else if (stOper == op_resource)
            {
                OGRLayer *resLayer = static_cast<GNMGdalStdCommutationsAnalyser*>
                        (poAnalyser)->ResourceDestribution();
                resultLayers.push_back(resLayer);
            }
            else
            {
                std::vector<OGRLayer*> resLayers =
                 static_cast<GNMGdalStdRoutingAnalyser*>(poAnalyser)
                        ->KShortestPaths(from,to,K);
                //resultLayers.insert(resLayers.end()-1,resLayers.begin(),resLayers.end());
                resultLayers.swap(resLayers);
            }

// Output & save block:

            if (resultLayers.empty())
            {
                GDALClose(poTgtDS);
                delete poAnalyser;
                GNMManager::GdalCloseNetwork(poNet);
                printf("Note: no layers will be created or updated \n");
                exit(1);
            }

            std::vector<OGRLayer*>::iterator itLayers;
            int i = 0;
            for (itLayers=resultLayers.begin(); itLayers!=resultLayers.end();
                 ++itLayers)
            {
                // TODO: replace 500 with maximum char count.
                std::string realLayerName;
                if (stOper == op_kpaths)
                {
                    char buf[500];
                    sprintf(buf,"%s%i",layer.data(),i);
                    realLayerName = buf;
                    i++;
                }
                else
                {
                    realLayerName = layer;
                }

                if ((*itLayers) == NULL || (*itLayers)->GetFeatureCount() == 0)
                {
                    printf("Note: nothing to save for layer %s, the layer will not "
                           "be created or updated\n",realLayerName.data());
                }
                else
                {
                    if (!useTextOutput)
                    {
                        if (poTgtDS->CopyLayer((*itLayers),realLayerName.data()) == NULL)
                        {
                            fprintf(stderr, "\nFAILURE: error while saving data to "
                            "the layer %s of the dataset %s\n",realLayerName.data(),
                                    dataset);
                        }
                        else
                        {
                            printf("Data saved to the layer %s successfully\n",
                                   realLayerName.data());
                        }
                    }
                    else
                    {
                        (*itLayers)->ResetReading();
                        OGRFeature *resFeat;
                        printf("\nPrinting features for layer %s: \n\n",
                               realLayerName.data());
                        while ((resFeat = (*itLayers)->GetNextFeature()) != NULL)
                        {
                            resFeat->DumpReadable(NULL,NULL);
                            OGRFeature::DestroyFeature(resFeat);
                        }
                    }
                }
            }

        }

        else
        {
            fprintf(stderr, "\nFAILURE: Failed to prepare graph for calculations\n");
        }

        delete poAnalyser;
        GDALClose(poTgtDS);
        GNMManager::GdalCloseNetwork(poNet);
    }

    else
    {
        printf("\nNeed an operation. See help what you can do with gnmanalyse.exe:\n");
        Usage();
    }

    OGRCleanupAll();

    return eErr == OGRERR_NONE ? 0 : 1;
}

