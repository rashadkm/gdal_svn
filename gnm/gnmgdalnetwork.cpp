/******************************************************************************
 * $Id$
 *
 * Name:     gnmgdalnetwork.cpp
 * Project:  GDAL/OGR Geography Network support (Geographic Network Model)
 * Purpose:  GNMGdalNetwork class.
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


bool GNMGdalNetwork::_isOwnLayer (OGRLayer *poLayer)
{
    std::set<OGRLayer*>::iterator it;
    it = _classesSet.find(poLayer);
    if (it == _classesSet.end())
        return false;
    return true;
}

OGRFeature *GNMGdalNetwork::_findConnection (OGRLayer *graph,GNMGFID src,
                                             GNMGFID tgt,GNMGFID con)
{
    char filter[100];
    sprintf(filter,"%s = %d and %s = %d and %s = %d",
            GNM_SYSFIELD_SOURCE,src,
            GNM_SYSFIELD_TARGET,tgt,
            GNM_SYSFIELD_CONNECTOR,con);
    graph->SetAttributeFilter(filter);
    graph->ResetReading();
    OGRFeature *f = graph->GetNextFeature();
    graph->SetAttributeFilter(NULL);
    return f;
}

bool GNMGdalNetwork::_isClassLayer (const char *layerName)
{
    OGRLayer *classLayer = _poDataSet->GetLayerByName(GNM_SYSLAYER_CLASSES);

    // TODO: implement the search via SetAttributeFilter as below.
    // Understand why the filter can not be set with query for example:
    // "layer_name = _gnm_connectivity_building-point_point".

    /*
    char filter[100];
    sprintf(filter,"%s = %s",GNM_SYSFIELD_LAYERNAME,layerName);
    if (classLayer->SetAttributeFilter(filter) != OGRERR_NONE)
        return false;
    classLayer->ResetReading();
    OGRFeature *classFeature;
    while ((classFeature = classLayer->GetNextFeature()) != NULL)
    {
        // It is a class layer.
        OGRFeature::DestroyFeature(classFeature);
        classLayer->SetAttributeFilter(NULL);
        return true;
    }
    classLayer->SetAttributeFilter(NULL);
    */

    // TEMP ------------------------------------------------------------------
    classLayer->ResetReading();
    OGRFeature *classFeature;
    while ((classFeature = classLayer->GetNextFeature()) != NULL)
    {

        if (strcmp(classFeature->GetFieldAsString(GNM_SYSFIELD_LAYERNAME),
                   layerName) == 0)
        {
            // It is a class layer.
            OGRFeature::DestroyFeature(classFeature);
            return true;
        }
        OGRFeature::DestroyFeature(classFeature);
    }
    // -----------------------------------------------------------------------

    // TODO: Check if this is a system layer.

    // We have not found any layer with this name.
    return false;
}

char *GNMGdalNetwork::_makeNewLayerName (const char *name,OGRwkbGeometryType geotype)
{
    char *newName = new char [100];

    // Define prefix for the new name.
    const char *conName = "gnm";

    // Define layer geometry type.
    char *typeName = new char [20];
    switch (wkbFlatten(geotype))
    {
        case wkbPoint: strcpy(typeName,"point"); break;
        case wkbLineString: strcpy(typeName,"line"); break;
        case wkbPolygon: strcpy(typeName, "plg"); break;
        case wkbGeometryCollection: strcpy(typeName, "coll"); break;
        default: strcpy(typeName,"undefgeom"); break;
    }

    sprintf(newName,"%s_%s_%s",conName,name,typeName);

    delete[] typeName;
    return newName;
}


/************************************************************************/
/*                             Constructor                              */
/************************************************************************/
GNMGdalNetwork::GNMGdalNetwork(): GNMNetwork()
{
    _poSpatialRef = NULL;

    //_formatName = "GDAL Network";

    _meta_vrsn_i = -1;
    _meta_srs_i = -1;
    _meta_gfidcntr_i = -1;
    _meta_name_i = -1;
    _meta_descr_i = -1;
}


/************************************************************************/
/*                             Destructor                               */
/************************************************************************/
GNMGdalNetwork::~GNMGdalNetwork()
{
    //delete[] _driverName;

    if (_poSpatialRef != NULL)
        _poSpatialRef->Release();

    // We delete _poDataSet here if it is owned by the GNMGdalNetwork
    // object.
    // if (_ownDataset)
    //     GDALClose(_poDataSet);
}


/************************************************************************/
/*                        GetConnection()                               */
/************************************************************************/
//* \brief Returns the instantly formed GNMConnection object by its FID from
//* system graph layer or NULL if failed to form. Must be freed by caller.
/**
 * \brief Returns the connection in OGRFeature form.
 *
 * Returns the connection in OGRFeature form by its FID from system graph layer.
 * NOTE: The returned OGRFeature should be freed via OGRFeature::DestroyFeature().
 *
 * @param nConFID Connection's freature FID.
 * @return Instantly formed connection's feature or NULL if failed.
 *
 * @since GDAL 2.0
 */
OGRFeature *GNMGdalNetwork::GetConnection(long nConFID)
{
    OGRLayer *layer = _poDataSet->GetLayerByName(GNM_SYSLAYER_GRAPH);
    OGRFeature *feature = layer->GetFeature(nConFID);
/*    if (feature == NULL)
        return NULL;

    GNMConnection *ret = new GNMConnection();
    ret->nSrcGFID = feature->GetFieldAsInteger(GNM_SYSFIELD_SOURCE);
    ret->nTgtGFID = feature->GetFieldAsInteger(GNM_SYSFIELD_TARGET);
    ret->nConGFID = feature->GetFieldAsInteger(GNM_SYSFIELD_CONNECTOR);
    ret->dCost = feature->GetFieldAsDouble(GNM_SYSFIELD_COST);
    ret->dInvCost = feature->GetFieldAsDouble(GNM_SYSFIELD_INVCOST);
    ret->dir = feature->GetFieldAsInteger(GNM_SYSFIELD_DIRECTION);
    return ret;
*/

    return feature;
}


/************************************************************************/
/*                        GetNextConnection()                          */
/************************************************************************/
/**
 * \brief Returns the next connection in OGRFeature form.
 *
 * Returns the next connection in OGRFeature form. Use GNMGdalNetwork::
 * ResetConnectionsReading() if you want to read features from the begining.
 * NOTE: The returned OGRFeature should be freed via OGRFeature::DestroyFeature().
 *
 * @return Connection's feature or NULL if failed or reached the last one.
 *
 * @since GDAL 2.0
 */
OGRFeature *GNMGdalNetwork::GetNextConnection ()
{
    return _poDataSet->GetLayerByName(GNM_SYSLAYER_GRAPH)->GetNextFeature();
}


/************************************************************************/
/*                        ResetConnectionsReading()                     */
/************************************************************************/
/**
 * \brief Resets connections reading.
 *
 * Resets connections reading by calling OGRLayer::ResetReading() on system
 * graph layer.
 *
 * @since GDAL 2.0
 */
void GNMGdalNetwork::ResetConnectionsReading ()
{
    _poDataSet->GetLayerByName(GNM_SYSLAYER_GRAPH)->ResetReading();
}


/************************************************************************/
/*                          CreateLayer()                               */
/************************************************************************/
/**
 * Creates layer in a common SRS of the whole network. There is no need
 * to pass the poSpatialRef param. The name of the new layer will be formed
 * according to special name convention: gnm_<pszName>_<eGType>. The geometry
 * type of layer will be set from eGType parameter.
 *
 * @since GDAL 2.0
 */
OGRLayer *GNMGdalNetwork::CreateLayer (const char *pszName,OGRFeatureDefn *poFeatureDefn,
                               OGRSpatialReference *poSpatialRef,
                               OGRwkbGeometryType eGType,char **papszOptions)
{
    // FORMAT NOTE: some datasets does not support names with "-" symbol.

    // Construct the special unique name for new layer, accordingly
    // to the special naming conventions.
    const char* newName = _makeNewLayerName(pszName,eGType);

    // Check if there is already a layer with this name.
    // This layer can not have system layer name so we can use _isClassLayer().
    if (this->_isClassLayer(newName))
    {
        //CPLErr
        delete[] newName;
        return NULL;
    }

    // Create layer in a common SRS.
    OGRLayer * newLayer;
    newLayer = GNMNetwork::CreateLayer(newName,poFeatureDefn,_poSpatialRef,
                                       eGType,papszOptions);
    if (newLayer == NULL)
    {
        delete[] newName;
        return NULL;
    }

    // Add system field.
    // Check if the passed layer definition has the gfid field already.
    int ix = poFeatureDefn->GetFieldIndex(GNM_SYSFIELD_GFID);
    if (ix == -1)
    {
        OGRFieldDefn newField(GNM_SYSFIELD_GFID,OFTInteger);
        if(newLayer->CreateField(&newField) != OGRERR_NONE)
        {
            //CPLError
            GNMNetwork::DeleteLayer(newName);
            delete[] newName;
            return NULL;
        }
    }
    else
    {
        // Assumed that user should firstly delete not-integer gfid column.
        OGRFieldDefn *fieldDefn = poFeatureDefn->GetFieldDefn(ix);
        if (fieldDefn->GetType() != OFTInteger)
        {
            //CPLError
            GNMNetwork::DeleteLayer(newName);
            delete[] newName;
            return NULL;
        }
    }

    // Register layer in the according system table.
    OGRLayer *classLayer = _poDataSet->GetLayerByName(GNM_SYSLAYER_CLASSES);
    OGRFeature *feature = OGRFeature::CreateFeature(classLayer->GetLayerDefn());
    feature->SetField(GNM_SYSFIELD_LAYERNAME, newName);
    if(classLayer->CreateFeature(feature) != OGRERR_NONE)
    {
        //CPLError
        OGRFeature::DestroyFeature(feature);
        GNMNetwork::DeleteLayer(newName);
        delete[] newName;
        return NULL;
    }
    OGRFeature::DestroyFeature(feature);

    // Save layer pointer.
    _classesSet.insert(newLayer);

    delete[] newName;
    return newLayer;
}


/************************************************************************/
/*                            DeleteLayer()                             */
/************************************************************************/
/**
 * Delete layer with all features and the according connections from the
 * network. Currently unsupported.
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalNetwork::DeleteLayer (const char *pszName)
{
    // TODO: Delete all features and its connections.

    return GNMERR_UNSUPPORTED;
}


/************************************************************************/
/*                            CopyLayer()                               */
/************************************************************************/
/**
 * \brief Copies the layer transforming all features in a single SRS.
 *
 * Copies the layer, transforming its features' coordinates to the
 * GDAL-network's common one (if it is a layer from an external dataset).
 * Useful when the connectivity has been created over the void dataset and
 * there is a need to import features.
 * NOTE: This method does not use the GDALDataset::CopyLayer().
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalNetwork::CopyLayer (OGRLayer *poSrcLayer, const char *pszNewName)
{
    if (poSrcLayer == NULL || pszNewName == NULL)
    {
        //CPLErr
        return GNMERR_FAILURE;
    }

    // TODO: Ensure that the passed layer is not the system layer of
    // this network (simply by name).

    // Create new layer in a network.
    OGRFeatureDefn *fDefn = poSrcLayer->GetLayerDefn();
    OGRLayer *newLayer = GNMGdalNetwork::CreateLayer(pszNewName, fDefn, _poSpatialRef,
                                           poSrcLayer->GetGeomType());
    if (newLayer == NULL)
    {
        //CPLErr
        return GNMERR_FAILURE;
    }

    //this->FlushCache();

    // Prepare the coordinate transformation object.
    bool allowTransformation = false;
    OGRCoordinateTransformation *transform;
    OGRSpatialReference *sourceSRS = poSrcLayer->GetSpatialRef();
    if (sourceSRS != NULL && sourceSRS->IsSame(_poSpatialRef) == FALSE)
    {
        transform = OGRCreateCoordinateTransformation(sourceSRS, _poSpatialRef);
        if (transform != NULL)
            allowTransformation = true;
    }

	// Read gfid counter.
	OGRLayer *metaLayer = _poDataSet->GetLayerByName(GNM_SYSLAYER_META);
	OGRFeature *gfidFeature = metaLayer->GetFeature(_meta_gfidcntr_i);
	GNMGFID gfidCounter = gfidFeature->GetFieldAsInteger(GNM_SYSFIELD_PARAMVALUE);

	//CPLSetConfigOption("PG_USE_COPY","NO");

	// Read all source features.
	OGRFeature *sourceFeature;
    poSrcLayer->ResetReading();
	std::vector<OGRFeature*> srcFeatures;
    while((sourceFeature = poSrcLayer->GetNextFeature()) != NULL)
    {
		srcFeatures.push_back(sourceFeature);
	}

	// Iterate source features and copy them to the new layer.
    int count = fDefn->GetFieldCount();
	newLayer->StartTransaction(); // We need to start and commit transaction for database datasets.
	std::vector<OGRFeature*>::iterator itSF;
	std::vector<OGRFeature*> newFeatures;
	for (itSF = srcFeatures.begin(); itSF != srcFeatures.end(); ++itSF)
	{
		// Create new feature with new layer definition.
        OGRFeature *newFeature = OGRFeature::CreateFeature(newLayer->GetLayerDefn());
        if (newFeature == NULL)
        {
            //CPLErr
			// IMPORTANT TODO: Delete new layer name from classes system table.
            // ISSUE: Delete all features copied earlier?
			// TODO: rollback transaction
            return GNMERR_FAILURE;
        }

        // Get the geometry from the source feature. Transform it if
        // the feature exists and a transformation object has been defined.
        OGRGeometry *newGeom = (*itSF)->StealGeometry();
        if (newGeom != NULL)
        {
            if (allowTransformation)
                newGeom->transform(transform);
            newFeature->SetGeometryDirectly(newGeom);
        }

        // Copy source feature fid.
        long fid = (*itSF)->GetFID();
        newFeature->SetFID(fid);

		// NOTE: we can't write the feature to the featLayer here, because it breaks the transaction
		// for the newLayer.

        // Assign new gfid.
        newFeature->SetField(GNM_SYSFIELD_GFID, gfidCounter);
        gfidCounter++;

		//CPLSetConfigOption

        // Copy source feature attribute values.
        for (int i = 0; i < count; i++)
        {
            if ((*itSF)->IsFieldSet(i) == TRUE)
            {
                OGRFieldType fType = fDefn->GetFieldDefn(i)->GetType();
                const char *fName = fDefn->GetFieldDefn(i)->GetNameRef();
                switch (fType)
                {
                    case OFTInteger:
                        newFeature->SetField(fName, (*itSF)->GetFieldAsInteger(i));
                    break;
                    case OFTReal:
                        // ISSUE: understand why several fields like ..\..\data\belarus ->
                        // field OSM_ID are copied incorrectly (the values extends the
                        // type maximum value).
                        newFeature->SetField(fName, (*itSF)->GetFieldAsDouble(i));
                    break;
                    default:
                        newFeature->SetField(fName, (*itSF)->GetFieldAsString(i));
                    break;
                    // TODO: widen this list.
                    //...
                }
            }
        }

        // Add a feature to the new layer.
        if(newLayer->CreateFeature(newFeature) != OGRERR_NONE)
        {
            //CPLErr
            OGRFeature::DestroyFeature(newFeature);
            OGRFeature::DestroyFeature((*itSF));
            if (allowTransformation)
                OCTDestroyCoordinateTransformation(
                        (OGRCoordinateTransformationH)transform);
			// IMPORTANT TODO: Delete new layer name from classes system table.
			newLayer->RollbackTransaction();
            return GNMERR_FAILURE;
        }

		newFeatures.push_back(newFeature);

        //OGRFeature::DestroyFeature(newFeature);
        OGRFeature::DestroyFeature(*itSF);
    }

	newLayer->CommitTransaction();

	// Register new features in the special system layer.
	const char *newLayerName = newLayer->GetName();
	OGRLayer *featLayer = _poDataSet->GetLayerByName(GNM_SYSLAYER_FEATURES);
	featLayer->StartTransaction();
	newLayer->ResetReading();
	//OGRFeature *newFeature;
	//while ((newFeature = newLayer->GetNextFeature()) != NULL)
	std::vector<OGRFeature*>::iterator itNF;
	for (itNF = newFeatures.begin(); itNF != newFeatures.end(); ++itNF)
	{
		OGRFeature *featFeature = OGRFeature::CreateFeature(featLayer->GetLayerDefn());
		featFeature->SetField(GNM_SYSFIELD_GFID,(*itNF)->GetFieldAsInteger(GNM_SYSFIELD_GFID));
        featFeature->SetField(GNM_SYSFIELD_LAYERNAME,newLayerName);
		featFeature->SetField(GNM_SYSFIELD_FID,(int)(*itNF)->GetFID());
        if (featLayer->CreateFeature(featFeature) != OGRERR_NONE)
        {
            //CPLErr
			// IMPORTANT TODO: Delete new layer name from classes system table.
            // TODO: Delete the according new feature.
            //OGRFeature::DestroyFeature(newFeature);
			OGRFeature::DestroyFeature((*itNF));
            OGRFeature::DestroyFeature(featFeature);

			featLayer->RollbackTransaction();
            return GNMERR_FAILURE;
        }
        //OGRFeature::DestroyFeature(newFeature);
		OGRFeature::DestroyFeature((*itNF));
        OGRFeature::DestroyFeature(featFeature);
	}
	featLayer->CommitTransaction();

    // Save gfid counter.
	//metaLayer->StartTransaction();
    gfidFeature->SetField(GNM_SYSFIELD_PARAMVALUE,gfidCounter);
    metaLayer->SetFeature(gfidFeature);
    OGRFeature::DestroyFeature(gfidFeature);
	//metaLayer->CommitTransaction();

    // Save layer pointer.
    _classesSet.insert(newLayer);

    // Free resources.
    if (allowTransformation)
        OCTDestroyCoordinateTransformation(
                    (OGRCoordinateTransformationH)transform);

    return GNMERR_NONE;
}


/************************************************************************/
/*                       CreateFeature()                               */
/************************************************************************/
/**
 * Creates feature in a layer (in a common SRS) assigning the new
 * GFID to it.
 *
 * @since GDAL 2.0
 */
GNMGFID GNMGdalNetwork::CreateFeature (OGRLayer *poLayer,OGRFeature *poFeature)
{
    if (poLayer == NULL || poFeature == NULL)
        return -1;

    // Check if this layer is a layer of inner dataset.
    if (!_isOwnLayer(poLayer))
        return -1;

    // Check if this layer is not a system layer.
    const char *layerName = poLayer->GetName();
    if (!_isClassLayer(layerName))
        return -1;

    // Load, assign, increment gfid counter.
    // TODO: if we create features in a loop it may be too loong each
    // time to load a GFID counter. Think about how to store counter
    // in memory (but this way it should be correctly saved anytime (in
    // GNMGdalNetwork destructor?).
    OGRLayer *metaLayer = _poDataSet->GetLayerByName(GNM_SYSLAYER_META);
    OGRFeature *gfidFeature = metaLayer->GetFeature(_meta_gfidcntr_i);
    GNMGFID gfidCounter = gfidFeature->GetFieldAsInteger(GNM_SYSFIELD_PARAMVALUE);
    poFeature->SetField(GNM_SYSFIELD_GFID, gfidCounter);

    // Create feature.
    if (poLayer->CreateFeature(poFeature) != OGRERR_NONE)
    {
        OGRFeature::DestroyFeature(gfidFeature);
        return -1;
    }

    // Write it to the special system layer.
    OGRLayer *featLayer = _poDataSet->GetLayerByName(GNM_SYSLAYER_FEATURES);
    OGRFeature *featFeature = OGRFeature::CreateFeature(featLayer->GetLayerDefn());
    featFeature->SetField(GNM_SYSFIELD_GFID,gfidCounter);
    featFeature->SetField(GNM_SYSFIELD_LAYERNAME,layerName);
    featFeature->SetField(GNM_SYSFIELD_FID,(int)poFeature->GetFID());
    if (featLayer->CreateFeature(featFeature) != OGRERR_NONE)
    {
        //CPLErr
        OGRFeature::DestroyFeature(featFeature);
        OGRFeature::DestroyFeature(gfidFeature);
        return -1;
    }
    OGRFeature::DestroyFeature(featFeature);

    // Increment and save gfid counter.
    gfidCounter++;
    gfidFeature->SetField(GNM_SYSFIELD_PARAMVALUE, gfidCounter);
    OGRLayer *gfidLayer = _poDataSet->GetLayerByName(GNM_SYSLAYER_META);
    gfidLayer->SetFeature(gfidFeature);
    OGRFeature::DestroyFeature(gfidFeature);
    return gfidCounter - 1;
}


/************************************************************************/
/*                           SetFeature()                               */
/************************************************************************/
/**
 * Modifies feature and makes the corresponding changes in a graph, according
 * to the rules (if the cost or direction fields of feature have been changed).
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalNetwork::SetFeature (OGRLayer *poLayer,OGRFeature *poFeature)
{
    if (poLayer == NULL || poFeature == NULL)
        return GNMERR_FAILURE;

    // Check if this layer is a layer of inner dataset.
    if (!_isOwnLayer(poLayer))
        return GNMERR_FAILURE;

    // Check if this layer is not a system layer.
    if (!_isClassLayer(poLayer->GetName()))
        return GNMERR_FAILURE;

    // Check if this layer is not a class layer of system edges.
    const char *layerName = poLayer->GetName();
    if (strcmp(layerName,GNM_CLASSLAYER_SYSEDGES) == 0)
        return GNMERR_FAILURE;

    // Set feature.
    if (poLayer->SetFeature(poFeature) != OGRERR_NONE)
    {
        return GNMERR_FAILURE;
    }

    // Get the graph feature and Update costs and direction in
    // a graph according to the rules.
    GNMGFID gfid = (GNMGFID)poFeature->GetFieldAsInteger(GNM_SYSFIELD_GFID);
    OGRLayer *graphLayer = _poDataSet->GetLayerByName(GNM_SYSLAYER_GRAPH);
    if (graphLayer != NULL)
    {
        char filter[100];
        sprintf(filter, "%s = %i", GNM_SYSFIELD_CONNECTOR, gfid);
        graphLayer->SetAttributeFilter(filter);
        // There can be only one connector among the graph connection features:
        OGRFeature *graphFeature = graphLayer->GetNextFeature();
        if (graphFeature != NULL)
        {
            GNMClassRule *rule = _findRuleForClass(layerName);
            if (rule != NULL)
            {
                double dValue;

                //if (rule->dirCostField != "")
                if (rule->dirCostOper != gnmOpUndefined)
                {
                    if (rule->dirCostOper == gnmOpConst)
                    {
                        dValue = rule->dirCostConst;
                    }
                    else
                    {
                        double fieldValue = poFeature->GetFieldAsDouble(rule->dirCostField.data());
                        if (fieldValue != 0.0)
                        {
                            if (rule->dirCostOper == gnmOpField)
                            {
                                dValue = fieldValue;
                            }
                            else if (rule->dirCostOper == gnmOpFieldAConst)
                            {
                                dValue = fieldValue + rule->dirCostConst;
                            }
                            else if (rule->dirCostOper == gnmOpFieldSConst)
                            {
                                dValue = fieldValue - rule->dirCostConst;
                            }
                            else if (rule->dirCostOper == gnmOpFieldMConst)
                            {
                                dValue = fieldValue * rule->dirCostConst;
                            }
                            else if (rule->dirCostOper == gnmOpFieldDConst)
                            {
                                dValue = fieldValue / rule->dirCostConst;
                            }
                        }
                        else
                        {
                            dValue = 0.0;
                        }
                    }

                    graphFeature->SetField(GNM_SYSFIELD_COST,dValue);
                         //poFeature->GetFieldAsDouble(rule->dirCostField.data()));
                }

                //if (rule->invCostField != "")
                if (rule->invCostOper != gnmOpUndefined)
                {
                    if (rule->invCostOper == gnmOpConst)
                    {
                        dValue = rule->invCostConst;
                    }
                    else
                    {
                        double fieldValue = poFeature->GetFieldAsDouble(rule->invCostField.data());
                        if (fieldValue != 0.0)
                        {
                            if (rule->invCostOper == gnmOpField)
                            {
                                dValue = fieldValue;
                            }
                            else if (rule->invCostOper == gnmOpFieldAConst)
                            {
                                dValue = fieldValue + rule->invCostConst;
                            }
                            else if (rule->invCostOper == gnmOpFieldSConst)
                            {
                                dValue = fieldValue - rule->invCostConst;
                            }
                            else if (rule->invCostOper == gnmOpFieldMConst)
                            {
                                dValue = fieldValue * rule->invCostConst;
                            }
                            else if (rule->invCostOper == gnmOpFieldDConst)
                            {
                                dValue = fieldValue / rule->invCostConst;
                            }
                        }
                        else
                        {
                            dValue = 0.0;
                        }
                    }

                    graphFeature->SetField(GNM_SYSFIELD_INVCOST,dValue);
                         //poFeature->GetFieldAsDouble(rule->invCostField.data()));
                }

                if (rule->dirField != "")
                {
                    graphFeature->SetField(GNM_SYSFIELD_DIRECTION,
                         poFeature->GetFieldAsInteger(rule->dirField.data()));
                }
            }
            OGRFeature::DestroyFeature(graphFeature);
        }
        graphLayer->SetAttributeFilter(NULL);
    }

    // ISSUE: Make geometry changes in connected features using rules?
    // For now we do not intend to make any geometry changes in the features which
    // are topologically connected to the current.

    return GNMERR_NONE;
}


/************************************************************************/
/*                       DeleteFeature()                               */
/************************************************************************/
/**
 * Deletes feature from the layer and all corresponding connections from
 * the graph.
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalNetwork::DeleteFeature (OGRLayer *poLayer,long nFID)
{
    if (poLayer == NULL)
        return GNMERR_FAILURE;

    // Check if this layer is a layer of inner dataset.
    if (!_isOwnLayer(poLayer))
        return GNMERR_FAILURE;

    // Check if this layer is not a system layer.
    if (!_isClassLayer(poLayer->GetName()))
        return GNMERR_FAILURE;

    // Get feature's gfid.
    OGRFeature *feature = poLayer->GetFeature(nFID);
    if (feature == NULL)
        return GNMERR_FAILURE;
    GNMGFID gfid = feature->GetFieldAsInteger(GNM_SYSFIELD_GFID);
    OGRFeature::DestroyFeature(feature);

    // Delete from layer.
    if (poLayer->DeleteFeature(nFID) != OGRERR_NONE)
    {
        return GNMERR_FAILURE;
    }

    // Delete from features layer.
    char filter1[100];
    sprintf(filter1, "%s = %i and %s = %i", GNM_SYSFIELD_GFID, gfid,
                                            GNM_SYSFIELD_FID, nFID);
    OGRLayer *featLayer = _poDataSet->GetLayerByName(GNM_SYSLAYER_FEATURES);
    featLayer->SetAttributeFilter(filter1);
    featLayer->ResetReading();
    OGRFeature *featFeature = featLayer->GetNextFeature();
    long featId = featFeature->GetFID();
    featLayer->DeleteFeature(featId);
    OGRFeature::DestroyFeature(featFeature);
    featLayer->SetAttributeFilter(NULL);

    // Delete all corresponding connections from graph layer.
    char filter2[100];
    sprintf(filter2, "%s = %i or %s = %i or %s = %i",
            GNM_SYSFIELD_SOURCE, gfid,
            GNM_SYSFIELD_TARGET, gfid,
            GNM_SYSFIELD_CONNECTOR, gfid);
    OGRLayer *graphLayer = _poDataSet->GetLayerByName(GNM_SYSLAYER_GRAPH);
    graphLayer->SetAttributeFilter(filter2);
    graphLayer->ResetReading();
    OGRFeature *graphFeature;
    std::vector<long> fids_to_del;
    while ((graphFeature = graphLayer->GetNextFeature()) != NULL)
    {
        fids_to_del.push_back(graphFeature->GetFID());
        OGRFeature::DestroyFeature(graphFeature);
    }
    graphLayer->SetAttributeFilter(NULL);
    for (unsigned i=0; i<fids_to_del.size(); i++)
    {
        graphLayer->DeleteFeature(fids_to_del.at(i));
    }

    return GNMERR_NONE;
}


/************************************************************************/
/*                        ConnectFeatures()                             */
/************************************************************************/
/**
 * \brief Connects two features (via third one if set) and assigns costs to
 * this connection.
 *
 * Sets the connection among given features writing its info to the graph.
 * The connections integrity fully supported: the features must exist; the
 * features must not already be edges if they are passed to the method as
 * vertexes; and so on.
 *
 * Note, that this method will read layers and its features if the according 
 * rules are set via GetFeature(), so for several drivers (e.g. PostgreSQL) 
 * it's not correct to call it while the sequential read via GetNextFeature()  
 * is performed.
 *
 * @param nSrcGFID The GFID of feature which will serve as the source vertex
 * in this connection. The feature actually can have any geometry type.
 * @param nTgtGFID The GFID of feature which will serve as the target vertex
 * in this connection. The feature actually can have any geometry type.
 * @param nConGFID The GFID of feature which will serve as the edge (connector)
 * in this connection. The feature actually can have any geometry type. Even more
 * if -1 is passed instead of real feature GFID the according system edge
 * will be created, that means that the connection becomes virtual rather than
 * physical. So it is not necessary to have a real feature in order to connect
 * two others.
 * @param dCost The cost (weight) value of the edge in this connection. If any
 * rules were set for the connector-feature's class, this value will be ignored
 * and will not be written to the graph. If you want than to change the cost of edge
 * anyway - you can use GNMGdalNetwork::ReconnectFeatures() for this.
 * @param dInvCost The inverse cost (weight) value of the edge. Same as dCost.
 * @param nDir The direction of the edge. There can be 3 types of direction:
 * bidirected, from source to target and from target to source. The rules of
 * assigning same as for dCost and dInvCost.
 *
 * @return GNMERR_NONE if ok, GNMERR_CONRULE_RESTRICT if the connection among
 * classes of these features is forbidden by rules (see rule concept), or other
 * error if failure.
 *
 * @see GNMGdalNetwork::ReconnectFeatures()
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalNetwork::ConnectFeatures (GNMGFID nSrcGFID,GNMGFID nTgtGFID,
                                GNMGFID nConGFID,double dCost,
                                double dInvCost,GNMDirection nDir)
{
    OGRFeature *feature;
    std::string srcLayer = "";
    std::string tgtLayer = "";
    std::string conLayer = "";
    long srcFid = -1;
    long tgtFid = -1;
    long conFid = -1;

    // Determine features classes and also check if the features exist.
    OGRLayer *featLayer = _poDataSet->GetLayerByName(GNM_SYSLAYER_FEATURES);
    OGRFeature *featFeature;
    char filter[100];
    sprintf(filter,"%s = %d or %s = %d or %s = %d",GNM_SYSFIELD_GFID,nSrcGFID,
                                            GNM_SYSFIELD_GFID,nTgtGFID,
                                            GNM_SYSFIELD_GFID,nConGFID);
    featLayer->SetAttributeFilter(filter);
    featLayer->ResetReading();
    while ((featFeature = featLayer->GetNextFeature()) != NULL)
    {
        if (featFeature->GetFieldAsInteger(GNM_SYSFIELD_GFID) == nSrcGFID)
        {
            srcLayer = featFeature->GetFieldAsString(GNM_SYSFIELD_LAYERNAME);
            srcFid = featFeature->GetFieldAsInteger(GNM_SYSFIELD_FID);
        }
        else if (featFeature->GetFieldAsInteger(GNM_SYSFIELD_GFID) == nTgtGFID)
        {
            tgtLayer = featFeature->GetFieldAsString(GNM_SYSFIELD_LAYERNAME);
            tgtFid = featFeature->GetFieldAsInteger(GNM_SYSFIELD_FID);
        }
        else if (featFeature->GetFieldAsInteger(GNM_SYSFIELD_GFID) == nConGFID)
        {
            conLayer = featFeature->GetFieldAsString(GNM_SYSFIELD_LAYERNAME);
            conFid = featFeature->GetFieldAsInteger(GNM_SYSFIELD_FID);
        }
        OGRFeature::DestroyFeature(featFeature);
    }
    featLayer->SetAttributeFilter(NULL);
    // If there are no such features return an error.
    // We check the connector only if the existed GFID passed.
    if (srcFid == -1 || tgtFid == -1 || (conFid == -1 && nConGFID != -1))
        return GNMERR_FAILURE;

    // Use rules to determine if the connection is legal.
    // If the rule for one of the connected classes (we search for source) has
    // not been found - we allow the connection by default. Otherwise we search
    // the available connection among listed for this class. If it is not found
    // we return an error, that the connection can not be established.
    GNMClassRule *rule = _findRuleForClass(srcLayer);
    if (rule != NULL)
    {
        _GNMVertEdgeClassNames pair = std::make_pair(tgtLayer,conLayer);
        _GNMLegalCons::iterator it = rule->conRules.find(pair);
        if (!rule->conRules.empty() && it == rule->conRules.end())
        {
            return GNMERR_CONRULE_RESTRICT;
        }
    }

    // Use rules to determine an edge's costs (weights) and direction.
    // If there is no according rules for the connector's class use user input.
    rule = _findRuleForClass(conLayer);
    if (rule != NULL)
    {
        feature = NULL;

        // Try to use direct cost rule.
        //if (rule->dirCostField != "")
        if (rule->dirCostOper != gnmOpUndefined)
        {
            if (rule->dirCostOper == gnmOpConst)
            {
                dCost = rule->dirCostConst;
            }
            else
            {
                double fieldValue = 0.0;
                feature = _poDataSet->GetLayerByName(conLayer.data())->GetFeature(conFid);
                if (feature != NULL)
                    fieldValue = feature->GetFieldAsDouble(rule->dirCostField.data());
                if (fieldValue != 0.0) // check in case if the rule name had been set incorrectly
                {
                    if (rule->dirCostOper == gnmOpField)
                    {
                        dCost = fieldValue;
                    }
                    else if (rule->dirCostOper == gnmOpFieldAConst)
                    {
                        dCost = fieldValue + rule->dirCostConst;
                    }
                    else if (rule->dirCostOper == gnmOpFieldSConst)
                    {
                        dCost = fieldValue - rule->dirCostConst;
                    }
                    else if (rule->dirCostOper == gnmOpFieldMConst)
                    {
                        dCost = fieldValue * rule->dirCostConst;
                    }
                    else if (rule->dirCostOper == gnmOpFieldDConst)
                    {
                        // Note: here can not be devision by zero because the
                        // comstant in the rule can not be 0.
                        dCost = fieldValue / rule->dirCostConst;
                    }
                }
            }
        }

        // Try to use inverse cost rule.
        if (rule->invCostOper != gnmOpUndefined)
        {
            if (rule->invCostOper == gnmOpConst)
            {
                dInvCost = rule->invCostConst;
            }
            else
            {
                double fieldValue = 0.0;
                if (feature == NULL)
                    feature = _poDataSet->GetLayerByName(conLayer.data())->GetFeature(conFid);
                if (feature != NULL)
                    fieldValue = feature->GetFieldAsDouble(rule->invCostField.data());
                if (fieldValue != 0.0)
                {
                    if (rule->invCostOper == gnmOpField)
                    {
                        dInvCost = fieldValue;
                    }
                    else if (rule->invCostOper == gnmOpFieldAConst)
                    {
                        dInvCost = fieldValue + rule->dirCostConst;
                    }
                    else if (rule->invCostOper == gnmOpFieldSConst)
                    {
                        dInvCost = fieldValue - rule->invCostConst;
                    }
                    else if (rule->invCostOper == gnmOpFieldMConst)
                    {
                        dInvCost = fieldValue * rule->invCostConst;
                    }
                    else if (rule->invCostOper == gnmOpFieldDConst)
                    {
                        dInvCost = fieldValue / rule->invCostConst;
                    }
                }
            }
        }

        // Try to use direction rule.
        if (rule->dirField != "")
        {
            if (feature == NULL)
                feature = _poDataSet->GetLayerByName(conLayer.data())->GetFeature(conFid);
            if (feature != NULL)
                nDir = feature->GetFieldAsInteger(rule->dirField.data());
        }

        if (feature != NULL)
            OGRFeature::DestroyFeature(feature);
    }

    // Set direction if incorrect.
    if (nDir != GNM_DIR_DOUBLE
        && nDir != GNM_DIR_SRCTOTGT
        && nDir != GNM_DIR_TGTTOSRC)
        nDir = GNM_DIR_DOUBLE;

    // Return error if the incorrect costs are set.
    if (dCost < 0 || dInvCost < 0)
        return GNMERR_FAILURE;

    // Return error if such connector is already a connector in other connection.
    // Also check if the given edge (connector) is a vertex in any other
    // connection or the opposite: if one of the given vertexes is an edge
    // already.
    // ISSUE: it may be faster to save the state of feature (edge/vertex) in
    // some system field in the feature itself. Think about it.
    OGRLayer *graphLayer = _poDataSet->GetLayerByName(GNM_SYSLAYER_GRAPH);
    char filter2[100];
    sprintf(filter2,"%s = %d or %s = %d or %s = %d or %s = %d or %s = %d",
            GNM_SYSFIELD_CONNECTOR,nConGFID,GNM_SYSFIELD_SOURCE,nConGFID,
            GNM_SYSFIELD_TARGET,nConGFID,GNM_SYSFIELD_CONNECTOR,nSrcGFID,
            GNM_SYSFIELD_CONNECTOR,nTgtGFID);
    graphLayer->SetAttributeFilter(filter2);
    graphLayer->ResetReading();
    OGRFeature *graphFeature = graphLayer->GetNextFeature();
    graphLayer->SetAttributeFilter(NULL);
    if (graphFeature != NULL)
    {
        OGRFeature::DestroyFeature(graphFeature);
        return GNMERR_FAILURE;
    }

    // There must not be a connection with the same source, target and
    // connector. Check it.
    feature = _findConnection(graphLayer,nSrcGFID,nTgtGFID,nConGFID);
    if (feature != NULL)
    {
        OGRFeature::DestroyFeature(feature);
        //CPLErr
        return GNMERR_FAILURE;
    }
    OGRFeature::DestroyFeature(feature);

    // If the direction is bidirected we must check if there is a target-source
    // equivalent of this connection.
    if (nDir == GNM_DIR_DOUBLE)
    {
        feature = _findConnection(graphLayer,nTgtGFID,nSrcGFID,nConGFID);
        if (feature != NULL)
        {
            OGRFeature::DestroyFeature(feature);
            //CPLErr
            return GNMERR_FAILURE;
        }
        OGRFeature::DestroyFeature(feature);
    }

    // Form new system edge feature if the connector = -1 is passed.
    if (nConGFID == -1)
    {
        OGRLayer *sysedgeLayer = _poDataSet->GetLayerByName(GNM_CLASSLAYER_SYSEDGES);
        OGRFeature *sysedgefeature;
        sysedgefeature = OGRFeature::CreateFeature(sysedgeLayer->GetLayerDefn());
        // The according GFID will be written to the field with the same name "gfid"
        // when the feature will be created in the layer. We assign the new feature's
        // gfid to nConGFID that means that we have a new connector-feature.
        nConGFID = CreateFeature(sysedgeLayer,sysedgefeature);
        if (nConGFID == -1)
            return GNMERR_FAILURE;
        OGRFeature::DestroyFeature(sysedgefeature);
    }

    // Add connection.
    feature = OGRFeature::CreateFeature(graphLayer->GetLayerDefn());
    feature->SetField(GNM_SYSFIELD_SOURCE, nSrcGFID);
    feature->SetField(GNM_SYSFIELD_TARGET, nTgtGFID);
    feature->SetField(GNM_SYSFIELD_CONNECTOR, nConGFID);
    feature->SetField(GNM_SYSFIELD_COST, dCost);
    feature->SetField(GNM_SYSFIELD_INVCOST, dInvCost);
    feature->SetField(GNM_SYSFIELD_DIRECTION, nDir);
	graphLayer->StartTransaction();
    if(graphLayer->CreateFeature(feature) != OGRERR_NONE)
    {
        // TODO: delete system edge if it was created.
        OGRFeature::DestroyFeature(feature);
		graphLayer->RollbackTransaction();
        return GNMERR_FAILURE;
    }
	graphLayer->CommitTransaction();
    OGRFeature::DestroyFeature(feature);

    return GNMERR_NONE;
}


/************************************************************************/
/*                       DisconnectFeatures()                           */
/************************************************************************/
/**
 * \brief Removes the connection from the graph.
 *
 * Tries to find the connection with the passed source, target and connector's
 * GFIDs in the graph and delete it if found.
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalNetwork::DisconnectFeatures (GNMGFID nSrcGFID,GNMGFID nTgtGFID,
                                         GNMGFID nConGFID)
{
    OGRLayer* graphLayer = _poDataSet->GetLayerByName(GNM_SYSLAYER_GRAPH);

    // Find connection and delete it from the graph.
    OGRFeature *graphFeature = _findConnection(graphLayer,nSrcGFID,nTgtGFID,nConGFID);
    if (graphFeature == NULL)
    {
        OGRFeature::DestroyFeature(graphFeature);
        //CPLErr
        return GNMERR_FAILURE;
    }
    long fid = graphFeature->GetFID();
    OGRFeature::DestroyFeature(graphFeature);

    if (graphLayer->DeleteFeature(fid) != OGRERR_NONE)
    {
        //CPLErr
        return GNMERR_FAILURE;
    }

    // Get feature class and fid in order to define if the connector
    // is a system edge.
    OGRLayer *featLayer = _poDataSet->GetLayerByName(GNM_SYSLAYER_FEATURES);
    OGRFeature *featFeature;
    char filter[100];
    sprintf(filter,"%s = %d",GNM_SYSFIELD_GFID,nConGFID);
    featLayer->SetAttributeFilter(filter);
    featLayer->ResetReading();
    featFeature = featLayer->GetNextFeature();
    featLayer->SetAttributeFilter(NULL);
    if (featFeature != NULL)
    {
        // If it is the system edge we delete it because it is no longer useful
        // and at the same time only one system edge with this GFID can occur in
        // the graph layer.
        if (strcmp(featFeature->GetFieldAsString(GNM_SYSFIELD_LAYERNAME),
                   GNM_CLASSLAYER_SYSEDGES) == 0)
        {
            DeleteFeature(_poDataSet->GetLayerByName(GNM_CLASSLAYER_SYSEDGES),
                          featFeature->GetFieldAsInteger(GNM_SYSFIELD_FID));
        }

        OGRFeature::DestroyFeature(featFeature);
    }

    return GNMERR_NONE;
}


/************************************************************************/
/*                         ReconnectFeatures()                          */
/************************************************************************/
/**
 * \brief Reconnects features changing the direction of the edge.
 *
 * Changes the direction of the connection (of the graph edge).
 * Does not use rules to determine the direction.
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalNetwork::ReconnectFeatures (GNMGFID nSrcGFID,GNMGFID nTgtGFID,
                                  GNMGFID nConGFID,GNMDirection nNewDir)
{
    OGRLayer* layer = _poDataSet->GetLayerByName(GNM_SYSLAYER_GRAPH);

    // Find connection and delete it from the graph.
    OGRFeature *feature = _findConnection(layer,nSrcGFID,nTgtGFID,nConGFID);
    if (feature == NULL)
    {
        OGRFeature::DestroyFeature(feature);
        //CPLErr
        return GNMERR_FAILURE;
    }

    // Set direction if incorrect.
    if (nNewDir != GNM_DIR_DOUBLE
        && nNewDir != GNM_DIR_SRCTOTGT
        && nNewDir != GNM_DIR_TGTTOSRC)
        nNewDir = GNM_DIR_DOUBLE;

    feature->SetField(GNM_SYSFIELD_DIRECTION, nNewDir);
    layer->SetFeature(feature);
    OGRFeature::DestroyFeature(feature);

    return GNMERR_NONE;
}


/************************************************************************/
/*                         ReconnectFeatures()                          */
/************************************************************************/
/**
 * \brief Reconnects features changing the cost and inverse cost of the edge.
 *
 * Changes the costs of the connection (of the graph edge).
 * Does not use rules to determine the costs.
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalNetwork::ReconnectFeatures (GNMGFID nSrcGFID,GNMGFID nTgtGFID,
                                  GNMGFID nConGFID,double dNewCost,double dNewInvCost)
{
    OGRLayer* layer = _poDataSet->GetLayerByName(GNM_SYSLAYER_GRAPH);

    // Find connection and delete it from the graph.
    OGRFeature *feature = _findConnection(layer,nSrcGFID,nTgtGFID,nConGFID);
    if (feature == NULL)
    {
        OGRFeature::DestroyFeature(feature);
        //CPLErr
        return GNMERR_FAILURE;
    }

    feature->SetField(GNM_SYSFIELD_COST,dNewCost);
    feature->SetField(GNM_SYSFIELD_INVCOST,dNewInvCost);
    layer->SetFeature(feature);
    OGRFeature::DestroyFeature(feature);

    return GNMERR_NONE;
}


/************************************************************************/
/*                     DisconnectAll()                              */
/************************************************************************/
/**
 * \brief Removes all connections from the graph.
 *
 * Simply erases the graph.
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalNetwork::DisconnectAll ()
{
    // ISSUE: What is better? Delete all fields? Delete all features? Delete Layer?
    // For now we use delete layer.

    // Search for graph layer index.
    int cnt = _poDataSet->GetLayerCount();
    for (int i=0; i<cnt; i++)
    {
        OGRLayer *layer = _poDataSet->GetLayer(i);
        if (strcmp(layer->GetName(),GNM_SYSLAYER_GRAPH) == 0)
        {
            // Delete graph layer.
            OGRErr err = _poDataSet->DeleteLayer(i);

            // Create graph layer.
            if (err != OGRERR_NONE)
            {
                 return GNMERR_FAILURE;
            }
            else
            {
                layer = _poDataSet->CreateLayer(GNM_SYSLAYER_GRAPH,NULL,wkbNone,NULL);
                if (layer == NULL)
                {
                    //CPLError
                    return GNMERR_FAILURE;
                }

                OGRFieldDefn newField1(GNM_SYSFIELD_SOURCE, OFTInteger);
                OGRFieldDefn newField2(GNM_SYSFIELD_TARGET, OFTInteger);
                OGRFieldDefn newField3(GNM_SYSFIELD_CONNECTOR, OFTInteger);
                OGRFieldDefn newField4(GNM_SYSFIELD_COST, OFTReal);
                OGRFieldDefn newField5(GNM_SYSFIELD_INVCOST, OFTReal);
                OGRFieldDefn newField6(GNM_SYSFIELD_DIRECTION, OFTInteger);
                if(layer->CreateField(&newField1) != OGRERR_NONE
                   || layer->CreateField(&newField2) != OGRERR_NONE
                   || layer->CreateField(&newField3) != OGRERR_NONE
                   || layer->CreateField(&newField4) != OGRERR_NONE
                   || layer->CreateField(&newField5) != OGRERR_NONE
                   || layer->CreateField(&newField6) != OGRERR_NONE)
                {
                    return GNMERR_FAILURE;
                }

                return GNMERR_NONE;
            }
        }
    }

    return GNMERR_FAILURE;
}


/************************************************************************/
/*                         GetFeatureByGFID ()                          */
/************************************************************************/
/**
 * Returns feature by its GFID. GFIDs are assigned automatically when the
 * feature appears in the network.
 *
 * @since GDAL 2.0
 */
OGRFeature *GNMGdalNetwork::GetFeatureByGFID (GNMGFID nGFID)
{
    // Get FID and layer name.
    OGRLayer *featLayer = _poDataSet->GetLayerByName(GNM_SYSLAYER_FEATURES);
    OGRFeature *featFeature;
    char filter[100];
    sprintf(filter,"%s = %d",GNM_SYSFIELD_GFID,nGFID);
    featLayer->SetAttributeFilter(filter);
    featLayer->ResetReading();
    featFeature = featLayer->GetNextFeature();
    featLayer->SetAttributeFilter(NULL);
    if (featFeature == NULL)
    {
        OGRFeature::DestroyFeature(featFeature);
        return NULL;
    }

    // Get the feature from its layer.
    const char* retLayerName = featFeature->GetFieldAsString(GNM_SYSFIELD_LAYERNAME);
    long retFid = featFeature->GetFieldAsInteger(GNM_SYSFIELD_FID);
    OGRLayer *retLayer = _poDataSet->GetLayerByName(retLayerName);
    OGRFeature *retFeature = retLayer->GetFeature(retFid);

    OGRFeature::DestroyFeature(featFeature);

    return retFeature;
}


/************************************************************************/
/*                            CreateRule ()                             */
/************************************************************************/
/**
 * \brief Creates rule in the network.
 *
 * Creates the rule in the network according to the special syntax. These
 * rules are declarative and make an effect for the network when they exist.
 * Each rule for class layer can be created even if the class does not exist
 * and it is not removed when the class is being deleted.
 *
 * @param pszRuleStr Rule string. See the description of rules in the GNM
 * architecture.
 *
 * @return GNMERR_NONE if ok.
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalNetwork::CreateRule (const char *pszRuleStr)
{
    // Add rule to the system layer.
    OGRLayer *rulesLayer = _poDataSet->GetLayerByName(GNM_SYSLAYER_RULES);
    OGRFeature *ruleFeature = OGRFeature::CreateFeature(rulesLayer->GetLayerDefn());
    ruleFeature->SetField(GNM_SYSFIELD_RULESTRING, pszRuleStr);
    if(rulesLayer->CreateFeature(ruleFeature) != OGRERR_NONE)
    {
        return GNMERR_FAILURE;
    }
    long fid = ruleFeature->GetFID();
    OGRFeature::DestroyFeature(ruleFeature);

    // Build rule.
    GNMErr err = _parseRuleString(pszRuleStr);
    if (err != GNMERR_NONE)
    {
        rulesLayer->DeleteFeature(fid);
        return err;
    }

    return GNMERR_NONE;
}


/************************************************************************/
/*                            ClearRules ()                             */
/************************************************************************/
/**
 * Simply delete all rules from the network.
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalNetwork::ClearRules ()
{
    // Search for rules layer index.
    int cnt = _poDataSet->GetLayerCount();
    for (int i=0; i<cnt; i++)
    {
        OGRLayer *layer = _poDataSet->GetLayer(i);
        if (strcmp(layer->GetName(),GNM_SYSLAYER_RULES) == 0)
        {
            // Delete rules layer.
            OGRErr err = _poDataSet->DeleteLayer(i);

            // Create rules layer.
            if (err != OGRERR_NONE)
            {
                 return GNMERR_FAILURE;
            }
            else
            {
                layer = _poDataSet->CreateLayer(GNM_SYSLAYER_RULES,NULL,wkbNone,NULL);
                if (layer == NULL)
                {
                    //CPLError
                    return GNMERR_FAILURE;
                }

                OGRFieldDefn newField1(GNM_SYSFIELD_RULESTRING, OFTString);
                if(layer->CreateField(&newField1) != OGRERR_NONE)
                {
                    return GNMERR_FAILURE;
                }
            }
        }
    }

    // Clear rules map.
    _classRules.clear();

    return GNMERR_NONE;
}


/************************************************************************/
/*                          GetDriverName()                             */
/************************************************************************/
/**
 * Returns the name of gdaldataset driver (e.g. ESRI Shapefile) which
 * is internal for the current GNMGdalNetwork format. This string must not be
 * freed outside.
 *
 * @since GDAL 2.0
 */
//const char *GNMGdalNetwork::GetDriverName ()
//{
//    return _driverName;
//}


/************************************************************************/
/*                         IsDriverSupported()                          */
/************************************************************************/
/**
 * Returns true if the given dataset's driver is supported by GDAL-
 * networks format.
 *
 * @since GDAL 2.0
 */
bool GNMGdalNetwork::IsDriverSupported (GDALDataset *poDS)
{
    // Check for the supported vector formats.
    const char *drName = poDS->GetDriverName();
    int i = 0;
    while(TRUE)
    {
        if (GNMGdalSupportedDrivers[i] == NULL)
            return false;
        else if (strcmp(drName, GNMGdalSupportedDrivers[i]) == 0)
            return true;
        i++;
    }
}


/************************************************************************/
/*                      IsDatasetFormatSupported()                      */
/************************************************************************/
/**
 * Returns the string list of supported GDAL drivers for GDAL-network format.
 * This list must be freed with CSLDestroy().
 *
 * @since GDAL 2.0
 */
char** GNMGdalNetwork::GetSupportedDrivers ()
{
    int i = 0;
    char **ret = NULL;
    while(TRUE)
    {
        if (GNMGdalSupportedDrivers[i] == NULL)
        {
            break;
        }
        else
        {
            ret = CSLAddString(ret,GNMGdalSupportedDrivers[i]);
        }
        i++;
    }
    return ret;
}


/************************************************************************/
/*                       GetSpatialReference()                          */
/************************************************************************/
/**
* Returns spatial reference of GNMGdalNetwork.
*
* @since GDAL 2.0
*/
const OGRSpatialReference* GNMGdalNetwork::GetSpatialReference() const
{
    return _poSpatialRef;
}


/************************************************************************/
/*                        GetMetaParamValues()                          */
/************************************************************************/
/**
 * Returns all metaparams as a CSL Name-Value array of pairs. The parameter
 * names see in the gnm.h defines.
 * NOTE: Use CSLDestroy to delete returned string list.
 *
 * @since GDAL 2.0
 */
char **GNMGdalNetwork::GetMetaParamValues ()
{
    char **ret = NULL;

    OGRLayer *layer = _poDataSet->GetLayerByName(GNM_SYSLAYER_META);
    OGRFeature *feature;
    layer->ResetReading();
    while ((feature = layer->GetNextFeature()) != NULL)
    {
        ret = CSLAddNameValue(ret,
                        feature->GetFieldAsString(GNM_SYSFIELD_PARAMNAME),
                        feature->GetFieldAsString(GNM_SYSFIELD_PARAMVALUE));
        OGRFeature::DestroyFeature(feature);
    }

    return ret;
}


/************************************************************************/
/*                        GetMetaParamValueName()                       */
/************************************************************************/
/**
 * Returns network's name.
 * NOTE: The returned string must be freed by caller via delete [].
 *
 * @since GDAL 2.0
 */
const char *GNMGdalNetwork::GetMetaParamValueName ()
{
    OGRLayer *layer = _poDataSet->GetLayerByName(GNM_SYSLAYER_META);
    OGRFeature *feature = layer->GetFeature(_meta_name_i);
    char *ret = new char [254];
    strcpy(ret, feature->GetFieldAsString(GNM_SYSFIELD_PARAMVALUE));
    OGRFeature::DestroyFeature(feature);
    return ret;
}


/************************************************************************/
/*                        GetMetaParamValueDescr()                      */
/************************************************************************/
/**
 * Returns network's description.
 * The returned string must be freed by caller via delete [].
 *
 * @since GDAL 2.0
 */
const char *GNMGdalNetwork::GetMetaParamValueDescr ()
{
    OGRLayer *layer = _poDataSet->GetLayerByName(GNM_SYSLAYER_META);
    OGRFeature *feature = layer->GetFeature(_meta_descr_i);
    if (feature == NULL)
        return NULL;
    const char *str = feature->GetFieldAsString(GNM_SYSFIELD_PARAMVALUE);
    if (strcmp(str,"") == 0)
        return NULL;
    char *ret = new char [254];
    strcpy(ret,str);
    OGRFeature::DestroyFeature(feature);
    return ret;
}


/************************************************************************/
/*                          GetClassLayes()                             */
/************************************************************************/
/**
 * Returns all layer names which are not system and their count.
 * NOTE: Use CSLDestroy to delete returned string list (std::pair::first).
 *
 * @return std::pair object. If there are no class layers in the connectivity
 * the std::pair::second will equal 0.
 *
 * @since GDAL 2.0
 */
std::pair<char**,int> GNMGdalNetwork::GetClassLayers ()
{
    char **ret = NULL;
    int count = 0;

    OGRLayer *layer = _poDataSet->GetLayerByName(GNM_SYSLAYER_CLASSES);
    OGRFeature *feature;
    layer->ResetReading();
    while ((feature = layer->GetNextFeature()) != NULL)
    {
        count++;
        ret = CSLAddString(ret,feature->GetFieldAsString(GNM_SYSFIELD_LAYERNAME));
        OGRFeature::DestroyFeature(feature);
    }

    std::pair<char **, int> pair;
    pair = std::make_pair(ret,count);

    return pair;
}


/************************************************************************/
/*                          GetClassRole()                              */
/************************************************************************/
/**
 * Returns the role of the class layer which is described by rules.
 *
 * @since GDAL 2.0
 */
std::string GNMGdalNetwork::GetClassRole (const char *pszLayerName)
{
    std::string retStr = "";
    GNMClassRule *rule = _findRuleForClass(pszLayerName);
    if (rule != NULL)
        retStr = rule->roleStr;
    return retStr;
}


/* ==================================================================== */
/*                        C API implementation                          */
/* ==================================================================== */


//GNMConnection *GNMGdalGetConnection (GNMNetworkH hNet,long nConFID)
/************************************************************************/
/*                        GetConnection()                               */
/************************************************************************/
//* \brief Returns the instantly formed GNMConnection object by its FID from
//* system graph layer or NULL if failed to form. Must be freed by caller.
/**
 * @see GNMGdalNetwork::GetConnection().
 *
 * @since GDAL 2.0
 */
OGRFeatureH GNMGdalGetConnection (GNMGdalNetworkH hNet,long nConFID)
{
    VALIDATE_POINTER1( hNet, "GNMGdalGetConnection", NULL );

    return (OGRFeatureH)((GNMGdalNetwork*)hNet)->GetConnection(nConFID);
}


/************************************************************************/
/*                        GNMGdalCreateLayer()                          */
/************************************************************************/
/**
 * @see GNMGdalNetwork::CreateLayer().
 *
 * @since GDAL 2.0
 */
OGRLayerH GNMGdalCreateLayer (GNMGdalNetworkH hNet,const char *pszName,
                                  OGRFeatureDefnH hFeatureDefn,
                                      OGRSpatialReferenceH hSpatialRef,
                                  OGRwkbGeometryType eGType,char **papszOptions)
{
    VALIDATE_POINTER1( hNet, "GNMGdalCreateLayer", NULL );

    return (OGRLayerH)
            ((GNMGdalNetwork*)hNet)->CreateLayer(pszName,(OGRFeatureDefn*)hFeatureDefn,
                                             (OGRSpatialReference*)hSpatialRef,
                                             eGType,papszOptions);
}


/************************************************************************/
/*                        GNMGdalCopyLayer()                            */
/************************************************************************/
/**
 * @see GNMGdalNetwork::CopyLayer().
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalCopyLayer (GNMGdalNetworkH hNet,OGRLayerH hLayer,const char *pszNewName)
{
    VALIDATE_POINTER1( hNet, "GNMGdalCopyLayer", GNMERR_FAILURE );

    return ((GNMGdalNetwork*)hNet)->CopyLayer((OGRLayer*)hLayer,pszNewName);
}


/************************************************************************/
/*                        GNMGdalCreateFeature()                        */
/************************************************************************/
/**
 * @see GNMGdalNetwork::CreateFeature().
 *
 * @since GDAL 2.0
 */
GNMGFID GNMGdalCreateFeature (GNMGdalNetworkH hNet,OGRLayerH hLayer,
                                      OGRFeatureH hFeature)
{
    VALIDATE_POINTER1( hNet, "GNMGdalCreateFeature", -1 );

    return ((GNMGdalNetwork*)hNet)->CreateFeature((OGRLayer*)hLayer,
                                                  (OGRFeature*)hFeature);
}


/************************************************************************/
/*                        GNMGdalSetFeature()                           */
/************************************************************************/
/**
 * @see GNMGdalNetwork::SetFeature().
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalSetFeature (GNMGdalNetworkH hNet,OGRLayerH hLayer,
                              OGRFeatureH hFeature)
{
    VALIDATE_POINTER1( hNet, "GNMGdalSetFeature", GNMERR_FAILURE );

    return ((GNMGdalNetwork*)hNet)->SetFeature((OGRLayer*)hLayer,
                                               (OGRFeature*)hFeature);
}


/************************************************************************/
/*                        GNMGdalDeleteFeature()                        */
/************************************************************************/
/**
 * @see GNMGdalNetwork::DeleteFeature ().
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalDeleteFeature (GNMGdalNetworkH hNet,OGRLayerH hLayer,long nFID)
{
    VALIDATE_POINTER1( hNet, "GNMGdalDeleteFeature", GNMERR_FAILURE );

    return ((GNMGdalNetwork*)hNet)->DeleteFeature((OGRLayer*)hLayer,nFID);
}


/************************************************************************/
/*                        GNMGdalConnectFeatures()                      */
/************************************************************************/
/**
 * @see GNMGdalNetwork::ConnectFeatures().
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalConnectFeatures (GNMGdalNetworkH hNet,GNMGFID nSrcGFID,
                                    GNMGFID nTgtGFID,GNMGFID nConGFID,
                                    double dCost,double dInvCost,
                                    GNMDirection nDir)
{
    VALIDATE_POINTER1( hNet, "GNMGdalConnectFeatures", GNMERR_FAILURE );

    return ((GNMGdalNetwork*)hNet)->ConnectFeatures(nSrcGFID,nTgtGFID,nConGFID,
                                                dCost,dInvCost,nDir);
}


/************************************************************************/
/*                        GNMGdalDisconnectFeatures()                   */
/************************************************************************/
/**
 * @see GNMGdalNetwork::DisconnectFeatures ().
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalDisconnectFeatures (GNMGdalNetworkH hNet, GNMGFID nSrcGFID,
                                       GNMGFID nTgtGFID,GNMGFID nConGFID)
{
    VALIDATE_POINTER1( hNet, "GNMGdalDisconnectFeatures", GNMERR_FAILURE );

    return ((GNMGdalNetwork*)hNet)->DisconnectFeatures(nSrcGFID,nTgtGFID,nConGFID);
}


/************************************************************************/
/*              GNMGdalReconnectDirFeatures()                           */
/************************************************************************/
/**
 * @see GNMGdalNetwork::ReconnectFeatures().
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalReconnectDirFeatures (GNMGdalNetworkH hNet,GNMGFID nSrcGFID,
                                      GNMGFID nTgtGFID,GNMGFID nConGFID,
                                      GNMDirection nNewDir)
{
    VALIDATE_POINTER1( hNet, "GNMGdalReconnectDirFeatures", GNMERR_FAILURE );

    return ((GNMGdalNetwork*)hNet)->ReconnectFeatures(nSrcGFID,nTgtGFID,nConGFID,
                                                nNewDir);
}


/************************************************************************/
/*                   GNMGdalReconnectCostFeatures()                     */
/************************************************************************/
/**
 * @see GNMGdalNetwork::ReconnectFeatures().
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalReconnectCostFeatures (GNMGdalNetworkH hNet,GNMGFID nSrcGFID,
                                      GNMGFID nTgtGFID,GNMGFID nConGFID,
                                      double dNewCost,double dNewInvCost)
{
    VALIDATE_POINTER1( hNet, "GNMGdalReconnectCostFeatures", GNMERR_FAILURE );

    return ((GNMGdalNetwork*)hNet)->ReconnectFeatures(nSrcGFID,nTgtGFID,nConGFID,
                                                dNewCost,dNewInvCost);
}


/************************************************************************/
/*                        GNMGdalDisconnectAll()                        */
/************************************************************************/
/**
 * @see GNMGdalNetwork::DisconnectAll().
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalDisconnectAll (GNMGdalNetworkH hNet)
{
    VALIDATE_POINTER1( hNet, "GNMGdalDisconnectAll", GNMERR_FAILURE );

    return ((GNMGdalNetwork*)hNet)->DisconnectAll();
}


/************************************************************************/
/*                        GNMGdalIsDriverSupported()                    */
/************************************************************************/
/**
 * @see GNMGdalNetwork::IsDriverSupported().
 *
 * @since GDAL 2.0
 */
bool GNMGdalIsDriverSupported (GDALDatasetH hDS)
{
    VALIDATE_POINTER1( hDS, "GNMGdalIsDriverSupported", false );

    return GNMGdalNetwork::IsDriverSupported((GDALDataset*)hDS);
}


/************************************************************************/
/*                        GNMGdalGetSupportedDrivers()                  */
/************************************************************************/
/**
 * @see GNMGdalNetwork::GetSupportedDrivers().
 *
 * @since GDAL 2.0
 */
char **GNMGdalGetSupportedDrivers ()
{
    return GNMGdalNetwork::GetSupportedDrivers();
}


/************************************************************************/
/*                        GNMGdalGetSpatialReference()                  */
/************************************************************************/
/**
 * @see GNMGdalNetwork::GetSpatialReference().
 *
 * @since GDAL 2.0
 */
const OGRSpatialReferenceH GNMGdalGetSpatialReference (GNMGdalNetworkH hNet)
{
    VALIDATE_POINTER1( hNet, "GNMGdalGetSpatialReference", NULL );

    return (OGRSpatialReferenceH)((GNMGdalNetwork*)hNet)->GetSpatialReference();
}


/************************************************************************/
/*                         GNMGdalGetMetaParamValues()                  */
/************************************************************************/
/**
 * @see GNMGdalNetwork::GetMetaParamValues ().
 *
 * @since GDAL 2.0
 */
char **GNMGdalGetMetaParamValues (GNMGdalNetworkH hNet)
{
    VALIDATE_POINTER1( hNet, "GNMGdalGetMetaParamValues", NULL );

    return ((GNMGdalNetwork*)hNet)->GetMetaParamValues();
}


/************************************************************************/
/*                    GNMGdalGetMetaParamValueName()                    */
/************************************************************************/
/**
 * @see GNMGdalNetwork::GetMetaParamValueName ().
 *
 * @since GDAL 2.0
 */
const char *GNMGdalGetMetaParamValueName (GNMGdalNetworkH hNet)
{
    VALIDATE_POINTER1( hNet, "GNMGdalGetMetaParamValueName", NULL );

    return ((GNMGdalNetwork*)hNet)->GetMetaParamValueName();
}


/************************************************************************/
/*                   GNMGdalGetMetaParamValueDescr()                    */
/************************************************************************/
/**
 * @see GNMGdalNetwork::GetMetaParamValueDescr().
 *
 * @since GDAL 2.0
 */
const char *GNMGdalGetMetaParamValueDescr (GNMGdalNetworkH hNet)
{
    VALIDATE_POINTER1( hNet, "GNMGdalGetMetaParamValueDescr", NULL );

    return ((GNMGdalNetwork*)hNet)->GetMetaParamValueDescr();
}


/************************************************************************/
/*                        GNMGdalCreateRule()                           */
/************************************************************************/
/**
 * @see GNMGdalNetwork::CreateRule().
 *
 * @since GDAL 2.0
 */
GNMErr GNMGdalCreateRule (GNMGdalNetworkH hNet, const char *pszRuleStr)
{
    VALIDATE_POINTER1( hNet, "GNMGdalCreateRule", GNMERR_FAILURE );

    return ((GNMGdalNetwork*)hNet)->CreateRule(pszRuleStr);
}


//GNMErr CPL_DLL GNMGdalAutoConnect (GNMGdalNetworkH hNet, OGRLayerH *hLayers, double dTolerance)
//{
//	VALIDATE_POINTER1( hNet, "GNMGdalAutoConnect", GNMERR_FAILURE );
//
//	return ((GNMGdalNetwork*)hNet)->AutoConnect((OGRLayer**)hLayers, dTolerance, NULL);
//}
