/******************************************************************************
 * $Id$
 *
 * Name:     gnm.h
 * Project:  GDAL/OGR Geography Network support (Geographic Network Model)
 * Purpose:  GNM general declarations.
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

#ifndef _GNM_H_INCLUDED
#define _GNM_H_INCLUDED

#include "ogrsf_frmts.h"

#include <set>

// Supported Dataset formats for GDAL-networks.
// FORMAT NOTE: the GDAL network will not be created for those formats,
// which does not appear in this list, because there are some format-specific
// issues which can cause an incorrect work of network.
static const char *GNMGdalSupportedDrivers[] = {
    "ESRI Shapefile",
    "PostGIS", "PostgreSQL",
    NULL };

// General constants.
#define GNM_VERSION "1.0.0"

// Error codes.
typedef int GNMErr;
#define GNMERR_NONE 0
#define GNMERR_FAILURE 1
#define GNMERR_UNSUPPORTED 2
#define GNMERR_NOTFOUND 3
#define GNMERR_CONRULE_RESTRICT 4

// Obligatory system layers.
#define GNM_SYSLAYER_META "_gnm_meta"
#define GNM_SYSLAYER_GRAPH "_gnm_graph"
#define GNM_SYSLAYER_CLASSES "_gnm_classes"
#define GNM_SYSLAYER_FEATURES "_gnm_features"
#define GNM_SYSLAYER_RULES "_gnm_rules"
// FORMAT NOTE: Shape dataset regards all layers in the directory as the layers
// of the dataset, even thouse which were added to the directory manually and
// were not provided with according system fields. So the _gnm_classes serves
// also to register those layers, which are "under" the network control.

// Obligatory class layers.
#define GNM_CLASSLAYER_SYSEDGES "gnm_sysedges"

// Names of options in the pair name & value, which can be passed
// when the network is being created / opened.
#define GNM_INIT_OPTIONPAIR_NAME "net_name"
#define GNM_INIT_OPTIONPAIR_DESCR "net_descr"
#define GNM_INIT_OPTIONPAIR_SRS "net_srs"

// Network's metadata parameters names.
#define GNM_METAPARAM_VERSION "gnm_version"
#define GNM_METAPARAM_SRS "common_srs"
#define GNM_METAPARAM_GFIDCNT "gfid_counter" // the current is that we should assign
#define GNM_METAPARAM_NAME "net_name"
#define GNM_METAPARAM_DESCR "net_descr" // network description

// System field names.
// FORMAT NOTE: Shapefile driver does not support field names more than 10 characters.
#define GNM_SYSFIELD_PARAMNAME "param_name"
#define GNM_SYSFIELD_PARAMVALUE "param_val"
#define GNM_SYSFIELD_SOURCE "source"
#define GNM_SYSFIELD_TARGET "target"
#define GNM_SYSFIELD_CONNECTOR "connector"
#define GNM_SYSFIELD_COST "cost"
#define GNM_SYSFIELD_INVCOST "inv_cost"
#define GNM_SYSFIELD_DIRECTION "direction"
#define GNM_SYSFIELD_GFID "gfid" // TODO: _gnm_gfid
#define GNM_SYSFIELD_LAYERNAME "layer_name"
#define GNM_SYSFIELD_FID "fid"
#define GNM_SYSFIELD_RULESTRING "rule_str"

// Rule strings key-words.
#define GNM_KW_NETWORK "NETWORK"
#define GNM_KW_CLASS "CLASS"
#define GNM_KW_CONNECTS "CONNECTS"
#define GNM_KW_WITH "WITH"
#define GNM_KW_VIA "VIA"
#define GNM_KW_COSTS "COSTS"
#define GNM_KW_INVCOSTS "INVCOSTS"
#define GNM_KW_DIRECTS "DIRECTS"
#define GNM_KW_BEHAVES "BEHAVES"
#define GNM_KW_MULT "*"
#define GNM_KW_ADD "+"
#define GNM_KW_SUB "-"
#define GNM_KW_DIV "/"

// Other string constants.
#define GNM_SRSFILENAME "_gnm_srs.prj"

// Types.
typedef int GNMGFID; // Alias for long. Can be replaced with GUIntBig.
                     // NOTE: replace %ld in any sprintf() when the long type
                     // for GFIDs will be used.

typedef int GNMDirection; // Direction of an edge.
#define GNM_DIR_DOUBLE 0 // double-directed
#define GNM_DIR_SRCTOTGT 1 // from source to target
#define GNM_DIR_TGTTOSRC 2 // from target to source

/*
struct GNMConnection // General connection type.
{
    GNMGFID nSrcGFID;
    GNMGFID nTgtGFID;
    GNMGFID nConGFID;
    double dCost;
    double dInvCost;
    GNMDirection dir;
};
*/

// Types for GDAL-network rules.

typedef std::pair<std::string,std::string> _GNMVertEdgeClassNames;
typedef std::set<_GNMVertEdgeClassNames> _GNMLegalCons;

typedef enum
{
    gnmOpUndefined = 0,
    gnmOpField = 1,
    gnmOpConst = 2,
    gnmOpFieldMConst = 3,
    gnmOpFieldAConst = 4,
    gnmOpFieldSConst = 5,
    gnmOpFieldDConst = 6
} GNMRuleOperation;

struct GNMClassRule
{
    std::string dirCostField;
    GNMRuleOperation dirCostOper;
    double dirCostConst;
    std::string invCostField;
    GNMRuleOperation invCostOper;
    double invCostConst;
    std::string dirField;
    std::string roleStr;
    _GNMLegalCons conRules;

    GNMClassRule()
    {
        // TODO: remove unnecessary assignements.
        dirCostField = "";
        dirCostOper = gnmOpUndefined;
        dirCostConst = 0.0;
        invCostField = "";
        invCostOper = gnmOpUndefined;
        invCostConst = 0.0;
        dirField = "";
        roleStr = "";
    }
};

typedef std::map<std::string, GNMClassRule*> GNMClassRuleMap; // Array of class rules.

typedef void *GNMNetworkH;
typedef void *GNMGdalNetworkH;

class GNMNetwork;
class GNMGdalNetwork;


/************************************************************************/
/*                               GNMManager                             */
/************************************************************************/
/**
 * Static class which manages networks of diffirent formats. By default
 * manages the GNMGdalNetwork.
 *
 * @since GDAL 2.0
 */
class CPL_DLL GNMManager
{
    public:

// Public GNM managing methods.

    static GNMNetwork *CreateConnectivity (GDALDataset *poDS,
                                                int bNative = FALSE,
                                                char **papszOptions = NULL);

    static GNMNetwork *Open (GDALDataset *poDS,
                             int bNative = FALSE,
                             char **papszOptions = NULL);

    static void Close (GNMNetwork *poNet,
                       int bNative = FALSE);

    static GNMErr RemoveConnectivity (GDALDataset *poDS,
                                      int bNative = FALSE);

    static bool HasConnectivity (GDALDataset *poDS,
                                 int bNative = FALSE);

// Public methods for managing GDAL-networks directly.

    static GNMGdalNetwork *GdalCreateNetwork (const char *pszName,
                                              const char *pszFormat,
                                              const char *pszSrsInput,
                                              char **papszOptions = NULL,
                                              char **papszDatasetOptions = NULL);

    static GNMGdalNetwork *GdalOpenNetwork (const char *pszName,
                                            char **papszOptions = NULL);

    static void GdalCloseNetwork (GNMGdalNetwork *poNet);

    static GNMErr GdalDeleteNetwork (const char *pszName);
};


/************************************************************************/
/*                            GNMNetwork                                */
/************************************************************************/
/**
 * Abstract interface class for editing/reading networks of different
 * formats.
 *
 * @since GDAL 2.0
 */
class CPL_DLL GNMNetwork: public GDALMajorObject
{
    private:

    bool _isOwnLayer (OGRLayer *poLayer);

    protected:

    //char *_formatName;

    GDALDataset *_poDataSet;
    bool _ownDataset;
    std::set <OGRLayer*> _classesSet; // Do not free these pointers manually.

    public:

    GNMNetwork ();
    ~GNMNetwork ();

// Common methods:

    //const char *GetFormatName ();

    GDALDataset *GetDataset ();

    void FlushCache ();

// Connections reading:

    //virtual GNMConnection *GetConnection (long nConFID) = 0;
    //virtual GNMConnection GetNextConnection () = 0;
    //virtual void ResetConnectionsReading () = 0;
    virtual OGRFeature *GetConnection (long nConFID) = 0;

    // TODO: make not pure virtual.
    virtual OGRFeature *GetNextConnection () = 0;

    // TODO: make not pure virtual.
    virtual void ResetConnectionsReading () = 0;

// Connections editing:

    virtual GNMErr ConnectFeatures (GNMGFID nSrcGFID,
                                    GNMGFID nTgtGFID,
                                    GNMGFID nConGFID,
                                    double dCost,
                                    double dInvCost,
                                    GNMDirection nDir) = 0;

    //virtual GNMErr ConnectFeatures (GNMGFID nSrcGFID,
                                    //GNMGFID nTgtGFID,
                                    //GNMGFID nConGFID) = 0;

    virtual GNMErr DisconnectFeatures (GNMGFID nSrcGFID,
                                       GNMGFID nTgtGFID,
                                       GNMGFID nConGFID) = 0;

    virtual GNMErr ReconnectFeatures (GNMGFID nSrcGFID,
                                      GNMGFID nTgtGFID,
                                      GNMGFID nConGFID,
                                      GNMDirection nNewDir);

    virtual GNMErr ReconnectFeatures (GNMGFID nSrcGFID,
                                      GNMGFID nTgtGFID,
                                      GNMGFID nConGFID,
                                      double dNewCost,
                                      double dNewInvCost);

    //virtual GNMErr AutoConnect (OGRLayer **papoLayers,
                                //double dTolerance,
                                //char **papszParams);
    virtual GNMErr AutoConnect (std::vector<OGRLayer*> vLayers,
                                double dTolerance,
                                char **papszParams);

    virtual GNMErr DisconnectAll ();

// Features reading:

    virtual OGRFeature *GetFeatureByGFID (GNMGFID nGFID) = 0;

// Features editing:

    virtual OGRLayer *CreateLayer (const char *pszName,
                                   OGRFeatureDefn *poFeatureDefn,
                                   OGRSpatialReference *poSpatialRef = NULL,
                                   OGRwkbGeometryType eGType = wkbPoint,
                                   char **papszOptions = NULL);

    virtual GNMErr DeleteLayer (const char *pszName);

    virtual GNMErr CopyLayer (OGRLayer *poSrcLayer,
                              const char *pszNewName);

    virtual GNMGFID CreateFeature (OGRLayer *poLayer,
                                   OGRFeature *poFeature);

    virtual GNMErr SetFeature (OGRLayer *poLayer,
                               OGRFeature *poFeature);

    virtual GNMErr DeleteFeature (OGRLayer *poLayer,
                                  long nFID);

// Rules editing:

    virtual GNMErr CreateRule (const char *pszRuleStr);
};


/************************************************************************/
/*                         GNMGdalNetwork                               */
/************************************************************************/
/**
 * General GDAL class for editing/reading networks. The supported dataset
 * drivers can be listed by GetSupportedDrivers() static method.
 * This class is used by default in any network operations when the concrete
 * network format is undefined.
 *
 * NOTE: The single and common spatial reference sytem is used for all layers
 * which are "under control" of the network. Each feature appears in the network
 * will be transformed to this SRS.
 *
 * @since GDAL 2.0
 */
class CPL_DLL GNMGdalNetwork: public GNMNetwork
{ 
    friend class GNMManager;

    private:

    //char *_driverName;

    protected:

    long _meta_vrsn_i;
    long _meta_srs_i;
    long _meta_gfidcntr_i;
    long _meta_name_i;
    long _meta_descr_i;

    OGRSpatialReference *_poSpatialRef;

    GNMClassRuleMap _classRules;

    bool _isOwnLayer (OGRLayer *poLayer);

    OGRFeature *_findConnection (OGRLayer *graph,
                                 GNMGFID src,
                                 GNMGFID tgt,
                                 GNMGFID con);

    bool _isClassLayer (const char *layerName);

    char *_makeNewLayerName (const char *name,
                             OGRwkbGeometryType geotype);

    GNMErr _parseRuleString (const char *ruleStr);

    GNMClassRule *_findRuleForClass (std::string clStr);

    public:

    GNMGdalNetwork();
    ~GNMGdalNetwork();

// Implemented interface:

    //virtual GNMConnection *GetConnection (long nConFID);
    virtual OGRFeature *GetConnection (long nConFID);

    virtual OGRFeature *GetNextConnection ();

    virtual void ResetConnectionsReading ();

    virtual OGRLayer *CreateLayer (const char *pszName,
                                   OGRFeatureDefn *poFeatureDefn,
                                   OGRSpatialReference *poSpatialRef = NULL,
                                   OGRwkbGeometryType eGType = wkbPoint,
                                   char **papszOptions = NULL);

    virtual GNMErr DeleteLayer (const char *pszName);

    virtual GNMErr CopyLayer (OGRLayer *poSrcLayer,
                              const char *pszNewName);

    virtual GNMGFID CreateFeature (OGRLayer *poLayer,
                                   OGRFeature *poFeature);

    virtual GNMErr SetFeature (OGRLayer *poLayer,
                               OGRFeature *poFeature);

    virtual GNMErr DeleteFeature (OGRLayer *poLayer,
                                  long nFID);

    virtual GNMErr ConnectFeatures (GNMGFID nSrcGFID,
                                    GNMGFID nTgtGFID,
                                    GNMGFID nConGFID,
                                    double dCost,
                                    double dInvCost,
                                    GNMDirection nDir);

    virtual GNMErr DisconnectFeatures (GNMGFID nSrcGFID,
                                       GNMGFID nTgtGFID,
                                       GNMGFID nConGFID);

    virtual GNMErr ReconnectFeatures (GNMGFID nSrcGFID,
                                      GNMGFID nTgtGFID,
                                      GNMGFID nConGFID,
                                      GNMDirection nNewDir);

    virtual GNMErr ReconnectFeatures (GNMGFID nSrcGFID,
                                      GNMGFID nTgtGFID,
                                      GNMGFID nConGFID,
                                      double dNewCost,
                                      double dNewInvCost);

    virtual GNMErr DisconnectAll ();

    virtual OGRFeature *GetFeatureByGFID (GNMGFID nGFID);

    virtual GNMErr CreateRule (const char *pszRuleStr);

// Specific methods:

    //const char *GetDriverName ();

    static bool IsDriverSupported (GDALDataset *poDS);

    static char** GetSupportedDrivers ();

    const OGRSpatialReference* GetSpatialReference() const;

    char **GetMetaParamValues ();

    const char *GetMetaParamValueName ();

    const char *GetMetaParamValueDescr ();

    std::pair<char**,int> GetClassLayers ();

    GNMErr ClearRules ();

    std::string GetClassRole (const char *pszLayerName);
};



#endif // _GNM_H_INCLUDED
