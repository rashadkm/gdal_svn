/******************************************************************************
 * $Id$
 *
 * Name:     gnmnetwork.cpp
 * Project:  GDAL/OGR Geography Network support (Geographic Network Model)
 * Purpose:  GNMNetwork class.
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


bool GNMNetwork::_isOwnLayer (OGRLayer *poLayer)
{
    bool ok = false;
    int count = _poDataSet->GetLayerCount();
    for (int i=0; i<count; i++)
    {
        if (_poDataSet->GetLayer(i) == poLayer)
        {
            ok = true;
        }
    }
    return ok;
}


/************************************************************************/
/*                           Constructor()                              */
/************************************************************************/
GNMNetwork::GNMNetwork()
{
    _poDataSet = NULL;
    _ownDataset = false;
}


/************************************************************************/
/*                           Destructor()                               */
/************************************************************************/
GNMNetwork::~GNMNetwork()
{
    //delete[] _formatName;

    // We delete _poDataSet here if it is owned by the GNMNetwork object.
    if (_ownDataset)
        GDALClose(_poDataSet);
}


/************************************************************************/
/*                           GetFormatName()                            */
/************************************************************************/
/**
 * Returns the name of network's fromat. This string should not
 * be modified or freed by caller.
 *
 * @since GDAL 2.0
 */
//const char *GNMNetwork::GetFormatName ()
//{
//  return _formatName;
//}


/************************************************************************/
/*                           GetDataset()                               */
/************************************************************************/
/**
 * Returns the inner network's dataset.
 *
 * @since GDAL 2.0
 */
GDALDataset *GNMNetwork::GetDataset ()
{
    return _poDataSet;
}


/************************************************************************/
/*                           FlushCache()                               */
/************************************************************************/
/**
 * Calls the GDALDataset::FlushCache() for the inner network's dataset.
 *
 * @since GDAL 2.0
 */
void GNMNetwork::FlushCache ()
{
    _poDataSet->FlushCache();
}


/************************************************************************/
/*                      ReconnectFeatures()                             */
/************************************************************************/
/**
 * Reconnects features changing the edges's (connector's) direction. By default
 * currently unsupported.
 *
 * @since GDAL 2.0
 */
GNMErr GNMNetwork::ReconnectFeatures (GNMGFID nSrcGFID,GNMGFID nTgtGFID,
                                  GNMGFID nConGFID,GNMDirection nNewDir)
{
    // TODO:
    // DisconnectFeatures()
    // ConnectFeatures()
    // return GNMERR_NONE;

    return GNMERR_UNSUPPORTED;
}


/************************************************************************/
/*                      ReconnectFeatures()                             */
/************************************************************************/
/**
 * Reconnects features changing the edges's (connector's) costs. By default
 * currently unsupported.
 *
 * @since GDAL 2.0
 */
GNMErr GNMNetwork::ReconnectFeatures (GNMGFID nSrcGFID,GNMGFID nTgtGFID,
                                  GNMGFID nConGFID,double dNewCost,
                                  double dNewInvCost)
{
    // TODO:
    // DisconnectFeatures()
    // ConnectFeatures()
    // return GNMERR_NONE;
    return GNMERR_UNSUPPORTED;
}


/************************************************************************/
/*                            AutoConnect()                             */
/************************************************************************/
/**
 * \brief Attempts to build the network topology automatically.
 *
 * Attempts to build the network topology automatically. By default simply
 * gets point and line layers from the papoLayers and searchs for each line
 * two poins: start and end, so it can be not so effective in perfomance.
 * The method uses ConnectFeatures() for each connection.
 *
 * This method can be reimplemented in subclusses to make a
 * format-specific topology building (for example for pgRouting
 * pgr_createTopology() can be called).
 *
 * @param papoLayers Layers which will participate in topology building. The
 * array must ends with NULL.
 * @param dTolerance Snapping tolerane.
 * @param papszParams Format-specific parameters such as names of columns.
 * Can be set as the pairs "ParamName=ParamValue".
 *
 * @since GDAL 2.0
 */
//GNMErr GNMNetwork::AutoConnect (OGRLayer **papoLayers,
                            //double dTolerance,
                            //char **papszParams)
GNMErr GNMNetwork::AutoConnect (std::vector<OGRLayer*> vLayers,
                            double dTolerance,
                            char **papszParams)
{
    //if (papoLayers == NULL)
	if (vLayers.size() == 0)
        return GNMERR_FAILURE;

    std::vector<OGRLayer*> lineLayers;
    std::vector<OGRLayer*> pointLayers;
    //int i = 0;
	std::vector<OGRLayer*>::iterator itL;
    //while (papoLayers[i] != NULL)
	for (itL = vLayers.begin(); itL != vLayers.end(); ++itL)
    {
        //if (papoLayers[i] != NULL && papoLayers[i]->GetGeomType() == wkbLineString)
		if ((*itL)->GetGeomType() == wkbLineString)
        {
            //lineLayers.push_back(papoLayers[i]);
			lineLayers.push_back((*itL));
        }
        //else if (papoLayers[i] != NULL && papoLayers[i]->GetGeomType() == wkbPoint)
		else if ((*itL)->GetGeomType() == wkbPoint)
        {
            //pointLayers.push_back(papoLayers[i]);
			pointLayers.push_back((*itL));
        }
        //i++;
    }
    if (lineLayers.size() == 0 || pointLayers.size() == 0)
    {
        //CPLErr
        return GNMERR_FAILURE;
    }

    OGRPoint startP;
    OGRPoint endP;

	// Load all line features into array. We can not read features and connect them
	// at the same time, because ConnectFeatures() can interrupt this sequential reading 
	// with its own features reading.
	std::vector<OGRFeature*> lineFeatures;
    std::vector<int>::size_type szl = lineLayers.size();
    for (unsigned i = 0; i < szl; i++)
	{
		OGRLayer *lineLayer = lineLayers[i];
        lineLayer->ResetReading();
        OGRFeature *lineFeature;
        while ((lineFeature = lineLayer->GetNextFeature()) != NULL)
		{
			lineFeatures.push_back(lineFeature);
		}
	}

	// Iterate lines.
	std::vector<OGRFeature*>::iterator itLF;
	for (itLF = lineFeatures.begin(); itLF != lineFeatures.end(); ++itLF)
	{
            GNMGFID lineGFID = (*itLF)->GetFieldAsInteger(GNM_SYSFIELD_GFID);

            OGRGeometry* lineGeom = (*itLF)->GetGeometryRef();
            static_cast<OGRLineString*>(lineGeom)->StartPoint(&startP);
            static_cast<OGRLineString*>(lineGeom)->EndPoint(&endP);

            // Form the surrounding area of the point, using tolerance.
            double incr = dTolerance / 2;
            double minSX = startP.getX() - incr;
            double minSY = startP.getY() - incr;
            double maxSX = startP.getX() + incr;
            double maxSY = startP.getY() + incr;
            double minEX = endP.getX() - incr;
            double minEY = endP.getY() - incr;
            double maxEX = endP.getX() + incr;
            double maxEY = endP.getY() + incr;

            // Get the existing point feature from the surrounding area.
            // TODO: take the nearest point if there are several points
            // in the snap area, because now we take only the first occured.
            GNMGFID pointStartGFID = -1;
            GNMGFID pointEndGFID = -1;

            std::vector<int>::size_type szp = pointLayers.size();
            for (unsigned j = 0; j < szp; j++)
            {
                OGRLayer *pointLayer = pointLayers[j];

                OGRFeature *pointFeature;

                if (pointStartGFID == -1)
                {
                    pointLayer->SetSpatialFilterRect(minSX, minSY, maxSX, maxSY);
                    pointLayer->ResetReading();
                    while((pointFeature = pointLayer->GetNextFeature()) != NULL)
                    {
                        pointStartGFID = pointFeature->GetFieldAsInteger(GNM_SYSFIELD_GFID);
                        OGRFeature::DestroyFeature(pointFeature);
                        break;
                    }
                }

                if (pointEndGFID == -1)
                {
                    pointLayer->SetSpatialFilterRect(minEX, minEY, maxEX, maxEY);
                    pointLayer->ResetReading();
                    while((pointFeature = pointLayer->GetNextFeature()) != NULL)
                    {
                        pointEndGFID = pointFeature->GetFieldAsInteger(GNM_SYSFIELD_GFID);
                        OGRFeature::DestroyFeature(pointFeature);
                        break;
                    }
                }

                if ((pointStartGFID != -1) && (pointEndGFID != -1))
                {
                    break;
                }
            }

            // We will create the connection only if there are two feature
            // points found. I. e. if there is a line without at least one
            // point feature on its end - it will be ignored. The connection
            // for this line may be set manually anytime.
            if ((pointStartGFID != -1) && (pointEndGFID != -1))
            {
                // The costs and direction will be replaced in concrete
                // ConnectFeatures() if the rules are set.
                GNMErr err = ConnectFeatures(pointStartGFID,pointEndGFID,lineGFID,
                                0.0, 0.0, GNM_DIR_DOUBLE);
                if (err != GNMERR_NONE)
                {
                    // TODO: Warning
                }
            }

			// Free feature resourses.
            OGRFeature::DestroyFeature(*itLF);
    }

    return GNMERR_NONE;
}


/************************************************************************/
/*                           DisconnectAll()                            */
/************************************************************************/
/**
 * Disconnect all features. Currently unsupported.
 *
 * @since GDAL 2.0
 */
GNMErr GNMNetwork::DisconnectAll ()
{
    // TODO:
    // ResetConnectionsReading ()
    // for () getNextConnection
    // DisconnectFeatures()
    // return GNMERR_NONE;
    return GNMERR_UNSUPPORTED;
}


/************************************************************************/
/*                           CreateLayer()                              */
/************************************************************************/
/**
 * Creates layer in a network. By default simply uses GDALDataset::CreateLayer()
 * but can be replaced in subclasses to add additioanl connectivity layers or
 * fields.
 *
 * @since GDAL 2.0
 */
OGRLayer *GNMNetwork::CreateLayer (const char *pszName, OGRFeatureDefn *poFeatureDefn,
                                   OGRSpatialReference *poSpatialRef,
                                   OGRwkbGeometryType eGType, char **papszOptions)
{
    // Create void layer in a dataset.
    OGRLayer *newLayer;
    newLayer = _poDataSet->CreateLayer(pszName,poSpatialRef,eGType,papszOptions);
    if (newLayer == NULL)
    {
        return NULL;
    }

    // Create fields for the layer.
    int count = poFeatureDefn->GetFieldCount();
    for (int i=0; i<count; i++)
    {
        if (newLayer->CreateField(poFeatureDefn->GetFieldDefn(i)) != OGRERR_NONE)
        {
            //CPLError
            // Try to delete layer.
            DeleteLayer(pszName);
            return NULL;
        }
    }

    return newLayer;
}


/************************************************************************/
/*                           DeleteLayer()                              */
/************************************************************************/
/**
 * Delete layer by its name. By default simply calls GDALDataset::
 * DeleteLayer() but can be replaced in subclasses to delete according
 * connectivity fields or features.
 *
 * @since GDAL 2.0
 */
GNMErr GNMNetwork::DeleteLayer (const char *pszName)
{
    int count = _poDataSet->GetLayerCount();
    for (int i=0; i<count; i++)
    {
        if (strcmp(_poDataSet->GetLayer(i)->GetName(),pszName) == 0)
        {
            _poDataSet->DeleteLayer(i);
            break;
        }
    }

    return GNMERR_NONE;
}


/************************************************************************/
/*                           CopyLayer ()                               */
/************************************************************************/
/**
 * Copies layer to the network. By default uses GDALDataset::CopyLayer()
 * but can be replaced in subclasses to assign additional connectivity info.
 *
 * @since GDAL 2.0
 */
GNMErr GNMNetwork::CopyLayer (OGRLayer *poSrcLayer, const char *pszNewName)
{
    if (_poDataSet->CopyLayer(poSrcLayer,pszNewName) == NULL)
        return GNMERR_FAILURE;

    return GNMERR_NONE;
}


/************************************************************************/
/*                           CreateFeature()                            */
/************************************************************************/
/**
 * Creates feature in a layer. By default checks if the passed layer
 * is a layer of inner dataset and then calls OGRLayer::CreateFeature(). Can
 * be replaced in subclasses to implement aditioanal actions such as connectivity
 * fields forming.
 *
 * @since GDAL 2.0
 */
GNMGFID GNMNetwork::CreateFeature (OGRLayer *poLayer, OGRFeature *poFeature)
{
    // Check if this layer is a layer of inner dataset.
    if (!_isOwnLayer(poLayer)) return -1;

    // Create feature.
    if (poLayer->CreateFeature(poFeature) != OGRERR_NONE)
    {
        return -1;
    }

    return poFeature->GetFID();
}


/************************************************************************/
/*                           SetFeature()                               */
/************************************************************************/
/**
 * Sets feature in a layer. By default checks if the passed layer
 * is a layer of inner dataset and then calls OGRLayer::SetFeature(). Can
 * be replaced in subclasses to implement aditioanal actions such as the
 * influence of the changed feature's parameters to the connected features
 * (geometries changes).
 *
 * @since GDAL 2.0
 */
GNMErr GNMNetwork::SetFeature (OGRLayer *poLayer, OGRFeature *poFeature)
{
    // Check if this layer is a layer of inner dataset.
    if (!_isOwnLayer(poLayer)) return GNMERR_FAILURE;

    // Set feature.
    if (poLayer->SetFeature(poFeature) != OGRERR_NONE)
    {
        return GNMERR_FAILURE;
    }

    return GNMERR_NONE;
}


/************************************************************************/
/*                           DeleteFeature()                            */
/************************************************************************/
/**
 * Deletes feature in a layer. By default checks if the passed layer
 * is a layer of inner dataset and then calls OGRLayer::DeleteFeature(). Can
 * be replaced in subclasses to implement aditioanal actions such as deletion
 * connectivity data.
 *
 * @since GDAL 2.0
 */
GNMErr GNMNetwork::DeleteFeature (OGRLayer *poLayer, long nFID)
{
    // Check if this layer is a layer of inner dataset.
    if (!_isOwnLayer(poLayer)) return GNMERR_FAILURE;

    if (poLayer->DeleteFeature(nFID) != OGRERR_NONE)
        return GNMERR_FAILURE;

    // TODO: DisconnectFeatures()

    return GNMERR_NONE;
}


/************************************************************************/
/*                           CreateRule()                               */
/************************************************************************/
/**
 * Adds the format-specific string rule or constaraint string. By
 * default returns an unsupported error. GDAL-networks use its own rule
 * syntax, see the GNMGdalNetwork class for more details.
 *
 * @see GNMGdalNetwork::CreateRule()
 *
 * @since GDAL 2.0
 */
GNMErr GNMNetwork::CreateRule (const char *pszRuleStr)
{
    return GNMERR_UNSUPPORTED;
}



/* ==================================================================== */
/*                        C API implementation                          */
/* ==================================================================== */


GDALDatasetH GNMGetDataset (GNMNetworkH hNet)
{
    VALIDATE_POINTER1( hNet, "GNMGetDataset", NULL );

    return (GDALDatasetH)((GNMNetwork*)hNet)->GetDataset();
}

