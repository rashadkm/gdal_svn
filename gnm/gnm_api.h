/******************************************************************************
 * $Id$
 *
 * Name:     gnm_api.h
 * Project:  GDAL/OGR Geography Network support (Geographic Network Model)
 * Purpose:  GNM C API.
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

#ifndef _GNM_API_H_INCLUDED
#define _GNM_API_H_INCLUDED


#include "gnm.h"
#include "gdal.h"
 

CPL_C_START
 
 
typedef void *GNMNetworkH;
typedef void *GNMGdalNetworkH;
typedef void *GNMStdAnalyserH;


/* ==================================================================== */
/*                            GNMManager                                */
/* ==================================================================== */

GNMGdalNetworkH CPL_DLL GNMGdalCreateNetwork (const char *pszName,
                                              const char *pszFormat,
                                              const char *pszSrsInput,
                                              char **papszOptions,
                                              char **papszDatasetOptions);

GNMGdalNetworkH CPL_DLL GNMGdalOpenNetwork (const char *pszName,
                                           char **papszOptions);

void CPL_DLL GNMGdalCloseNetwork (GNMGdalNetworkH hNet);

GNMErr CPL_DLL GNMGdalDeleteNetwork (const char *pszName);


/* ==================================================================== */
/*                          GNMNetwork                                  */
/* ==================================================================== */


GDALDatasetH CPL_DLL GNMGetDataset (GNMNetworkH hNet);

													
/* ==================================================================== */
/*                          GNMGdalNetwork                              */
/* ==================================================================== */

//GNMConnection CPL_DLL *GNMGdalGetConnection (GNMNetworkH hNet,
OGRFeatureH CPL_DLL GNMGdalGetConnection (GNMGdalNetworkH hNet,
                                         long nConFID);

OGRLayerH CPL_DLL GNMGdalCreateLayer (GNMGdalNetworkH hNet,
                                  const char *pszName,
                                  OGRFeatureDefnH hFeatureDefn,
                                  OGRSpatialReferenceH hSpatialRef,
                                  OGRwkbGeometryType eGType,
                                  char **papszOptions);

GNMErr CPL_DLL GNMGdalCopyLayer (GNMGdalNetworkH hNet,
                             OGRLayerH hLayer,
                             const char *pszNewName);

GNMGFID CPL_DLL GNMGdalCreateFeature (GNMGdalNetworkH hNet,
                                  OGRLayerH hLayer,
                                  OGRFeatureH hFeature);
								   
GNMErr CPL_DLL GNMGdalSetFeature (GNMGdalNetworkH hNet,
                              OGRLayerH hLayer,
                              OGRFeatureH hFeature);

GNMErr CPL_DLL GNMGdalDeleteFeature (GNMGdalNetworkH hNet,
                                 OGRLayerH hLayer,
                                 long nFID);

GNMErr CPL_DLL GNMGdalConnectFeatures (GNMGdalNetworkH hNet,
                                    GNMGFID nSrcGFID,
                                    GNMGFID nTgtGFID,
                                    GNMGFID nConGFID,
                                    double dCost,
                                    double dInvCost,
                                    GNMDirection nDir);

GNMErr CPL_DLL GNMGdalDisconnectFeatures (GNMGdalNetworkH hNet,
                                       GNMGFID nSrcGFID,
                                       GNMGFID nTgtGFID,
                                       GNMGFID nConGFID);

GNMErr CPL_DLL GNMGdalReconnectDirFeatures (GNMGdalNetworkH hNet,
                                      GNMGFID nSrcGFID,
                                      GNMGFID nTgtGFID,
                                      GNMGFID nConGFID,
                                      GNMDirection nNewDir);
									  
GNMErr CPL_DLL GNMGdalReconnectCostFeatures (GNMGdalNetworkH hNet,
                                      GNMGFID nSrcGFID,
                                      GNMGFID nTgtGFID,
                                      GNMGFID nConGFID,
                                      double dNewCost,
                                      double dNewInvCost);

//GNMErr CPL_DLL GNMGdalAutoConnect (GNMGdalNetworkH hNet,
//								   OGRLayerH *hLayers,
//                                   double dTolerance);

GNMErr CPL_DLL GNMGdalDisconnectAll (GNMGdalNetworkH hNet);

bool CPL_DLL GNMGdalIsDriverSupported (GDALDatasetH hDS);

char CPL_DLL **GNMGdalGetSupportedDrivers ();

const OGRSpatialReferenceH CPL_DLL GNMGdalGetSpatialReference (GNMGdalNetworkH hNet);

char CPL_DLL **GNMGdalGetMetaParamValues (GNMGdalNetworkH hNet);

const char CPL_DLL *GNMGdalGetMetaParamValueName (GNMGdalNetworkH hNet);

const char CPL_DLL *GNMGdalGetMetaParamValueDescr (GNMGdalNetworkH hNet);

GNMErr CPL_DLL GNMGdalCreateRule (GNMGdalNetworkH hNet, const char *pszRuleStr);


/* ==================================================================== */
/*                          GNMStdAnalyser                              */
/* ==================================================================== */


GNMStdAnalyserH CPL_DLL GNMCreateStdAnalyser ();

void CPL_DLL GNMDeleteStdAnalyser (GNMStdAnalyserH hAnalyser);


CPL_C_END


#endif //_GNM_API_H_INCLUDED



