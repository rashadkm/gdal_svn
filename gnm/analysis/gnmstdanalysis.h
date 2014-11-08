/******************************************************************************
 * $Id$
 *
 * Name:     gnmstdanalysis.h
 * Project:  GDAL/OGR Geography Network support (Geographic Network Model)
 * Purpose:  GNM standard analysis classes.
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

#ifndef _GNMSTDANALYSIS_H_INCLUDED
#define _GNMSTDANALYSIS_H_INCLUDED

#include "gnm.h"

#include <queue>

// Types declarations.

struct GNMStdEdge
{
    GNMGFID srcVertGFID;
    GNMGFID tgtVertGFID;
    //GNMDirection dir;
    bool bIsBidir;
    double dirCost;
    double invCost;
};

struct GNMStdVertex
{
    std::vector<GNMGFID> outEdgeGFIDs;
    bool bIsBlkd;
};


/************************************************************************/
/*                           GNMStdAnalyser                             */
/************************************************************************/
/**
 * The simple graph class, which holds the appropriate for
 * calculations graph in memory (based on STL containers) and provides
 * several basic algorithms of this graph's analysis.
 *
 * @since GDAL 2.0
 */
class CPL_DLL GNMStdAnalyser
{
    private:

    void _traceTargets(std::queue<GNMGFID> vertexQueue,
                      std::set<GNMGFID> *markedVertIds,
                      std::vector<GNMGFID> *connectedIds);

    protected:

    GNMNetwork *_poNet; // if it is NULL the graph must be void

    // Graph arrays.
    // TODO:
    // Make multimaps with gfid keys. So the feature can occure in a graph
    // several times: 1) if there is a need to form a graph structure, which
    // vary from the network_graph; 2) so we can remove the direction field
    // in an edge and make two opposit one-direction edges instead.
    std::map<GNMGFID,GNMStdVertex> verts;
    std::map<GNMGFID,GNMStdEdge> edges;

    GNMStdAnalyser ();
    ~GNMStdAnalyser ();

    void _addVertex(GNMGFID gfid);
    //GNMErr _addEdge (GNMGFID conGfid,GNMGFID srcGfid,GNMGFID tgtGfid,GNMDirection dir);
    GNMErr _addEdge (GNMGFID conGfid,GNMGFID srcGfid,GNMGFID tgtGfid,bool isBidir);
    GNMErr _costEdge (GNMGFID gfid, double cost, double invCost);
    GNMErr _block (GNMGFID gfid);
    GNMErr _unblock (GNMGFID gfid);
    std::vector<GNMGFID> _getOutEdges (GNMGFID vertGfid);
    std::vector<GNMGFID> _getTgtVert (GNMGFID edgeGfid);
    GNMGFID _getOppositVert(GNMGFID edgeGfid, GNMGFID vertGfid);
    bool _isVertBlocked (GNMGFID vertGfid);

    public:

    // TODO: add to the parameters of PrepareGraph() the following:
    //, unsigned int nGraphFlags, char **papszOptions);
    // so to implement different types of graph: directed/undirected,
    // weighted/unweighted ...
    virtual GNMErr PrepareGraph (GNMNetwork *poNet);

    std::map<GNMGFID,GNMGFID> DijkstraShortestPathTree (GNMGFID nStartVertGFID);

    std::vector<std::pair<GNMGFID,GNMGFID> > DijkstraShortestPath (GNMGFID nStartVertGFID,
                                               GNMGFID nEndVertGFID);

    std::vector<GNMGFID> ConnectedComponents (std::vector<GNMGFID> startVertGFIDs);

    std::vector<std::vector<std::pair<GNMGFID,GNMGFID> > > YensShortestPaths (
            GNMGFID nStartVertGFID,
            GNMGFID nEndVertGFID,
            int nK);

    GNMErr BlockVertex (GNMGFID gfid) {return _block(gfid);}

    GNMErr UnblockVertex (GNMGFID gfid) {return _unblock(gfid);}

    // ... Extend the class adding new generic algorithms ...
};


/************************************************************************/
/*                        GNMGdalStdAnalyser                            */
/************************************************************************/
/**
 * Extends GNMStdAnalyser to provide the basis for implementing
 * GDAL-networks' analysis classes.
 *
 * @since GDAL 2.0
 */
class CPL_DLL GNMGdalStdAnalyser: public GNMStdAnalyser
{
    protected:

    OGRLayer *_makeResultLineLayer(std::vector<GNMGFID> lineGFIDs,
                                   const char *pszLayerName);

    public:

    GNMGdalStdAnalyser ();
    ~GNMGdalStdAnalyser ();

    GNMErr BlockFeature (GNMGFID nGFID);
    GNMErr UnblockFeature (GNMGFID nGFID);
    void UnblockAllFeatures ();

    // Creates an additional system field/layer for storing blocked features.
    virtual GNMErr PrepareGraph (GNMNetwork *poNet);
};


/************************************************************************/
/*                       GNMGdalStdRoutingAnalyser                      */
/************************************************************************/
/**
 * Extends GNMGdalStdAnalyser to provide routing functionality.
 *
 * @since GDAL 2.0
 */
class CPL_DLL GNMGdalStdRoutingAnalyser: public GNMGdalStdAnalyser
{
    public:

    GNMGdalStdRoutingAnalyser ();
    ~GNMGdalStdRoutingAnalyser ();

    OGRLayer *DijkstraShortestPath (GNMGFID nStartGFID,
                                 GNMGFID nEndGFID);

    std::vector<OGRLayer*> KShortestPaths (GNMGFID nStartGFID,
                                         GNMGFID nEndGFID,
                                         int nK);

    // ... Extend the class adding new routing methods ...
};


/************************************************************************/
/*                      GNMStdCommutationsAnalyser                      */
/************************************************************************/
/**
 * Extends GNMGdalStdAnalyser to provide network commutations analysis.
 *
 * @since GDAL 2.0
 */
class CPL_DLL GNMGdalStdCommutationsAnalyser: public GNMGdalStdAnalyser
{
    public:

    GNMGdalStdCommutationsAnalyser ();
    ~GNMGdalStdCommutationsAnalyser ();

    OGRLayer *ResourceDestribution ();

    // ... Extend the class adding new commutation analysis methods ...
};

#endif

