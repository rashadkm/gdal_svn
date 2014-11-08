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
#include "analysis/gnmstdanalysis.h"

typedef void GNMNetworkShadow;
typedef void GNMGdalNetworkShadow;
typedef void GDALDatasetShadow;
typedef void OGRLayerShadow;
typedef void GNMGdalStdAnalyserShadow;
typedef void GNMGdalStdRoutingAnalyserShadow;
typedef void OGRFeatureShadow;
%}


%include "std_vector.i"
%include "std_pair.i"
namespace std {
    %template(Pair) pair<int,int>;
    %template(PairVector) vector<pair<int,int> >;
}


// Duplicate error codes.
typedef int GNMErr;
#define GNMERR_NONE 0
#define GNMERR_FAILURE 1
#define GNMERR_UNSUPPORTED 2
#define GNMERR_NOTFOUND 3
#define GNMERR_CONRULE_RESTRICT 4


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


/************************** new_GdalStdAnalyser **************************************/
// How else to allocate new objects which does not have special creating methods 
// (like e.g. OGRFeature::CreateFeature()) in python ?
%newobject new_GdalStdAnalyser;
%inline %{
GNMGdalStdAnalyserShadow *new_GdalStdAnalyser ()
{
    return (GNMGdalStdAnalyserShadow *) new GNMGdalStdAnalyser();
}
%}
/************************** new_GdalStdRoutingAnalyser **************************************/
%newobject new_GdalStdRoutingAnalyser;
%inline %{
GNMGdalStdRoutingAnalyserShadow *new_GdalStdRoutingAnalyser ()
{
    return (GNMGdalStdRoutingAnalyserShadow *) new GNMGdalStdRoutingAnalyser();
}
%}

/************************** GdalStdAnalyserPrepareGraph **************************************/
// TODO: Implement GNMGdalStdAnalyser class and all its subclasses here so to call analysing methods from objects.
// TODO: Or create C methods.
%inline %{
GNMErr GdalStdAnalyserPrepareGraph (GNMGdalStdAnalyserShadow *analyser, GNMGdalNetworkShadow *network)
{
    return ((GNMGdalStdAnalyser*)analyser)->PrepareGraph((GNMGdalNetwork*)network);
}
%}


/************************** GdalStdAnalyserDijkstra **************************************/
%inline %{
std::vector<std::pair<int,int> > GdalStdAnalyserDijkstra (GNMGdalStdAnalyserShadow *analyser, int startVertGFID, int endVertGFID)
{
    return (std::vector<std::pair<int,int> >)((GNMGdalStdAnalyser*)analyser)->DijkstraShortestPath(startVertGFID,endVertGFID);
}
%}


/************************** GdalGetFeatureByGFID **************************************/
// TODO: Create and use C method here.
%newobject GdalGetFeatureByGFID;
%inline %{
OGRFeatureShadow *GdalGetFeatureByGFID (GNMGdalNetworkShadow *network, int GFID)
{
    return (OGRFeatureShadow*)((GNMGdalNetwork*)network)->GetFeatureByGFID(GFID);
}
%}


/************************** GdalStdAnalyserBlockVertex **************************************/
%inline %{
GNMErr GdalStdAnalyserBlockVertex (GNMGdalStdAnalyserShadow *analyser, int GFID)
{
    return ((GNMGdalStdAnalyser*)analyser)->BlockVertex(GFID);
}
%}

/************************** GdalStdAnalyserUnblockVertex **************************************/
%inline %{
GNMErr GdalStdAnalyserUnblockVertex (GNMGdalStdAnalyserShadow *analyser, int GFID)
{
    return ((GNMGdalStdAnalyser*)analyser)->UnblockVertex(GFID);
}
%}