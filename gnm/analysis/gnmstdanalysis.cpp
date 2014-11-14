/******************************************************************************
 * $Id$
 *
 * Name:     gnmstdanalysis.cpp
 * Project:  GDAL/OGR Geography Network support (Geographic Network Model)
 * Purpose:  GNMStdAnalyser and subclasses implementation.
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

#include "gnmstdanalysis.h"
#include <limits>
#include "gnm_api.h"

/************************************************************************/
/*                   Constructor&destructor                             */
/************************************************************************/
GNMStdAnalyser::GNMStdAnalyser()
{
    _poNet = NULL;
}
GNMStdAnalyser::~GNMStdAnalyser()
{
}


/************************************************************************/
/*                        _traceTargets()                               */
/************************************************************************/
// This method is an implementation of one Breadth-first search
// iteration.
void GNMStdAnalyser::_traceTargets(std::queue<GNMGFID> vertexQueue,
      std::set<GNMGFID> *markedVertIds,std::vector<GNMGFID> *connectedIds)
{
    std::vector<GNMGFID>::iterator it;
    std::queue<GNMGFID> neighbours_queue;

    // See all given vertexes except thouse that have been already seen.
    while (!vertexQueue.empty())
    {
        GNMGFID cur_vert_id = vertexQueue.front();

        // There may be duplicate unmarked vertexes in a current queue. Check it.
        if (markedVertIds->find(cur_vert_id) == markedVertIds->end())
        {
            markedVertIds->insert(cur_vert_id);

            // See all outcome edges, add them to connected and than see the target
            // vertex of each edge. Add it to the queue, which will be recursively
            // seen the same way on the next iteration.
            std::vector<GNMGFID> outcome_edge_ids = _getOutEdges(cur_vert_id);
            for (it = outcome_edge_ids.begin(); it != outcome_edge_ids.end(); ++it)
            {
                GNMGFID cur_edge_id = *it;

                // ISSUE: think about to return a sequence of vertexes and edges
                // (which is more universal), as now we are going to return only
                // sequence of edges.
                connectedIds->push_back(cur_edge_id);

                // Get the only target vertex of this edge. If edge is bidirected
                // get not that vertex that with cur_vert_id.
                GNMGFID target_vert_id;
                /*
                std::vector<GNMGFID> target_vert_ids = _getTgtVert(cur_edge_id);
                std::vector<GNMGFID>::iterator itt;
                for (itt = target_vert_ids.begin(); itt != target_vert_ids.end(); ++itt)
                {
                    if ((*itt) != cur_vert_id)
                    {
                        target_vert_id = *itt;
                        break;
                    }
                }
                */
                target_vert_id = _getOppositVert(cur_edge_id,cur_vert_id);

                // Avoid marked or blocked vertexes.
                if ((markedVertIds->find(target_vert_id) == markedVertIds->end())
                && (!_isVertBlocked(target_vert_id)))
                    neighbours_queue.push(target_vert_id);
            }
        }

        vertexQueue.pop();
    }

    if (!neighbours_queue.empty())
        _traceTargets(neighbours_queue, markedVertIds, connectedIds);
}

/************************************************************************/
/*                        _addVertex()                               */
/************************************************************************/
void GNMStdAnalyser::_addVertex (GNMGFID gfid)
{
    GNMStdVertex vertex;
    vertex.bIsBlkd = false;
    verts.insert(std::pair<GNMGFID,GNMStdVertex>(gfid, vertex));
    // Note: if there are vertexes with these ids already - nothing will be added.
}

/************************************************************************/
/*                        _addEdge()                               */
/************************************************************************/
GNMErr GNMStdAnalyser::_addEdge (GNMGFID conGfid,GNMGFID srcGfid,GNMGFID tgtGfid,
                                 bool isBidir)
{
    std::map<GNMGFID,GNMStdVertex>::iterator itSrs;
    std::map<GNMGFID,GNMStdVertex>::iterator itTgt;

    // We do not add edge if src and tgt vertexes do not exist.
    itSrs = verts.find(srcGfid);
    itTgt = verts.find(tgtGfid);
    if (itSrs == verts.end() || itTgt == verts.end())
        return GNMERR_FAILURE;

    // Insert edge to the array of edges.
    GNMStdEdge edge;
    edge.srcVertGFID = srcGfid;
    edge.tgtVertGFID = tgtGfid;
    //edge.dir = dir;
    edge.bIsBidir = isBidir;
    std::pair<std::map<GNMGFID,GNMStdEdge>::iterator,bool> pCon;
    // We do not add edge if an edge with the same id already exist
    // because each edge must have only one source and one target vertex.
    pCon = edges.insert(std::pair<GNMGFID,GNMStdEdge>(conGfid, edge));
    if (pCon.second == false)
        return GNMERR_FAILURE;

    // Insert edge to the array of vertexes.
    /*
    if (dir == GNM_DIR_SRCTOTGT)
    {
        (*itSrs).second.outEdgeGFIDs.push_back(conGfid);
    }
    else if (dir == GNM_DIR_TGTTOSRC)
    {
        (*itTgt).second.outEdgeGFIDs.push_back(conGfid);
    }
    else
    {
        if (edge.dir != GNM_DIR_DOUBLE) edge.dir = GNM_DIR_DOUBLE;
        (*itSrs).second.outEdgeGFIDs.push_back(conGfid);
        (*itTgt).second.outEdgeGFIDs.push_back(conGfid);
    }
    */
    if (isBidir)
    {
        (*itSrs).second.outEdgeGFIDs.push_back(conGfid);
        (*itTgt).second.outEdgeGFIDs.push_back(conGfid);
    }
    else
    {
        (*itSrs).second.outEdgeGFIDs.push_back(conGfid);
    }

    return GNMERR_NONE;
}


/************************************************************************/
/*                        _costEdge()                                   */
/************************************************************************/
GNMErr GNMStdAnalyser::_costEdge (GNMGFID gfid, double cost, double invCost)
{
    std::map<GNMGFID,GNMStdEdge>::iterator it;

    it = edges.find(gfid);
    if (it == edges.end()) return GNMERR_FAILURE;

    (*it).second.dirCost = cost;
    (*it).second.invCost = invCost;
    return GNMERR_NONE;
}

/************************************************************************/
/*                        _block()                                      */
/************************************************************************/
GNMErr GNMStdAnalyser::_block (GNMGFID gfid)
{
    // Only vertex blocking available.
    std::map<GNMGFID,GNMStdVertex>::iterator it;
    it = verts.find(gfid);

    if (it == verts.end())
        return GNMERR_FAILURE;

    if ((*it).second.bIsBlkd)
        return GNMERR_FAILURE;

    (*it).second.bIsBlkd = true;
    return GNMERR_NONE;
}

/************************************************************************/
/*                        _unblock()                               */
/************************************************************************/
GNMErr GNMStdAnalyser::_unblock (GNMGFID gfid)
{
    // Only vertex blocking available.
    std::map<GNMGFID,GNMStdVertex>::iterator it;
    it = verts.find(gfid);

    if (it == verts.end())
        return GNMERR_FAILURE;

    if (!(*it).second.bIsBlkd)
        return GNMERR_FAILURE;

    (*it).second.bIsBlkd = false;
    return GNMERR_NONE;
}

/************************************************************************/
/*                            _getOutEdges()                            */
/************************************************************************/
std::vector<GNMGFID> GNMStdAnalyser::_getOutEdges (GNMGFID vertGfid)
{
    std::vector<GNMGFID> vect;
    std::map<GNMGFID,GNMStdVertex>::iterator it;
    it = verts.find(vertGfid);
    if (it != verts.end()) vect = (*it).second.outEdgeGFIDs;
    return vect;
}

/************************************************************************/
/*                        _getTgtVert()                                */
/************************************************************************/
std::vector<GNMGFID> GNMStdAnalyser::_getTgtVert (GNMGFID edgeGfid)
{
    // Note: If the edge is bidirected this method returns two target vertexes.

    std::vector<GNMGFID> ids;
    std::map<GNMGFID,GNMStdEdge>::iterator it;
    it = edges.find(edgeGfid);
    if (it != edges.end())
    {
        /*
        if ((*it).second.dir == GNM_DIR_SRCTOTGT)
        {
            ids.push_back((*it).second.tgtVertGFID);
        }
        else if ((*it).second.dir == GNM_DIR_TGTTOSRC)
        {
            ids.push_back((*it).second.srcVertGFID);
        }
        else
        {
            ids.push_back((*it).second.tgtVertGFID);
            ids.push_back((*it).second.srcVertGFID);
        }
        */
        if ((*it).second.bIsBidir)
        {
            ids.push_back((*it).second.tgtVertGFID);
            ids.push_back((*it).second.srcVertGFID);
        }
        else
        {
            ids.push_back((*it).second.tgtVertGFID);
        }
    }
    return ids;
}


/************************************************************************/
/*                        _getOppositVert()                             */
/************************************************************************/
GNMGFID GNMStdAnalyser::_getOppositVert(GNMGFID edgeGfid, GNMGFID vertGfid)
{
    GNMGFID id = -1;
    std::map<GNMGFID,GNMStdEdge>::iterator it;
    it = edges.find(edgeGfid);
    if (it != edges.end())
    {
        if (vertGfid == (*it).second.srcVertGFID)
        {
            id = (*it).second.tgtVertGFID;
        }
        else if (vertGfid == (*it).second.tgtVertGFID)
        {
            id = (*it).second.srcVertGFID;
        }
    }
    return id;
}


/************************************************************************/
/*                        _isVertBlocked()                              */
/************************************************************************/
bool GNMStdAnalyser::_isVertBlocked (GNMGFID vertGfid)
{
    bool bl = false;
    std::map<GNMGFID,GNMStdVertex>::iterator it;
    it = verts.find(vertGfid);
    if (it != verts.end()) bl = (*it).second.bIsBlkd;
    return bl;
}


/************************************************************************/
/*                        DijkstraShortestPathTree()                    */
/************************************************************************/
/**
 * Calculates and builds the shortest path tree with the Dijkstra algorithm
 * starting from the nStartVertGFID GFID.
 * NOTE: method takes in account the blocking state of features, i.e. the
 * blocked features are the barriers during the routing process.
 *
 * @param nStartVertGFID Vertex GFID from which to start tree building.
 *
 * @return std::map where the first is vertex GFID anf the second is the GFID
 * of an edge, which is the shortest way to the current vertex. The GFID to the
 * start vertex is -1. If the vertex is isolated the returned map will be empty.
 *
 * @since GDAL 2.0
 */
std::map<GNMGFID,GNMGFID> GNMStdAnalyser::DijkstraShortestPathTree (GNMGFID nStartVertGFID)
{
    std::map<GNMGFID,GNMGFID> shortest_tree; // means < vertex id, edge id >

    // Initialize all vertexes in graph with infinity mark.
    double inf = std::numeric_limits<double>::infinity();
    std::map<GNMGFID,double> marks;
    std::map<GNMGFID,GNMStdVertex>::iterator itv;
    for (itv = verts.begin(); itv != verts.end(); ++itv)
    {
        marks.insert(std::pair<GNMGFID,double>((*itv).first,inf));
    }
    marks[nStartVertGFID] = 0.0;
    shortest_tree.insert(std::pair<GNMGFID,GNMGFID>(nStartVertGFID,-1));

    // Initialize all vertexes as unseen (there are no seen vertexes).
    std::set<GNMGFID> seen;

    // We use multimap to maintain the ascending order of costs and because
    // there can be different vertexes with the equal cost.
    std::multimap<double,GNMGFID> to_see;
    std::multimap<double,GNMGFID>::iterator it;
    to_see.insert(std::pair<double,GNMGFID>(0.0,nStartVertGFID));

    // Continue iterations while there are some vertexes to see.
    while (!to_see.empty())
    {
        // We must see vertexes with minimal costs at first.
        // In multimap the first cost is the minimal.
        it = to_see.begin();
        GNMGFID cur_vert_id = (*it).second;
        double cur_vert_mark = (*it).first;
        seen.insert((*it).second);
        to_see.erase(it);

        // For all neighbours for the current vertex.
        std::vector<GNMGFID> outcome_edge_ids = _getOutEdges(cur_vert_id);
        std::vector<GNMGFID>::iterator itt;
        for (itt = outcome_edge_ids.begin(); itt != outcome_edge_ids.end(); ++itt)
        {
            GNMGFID cur_edge_id = *itt;

            // We go in any edge from source to target so we take only
            // direct cost (even if an edge is biderected).
            double cur_edge_cost = edges[cur_edge_id].dirCost;

            // Get the only target vertex of this edge. If edge is bidirected
            // get not that vertex that cur_vert_id.
            GNMGFID target_vert_id;
            /*
            std::vector<GNMGFID> target_vert_ids = _getTgtVert(cur_edge_id);
            std::vector<GNMGFID>::iterator ittt;
            for (ittt = target_vert_ids.begin(); ittt != target_vert_ids.end(); ++ittt)
            {
                if ((*ittt) != cur_vert_id)
                {
                    target_vert_id = *ittt;
                    break;
                }
            }
            */
            // While we see outcome edges of cur_vert_id we definitly know
            // that target_vert_id will be target for cur_edge_id.
            target_vert_id = _getOppositVert(cur_edge_id,cur_vert_id);

            // Calculate a new mark assuming the full path cost (mark of the
            // current vertex) to this vertex.
            double new_vert_mark = cur_vert_mark + cur_edge_cost;

            // Update mark of the vertex if needed.
            if (seen.find(target_vert_id) == seen.end()
                && new_vert_mark < marks[target_vert_id]
                && !_isVertBlocked(target_vert_id))
            {
                marks[target_vert_id] = new_vert_mark;

                // NOTE: we can not std::map::insert() here because if there was
                // eirlier an edge to current vertex with grater mark - it will not
                // be overwritten. So we use operator [] to replace it.
                shortest_tree[target_vert_id] = cur_edge_id;

                // The vertex with minimal cost will be inserted to the begining.
                to_see.insert(std::pair<double,GNMGFID>(new_vert_mark,target_vert_id));
            }
        }
    }

    return shortest_tree;
}


/************************************************************************/
/*                        DijkstraShortestPath()                        */
/************************************************************************/
/**
 *
 *
 * @param
 *
 * @return Array of corresponding vertexes and edges, where the first in a pair
 * is a vertex GFID and the second is an edge GFID leading to this vertex. The
 * elements in this array are sorted by the path segments order, i.e. the first
 * is the pair(nStartVertGFID,-1) and the last is (nEndVertGFID,<some edge>).
 * If there is no any path between start and end vertex the returned array will
 * be empty.
 *
 * @since GDAL 2.0
 */
std::vector<std::pair<GNMGFID,GNMGFID> > GNMStdAnalyser::DijkstraShortestPath (
        GNMGFID nStartVertGFID,GNMGFID nEndVertGFID)
{
    std::map<GNMGFID,GNMGFID> shortest_tree; // means < vertex id, edge id >
    std::map<GNMGFID,GNMGFID>::iterator it;

    shortest_tree = GNMStdAnalyser::DijkstraShortestPathTree(nStartVertGFID);

    // We search for a path in the resulting tree, starting from end point to
    // start point.
    //std::vector<GNMGFID> shortest_path;
    //std::map<GNMGFID,GNMGFID> shortest_path;
    std::vector<std::pair<GNMGFID,GNMGFID> > shortest_path;
    GNMGFID next_vert_id = nEndVertGFID;
    bool fl = false;
    while (true)
    {
        it = shortest_tree.find(next_vert_id);

        if (it == shortest_tree.end())
        {
            // We haven't found the start vertex - there is no path between
            // to given vertexes in a shortest-path tree.
            break;
        }
        else if ((*it).first == nStartVertGFID)
        {
            // We've reached the start vertex and return an array.
            shortest_path.push_back(std::make_pair(next_vert_id,-1));
            fl = true;
            break;
        }
        else
        {
            // There is only one edge which leads to this vertex, because we
            // analyse a tree. We add this edge with its target vertex into
            // final array.
            //shortest_path.push_back((*it).second);
            //shortest_path.insert(std::make_pair(next_vert_id,(*it).second));
            shortest_path.push_back(std::make_pair(next_vert_id,(*it).second));

            // An edge has only two vertexes, so we get the opposit one to the
            // current vertex in order to continue search backwards.
            next_vert_id = _getOppositVert((*it).second,(*it).first);
        }
    }

    if (fl == false)
    {
        //std::vector<GNMGFID> void_path;
        //std::map<GNMGFID,GNMGFID> void_path;
        std::vector<std::pair<GNMGFID,GNMGFID> > void_path;
        return void_path;
    }

    // Revert array because the first vertex is now the last in path.
    int size = shortest_path.size();
    for (int i=0; i<size/2; i++)
    {
        std::pair<GNMGFID,GNMGFID> buf;
        buf = shortest_path[i];
        shortest_path[i] = shortest_path[size-i-1];
        shortest_path[size-i-1] = buf;
    }
    return shortest_path;
}


/************************************************************************/
/*                        ConnectedComponents()                         */
/************************************************************************/
/**
 * Uses the recursive Breadth-first search algorithm to find the connected
 * to the given vector of GFIDs components.
 * NOTE: method takes in account the blocking state of features, i.e. the
 * blocked features are the barriers during the routing process.
 *
 * @param startVertGFIDs Vector of vertex's GFIDs from which to start search.
 *
 * @return std::vector where the elements are the edges' GFIDs which are
 * connected to the start vertexes. If there is no connected edges - the
 * vector will be returned empty.
 *
 * @since GDAL 2.0
 */
std::vector<GNMGFID> GNMStdAnalyser::ConnectedComponents (std::vector<GNMGFID>
                                                          startVertGFIDs)
{
    std::vector<GNMGFID> connected_ids;
    std::set<GNMGFID> marked_vert_ids;

    std::queue<GNMGFID> start_queue;
    std::vector<GNMGFID>::iterator it;
    for (it = startVertGFIDs.begin(); it != startVertGFIDs.end(); ++it)
    {
        start_queue.push(*it);
    }

    // Begin the iterations of the Breadth-first search.
    _traceTargets(start_queue, &marked_vert_ids, &connected_ids);

    return connected_ids;
}


/************************************************************************/
/*                        YensShortestPaths()                           */
/************************************************************************/
/**
 *
 *
 * @param
 *
 * @return
 *
 * @since GDAL 2.0
 */
std::vector<std::vector<std::pair<GNMGFID,GNMGFID> > > GNMStdAnalyser::YensShortestPaths (
        GNMGFID nStartVertGFID,
        GNMGFID nEndVertGFID,
        int nK)
{
    // Resulting array with paths.
    // A will be sorted by the path costs' descending.
    std::vector<std::vector<std::pair<GNMGFID,GNMGFID> > > A;

    // Temporary array for storing paths-candidates.
    // B will be automatically sorted by the cost descending order. We
    // need multimap because there can be physically different paths but
    // with the same costs.
    std::multimap<double,std::vector<std::pair<GNMGFID,GNMGFID> > > B;

    if (nK <= 0)
        return A; // return empty array if K is incorrect.

    // Firstly get the very shortest path.
    // Note, that it is important to obtain the path from DijkstraShortestPath() as
    // vector, rather than the map, because we need the correct order of the path
    // segments in the Yen's algorithm iterations.
    std::vector<std::pair<GNMGFID,GNMGFID> > first_path;
    first_path = DijkstraShortestPath(nStartVertGFID, nEndVertGFID);
    if (first_path.empty())
        return A; // return empty array if there is no path between points.

    A.push_back(first_path);

    for (int k=0; k<nK-1; k++) // -1 because we have already found one
    {
        std::map<GNMGFID,double> deleted_edges; // for infinity costs assignement

        std::vector<std::pair<GNMGFID,GNMGFID> >::iterator itAk;
        itAk = A[k].begin();
        for (int i=0; i<A[k].size()-1; i++) // avoid end node
        {
            // Get the current node.
            GNMGFID spur_node = A[k][i].first;

            // Get the root path from the 0 to the current node.
            ++itAk; // Equivalent to A[k][i] // because we will use std::vector::assign, which assigns [..) range, not [..]
            std::vector<std::pair<GNMGFID,GNMGFID> > root_path;
            root_path.assign(A[k].begin(),itAk);

            // Remove old incedence edges of all other best paths.
            // I.e. if the spur vertex can be reached in already found best paths we must
            // remove the following edge after the end of root path from the graph in
            // order not to take in account these already seen best paths.
            // I.e. it ensures that the spur path will be different.
            std::vector<std::vector<std::pair<GNMGFID,GNMGFID> > >::iterator itA;
            for (itA = A.begin(); itA != A.end(); ++itA)
            {
                if (i >= (*itA).size()) // check if the number of node exceed the number of last node in the path array (i.e. if one of the A paths has less amount of segments than the current candidate path)
                    continue;

                std::vector<std::pair<GNMGFID,GNMGFID> > root_path_other;
                root_path_other.assign((*itA).begin(),(*itA).begin() + i + 1); // + 1, because we will use std::vector::assign, which assigns [..) range, not [..]

                // Get the edge which follows the spur node for current path
                // and delete it.
                // Note: we do not delete edges due to performance reasons, because
                // the deletion of edge and all its GFIDs in veretx records is slower
                // than the infinity cost assignement.

                if ((root_path == root_path_other)
                        && (i < root_path_other.size())) // also check if node number exceed the number of the last node in root array. otherwisw iterator error
                {
                    GNMGFID after_spur_edge = (*((*itA).begin() + i + 1)).second;
                    deleted_edges.insert(std::make_pair(after_spur_edge,
                                                 edges[after_spur_edge].dirCost));
                    edges[after_spur_edge].dirCost
                            = std::numeric_limits<double>::infinity();
                    //edges[].invCost
                            //= std::numeric_limits<double>::infinity();
                }
            }

            // Remove root path nodes from the graph. If we do not delete them
            // the path will be found backwards and some parts of the path will
            // duplicate the parts of old paths.
            // Note: we "delete" all the incedence to the root nodes edges, so
            // to restore them in a common way.
            std::vector<std::pair<GNMGFID,GNMGFID> >::iterator itR;
            for (itR = root_path.begin(); itR != root_path.end()-1; ++itR) // end()-1, because we should not remove the spur node
            {
                GNMGFID vert_to_del = (*itR).first;
                for (int l=0; l<verts[vert_to_del].outEdgeGFIDs.size(); l++)
                {
                    GNMGFID edge_to_del = verts[vert_to_del].outEdgeGFIDs[l];
                    deleted_edges.insert(std::make_pair(edge_to_del,
                                           edges[edge_to_del].dirCost));
                    edges[edge_to_del].dirCost
                            = std::numeric_limits<double>::infinity();
                    //edges[].invCost
                            //= std::numeric_limits<double>::infinity();
                }
            }

            // Find the new best path in the modified graph.
            std::vector<std::pair<GNMGFID,GNMGFID> > spur_path;
            spur_path = DijkstraShortestPath(spur_node, nEndVertGFID);

            // Firstly Restore deleted edges, in order to calculate the summary
            // cost of the path correctly later, because the costs will be gathered
            // from the initial graph.
            // We must do it here, after each edge removing, because the later
            // Dijkstra searchs must consider these edges.
            //std::map<GNMGFID,GNMStdEdge>::iterator itDel;
            std::map<GNMGFID,double>::iterator itDel;
            for (itDel = deleted_edges.begin(); itDel != deleted_edges.end(); ++itDel)
            {
                //edges.insert(itDel);
                edges[(*itDel).first].dirCost = (*itDel).second;
            }
            deleted_edges.clear();

            // If the part of a new best path has been found we form a full one
            // and add it to the candidates array.
            if (!spur_path.empty())
            {
                root_path.insert(root_path.end(),spur_path.begin()+1,spur_path.end()); // + 1 so not to consider the first node in the found path, which is already the last node in the root path
                // Calculate the summary cost of the path.
                // TODO: get the summary cost from the Dejkstra method?
                double sum_cost = 0.0;
                std::vector<std::pair<GNMGFID,GNMGFID> >::iterator itR;
                for (itR = root_path.begin(); itR != root_path.end();
                     ++itR)
                {
                    // TODO: check: Note, that here the current cost can not be
                    // infinfity, because everytime we assign infinity costs for
                    // edges of old paths, we anyway have the alternative edges with
                    // non-infinity costs.
                    sum_cost += edges[(*itR).second].dirCost;
                }

                B.insert(std::make_pair(sum_cost,root_path));
            }
        }

        //
        if (B.empty())
            break;

        // The best path is the first, because the map is sorted accordingly.
        // Note, that here we won't clear the path candidates array
        // and select the best path from all of the rest paths, even from thouse which
        // were found on previous iterations. That's why we need k iterations at all.
        // Note, that if there were two paths with the same costs and it is the LAST
        // iteration the first occured path will be added, rather than random.
        A.push_back((*B.begin()).second);

        // Sometimes B contains fully duplicate paths. Such duplicates have been
        // formed during the search of alternative for almost the same paths which
        // were already in A.
        // We allowed to add them into B so here we must delete all duplicates.
        while (!B.empty() && (*B.begin()).second == A.back())
        {
            B.erase(B.begin());
        }
    }

    return A;
}


/************************************************************************/
/*                        PrepareGraph()                                */
/************************************************************************/
/**
 * \brief Make the inner graph representation which is convinient for calculations.
 *
 * The method uses GNMNetwork::GetNextConnection() to read all connections
 * and to build the graph. The inner graph is based on std containers and represents
 * a weighted, mixed graph.
 *
 * @param poNet Network wich will be analysed.
 * @return GNMERR_NONE if the preparing was successful.
 *
 * @since GDAL 2.0
 */
GNMErr GNMStdAnalyser::PrepareGraph (GNMNetwork *poNet)
{
    if (poNet == NULL)
        return GNMERR_FAILURE;

    // ISSUE: think about the way how not to load the whole graph in memory
    // in this method. As an idea: to split all features in graph into
    // chunks and to load all features of one chunk when the each algorithm
    // "reaches" the corresponding chunk during the graph routing.
    // See LOD (load-on-demand analysis).

    // Rebuild graph from the begining and load it into memory.
    poNet->ResetConnectionsReading();
    //GNMConnection *poCon;
    OGRFeature *poCon;
    while ((poCon = poNet->GetNextConnection()) != NULL)
    {
        //_addVertex(poCon->nSrcGFID);
        //_addVertex(poCon->nTgtGFID);
        //_addEdge(poCon->nConGFID,poCon->nSrcGFID,poCon->nTgtGFID,poCon->dir);
        //_costEdge(poCon->nConGFID,poCon->dCost,poCon->dInvCost);
        //delete poCon;
        _addVertex(poCon->GetFieldAsInteger(GNM_SYSFIELD_SOURCE));
        _addVertex(poCon->GetFieldAsInteger(GNM_SYSFIELD_TARGET));
        /*
        if (_addEdge(poCon->GetFieldAsInteger(GNM_SYSFIELD_CONNECTOR),
                 poCon->GetFieldAsInteger(GNM_SYSFIELD_SOURCE),
                 poCon->GetFieldAsInteger(GNM_SYSFIELD_TARGET),
                 poCon->GetFieldAsInteger(GNM_SYSFIELD_DIRECTION))
                == GNMERR_NONE)
        {
            _costEdge(poCon->GetFieldAsInteger(GNM_SYSFIELD_CONNECTOR),
                  poCon->GetFieldAsDouble(GNM_SYSFIELD_COST),
                  poCon->GetFieldAsDouble(GNM_SYSFIELD_INVCOST));
        }
        */
        GNMGFID conGfid = poCon->GetFieldAsInteger(GNM_SYSFIELD_CONNECTOR);
        GNMGFID srcGfid;
        GNMGFID tgtGfid;
        GNMDirection dir = poCon->GetFieldAsInteger(GNM_SYSFIELD_DIRECTION);
        bool isBidir = false;
        if (dir == GNM_DIR_TGTTOSRC)
        {
            tgtGfid = poCon->GetFieldAsInteger(GNM_SYSFIELD_SOURCE);
            srcGfid = poCon->GetFieldAsInteger(GNM_SYSFIELD_TARGET);
        }
        else
        {
            srcGfid = poCon->GetFieldAsInteger(GNM_SYSFIELD_SOURCE);
            tgtGfid = poCon->GetFieldAsInteger(GNM_SYSFIELD_TARGET);
            if (dir != GNM_DIR_SRCTOTGT)
            {
                isBidir = true;
            }
        }
        if ( _addEdge(conGfid,srcGfid,tgtGfid,isBidir) == GNMERR_NONE)
        {
            _costEdge(conGfid,
                  poCon->GetFieldAsDouble(GNM_SYSFIELD_COST),
                  poCon->GetFieldAsDouble(GNM_SYSFIELD_INVCOST));
        }
        OGRFeature::DestroyFeature(poCon);
    }

    _poNet = poNet;

    return GNMERR_NONE;
}


/************************************************************************/
/*                   Constructor&destructor                             */
/************************************************************************/
GNMGdalStdAnalyser::GNMGdalStdAnalyser(): GNMStdAnalyser()
{
}
GNMGdalStdAnalyser::~GNMGdalStdAnalyser()
{
}


/************************************************************************/
/*                        PrepareGraph()                                */
/************************************************************************/
/**
 * Overrides the GNMStdAnalyser::PrepareGraph() and prepares the graph of the
 * given network for GDAL-networks specific calculations.
 * NOTE: method creates the additional _gnm_blkd layer in order to store the
 * GFIDs of blocked features.
 * Returns GNMERR_NONE if preparing was successful.
 *
 * @see GNMStdAnalyser::PrepareGraph()
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalStdAnalyser::PrepareGraph (GNMNetwork *poNet)
{
    // We must firstly check if the given network has the GDAL-network format.
    if (poNet == NULL)// || strcmp(poNet->GetFormatName(),"GDAL Network") != 0)
        return GNMERR_FAILURE;

    if(GNMStdAnalyser::PrepareGraph(poNet) != GNMERR_NONE)
        return GNMERR_FAILURE;

    GDALDataset *poDS = _poNet->GetDataset();

    // Check that accordig system layer exists.
    // Create the according system layer for storing blocked features.

    OGRLayer *blockLayer = poDS->GetLayerByName("_gnm_blkd");
    if (blockLayer == NULL)
    {
        blockLayer = poDS->CreateLayer("_gnm_blkd",NULL,wkbNone);
        if (blockLayer == NULL)
            return GNMERR_FAILURE;
    }

    OGRFeatureDefn *blDefn = blockLayer->GetLayerDefn();
    int blInd = blDefn->GetFieldIndex(GNM_SYSFIELD_GFID);
    if (blInd != -1 && blDefn->GetFieldDefn(blInd)->GetType() != OFTInteger)
    {
        blockLayer->DeleteField(blInd);
        blInd = -1;
    }

    if (blInd == -1)
    {
        OGRFieldDefn blField(GNM_SYSFIELD_GFID, OFTInteger);
        if(blockLayer->CreateField(&blField) != OGRERR_NONE)
            return GNMERR_FAILURE;
    }

    // Block the corresponding features in the inner graph (if we load the
    // whole graph in memory here).
    OGRFeature *blFeature;
    blockLayer->ResetReading();
    while ((blFeature = blockLayer->GetNextFeature()) != NULL)
    {
        GNMGFID gfid = (GNMGFID)blFeature->GetFieldAsInteger(GNM_SYSFIELD_GFID);
        _block(gfid);
        OGRFeature::DestroyFeature(blFeature);
    }

    return GNMERR_NONE;
}


/************************************************************************/
/*                       _makeResultLineLayer()                         */
/************************************************************************/
OGRLayer *GNMGdalStdAnalyser::_makeResultLineLayer (std::vector<GNMGFID> lineGFIDs,
                                                    const char *pszLayerName)
{
    if (_poNet == NULL)
        return NULL;

    // Create new layer in memory.
    GDALDriver *poMemDriver = GetGDALDriverManager()->GetDriverByName("Memory");
    if (poMemDriver == NULL)
        return NULL;
    GDALDataset *poMemDS;
    poMemDS = poMemDriver->Create("dummy_name", 0, 0, 0, GDT_Unknown, NULL);
    const OGRSpatialReference *poSRS;
    poSRS = static_cast<GNMGdalNetwork*>(_poNet)->GetSpatialReference();
    OGRSpatialReference *newSRS = poSRS->Clone();
    OGRLayer *poMemLayer = poMemDS->CreateLayer(pszLayerName, newSRS, wkbLineString);

    GDALDataset *poDS = _poNet->GetDataset();

    std::vector<GNMGFID>::iterator it;
    for (it = lineGFIDs.begin(); it != lineGFIDs.end(); ++it)
    {
        if ((*it) < 0)
            continue;

        // Extract (line) feature with its geometry.
        OGRFeature *f = _poNet->GetFeatureByGFID(*it);
        //if (f == NULL)
            //return GNMERR_NONE;
        OGRGeometry *geom = f->GetGeometryRef();

        // We try to create a line geometry.
        OGRLineString ln;

        // If the geometry of the current feature is not a line one, we
        // try to create a new line from start and end points in the connection.
        if (geom == NULL || geom->getGeometryType() != wkbLineString)
        {
            OGRLayer *graphLayer = poDS->GetLayerByName(GNM_SYSLAYER_GRAPH);
            OGRFeature *graphFeature;
            char filter[100];
            // We are looking for the connection in a graph, where our feature
            // playes the role of the connector.
            sprintf(filter,"%s = %d",GNM_SYSFIELD_CONNECTOR,*it);
            graphLayer->SetAttributeFilter(filter);
            graphLayer->ResetReading();
            graphFeature = graphLayer->GetNextFeature(); // it can not be multiple features
            //if (graphFeature != NULL)
            //{
                OGRFeature *f1 = _poNet->GetFeatureByGFID(
                            graphFeature->GetFieldAsInteger(GNM_SYSFIELD_SOURCE));
                OGRFeature *f2 = _poNet->GetFeatureByGFID(
                            graphFeature->GetFieldAsInteger(GNM_SYSFIELD_TARGET));
                OGRGeometry *geom1 = f1->GetGeometryRef();
                OGRGeometry *geom2 = f2->GetGeometryRef();
                if (geom1 != NULL && geom2 != NULL
                    && geom1->getGeometryType() == wkbPoint
                    && geom2->getGeometryType() == wkbPoint)
                {
                    OGRPoint pt1 = *static_cast<OGRPoint*>(geom1->clone());
                    ln.addPoint(&pt1);
                    OGRPoint pt2 = *static_cast<OGRPoint*>(geom2->clone());
                    ln.addPoint(&pt2);
                    // If the criterion is not matched the ln will stay empty.
                }
                OGRFeature::DestroyFeature(f1);
                OGRFeature::DestroyFeature(f2);
            //}
            graphLayer->SetAttributeFilter(NULL);
            OGRFeature::DestroyFeature(graphFeature);
        }

        // If this edge has own line geometry, we add it to the new layer.
        else
        {
            // Note: here we must clone the geometry, because the feature
            // with it will be destroyed at the end of the loop.
            ln = *static_cast<OGRLineString*>(geom->clone());
        }

        // Fill new layer if the geometry was determined. Otherwise do nothing.
        if (ln.IsEmpty() == FALSE)
        {
            OGRFeature *lineFeature = OGRFeature::CreateFeature(poMemLayer->GetLayerDefn());
            lineFeature->SetGeometry(&ln);
            poMemLayer->CreateFeature(lineFeature);
            OGRFeature::DestroyFeature(lineFeature);
            // If something failed do nothing.
            // CPL Warning
        }

        OGRFeature::DestroyFeature(f);
    }

    return poMemLayer;
}


/************************************************************************/
/*                        BlockFeature()                                */
/************************************************************************/
/**
 * Blocks the feature by its GFID saving this state to the specific layer.
 * The blocked features will no longer participate in the calculations and
 * this way the commutations or moving restrictions in a network can be
 * simulated. Returns GNMERR_NONE if blocking was successful.
 *
 * @see GNMGdalStdAnalyser::UnblockFeature()
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalStdAnalyser::BlockFeature (GNMGFID nGFID)
{
    GNMErr err = _block(nGFID);
    if (err != GNMERR_NONE)
        return GNMERR_FAILURE;

    // Add to the special layer. The feature will not be blocked if it
    // is blocked already so there will be no multiple gfids in the blocked
    // features list.
    OGRLayer *blLayer = _poNet->GetDataset()->GetLayerByName("_gnm_blkd");
    if (blLayer != NULL)
    {
        OGRFeature *blFeature = OGRFeature::CreateFeature(blLayer->GetLayerDefn());
        if (blFeature != NULL)
        {
            blFeature->SetField(GNM_SYSFIELD_GFID,nGFID);
            blLayer->CreateFeature(blFeature);
            OGRFeature::DestroyFeature(blFeature);
        }
    }

    return GNMERR_NONE;
}


/************************************************************************/
/*                        UnblockFeature()                              */
/************************************************************************/
/**
 * Unblocks the feature by its GFID removing the state of feature from the
 * special layer.
 *
 * @see GNMGdalStdAnalyser::BlockFeature()
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalStdAnalyser::UnblockFeature (GNMGFID nGFID)
{
    GNMErr err = _unblock(nGFID);
    if (err != GNMERR_NONE)
        return GNMERR_FAILURE;

    // Remove from the special layer.
    OGRLayer *blLayer = _poNet->GetDataset()->GetLayerByName("_gnm_blkd");
    if (blLayer != NULL)
    {
        char filter[100];
        sprintf(filter,"%s = %d",GNM_SYSFIELD_GFID,nGFID);
        blLayer->SetAttributeFilter(filter);
        blLayer->ResetReading();
        OGRFeature *blFeature = blLayer->GetNextFeature();
        if (blFeature != NULL)
        {
            long fid = blFeature->GetFID();
            blLayer->SetAttributeFilter(NULL);
            OGRFeature::DestroyFeature(blFeature);
            blLayer->DeleteFeature(fid);
        }
    }

    return GNMERR_NONE;
}


/************************************************************************/
/*                        UnblockAllFeatures()                          */
/************************************************************************/
/**
 * Unblockes all features and simply clears the specific layer with blocked
 * features.
 *
 * @see GNMGdalStdAnalyser::BlockFeature()
 *
 * @since GDAL 2.0
 */
void GNMGdalStdAnalyser::UnblockAllFeatures ()
{
    GDALDataset *poDS = _poNet->GetDataset();
    OGRLayer *blLayer = poDS->GetLayerByName("_gnm_blkd");
    std::vector<long> fids;
    if (blLayer != NULL)
    {
        OGRFeature *blFeature;
        blLayer->ResetReading();
        while ((blFeature = blLayer->GetNextFeature()) != NULL)
        {
            GNMGFID gfid = (GNMGFID)blFeature->GetFieldAsInteger(GNM_SYSFIELD_GFID);
            _unblock(gfid);
            fids.push_back(blFeature->GetFID());
            OGRFeature::DestroyFeature(blFeature);
        }
    }

    // Clear the special layer.
    if (poDS->TestCapability(ODsCDeleteLayer) == TRUE)
    {
        int cnt = poDS->GetLayerCount();
        for (int i=0; i<cnt; i++)
        {
            if (strcmp(poDS->GetLayer(i)->GetName(),"_gnm_blkd") == 0)
            {
                if (poDS->DeleteLayer(i) == GNMERR_NONE)
                {
                    OGRLayer *newBlLayer = poDS->CreateLayer("_gnm_blkd",NULL,wkbNone);
                    if (newBlLayer != NULL)
                    {
                        OGRFieldDefn blField(GNM_SYSFIELD_GFID, OFTInteger);
                        newBlLayer->CreateField(&blField);
                    }
                }

                break;
            }
        }
    }
    else
    {
        // Try to delete feature by feature.
        for (long i=0; i<fids.size(); i++)
        {
            blLayer->DeleteFeature(fids[i]);
        }
    }
}



/************************************************************************/
/*                        Constructor&Destructor                        */
/************************************************************************/
GNMGdalStdRoutingAnalyser::GNMGdalStdRoutingAnalyser (): GNMGdalStdAnalyser()
{
}
GNMGdalStdRoutingAnalyser::~GNMGdalStdRoutingAnalyser ()
{
}


/************************************************************************/
/*                        DijkstraShortestPath()                        */
/************************************************************************/
/**
 * Returns the shortest path between nStartGFID and nEndGFID features. Method
 * uses GNMStdAnalyser::DijkstraShortestPathTree and after that searchs in
 * the resulting tree the path from end to start point.
 *
 * @return OGRLayer which was formed as a Memory layer and has according
 * spatial reference system and line geometry type. This layer is full of line
 * features which form a path and can be used to create a real-format layer
 * in a dataset if it is needed. If the path has not been found the resulting
 * layer will be void. If there were some errors during layer creation NULL
 * will be returned.
 *
 * @see GNMStdAnalyser::DijkstraShortestPathTree()
 *
 * @since GDAL 2.0
 */
OGRLayer *GNMGdalStdRoutingAnalyser::DijkstraShortestPath (GNMGFID nStartGFID,
                                                           GNMGFID nEndGFID)
{
    std::map<GNMGFID,GNMGFID> shortest_tree; // means < vertex id, edge id >
    std::map<GNMGFID,GNMGFID>::iterator it;

    shortest_tree = GNMStdAnalyser::DijkstraShortestPathTree(nStartGFID);

    // We search for a path in the resulting tree, starting from end point to
    // start point.
    std::vector<GNMGFID> result_ids;
    GNMGFID next_vert_id = nEndGFID;
    bool fl = false;
    while (true)
    {
        it = shortest_tree.find(next_vert_id);

        if (it == shortest_tree.end())
        {
            break;
        }
        else if ((*it).first == nStartGFID)
        {
            fl = true;
            break;
        }
        else
        {
            result_ids.push_back((*it).second);
            //next_vert_id = _getSrcVert((*it).second,(*it).first);
            next_vert_id = _getOppositVert((*it).second,(*it).first);
        }
    }

    if (fl == false)
    {
        CPLError(CE_Warning,CPLE_None,"No path from %i to %i found,"
                 " the void layer returned",nStartGFID,nEndGFID);
    }

    return _makeResultLineLayer(result_ids,"resource");
}



/************************************************************************/
/*                          KShortestPaths()                            */
/************************************************************************/
/**
 *
 *
 * @return
 *
 * @see
 *
 * @since GDAL 2.0
 */
std::vector<OGRLayer*> GNMGdalStdRoutingAnalyser::KShortestPaths (
        GNMGFID nStartGFID, GNMGFID nEndGFID, int nK)
{
    std::vector<OGRLayer*> resultArray;
    std::vector<std::vector<std::pair<GNMGFID,GNMGFID> > > A;

    A = YensShortestPaths(nStartGFID,nEndGFID,nK);

    if (A.empty())
    {
        CPLError(CE_Warning,CPLE_None,"No paths from %i to %i found,"
                 " the void set of layers returned",nStartGFID,nEndGFID);
        return resultArray;
    }

    if (A.size() < nK)
    {
        CPLError(CE_Warning,CPLE_None,"The K was set to %i, but only %i"
                 " path(s) has been found",nK,A.size());
    }

    for (int i=0; i<A.size(); i++)
    {
        // Extract only edge gfids and form the final set of layers.
        std::vector<GNMGFID> edge_list;
        std::vector<std::pair<GNMGFID,GNMGFID> >::iterator itAi;
        for (itAi=A[i].begin(); itAi!=A[i].end(); ++itAi)
        {
            edge_list.push_back((*itAi).second);
        }
        OGRLayer *layer = _makeResultLineLayer(edge_list,"shortestpath");
        resultArray.push_back(layer);
    }

    return resultArray;
}



/************************************************************************/
/*                      Constructor&destructor                          */
/************************************************************************/
GNMGdalStdCommutationsAnalyser::GNMGdalStdCommutationsAnalyser ():
                                            GNMGdalStdAnalyser()
{
}
GNMGdalStdCommutationsAnalyser::~GNMGdalStdCommutationsAnalyser ()
{
}


/************************************************************************/
/*                        ResourceDestribution()                        */
/************************************************************************/
/**
 * Returns the resource destribution in the network. Method uses GNMStdAnalyser::
 * ConnectedComponents() search starting from the features which were marked
 * by rules as 'emitters'. See GDAL-networks rule syntax for more details.
 * The commutations analysis can be simulated via blocking/unblocking features
 * methods.
 *
 * @return OGRLayer which was formed as a Memory layer and has according
 * spatial reference system and line geometry type. This layer is full of line
 * features which form a 'resource destribution' and can be used to create a
 * real-format layer in a dataset if it is needed. If the 'emitter' features have
 * not been found or there are no connected edges to the 'emitter' vertexes the
 * resulting layer will be void. If there were some errors during layer creation
 * NULL will be returned.
 *
 * @see GNMStdAnalyser::ConnectedComponents()
 * @see GNMGdalNetwork::CreateRule()
 *
 * @since GDAL 2.0
 */
OGRLayer *GNMGdalStdCommutationsAnalyser::ResourceDestribution ()
{
    // Get the features' GFIDs which play the role 'emitter', according
    // to the rules.
    std::vector<GNMGFID> emitter_gfids;
    std::pair<char**,int> pair;
    pair = static_cast<GNMGdalNetwork*>(_poNet)->GetClassLayers();
    GDALDataset *poDS = _poNet->GetDataset();
    for (int i=0; i<pair.second; i++)
    {
        // We save all 'emitter' features' gfids so they will be the
        // source points in a connected components search.
        if (strcmp(static_cast<GNMGdalNetwork*>(_poNet)
                   ->GetClassRole(pair.first[i]).data(),"emitter") == 0)
        {
            OGRLayer *poLayer = poDS->GetLayerByName(pair.first[i]);
            poLayer->ResetReading();
            OGRFeature *poFeature;
            while ((poFeature = poLayer->GetNextFeature()) != NULL)
            {
                GNMGFID gfid = (GNMGFID)poFeature->GetFieldAsInteger(GNM_SYSFIELD_GFID);
                if (!_isVertBlocked(gfid))
                    emitter_gfids.push_back(gfid);
                OGRFeature::DestroyFeature(poFeature);
            }
        }
    }
    CSLDestroy(pair.first);

    if (emitter_gfids.empty())
    {
        CPLError(CE_Warning,CPLE_None,"No resource distributed in tne network,"
                 " the void layer returned");
    }

    std::vector<GNMGFID> result_ids = ConnectedComponents(emitter_gfids);
    return _makeResultLineLayer(result_ids,"shortestpath");
}




GNMStdAnalyserH GNMCreateStdAnalyser ()
{
	return (GNMStdAnalyserH) new GNMStdAnalyser();
}

void GNMDeleteStdAnalyser (GNMStdAnalyserH hAnalyser)
{
	delete (GNMStdAnalyser*)hAnalyser;
}