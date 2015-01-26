/******************************************************************************
 * Project:  geography network utility
 * Purpose:  Manage GNM networks
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
//#include "frmts/gnm_frmts.h"
#include "commonutils.h"
#include "ogr_p.h"

enum operation
{
    op_unknown = 0,
    op_info,
    op_create,
    op_import,
    op_connect,
    op_disconnect,
    op_rule,
    op_autoconnect,
    op_remove
};

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/
static void Usage(const char* pszAdditionalMsg, int bShort = TRUE)
{
    //OGRSFDriverRegistrar *poR = OGRSFDriverRegistrar::GetRegistrar();

    printf("Usage: gnmmanage [--help][-q][-quiet][--utility_version][--long-usage]\n"
           "                 [info]\n"
           "                 [create [-f format_name] [-t_srs srs_name] [-dsco NAME=VALUE]... ]\n"
           "                 [import src_dataset_name] [-l layer_name]\n"
           "                 [connect gfid_src gfid_tgt gfid_con [-c cost] [-ic inv_cost] [-dir dir]]\n"
           "                 [disconnect gfid_src gfid_tgt gfid_con]\n"
           "                 [rule rule_str]\n"
           "                 [autoconnect tolerance]\n"
           "                 [remove]\n"
           "                 gnm_name\n");

    if (bShort)
    {
        printf("\nNote: gnmmanage --long-usage for full help.\n");
        if (pszAdditionalMsg)
            fprintf(stderr, "\nFAILURE: %s\n", pszAdditionalMsg);
        exit(1);
    }

    printf("\n   info: different information about network: system and class layers, network metadata, network spatial reference\n"
           "   create: create network\n"
           "      -f format_name: output file format name, possible values are:\n");
    int i;
    for (i = 0; GNMGdalSupportedDrivers[i] != NULL; ++i)
        printf("      [%s]\n", GNMGdalSupportedDrivers[i]);

    printf("      -t_srs srs_name: spatial reference input\n"
           "      -dsco NAME=VALUE: network creation option set as pair=value\n"
           "   import src_dataset_name: import external layer where src_dataset_name is a dataset name to copy from\n"
           "      -l layer_name: layer name in dataset. If unset, 0 layer is copied\n"
           "   connect gfid_src gfid_tgt gfid_con: make a topological connection, where the gfid_src and gfid_tgt are vertexes and gfid_con is edge (gfid_con can be -1, so the virtual connection will be created)\n"
           "      -c cost -ic inv_cost -dir dir: manually assign the following values: the cost (weight), inverse cost and direction of the edge (optional)\n"
           "   disconnect gfid_src gfid_tgt gfid_con: removes the connection from the graph\n"
           "   rule rule_str: creates a rule in the network by the given rule_str string\n"
           "   autoconnect tolerance: create topology automatically with the given double tolerance\n"
           "   remove: delete network\n"
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
    const char *pszFormat = "ESRI Shapefile";
    const char *pszSRS = "EPSG:4326";

    GNMGFID srcGfid = -1;
    GNMGFID tgtGfid = -1;
    GNMGFID conGfid = -1;
    double dirCost = -1.0;
    double invCost = -1.0;
    GNMDirection dir = -1;

    const char *pszRuleStr = "";
    const char *pszDataSource = NULL;
    char **papszDSCO = NULL;
    const char *pszInputDataset = NULL;
    const char *pszInputLayer = NULL;

    double tol = 0.0;

    operation stOper = op_unknown;

    int bDisplayProgress = FALSE;

    // Check strict compilation and runtime library version as we use C++ API
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

        else if ( EQUAL(papszArgv[iArg],"info") )
        {
            stOper = op_info;
        }

        else if( EQUAL(papszArgv[iArg],"-f") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            pszFormat = papszArgv[++iArg];
        }

        else if( EQUAL(papszArgv[iArg],"-dsco") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            papszDSCO = CSLAddString(papszDSCO, papszArgv[++iArg] );
        }

        else if( EQUAL(papszArgv[iArg],"create") )
        {
            stOper = op_create;
        }

        else if( EQUAL(papszArgv[iArg],"-t_srs") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            pszSRS = papszArgv[++iArg];
        }

        else if( EQUAL(papszArgv[iArg],"import") )
        {
            stOper = op_import;
            pszInputDataset = papszArgv[++iArg];
        }

        else if( EQUAL(papszArgv[iArg],"-l") )
        {
            pszInputLayer = papszArgv[++iArg];
        }

        else if( EQUAL(papszArgv[iArg],"connect") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(3);
            stOper = op_connect;
            srcGfid = atoi(papszArgv[++iArg]);
            tgtGfid = atoi(papszArgv[++iArg]);
            conGfid = atoi(papszArgv[++iArg]);
        }

        else if( EQUAL(papszArgv[iArg],"-c") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            dirCost = CPLAtof(papszArgv[++iArg]);
        }
        else if( EQUAL(papszArgv[iArg],"-ic") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            invCost = CPLAtof(papszArgv[++iArg]);
        }
        else if( EQUAL(papszArgv[iArg],"-dir") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(1);
            dir = atoi(papszArgv[++iArg]);
        }

        else if( EQUAL(papszArgv[iArg],"disconnect") )
        {
            CHECK_HAS_ENOUGH_ADDITIONAL_ARGS(3);
            stOper = op_disconnect;
            srcGfid = atoi(papszArgv[++iArg]);
            tgtGfid = atoi(papszArgv[++iArg]);
            conGfid = atoi(papszArgv[++iArg]);
        }

        else if ( EQUAL(papszArgv[iArg],"autoconnect") )
        {
            stOper = op_autoconnect;
            tol = CPLAtof(papszArgv[++iArg]);
        }

        else if ( EQUAL(papszArgv[iArg],"rule") )
        {
            stOper = op_rule;
            pszRuleStr = papszArgv[++iArg];
        }

        else if ( EQUAL(papszArgv[iArg],"remove") )
        {
            stOper = op_remove;
        }

        else if( papszArgv[iArg][0] == '-' )
        {
            Usage(CPLSPrintf("Unknown option name '%s'", papszArgv[iArg]));
        }

        else if( pszDataSource == NULL )
            pszDataSource = papszArgv[iArg];
    }

    if(stOper == op_info)
    {
        if(pszDataSource == NULL)
            Usage("no dataset provided");

        //TODO for output:
        // supported drivers +
        // spatial reference +
        // system and class layers +
        // all metadata +
        // stats about graph
        // rules
        // blocked features

        //Output info
        GNMGdalNetwork *poNet;
        poNet = GNMManager::GdalOpenNetwork(pszDataSource);
        if (poNet != NULL)
        {
            printf("\nInformation about network at %s: \n",pszDataSource);

            GDALDataset* poDS = poNet->GetDataset();
            if (poDS != NULL)
            {
                printf("\nNetwork\'s system and class layers: \n");

                for (int iLayer = 0; iLayer < poDS->GetLayerCount(); iLayer++)
                {
                    OGRLayer        *poLayer = poDS->GetLayer(iLayer);

                    if (poLayer == NULL)
                    {
                        printf("FAILURE: Couldn't fetch advertised layer %d!\n",
                            iLayer);
                        exit(1);
                    }

                    printf("%d: %s",
                        iLayer + 1,
                        poLayer->GetName());

                    int nGeomFieldCount =
                        poLayer->GetLayerDefn()->GetGeomFieldCount();
                    if (nGeomFieldCount > 1)
                    {
                        printf(" (");
                        for (int iGeom = 0; iGeom < nGeomFieldCount; iGeom++)
                        {
                            if (iGeom > 0)
                                printf(", ");
                            OGRGeomFieldDefn* poGFldDefn =
                                poLayer->GetLayerDefn()->GetGeomFieldDefn(iGeom);
                            printf("%s",
                                OGRGeometryTypeToName(
                                poGFldDefn->GetType()));
                        }
                        printf(")");
                    }
                    else if (poLayer->GetGeomType() != wkbUnknown)
                        printf(" (%s)",
                        OGRGeometryTypeToName(
                        poLayer->GetGeomType()));

                    printf("\n");
                }
            }

            char** params = poNet->GetMetaParamValues();

            if (params == NULL)
            {
                fprintf(stderr, "\nFAILURE: Failed to read network metadata at %s\n",pszDataSource);
            }
            else
            {
                printf("\nNetwork\'s metadata [Name=Value]:\n");

                // We don't know the actual count of parameters.
                int i = 0;
                while (params[i] != NULL)
                {
                    printf("    [%s]\n",params[i]);
                    i++;
                }
            }
            CSLDestroy(params);

            char    *pszWKT;
            const OGRSpatialReference* poSRS = poNet->GetSpatialReference();
            if (poSRS == NULL)
                pszWKT = CPLStrdup("(unknown)");
            else
            {
                poSRS->exportToPrettyWkt(&pszWKT);
            }

            printf("\nNetwork\'s SRS WKT:\n%s\n", pszWKT);
            CPLFree(pszWKT);

            GNMManager::GdalCloseNetwork(poNet);
        }
        else
        {
            fprintf(stderr, "\nFAILURE: Failed to open network at %s\n",pszDataSource);
        }
    }

    else if(stOper == op_create)
    {
        char** options = NULL;
        const char *pszGNMName = CSLFetchNameValue(papszDSCO,GNM_INIT_OPTIONPAIR_NAME);

        // TODO: read other parameters.

        if( pszDataSource == NULL)
            Usage("no dataset name provided");
        else if(pszFormat == NULL)
            Usage("no driver name provided");
        else  if(pszGNMName == NULL)
            printf("\nWarning: No network name provided, using default ...\n");
        else
        {
            options = CSLAddNameValue(options, GNM_INIT_OPTIONPAIR_NAME, pszGNMName);
        }

        GNMGdalNetwork *poNet;
        poNet = GNMManager::GdalCreateNetwork(pszDataSource,pszFormat,
                                               pszSRS,options,NULL);
        CSLDestroy(options);
        CSLDestroy(papszDSCO);

        if (poNet != NULL)
        {
            if (bQuiet == FALSE)
                printf("\nNetwork created successfully in a "
                   "new dataset at %s\n",pszDataSource);
            GNMManager::GdalCloseNetwork(poNet);
        }
        else
        {
            fprintf(stderr, "\nFAILURE: Failed to create network in a new dataset at "
                   "%s and with driver %s\n",pszDataSource,pszFormat);
        }
    }

    else if(stOper == op_import)
    {
       if(pszDataSource == NULL)
            Usage("no dataset provided");
       if(pszInputDataset == NULL)
            Usage("no input dataset name provided");

        GNMGdalNetwork *poNet;
        poNet = GNMManager::GdalOpenNetwork(pszDataSource);

        if (poNet != NULL)
        {
            GDALDataset *poSrcDS;
            poSrcDS = (GDALDataset*) GDALOpenEx(pszInputDataset,
                                                GDAL_OF_VECTOR | GDAL_OF_READONLY,
                                                NULL, NULL, NULL );
            if(poSrcDS == NULL)
            {
                fprintf(stderr, "\nFAILURE: Can not open dataset at %s\n",
                        pszInputDataset);
                exit(1);
            }

            OGRLayer *poSrcLayer;
            if (pszInputLayer != NULL)
                poSrcLayer = poSrcDS->GetLayerByName(pszInputLayer);
            else
                poSrcLayer = poSrcDS->GetLayer(0);

            if (poSrcLayer == NULL)
            {
                if (pszInputLayer != NULL)
                    fprintf(stderr, "\nFAILURE: Can not open layer %s in %s\n",
                        pszInputLayer,pszInputDataset);
                else
                    fprintf(stderr, "\nFAILURE: Can not open layer in %s\n",
                    pszInputDataset);

                GDALClose(poSrcDS);
                exit(1);
            }

            GNMErr err = poNet->CopyLayer(poSrcLayer, poSrcLayer->GetName());
            if (err != GNMERR_NONE)
            {
                if (pszInputLayer != NULL)
                    fprintf(stderr, "\nFAILURE: Can not copy layer %s from %s\n",
                        pszInputLayer,pszInputDataset);
                else
                    fprintf(stderr, "\nFAILURE: Can not copy layer from %s\n",
                    pszInputDataset);
                GDALClose(poSrcDS);
                exit(1);
            }

            if (bQuiet == FALSE)
            {
                if (pszInputLayer != NULL)
                    printf("\nLayer %s successfully copied from %s and added to the network at %s\n",
                    pszInputLayer, pszInputDataset, pszDataSource);
                else
                    printf("\nLayer successfully copied from %s and added to the network at %s\n",
                    pszInputDataset, pszDataSource);
            }

            GDALClose(poSrcDS);
            GNMManager::GdalCloseNetwork(poNet);
        }
        else
        {
            printf("\nFailed to open network at %s\n",pszDataSource);
        }
    }

    else if (stOper == op_connect)
    {
        if(pszDataSource == NULL)
             Usage("no dataset provided");

         GNMGdalNetwork *poNet;
         poNet = GNMManager::GdalOpenNetwork(pszDataSource);

         if (poNet != NULL)
         {
             GNMErr err = poNet->ConnectFeatures(srcGfid,tgtGfid,conGfid,dirCost,invCost,dir);
             if (err == GNMERR_NONE)
             {
                printf("\nFeatures connected successfully\n");
             }
             else if (err == GNMERR_CONRULE_RESTRICT)
             {
                printf("\nFailed to connect features: there is no rule allowing the connection for these classes of features\n");
             }
             else
             {
                printf("\nFailed to connect features\n");
             }
             GNMManager::GdalCloseNetwork(poNet);
         }
         else
         {
             printf("\nFailed to open network at %s\n",pszDataSource);
         }
    }

    else if (stOper == op_disconnect)
    {
        if(pszDataSource == NULL)
             Usage("no dataset provided");

         GNMGdalNetwork *poNet;
         poNet = GNMManager::GdalOpenNetwork(pszDataSource);

         if (poNet != NULL)
         {
             GNMErr err = poNet->DisconnectFeatures(srcGfid,tgtGfid,conGfid);
             if (err == GNMERR_NONE)
             {
                printf("\nFeatures disconnected successfully\n");
             }
             else
             {
                printf("\nFailed to disconnect features\n");
             }
             GNMManager::GdalCloseNetwork(poNet);
         }
         else
         {
             printf("\nFailed to open network at %s\n",pszDataSource);
         }
    }

    else if (stOper == op_rule)
    {
        if(pszDataSource == NULL)
             Usage("no dataset provided");

         GNMGdalNetwork *poNet;
         poNet = GNMManager::GdalOpenNetwork(pszDataSource);

         if (poNet != NULL)
         {
             if (poNet->CreateRule(pszRuleStr) == GNMERR_NONE)
             {
                printf("\nRule created successfully\n");
             }
             else
             {
                 printf("\nUnable to create rule in network\n");
             }
             GNMManager::GdalCloseNetwork(poNet);
         }
         else
         {
             printf("\nFailed to open network at %s\n",pszDataSource);
         }
    }

    else if (stOper == op_autoconnect)
    {
        if( pszDataSource == NULL)
            Usage("no dataset name provided");

        GNMGdalNetwork *poNet;
        poNet = GNMManager::GdalOpenNetwork(pszDataSource);

        if (poNet != NULL)
        {
            printf("\nBuilding topology ...\n");
            GDALDataset *poDS = poNet->GetDataset();
            int cnt = poDS->GetLayerCount();
			std::vector<OGRLayer*> layersToConnect;
            //OGRLayer **layers = new OGRLayer*[cnt+1];
            for (int i = 0; i < cnt; i++)
            {
				//layers[i] = poDS->GetLayer(i);
				layersToConnect.push_back(poDS->GetLayer(i));
            }
            //layers[cnt] = NULL;

            //GNMErr err = poNet->AutoConnect(layers,tol,NULL);
			GNMErr err = poNet->AutoConnect(layersToConnect,tol,NULL);
            if (err != GNMERR_NONE)
            {
                fprintf(stderr, "\nFAILURE: Building topology failed\n");
            }
            else
            {
                printf("\nTopology has been built successfully\n");
            }
            //delete[] layers;
            GNMManager::GdalCloseNetwork(poNet);
        }
        else
        {
            printf("\nFailed to open network at %s\n",pszDataSource);
        }

    }

    else if (stOper == op_remove)
    {
        if( pszDataSource == NULL)
            Usage("no dataset name provided");

        // Try to delete network.
        if (GNMManager::GdalDeleteNetwork(pszDataSource) == GNMERR_NONE)
        {
            printf("\nNetwork at %s has been deleted\n",pszDataSource);
        }
        else
        {
            printf("\nFailed to process network at %s\n",pszDataSource);
        }
    }

    else
    {
        printf("\nNeed an operation. See help what you can do with gnmmanage.exe:\n");
        Usage();
    }

/* -------------------------------------------------------------------- */
/*      Close down.                                                     */
/* -------------------------------------------------------------------- */

    OGRCleanupAll();

    return eErr == OGRERR_NONE ? 0 : 1;
}

