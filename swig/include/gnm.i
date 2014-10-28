/******************************************************************************
 * $Id$
 *
 * Project:  GNM Core SWIG Interface declarations.
 * Purpose:  GNM declarations.
 * Author:   Mikhail Gusev, gusevmihs at gmail dot com
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
 *****************************************************************************/

//%include gdal.i

%module gnm



%{
#include "gnm_api.h"

typedef void GNMNetworkShadow;
typedef void GNMGdalNetworkShadow;
typedef void GDALDatasetShadow;
typedef void OGRLayerShadow;
%}

//typedef int GNMGFIDShadow;
//typedef int GNMDirectionShadow;


//#if defined(SWIGPYTHON)
//%include "gdal_python.i"
//#endif


/************************** GdalCreateNetwork **********************************/
%newobject GdalCreateNetwork;
%inline %{
GNMGdalNetworkShadow* GdalCreateNetwork (const char *pszName,const char *pszFormat,
                                      const char *pszSrsInput,char **papszOptions = NULL,
                                      char **papszDatasetOptions = NULL)
{
    return (GNMGdalNetworkShadow*)GNMGdalCreateNetwork(pszName,pszFormat,
                                  pszSrsInput,papszOptions,papszDatasetOptions);
}
%}


/************************** GdalOpenNetwork **********************************/
%newobject GdalOpenNetwork;
%inline %{
GNMGdalNetworkShadow* GdalOpenNetwork (const char *pszName,char **papszOptions=NULL)
{
    return (GNMGdalNetworkShadow*)GNMGdalOpenNetwork(pszName,papszOptions);
}
%}


/************************** GdalCopyLayer **********************************/
%inline %{
int GdalCopyLayer (GNMGdalNetworkShadow *sNet,
                             OGRLayerShadow *sLayer,
                             const char *pszNewName)
{
    return (int) GNMGdalCopyLayer ((GNMGdalNetworkH)sNet, (OGRLayerH)sLayer, pszNewName);
}
%}


/************************** GdalDeleteNetwork **********************************/
%inline %{
void GdalCloseNetwork (GNMGdalNetworkShadow *sNet)
{
    GNMGdalCloseNetwork ((GNMGdalNetworkH)sNet);
}
%}


/************************** GdalDeleteNetwork **********************************/
%inline %{
int GdalDeleteNetwork (const char *pszNewName)
{
    return (int)GNMGdalDeleteNetwork (pszNewName);
}
%}


/************************** GetDataset **********************************/
%newobject GdalGetDataset;
%inline %{
GDALDatasetShadow *GdalGetDataset (GNMGdalNetworkShadow *sNet)
{
    return (GDALDatasetShadow *) GNMGetDataset ((GNMGdalNetworkH)sNet);
}
%}


/************************** GdalCreateRule **********************************/
%inline %{
int GdalCreateRule (GNMGdalNetworkShadow *sNet,const char *pszRuleStr)
{
    return (int) GNMGdalCreateRule ((GNMGdalNetworkH)sNet, pszRuleStr);
}
%}


/************************** GdalConnect **************************************/
/*
GNMErr GNMGdalConnectFeatures (GNMGdalNetworkH hNet,GNMGFID nSrcGFID,
                                    GNMGFID nTgtGFID,GNMGFID nConGFID,
                                    double dCost,double dInvCost,
                                    GNMDirection nDir)
{
    VALIDATE_POINTER1( hNet, "GNMGdalConnectFeatures", GNMERR_FAILURE );

    return ((GNMGdalNetwork*)hNet)->ConnectFeatures(nSrcGFID,nTgtGFID,nConGFID,
                                                dCost,dInvCost,nDir);
}
*/
%inline %{
int GdalConnect (GNMGdalNetworkShadow *sNet, int nSrcGFID,
                 int nTgtGFID, int nConGFID,
                 double dCost, double dInvCost,
                 int nDir)
{
    return (int) GNMGdalConnectFeatures ((GNMGdalNetworkH)sNet,
    (GNMGFID)nSrcGFID,(GNMGFID)nTgtGFID,(GNMGFID)nConGFID,
    dCost,dInvCost,(GNMDirection)nDir);
}
%}

