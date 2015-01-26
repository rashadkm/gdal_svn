/******************************************************************************
 * $Id$
 *
 * Name:     gnmmanager.cpp
 * Project:  GDAL/OGR Geography Network support (Geographic Network Model)
 * Purpose:  GNMManager class.
 * Author:   Mikhail Gusev (gusevmihs at gmail dot com)
 *
 ******************************************************************************
 * Copyright (c) 2014, Mikhail Gusev
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
#include "gnm_api.h"

#include "frmts/gnm_frmts.h"


/************************************************************************/
/*                      CreateConnectivity()                            */
/************************************************************************/
/**
 * \brief Creates the connectivity over the dataset.
 *
 * Creates the connectivity over the dataset and returns the resulting
 * network.
 * NOTE: This method does not create any connections among features
 * but creates the necessary set of fields, layers, etc.
 * NOTE: After the successfull creation the passed dataset must not be
 * mofified outside (but can be readed as usual).
 * NOTE: For the GDAL-network format the creation is forbidden if the
 * passed dataset already has some layers.
 *
 * @param poDS The dataset to create a connectivity over it.
 * @param bNative If the format is not defined (bNative = FALSE) the standard
 * GDAL network format is used. See GNMGdalNetwork class for details. Otherwise
 * method tries to load the appropriate GNMFormat and to create network via it.
 * @param papszOptions Additional options.
 * @return The concrete network object or NULL if failed to create.
 *
 * @see GNMManager::GdalCreateNetwork()
 *
 * @since GDAL 2.0
 */
GNMNetwork *GNMManager::CreateConnectivity (GDALDataset *poDS,int bNative,
                                            char **papszOptions)
{
    if (poDS == NULL)
        return NULL;

    if (HasConnectivity(poDS,bNative))
        return NULL;

    const char *driverName = poDS->GetDriverName();
    if (strcmp(driverName,"") == 0)
        return NULL;

    if (bNative == FALSE)
    {
        // We use GDAL-network. Create according system layers and fields in a
        // dataset and initialize network.

        if (!GNMGdalNetwork::IsDriverSupported(poDS))
            return NULL;

        // IMPORTANT NOTE: There are two possible concepts for GDAL-networks:
        // 1. The GNMGdalNetwork has one common SRS and one naming convention
        // for layers. So if the passed dataset already has some layers we must
        // transform their coordinates and rename these layers. But this means
        // that we should restore the old state of dataset when there will
        // be a need to remove connectivity.
        // 2. We don't use one common srs and any naming conventions for layers
        // so we do not need to transform and rename in this creation method.
        // For now we will use the first case and will not allow to create
        // connectivity, when there are already some layers in the dataset (it
        // must be void).
        int classCount = poDS->GetLayerCount();
        if (classCount > 0)
            return NULL;

        if (poDS->TestCapability(ODsCCreateLayer) == FALSE)
        {
            //CPLError
            return NULL;
        }
        if (poDS->TestCapability(ODsCDeleteLayer) == FALSE)
        {
            //CPLError
            return NULL;
        }

        //if (classCount > 0 && poDS->GetLayer(0)->
        //        TestCapability(OLCCreateField) == FALSE)
        //// TODO: test delete feature here
        //{
        //   //CPLError
        //    return NULL;
        //}

        // Extract SRS input from papszOptions. The SRS must be specified.
        const char *pszSrsInput = CSLFetchNameValue(papszOptions,
                                                    GNM_INIT_OPTIONPAIR_SRS);
        if (pszSrsInput == NULL)
            return NULL;

        // Initializing new GDAL network.
        GNMGdalNetwork *poNet = new GNMGdalNetwork();

        //poNet->_driverName = new char [50];
        //strcpy(poNet->_driverName,driverName);

    // ------------ Create the necessary system layers and fields -------------

        OGRLayer *newLayer;
        OGRFeature *feature;
        OGRFieldDefn newField1("",OFTInteger);
        OGRFieldDefn newField2("",OFTInteger);
        OGRFieldDefn newField3("",OFTInteger);
        OGRFieldDefn newField4("",OFTInteger);
        OGRFieldDefn newField5("",OFTInteger);
        OGRFieldDefn newField6("",OFTInteger);

    // ---------------------------- Create meta layer ---------------------

        newLayer = poDS->CreateLayer(GNM_SYSLAYER_META, NULL, wkbNone, NULL);
        if (newLayer == NULL)
            return NULL;

        newField1.Set(GNM_SYSFIELD_PARAMNAME, OFTString);
        newField2.Set(GNM_SYSFIELD_PARAMVALUE, OFTString);
        if(newLayer->CreateField(&newField1) != OGRERR_NONE
           || newLayer->CreateField(&newField2) != OGRERR_NONE)
        {
            //CPLError
            delete poNet;
            poDS->DeleteLayer(0);
            return NULL;
        }

        // Write obligatory metaparameters.

        // Create SRS object and write its user input in text form.
        poNet->_poSpatialRef = new OGRSpatialReference();
        if (poNet->_poSpatialRef->SetFromUserInput(pszSrsInput) != OGRERR_NONE)
        {
            //CPLError
            // NOTE: we do not need to delete _poSpatialRef here. It will be
            // made in destructor of GNMGdalNetwork.
            delete poNet;
            poDS->DeleteLayer(0);
            return NULL;
        }
        feature = OGRFeature::CreateFeature(newLayer->GetLayerDefn());
        feature->SetField(GNM_SYSFIELD_PARAMNAME, GNM_METAPARAM_SRS);

        // User usually provides short srs in input parameteres,
        // but if we create network programmatically, we can set SRS via
        // OGRSpatialReference. The usual way to store SRS is WKT, which can
        // be more than 255 chars, so we need to store srs in file for shape
        // file/dbf case.
        const char *srsToSave = "file";
        char *srsFilePath = NULL;
        if (strcmp(poDS->GetDriverName(),"ESRI Shapefile") == 0)
        {
            // Form wkt srs.
            char *wktSrs = NULL;
            if (poNet->_poSpatialRef->exportToWkt(&wktSrs) != OGRERR_NONE)
            {
                //CPLErr
                delete poNet;
                poDS->DeleteLayer(0);
                return NULL;
            }

            // Form file name extracting the path of dataset from options.
            // allocate enough size string:
            srsFilePath = new char [strlen(poDS->GetDescription()) +
              strlen(GNM_SRSFILENAME) + 2]; // if we set + 1 heap corrupted error returned
            sprintf(srsFilePath,"%s/%s",poDS->GetDescription(),GNM_SRSFILENAME);

            // Open file for writing wkt srs.
            VSILFILE *fpSrsPrj;
            fpSrsPrj = VSIFOpenL(srsFilePath, "w");
            if (fpSrsPrj == NULL)
            {
                //CPLErr
                delete[] srsFilePath;
                delete poNet;
                poDS->DeleteLayer(0);
                return NULL;
            }

            // Write wkt string to file.
            VSIFPrintfL(fpSrsPrj,"%s",wktSrs);

            VSIFCloseL(fpSrsPrj);
            CPLFree(wktSrs);
        }

        // TODO: widen these cases when new supported drivers will be added.

        else // save user input
        {
            srsToSave = pszSrsInput;
        }

        feature->SetField(GNM_SYSFIELD_PARAMVALUE, srsToSave);
        if(newLayer->CreateFeature(feature) != OGRERR_NONE)
        {
            //CPLError
            if (srsFilePath != NULL)
            {
                VSIUnlink(srsFilePath);
                delete[] srsFilePath;
            }
            delete poNet;
            poDS->DeleteLayer(0);
            return NULL;
        }

        poNet->_meta_srs_i = feature->GetFID();
        OGRFeature::DestroyFeature(feature);

        // Write version.
        feature = OGRFeature::CreateFeature(newLayer->GetLayerDefn());
        feature->SetField(GNM_SYSFIELD_PARAMNAME, GNM_METAPARAM_VERSION);
        feature->SetField(GNM_SYSFIELD_PARAMVALUE, GNM_VERSION);
        if(newLayer->CreateFeature(feature) != OGRERR_NONE)
        {
            //CPLError
            if (srsFilePath != NULL)
            {
                VSIUnlink(srsFilePath);
                delete[] srsFilePath;
            }
            delete poNet;
            poDS->DeleteLayer(0);
            return NULL;
        }
        poNet->_meta_vrsn_i = feature->GetFID();
        OGRFeature::DestroyFeature(feature);

        // Write GFID counter.
        feature = OGRFeature::CreateFeature(newLayer->GetLayerDefn());
        feature->SetField(GNM_SYSFIELD_PARAMNAME, GNM_METAPARAM_GFIDCNT);
        feature->SetField(GNM_SYSFIELD_PARAMVALUE, "0");
        if(newLayer->CreateFeature(feature) != OGRERR_NONE)
        {
            //CPLError
            if (srsFilePath != NULL)
            {
                VSIUnlink(srsFilePath);
                delete[] srsFilePath;
            }
            delete poNet;
            poDS->DeleteLayer(0);
            return NULL;
        }
        poNet->_meta_gfidcntr_i = feature->GetFID();
        OGRFeature::DestroyFeature(feature);

        // Write name.
        feature = OGRFeature::CreateFeature(newLayer->GetLayerDefn());
        feature->SetField(GNM_SYSFIELD_PARAMNAME, GNM_METAPARAM_NAME);
        const char *conName = CSLFetchNameValue(papszOptions, GNM_INIT_OPTIONPAIR_NAME);
        if (conName != NULL)
            feature->SetField(GNM_SYSFIELD_PARAMVALUE, conName);
        else // default name
            feature->SetField(GNM_SYSFIELD_PARAMVALUE, "_gnm");
        if(newLayer->CreateFeature(feature) != OGRERR_NONE)
        {
            //CPLError
            if (srsFilePath != NULL)
            {
                VSIUnlink(srsFilePath);
                delete[] srsFilePath;
            }
            delete poNet;
            poDS->DeleteLayer(0);
            return NULL;
        }
        poNet->_meta_name_i = feature->GetFID();
        OGRFeature::DestroyFeature(feature);

        // Write additional metaparameters.

        // Write description.
        const char *conDescr = CSLFetchNameValue(papszOptions, GNM_INIT_OPTIONPAIR_DESCR);
        if (conDescr != NULL)
        {
            feature = OGRFeature::CreateFeature(newLayer->GetLayerDefn());
            feature->SetField(GNM_SYSFIELD_PARAMNAME, GNM_METAPARAM_DESCR);
            feature->SetField(GNM_SYSFIELD_PARAMVALUE, conDescr);
            if(newLayer->CreateFeature(feature) != OGRERR_NONE)
            {
                // CPL Warning
            }
            poNet->_meta_descr_i = feature->GetFID();
            OGRFeature::DestroyFeature(feature);
        }

    // ---------------------------- Create graph layer ---------------------

        newLayer = poDS->CreateLayer(GNM_SYSLAYER_GRAPH,NULL,wkbNone,NULL);
        if (newLayer == NULL)
        {
            //CPLError
            if (srsFilePath != NULL)
            {
                VSIUnlink(srsFilePath);
                delete[] srsFilePath;
            }
            delete poNet;
            poDS->DeleteLayer(0);
            return NULL;
        }

        newField1.Set(GNM_SYSFIELD_SOURCE, OFTInteger);
        newField2.Set(GNM_SYSFIELD_TARGET, OFTInteger);
        newField3.Set(GNM_SYSFIELD_CONNECTOR, OFTInteger);
        newField4.Set(GNM_SYSFIELD_COST, OFTReal);
        newField5.Set(GNM_SYSFIELD_INVCOST, OFTReal);
        newField6.Set(GNM_SYSFIELD_DIRECTION, OFTInteger);
        if(newLayer->CreateField(&newField1) != OGRERR_NONE
           || newLayer->CreateField(&newField2) != OGRERR_NONE
           || newLayer->CreateField(&newField3) != OGRERR_NONE
           || newLayer->CreateField(&newField4) != OGRERR_NONE
           || newLayer->CreateField(&newField5) != OGRERR_NONE
           || newLayer->CreateField(&newField6) != OGRERR_NONE)
        {
            //CPLError
            if (srsFilePath != NULL)
            {
                VSIUnlink(srsFilePath);
                delete[] srsFilePath;
            }
            delete poNet;
            for (int i=0;i<2;i++) // delete meta and graph layers
                poDS->DeleteLayer(0);
            return NULL;
        }

    // ---------------------------- Create classes layer ---------------------

        newLayer = poDS->CreateLayer(GNM_SYSLAYER_CLASSES,NULL,wkbNone,NULL);
        if (newLayer == NULL)
        {
            //CPLError
            if (srsFilePath != NULL)
            {
                VSIUnlink(srsFilePath);
                delete[] srsFilePath;
            }
            delete poNet;
            for (int i=0;i<2;i++) // delete meta and graph layers
                poDS->DeleteLayer(0);
            return NULL;
        }

        newField1.Set(GNM_SYSFIELD_LAYERNAME, OFTString);
        if(newLayer->CreateField(&newField1) != OGRERR_NONE)
        {
            //CPLError
            if (srsFilePath != NULL)
            {
                VSIUnlink(srsFilePath);
                delete[] srsFilePath;
            }
            delete poNet;
            for (int i=0;i<3;i++)
                poDS->DeleteLayer(0); // delete meta, graph and class layers
            return NULL;
        }

        // Create the first class layer record: for system edges class layer.
        // The _gnm_sysedges layer itself will be created a bit later.
        feature = OGRFeature::CreateFeature(newLayer->GetLayerDefn());
        feature->SetField(GNM_SYSFIELD_LAYERNAME, GNM_CLASSLAYER_SYSEDGES);
        if(newLayer->CreateFeature(feature) != OGRERR_NONE)
        {
            //CPLError
            if (srsFilePath != NULL)
            {
                VSIUnlink(srsFilePath);
                delete[] srsFilePath;
            }
            delete poNet;
            for (int i=0;i<3;i++)
                poDS->DeleteLayer(0);
            return NULL;
        }
        OGRFeature::DestroyFeature(feature);

    // ---------------------------- Create features layer ------------------

        newLayer = poDS->CreateLayer(GNM_SYSLAYER_FEATURES,NULL,wkbNone,NULL);
        if (newLayer == NULL)
        {
            //CPLError
            if (srsFilePath != NULL)
            {
                VSIUnlink(srsFilePath);
                delete[] srsFilePath;
            }
            delete poNet;
            for (int i=0;i<3;i++)
                poDS->DeleteLayer(0); // delete meta, graph and class layers
            return NULL;
        }

        newField1.Set(GNM_SYSFIELD_GFID, OFTInteger);
        newField2.Set(GNM_SYSFIELD_LAYERNAME, OFTString);
        newField3.Set(GNM_SYSFIELD_FID, OFTInteger);
        if(newLayer->CreateField(&newField1) != OGRERR_NONE
           || newLayer->CreateField(&newField2) != OGRERR_NONE
           || newLayer->CreateField(&newField3) != OGRERR_NONE)
        {
            //CPLError
            if (srsFilePath != NULL)
            {
                VSIUnlink(srsFilePath);
                delete[] srsFilePath;
            }
            delete poNet;
            for (int i=0;i<4;i++)
                poDS->DeleteLayer(0);
            return NULL;
        }

    // ---------------------------- Create rules layer ---------------------

        newLayer = poDS->CreateLayer(GNM_SYSLAYER_RULES,NULL,wkbNone,NULL);
        if (newLayer == NULL)
        {
            //CPLError
            if (srsFilePath != NULL)
            {
                VSIUnlink(srsFilePath);
                delete[] srsFilePath;
            }
            delete poNet;
            for (int i=0;i<4;i++)
                poDS->DeleteLayer(0);
            return NULL;
        }

        newField1.Set(GNM_SYSFIELD_RULESTRING, OFTString);
        if(newLayer->CreateField(&newField1) != OGRERR_NONE)
        {
            //CPLError
            if (srsFilePath != NULL)
            {
                VSIUnlink(srsFilePath);
                delete[] srsFilePath;
            }
            delete poNet;
            for (int i=0;i<5;i++)
                poDS->DeleteLayer(0);
            return NULL;
        }

    // -------------------- Create system edges class layer ---------------------

        newLayer = poDS->CreateLayer(GNM_CLASSLAYER_SYSEDGES,NULL,wkbNone,NULL);
        if (newLayer == NULL)
        {
            //CPLError
            if (srsFilePath != NULL)
            {
                VSIUnlink(srsFilePath);
                delete[] srsFilePath;
            }
            delete poNet;
            for (int i=0;i<5;i++)
                poDS->DeleteLayer(0);
            return NULL;
        }

        newField1.Set(GNM_SYSFIELD_GFID, OFTInteger);
        if(newLayer->CreateField(&newField1) != OGRERR_NONE)
        {
            //CPLError
            if (srsFilePath != NULL)
            {
                VSIUnlink(srsFilePath);
                delete[] srsFilePath;
            }
            delete poNet;
            for (int i=0;i<6;i++)
                poDS->DeleteLayer(0);
            return NULL;
        }

        // Insert the pointer to this class into the array of classes.
        poNet->_classesSet.insert(newLayer);

        if (srsFilePath != NULL)
            delete[] srsFilePath;

    // ---------------- Return the resulting network  ----------------------

        poNet->_poDataSet = poDS;
        return poNet;
    }

    else if (bNative == TRUE)
    {
        // Initialize a native network.
        // Try to use the special network format if it is found
        // among registered network formats.
        for (unsigned i=0; i<_regNetFrmts.size(); i++)
        {
            if (strcmp(_regNetFrmts[i].data(),driverName) == 0)
            {
                GNMFormat *poFormat = (GNMFormat*)GetFormatByName(driverName);
                GNMNetwork *poNet = poFormat->create(poDS,papszOptions);
                delete poFormat;
                return poNet;
            }
        }
    }

    return NULL;
}


/************************************************************************/
/*                                  Open()                              */
/************************************************************************/
/**
 * \brief Opens the network.
 *
 * Opens the network.
 * NOTE: This method checks if the connectivity exists in a dataset via
 * GNMManager::HasConnectivity() firstly.
 * NOTE: After the successfull opening the passed dataset must not be
 * mofified outside (but can be readed as usual).
 *
 * @param poDS The dataset to open a network.
 * @param bNative If bNative set to TRUE method tries to open a network as
 * native network format, otherwise openes as the GDAL-network.
 * @param papszOptions Additional options.
 *
 * @return The concrete network object or NULL if failed to open.
 *
 * @see GNMManager::GdalOpenNetwork()
 *
 * @since GDAL 2.0
 */
GNMNetwork *GNMManager::Open (GDALDataset *poDS,int bNative,char **papszOptions)
{
    if (poDS == NULL)
        return NULL;

    if (!HasConnectivity(poDS,bNative))
        return NULL;

    const char *driverName = poDS->GetDriverName();
    if (strcmp(driverName,"") == 0)
        return NULL;

    if (bNative == FALSE)
    {
        GNMGdalNetwork *poNet = new GNMGdalNetwork();

        //poNet->_driverName = new char [50];
        //strcpy(poNet->_driverName,driverName);

        OGRLayer* layer = poDS->GetLayerByName(GNM_SYSLAYER_META);
        OGRFeature *feature;

        // Load and check metadata parameters.
        layer->ResetReading();
        while ((feature = layer->GetNextFeature()) != NULL)
        {
            const char *paramName = feature->GetFieldAsString(GNM_SYSFIELD_PARAMNAME);
            const char *paramValue = NULL;
            paramValue = feature->GetFieldAsString(GNM_SYSFIELD_PARAMVALUE);

            if (strcmp(paramName,GNM_METAPARAM_VERSION) == 0
                && paramValue != NULL
                && strcmp(paramValue,"") != 0)
            {
                // TODO: somehow check the version as regular expression: X.X.X
                // TODO: Check if GNM version is equal to opening connectivity version.
                poNet->_meta_vrsn_i = feature->GetFID();
            }

            else if (strcmp(paramName,GNM_METAPARAM_SRS) == 0)
            {
                // Load SRS.
                if (paramValue == NULL)
                {
                    //CPLErr
                    delete poNet;
                    OGRFeature::DestroyFeature(feature);
                    return NULL;
                }
                else
                {
                    // Load srs from separate file if needed (see comments in
                    // the creation method about this issue).

                    // Create new srs object.
                    poNet->_poSpatialRef = new OGRSpatialReference();

                    if (strcmp(paramValue,"file") == 0)
                    {
                        char *srsFilePath = new char [strlen(poDS->GetDescription()) +
                                strlen(GNM_SRSFILENAME) + 2];
                        sprintf(srsFilePath,"%s/%s",poDS->GetDescription(),GNM_SRSFILENAME);

                        // Read wkt srs and import it. We can load from wkt
                        // because user defined user-input of srs during the
                        // network creation and after that srs was exported
                        // to wkt.

                        char **srsWktStr = CSLLoad(srsFilePath);
                        char *toImport = *srsWktStr; // we must save the pointer to delete it later (see importFromWkt description why)
                        if (srsWktStr == NULL ||
                            poNet->_poSpatialRef->importFromWkt(&toImport)
                                != OGRERR_NONE)
                        {
                            delete poNet; // which deletes srs
                            CSLDestroy(srsWktStr);
                            OGRFeature::DestroyFeature(feature);
                            delete[] srsFilePath;
                            return NULL;
                        }

                        // Free resources.
                        CSLDestroy(srsWktStr);
                        delete[] srsFilePath;
                    }
                    else
                    {
                        // Load SRS from the system layer. The user input was
                        // defined during the creation of the network.
                        if (poNet->_poSpatialRef->SetFromUserInput(paramValue)
                                != OGRERR_NONE)
                        {
                            delete poNet;
                            OGRFeature::DestroyFeature(feature);
                            return NULL;
                        }
                    }

                    poNet->_meta_srs_i = feature->GetFID();
                }
            }

            else if (strcmp(paramName,GNM_METAPARAM_GFIDCNT) == 0)
            {
                // TODO: Check if the counter < 0.
                poNet->_meta_gfidcntr_i = feature->GetFID();
            }

            else if (strcmp(paramName,GNM_METAPARAM_NAME) == 0)
            {
                poNet->_meta_name_i = feature->GetFID();
            }

            else if (strcmp(paramName,GNM_METAPARAM_DESCR) == 0)
            {
                poNet->_meta_descr_i = feature->GetFID();
            }

            OGRFeature::DestroyFeature(feature);
        }

        // Load the set of class layers.
        layer = poDS->GetLayerByName(GNM_SYSLAYER_CLASSES);
        layer->ResetReading();
        while ((feature = layer->GetNextFeature()) != NULL)
        {
            poNet->_classesSet.insert(poDS->GetLayerByName(
                              feature->GetFieldAsString(GNM_SYSFIELD_LAYERNAME)));
            OGRFeature::DestroyFeature(feature);
        }

        // Build all rule strings.
        layer = poDS->GetLayerByName(GNM_SYSLAYER_RULES);
        layer->ResetReading();
        while ((feature = layer->GetNextFeature()) != NULL)
        {
            if (poNet->_parseRuleString(feature->GetFieldAsString(GNM_SYSFIELD_RULESTRING)))
            {
                delete poNet;
                OGRFeature::DestroyFeature(feature);
                return NULL;
            }
            OGRFeature::DestroyFeature(feature);
        }

        // Return the resulting network.
        poNet->_poDataSet = poDS;
        return poNet;
    }

    else if (bNative == TRUE)
    {
        // Initialize a native network.
        // Try to use the special network format if it is found
        // among registered network formats.
        for (unsigned i=0; i<_regNetFrmts.size(); i++)
        {
            if (strcmp(_regNetFrmts[i].data(),driverName) == 0)
            {
                GNMFormat *poFormat = (GNMFormat*)GetFormatByName(driverName);
                GNMNetwork *poNet = poFormat->open(poDS,papszOptions);
                delete poFormat;
                return poNet;
            }
        }
    }

    return NULL;
}


/************************************************************************/
/*                                  Close()                             */
/************************************************************************/
/**
 * \brief Closes network.
 *
 * Frees the resources of the network.
 *
 * @param poNet The network to close.
 * @param bNative If bNative set to TRUE method tries to close network with its
 * specific format.
 *
 * @see GNMManager::GdalCloseNetwork()
 *
 * @since GDAL 2.0
 */
void GNMManager::Close (GNMNetwork *poNet,int bNative)
{
    // ISSUE: what about reference/dereference?

    if (poNet == NULL)
        return;

    if (bNative == FALSE)
    {
        delete poNet;
    }

    else if (bNative == TRUE)
    {
        // Try to use the special network format if it is found
        // among registered network formats.
        const char *driverName = poNet->GetDataset()->GetDriverName();
        for (unsigned i=0; i<_regNetFrmts.size(); i++)
        {
            if (strcmp(_regNetFrmts[i].data(),driverName) == 0)
            {
                GNMFormat *poFormat = (GNMFormat*)GetFormatByName(driverName);
                poFormat->close(poNet);
                delete poFormat;
            }
        }
    }
}


/************************************************************************/
/*                      RemoveConnectivity()                            */
/************************************************************************/
/**
 * \brief Removes the connectivity.
 *
 * Removes the connectivity, trying to delete system layers/fields/etc
 * leaving the "pure of connectivity" dataset and frees the the allocated
 * resources. Does not work for GDAL-networks: use GdalDeleteNetwork() for
 * network deletion. The network should be closed before calling this method.
 *
 * @see GNMManager::GdalDeleteNetwork()
 *
 * @since GDAL 2.0
 */
GNMErr GNMManager::RemoveConnectivity(GDALDataset *poDS,int bNative)
{
    if (poDS == NULL)
        return GNMERR_FAILURE;

    if (bNative == FALSE)
    {
        // We do not support the deleteion of system layers/fields
        // in the GNMGdalNetwork separate from the whole network.
        return GNMERR_FAILURE;
    }

    else if (bNative == TRUE)
    {
        // Try to use the special network format if it is found
        // among registered network formats.
        const char *driverName = poDS->GetDriverName();
        for (unsigned i=0; i<_regNetFrmts.size(); i++)
        {
            if (strcmp(_regNetFrmts[i].data(),driverName) == 0)
            {
                GNMFormat *poFormat = (GNMFormat*)GetFormatByName(driverName);
                GNMErr err = poFormat->remove(poDS);
                delete poFormat;
                return err;
            }
        }
    }

    return GNMERR_FAILURE;
}


/************************************************************************/
/*                        HasConnectivity()                             */
/************************************************************************/
/**
 * \brief Checks if the connectivity exists in the given dataset.
 *
 * Checks if the connectivity exists in the given dataset. Set bNative to TRUE
 * if it is necessary to check the connectivity of specific format.
 *
 * @return True if exists or false if not.
 *
 * @since GDAL 2.0
 */
bool GNMManager::HasConnectivity (GDALDataset *poDS,int bNative)
{
    if (poDS == NULL)
        false;

    if (bNative == FALSE)
    {
        // Check for the supported vector formats.
        if (!GNMGdalNetwork::IsDriverSupported(poDS))
            return false;

        // Check the existance of system layers and their correct fields.
        bool hasSrsFile = false;
        OGRLayer *layer;
        layer = poDS->GetLayerByName(GNM_SYSLAYER_META);
        if (layer == NULL
            || layer->GetLayerDefn()->GetFieldIndex(GNM_SYSFIELD_PARAMNAME) == -1
            || layer->GetLayerDefn()->GetFieldIndex(GNM_SYSFIELD_PARAMVALUE) == -1)
        {
            return false;
        }
        else
        {
            // Check for the obligated metadata.
            bool hasVersion = false;
            bool hasSRS = false;
            bool hasIdCounter = false;
            bool hasName = false;
            OGRFeature *feature;
            layer->ResetReading();
            while ((feature = layer->GetNextFeature()) != NULL)
            {
                const char *paramName = feature->GetFieldAsString(GNM_SYSFIELD_PARAMNAME);
                if (strcmp(paramName,GNM_METAPARAM_SRS) == 0)
                {
                    hasSRS = true;
                    const char *paramValue = feature->GetFieldAsString(GNM_SYSFIELD_PARAMVALUE);
                    if (strcmp(paramValue,"file") == 0)
                    {
                        hasSrsFile = true; // to check its existance a bit later
                    }
                }
                else if (strcmp(paramName,GNM_METAPARAM_VERSION) == 0) hasVersion = true;
                else if (strcmp(paramName,GNM_METAPARAM_GFIDCNT) == 0) hasIdCounter = true;
                else if (strcmp(paramName,GNM_METAPARAM_NAME) == 0) hasName = true;
                OGRFeature::DestroyFeature(feature);
            }
            if (!hasSRS || !hasVersion || !hasIdCounter || !hasName)
                return false;
        }

        // Check for graph layer.
        layer = poDS->GetLayerByName(GNM_SYSLAYER_GRAPH);
        if (layer == NULL
            || layer->GetLayerDefn()->GetFieldIndex(GNM_SYSFIELD_SOURCE) == -1
            || layer->GetLayerDefn()->GetFieldIndex(GNM_SYSFIELD_TARGET) == -1
            || layer->GetLayerDefn()->GetFieldIndex(GNM_SYSFIELD_CONNECTOR) == -1
            || layer->GetLayerDefn()->GetFieldIndex(GNM_SYSFIELD_COST) == -1
            || layer->GetLayerDefn()->GetFieldIndex(GNM_SYSFIELD_INVCOST) == -1
            || layer->GetLayerDefn()->GetFieldIndex(GNM_SYSFIELD_DIRECTION) == -1)
        {
            return false;
        }

        // Check for features layer.
        layer = poDS->GetLayerByName(GNM_SYSLAYER_FEATURES);
        if (layer == NULL
            || layer->GetLayerDefn()->GetFieldIndex(GNM_SYSFIELD_GFID) == -1
            || layer->GetLayerDefn()->GetFieldIndex(GNM_SYSFIELD_LAYERNAME) == -1
            || layer->GetLayerDefn()->GetFieldIndex(GNM_SYSFIELD_FID) == -1)
        {
            return false;
        }

        // Check for rules layer.
        layer = poDS->GetLayerByName(GNM_SYSLAYER_RULES);
        if (layer == NULL
            || layer->GetLayerDefn()->GetFieldIndex(GNM_SYSFIELD_RULESTRING) == -1)
        {
            return false;
        }

        //// Check for system edges layer.
        //layer = poDS->GetLayerByName(GNM_CLASSLAYER_SYSEDGES);
        //if (layer == NULL
        //    || layer->GetLayerDefn()->GetFieldIndex(GNM_SYSFIELD_GFID) == -1)
        //{
        //    return false;
        //}

        // Check for classes layer.
        layer = poDS->GetLayerByName(GNM_SYSLAYER_CLASSES);
        if (layer == NULL
            || layer->GetLayerDefn()->GetFieldIndex(GNM_SYSFIELD_LAYERNAME) == -1)
        {
            return false;
        }

        // Check if all classes have the gfid field.
        // It is inappropriate for some format cases to use GetLayerCount(), so we
        // use the _gnm_classes system layer in order to check each class layer.
        // See format note at gnm.h.
        layer->ResetReading();
        OGRFeature *feature;
        while ((feature = layer->GetNextFeature()) != NULL)
        {
            const char *layerName = feature->GetFieldAsString(GNM_SYSFIELD_LAYERNAME);
            OGRLayer *checkLayer = poDS->GetLayerByName(layerName);
            if (checkLayer->GetLayerDefn()->GetFieldIndex(GNM_SYSFIELD_GFID) == -1)
            {
                OGRFeature::DestroyFeature(feature);
                return false;
            }
            OGRFeature::DestroyFeature(feature);
        }

        // Check for srs file if the network contains it.
        if (hasSrsFile)
        {
            // Open file by the path of DS and special name to check it.
            VSILFILE *fpSrsPrj;
            char *srsPrjStr = new char [strlen(poDS->GetDescription()) +
                    strlen(GNM_SRSFILENAME) + 2];
            sprintf(srsPrjStr,"%s/%s",poDS->GetDescription(),GNM_SRSFILENAME);
            fpSrsPrj = VSIFOpenL(srsPrjStr, "r");
            if (fpSrsPrj == NULL)
            {
                //CPLErr
                VSIFCloseL(fpSrsPrj);
                delete[] srsPrjStr;
                return false;
            }
            VSIFCloseL(fpSrsPrj);
            delete[] srsPrjStr;
        }

        return true;
    }

    else if (bNative == TRUE)
    {
        // Try to use the special network format if it is found
        // among registered network formats.
        const char *driverName = poDS->GetDriverName();
        for (unsigned i=0; i<_regNetFrmts.size(); i++)
        {
            if (strcmp(_regNetFrmts[i].data(),driverName) == 0)
            {
                GNMFormat *poFormat = (GNMFormat*)GetFormatByName(driverName);
                bool ret = poFormat->has(poDS);
                delete poFormat;
                return ret;
            }
        }
    }

    return false;
}


/************************************************************************/
/*                      GdalCreateNetwork()                             */
/************************************************************************/
/**
 * \brief Creates a network in a GDAL-network format.
 *
 * Creates a connectivity on an instantly created void dataset directly
 * in a GDAL-network format returning the resulting network, that means
 * that the resulting network will be created void (without any features
 * or connections). Use GNMGdalNetwork::CopyLayer() to populate network
 * with features and GNMGdalNetwork::ConnectFeatures() or GNMGdalNetwork::
 * AutoConnect() to set the connections among them.
 * NOTE: the created GNMGdalNetwork must be freed via GNMManager::
 * Close() or GNMManager::GdalCloseNetwork().
 * NOTE: the created according dataset remains owned by the GNMGdalNetwork.
 * You can get it with GNMGdalNetwork::GetDataset() but don't modify or free
 * this dataset manually.
 *
 * @param pszName The path and the name of the creted dataset.
 * @param pszFormat The name of the dataset format (e.g. ESRI Shapefile).
 * @param pszSrsInput The name of the SRS in order to set it from user
 * input (via SetFromUserInput() method).
 * @param papszConOptions Various creation option-pairs formed with
 * CSLAddNameValue(), for example:
 * 1. "con_name" "my_network" (Inner network's name).
 * 2. "con_descr" "This is my network" (Network's description).
 * @param papszDatasetOptions List of driver specific control parameters.
 * @return GNMGdalNetwork object or NULL if failed.
 *
 * @see GNMManager::CreateConnectivity()
 *
 * @since GDAL 2.0
 */
GNMGdalNetwork *GNMManager::GdalCreateNetwork (const char *pszName,
                                          const char *pszFormat,
                                          const char *pszSrsInput,
                                          char **papszOptions,
                                          char **papszDatasetOptions)
{
    //GDALAllRegister();

    if (pszName == NULL || pszFormat == NULL || pszSrsInput == NULL)
        return NULL;

    GDALDriver *poDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    if(poDriver == NULL)
    {
        //CPLErr
        return NULL;
    }

	GDALDataset *poDS;
	// As PostGIS driver does not support creation of dataset, we try to open the existed database.
	if (strcmp(pszFormat,"PostGIS") == 0 || strcmp(pszFormat,"PostgreSQL") == 0)
	{
		poDS = (GDALDataset*) GDALOpenEx(pszName, GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                                     NULL, NULL, NULL);
	}
	else
	{
		poDS = poDriver->Create(pszName, 0, 0, 0, GDT_Unknown, papszDatasetOptions);
	}
    if(poDS == NULL)
    {
        //CPLErr
        return NULL;
    }

    // Replace or create SRS string in options.
    // NOTE: We duplicate stringlist otherwise error when try to CSLDestroy
    // papszOptions outside after the call of GdalCreateNetwork().
    char **options = NULL;
    options = CSLDuplicate(papszOptions);
    options = CSLSetNameValue(options,GNM_INIT_OPTIONPAIR_SRS,pszSrsInput);

    // Create network.
    GNMGdalNetwork *poNet;
    poNet = (GNMGdalNetwork*)CreateConnectivity(poDS,FALSE,options);
    CSLDestroy(options);
    if (poNet == NULL)
    {
        //CPLErr
        GDALClose(poDS);
        return NULL;
    }

    poNet->_ownDataset = true;
    return poNet;
}


/************************************************************************/
/*                      GdalOpenNetwork()                               */
/************************************************************************/
/**
 * \brief Openes network in a GDAL-network format.
 *
 * Openes network in a GNMGdalNetwork format directly.
 * NOTE: the created according dataset remains owned by the GNMGdalNetwork.
 * Don't modify or free it manually.
 *
 * @param pszName The path and the name of the opened dataset with connectivity.
 * @return GNMGdalNetwork object or NULL if failed.
 *
 * @see GNMManager::Open()
 *
 * @since GDAL 2.0
 */
GNMGdalNetwork *GNMManager::GdalOpenNetwork (const char *pszName,char **papszOptions)
{
    //GDALAllRegister();

    GDALDataset *poDS;
    poDS = (GDALDataset*) GDALOpenEx(pszName, GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                                     NULL, NULL, NULL );
    if(poDS == NULL)
    {
        //CPLErr
        return NULL;
    }

    GNMGdalNetwork *poNet;
    poNet = (GNMGdalNetwork*)Open(poDS,FALSE,papszOptions);
    if (poNet == NULL)
    {
        //CPLErr
        GDALClose(poDS);
        return NULL;
    }

    poNet->_ownDataset = true;
    return poNet;
}


/************************************************************************/
/*                      GdalCloseNetwork()                              */
/************************************************************************/
/**
 * \brief Frees the resources of GDAL-network.
 *
 * Closes GNMGdalNetwork freeing allocated resources.
 *
 * @param poNet Network to close.
 *
 * @see GNMManager::Close()
 *
 * @since GDAL 2.0
 */
void GNMManager::GdalCloseNetwork (GNMGdalNetwork *poNet)
{
    if (poNet != NULL)
        delete poNet;
}


/************************************************************************/
/*                      GdalDeleteNetwork()                             */
/************************************************************************/
/**
 * \brief Deletes GDAL-network.
 *
 * Deletes GDAL-network with its dataset. The deletion of layers is made with
 * the help of the network layers list, that means that only the system/class
 * layers of the dataset will be deleted (i.e. the entire network), contrary to
 * the way when for example for Shapefiles the whole directory of shapes is deleted.
 * NOTE: Any opened network by the passed dataset name (path) must be closed
 * before calling this method.
 *
 * @param pszName The path and the name of the dataset to delete.
 * @return GNMERR_NONE if ok.
 *
 * @see GNMManager::RemoveConnectivity()
 *
 * @since GDAL 2.0
 */
GNMErr GNMManager::GdalDeleteNetwork (const char *pszName)
{
    GDALDataset *poDS = (GDALDataset*) GDALOpenEx(pszName,
                                     GDAL_OF_VECTOR | GDAL_OF_UPDATE,
                                     NULL, NULL, NULL);
    if(poDS == NULL)
    {
        return GNMERR_FAILURE;
    }

    // We also validate this way that the passed dataset has
    // GDAL-network format.
    GNMGdalNetwork *poNet = (GNMGdalNetwork*)Open(poDS);
    if (poNet == NULL)
    {
        // ISSUE: What if the passed network/dataset is a corrupt network and
        // we can not open it?
        // May be it is ok to try to delete the dataset with GDALDriver::Delete()
        // (closing ds before)? But in this case there are some format issues.
        return GNMERR_FAILURE;
    }

    std::pair <char**,int> names = poNet->GetClassLayers();
    names.first = CSLAddString(names.first, GNM_SYSLAYER_META);
    names.first = CSLAddString(names.first, GNM_SYSLAYER_GRAPH);
    names.first = CSLAddString(names.first, GNM_SYSLAYER_CLASSES);
    names.first = CSLAddString(names.first, GNM_SYSLAYER_FEATURES);
    names.first = CSLAddString(names.first, GNM_SYSLAYER_RULES);
    names.second += 5;

    // Free connectivity resources.
    GdalCloseNetwork(poNet);//Close(poNet);

    // Delete all class and system layers.
    // We won't use GDALDriver::Delete here, because some drivers
    // can delete all directory and even those files which are not
    // included in the connectivity and which were added to the
    // directory manually.
    for (int i = 0; i < names.second; i++)
    {
        int allLayerCount = poDS->GetLayerCount();
        for (int j = 0; j < allLayerCount; j++)
        {
            if (strcmp(poDS->GetLayer(j)->GetName(), names.first[i]) == 0)
            {
                if (poDS->DeleteLayer(j) != OGRERR_NONE)
                {
                    GDALClose(poDS);
                    CSLDestroy(names.first);
                    return GNMERR_FAILURE;
                }
                break;
            }
        }
    }

    // Delete SRS file if it exists.
    char *srsFilePath = new char [strlen(poDS->GetDescription()) +
                strlen(GNM_SRSFILENAME) + 2];
    sprintf(srsFilePath,"%s/%s",poDS->GetDescription(),GNM_SRSFILENAME);
    VSIUnlink(srsFilePath);
    delete[] srsFilePath;

    GDALClose(poDS);
    CSLDestroy(names.first);
    return GNMERR_NONE;
}


/* ==================================================================== */
/*                        C API implementation                          */
/* ==================================================================== */


/************************************************************************/
/*                      GNMGdalCreateNetwork()                          */
/************************************************************************/
/**
 * \brief Creates GDAL-network.
 *
 * @see GNMManager::GdalCreateNetwork()
 *
 * @since GDAL 2.0
 */
GNMGdalNetworkH GNMGdalCreateNetwork (const char *pszName,const char *pszFormat,
                                      const char *pszSrsInput,char **papszOptions,
                                      char **papszDatasetOptions)
{
    VALIDATE_POINTER1( pszName, "GNMGdalCreateNetwork", NULL );
    VALIDATE_POINTER1( pszFormat, "GNMGdalCreateNetwork", NULL );
    VALIDATE_POINTER1( pszSrsInput, "GNMGdalCreateNetwork", NULL );

    return (GNMGdalNetworkH) GNMManager::GdalCreateNetwork(pszName,pszFormat,
                                  pszSrsInput,papszOptions,papszDatasetOptions);
}


/************************************************************************/
/*                      GNMGdalOpenNetwork()                          */
/************************************************************************/
/**
 * \brief Opens GDAL-network.
 *
 * @see GNMManager::GdalOpenNetwork()
 *
 * @since GDAL 2.0
 */
GNMGdalNetworkH GNMGdalOpenNetwork (const char *pszName,char **papszOptions)
{
    VALIDATE_POINTER1( pszName, "GNMGdalOpenNetwork", NULL );

    return (GNMGdalNetworkH) GNMManager::GdalOpenNetwork(pszName,papszOptions);
}


/************************************************************************/
/*                      GNMGdalCloseNetwork()                          */
/************************************************************************/
/**
 * \brief Closes GDAL-network.
 *
 * @see GNMManager::GdalCloseNetwork()
 *
 * @since GDAL 2.0
 */
void GNMGdalCloseNetwork (GNMGdalNetworkH hNet)
{
    VALIDATE_POINTER0( hNet, "GNMGdalCloseNetwork" );

    GNMManager::GdalCloseNetwork((GNMGdalNetwork*)hNet);
}


/************************************************************************/
/*                      GNMGdalDeleteNetwork()                          */
/************************************************************************/
/**
 * \brief Deletes GDAL-network.
 *
 * @see GNMManager::GdalDeleteNetwork()
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalDeleteNetwork (const char *pszName)
{
    VALIDATE_POINTER1( pszName, "GNMGdalDeleteNetwork", GNMERR_FAILURE );

    return GNMManager::GdalDeleteNetwork(pszName);
}
