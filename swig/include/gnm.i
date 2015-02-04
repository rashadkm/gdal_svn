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

typedef void GNMGdalNetworkShadow;
typedef void GNMStdAnalyserShadow;

typedef void GDALDatasetShadow;
typedef void OGRLayerShadow;
typedef void OGRFeatureShadow;
%}


%include "std_vector.i"
%include "std_pair.i"
namespace std {
    %template(Pair) pair<int,int>;
    %template(PairVector) vector<pair<int,int> >;
    %template(Vector) vector<int>;
    %template(VectorVectorPair) vector<vector<pair<int,int> > >;
    %template(LayerVector) vector<OGRLayerShadow*>;
}


// Duplicate error codes.
typedef int GNMErr;
#define GNMERR_NONE 0
#define GNMERR_FAILURE 1
#define GNMERR_UNSUPPORTED 2
#define GNMERR_NOTFOUND 3
#define GNMERR_CONRULE_RESTRICT 4


/************************************************************************/
/*                     GNMGdalNetworkShadow                             */
/************************************************************************/


%rename (GdalNetwork) GNMGdalNetworkShadow;
class GNMGdalNetworkShadow
{
    GNMGdalNetworkShadow(){}
    
    public:
    
    %extend 
    {
    
        ~GNMGdalNetworkShadow()
        {
            GNMGdalCloseNetwork ((GNMGdalNetworkH)self);
        }

        GNMErr CopyLayer (OGRLayerShadow *sLayer, const char *pszNewName)
        {
            return GNMGdalCopyLayer ((GNMGdalNetworkH)self, (OGRLayerH)sLayer, pszNewName);
        }

        GDALDatasetShadow *GetDataset ()
        {
            return (GDALDatasetShadow *) GNMGetDataset ((GNMGdalNetworkH)self);
        }

        GNMErr CreateRule (const char *pszRuleStr)
        {
            return GNMGdalCreateRule ((GNMGdalNetworkH)self, pszRuleStr);
        }

        GNMErr Connect (int nSrcGFID,int nTgtGFID, int nConGFID,double dCost, double dInvCost,int nDir)
        {
            return GNMGdalConnectFeatures ((GNMGdalNetworkH)self,
            (GNMGFID)nSrcGFID,(GNMGFID)nTgtGFID,(GNMGFID)nConGFID,
            dCost,dInvCost,(GNMDirection)nDir);
        }
        
        %newobject GetFeatureByGFID;
        OGRFeatureShadow *GetFeatureByGFID (int GFID)
        {
            return (OGRFeatureShadow*)((GNMGdalNetwork*)self)->GetFeatureByGFID(GFID);
        }
        
        //GNMErr AutoConnect (OGRLayerShadow** sLayers, double dSnapTol)
        GNMErr AutoConnect (std::vector<OGRLayerShadow*> sLayers, double dSnapTol)
        {
            //return GNMGdalAutoConnect((GNMGdalNetwork*)self,sLayers,dSnapTol);
            std::vector<OGRLayer*> _sLayers;
            for (int i=0; i<sLayers.size(); i++)
            {
                _sLayers.push_back((OGRLayer*)sLayers[i]);
            }
            return ((GNMGdalNetwork*)self)->AutoConnect(_sLayers,dSnapTol,NULL);
        }          
    }
};


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

%newobject GdalOpenNetwork;
%inline %{
GNMGdalNetworkShadow* GdalOpenNetwork (const char *pszName,char **papszOptions=NULL)
{
    return (GNMGdalNetworkShadow*)GNMGdalOpenNetwork(pszName,papszOptions);
}
%}

%inline %{
void GdalCloseNetwork (GNMGdalNetworkShadow *sNet)
{
    GNMGdalCloseNetwork ((GNMGdalNetworkH)sNet);
}
%}

%inline %{
GNMErr GdalDeleteNetwork (const char *pszNewName)
{
    return GNMGdalDeleteNetwork (pszNewName);
}
%}



/************************************************************************/
/*                     GNMStdAnalyserShadow                             */
/************************************************************************/

%newobject CreateStdAnalyser;
%inline %{
GNMStdAnalyserShadow *CreateStdAnalyser ()
{
    return (GNMStdAnalyserShadow*) GNMCreateStdAnalyser();
}
%}

%rename (StdAnalyser) GNMStdAnalyserShadow;
class GNMStdAnalyserShadow
{
    GNMStdAnalyserShadow(){}
    
    public:
    
    %extend 
    {
        ~GNMStdAnalyserShadow()
        {
            GNMDeleteStdAnalyser((GNMStdAnalyserH)self);
        }
        
        GNMErr PrepareGraph (GNMGdalNetworkShadow *network)
        {
            return ((GNMStdAnalyser*)self)->PrepareGraph((GNMGdalNetwork*)network);
        }
        
        std::vector<std::pair<int,int> > DijkstraShortestPath (int startVertGFID, int endVertGFID)
        {
            return (std::vector<std::pair<int,int> >)((GNMStdAnalyser*)self)->DijkstraShortestPath(startVertGFID,endVertGFID);
        }
        
        std::vector<int> ConnectedComponents (std::vector<int> startVertGFIDs)
        {
            return (std::vector<int>)((GNMStdAnalyser*)self)->ConnectedComponents(startVertGFIDs);
        }
        
        std::vector<std::vector<std::pair<int,int> > > YensShortestPaths (int startVertGFID, int endVertGFID, int K)
        {
            return (std::vector<std::vector<std::pair<int,int> > >)((GNMStdAnalyser*)self)->YensShortestPaths(startVertGFID,endVertGFID,K);
        }
        
        GNMErr BlockVertex (int GFID)
        {
            return ((GNMStdAnalyser*)self)->BlockVertex(GFID);
        }
        
        GNMErr UnblockVertex (int GFID)
        {
            return ((GNMStdAnalyser*)self)->UnblockVertex(GFID);
        }
    }
};


