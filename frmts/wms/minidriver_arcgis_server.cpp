/******************************************************************************
 * $Id$
 *
 * Project:  Arc GIS Server Client Driver
 * Purpose:  Implementation of Dataset and RasterBand classes for WMS
 *           and other similar services.
 * Author:   Alexander Lisovenko
 *
 ******************************************************************************
 * Copyright (c) 2014 NextGIS <info@nextgis.ru>
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

#include "wmsdriver.h"
#include "minidriver_arcgis_server.h"

CPP_GDALWMSMiniDriverFactory(AGS)

GDALWMSMiniDriver_AGS::GDALWMSMiniDriver_AGS()
{
}

GDALWMSMiniDriver_AGS::~GDALWMSMiniDriver_AGS()
{
}

CPLErr GDALWMSMiniDriver_AGS::Initialize(CPLXMLNode *config)
{
	CPLErr ret = CE_None;

	CPLString xmlConfigContent(CPLSerializeXMLTree(config));

    if (ret == CE_None) {
        const char *base_url = CPLGetXMLValue(config, "ServerURL", "");
        if (base_url[0] != '\0') {
            /* Try the old name */
            base_url = CPLGetXMLValue(config, "ServerUrl", "");
        }
        if (base_url[0] != '\0') {
            m_base_url = base_url;
        } else {
            CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS, ArcGIS Server mini-driver: ServerURL missing.");
            ret = CE_Failure;
        }
    }

	if (ret == CE_None) {
        m_image_format = CPLGetXMLValue(config, "ImageFormat", "png");
        m_transparent = CPLGetXMLValue(config, "Transparent","");
		// the transparent flag needs to be "true" or "false" in lower case according to the ArcGIS Server REST API
        for(int i=0; i<(int)m_transparent.size();i++)
        {
			m_transparent[i] = (char) tolower(m_transparent[i]);
        }

		/*
		m_layers = CPLGetXMLValue(config, "Layers", "");
        m_styles = CPLGetXMLValue(config, "Styles", "");
		*/
    }
	if (ret == CE_None) 
	{
		m_irs = CPLGetXMLValue(config, "ImageSR", "102100");
	}

	if (ret == CE_None)
	{
        const char *bbox_order = CPLGetXMLValue(config, "BBoxOrder", "xyXY");
        if (bbox_order[0] != '\0') {
            int i;
            for (i = 0; i < 4; ++i) {
                if ((bbox_order[i] != 'x') && (bbox_order[i] != 'y') && (bbox_order[i] != 'X') && (bbox_order[i] != 'Y')) break;
            }
            if (i == 4) {
                m_bbox_order = bbox_order;
            } else {
                CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS, ArcGIS Server mini-driver: Incorrect BBoxOrder.");
                ret = CE_Failure;
            }
        } else {
            CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS, ArcGIS Server mini-driver: BBoxOrder missing.");
            ret = CE_Failure;
        }
    }
	
    return ret;
}

void GDALWMSMiniDriver_AGS::GetCapabilities(GDALWMSMiniDriverCapabilities *caps) {
    caps->m_capabilities_version = 1;
    caps->m_has_arb_overviews = 1;
    caps->m_has_image_request = 1;
    caps->m_has_tiled_image_requeset = 1;
    caps->m_max_overview_count = 32;
}

void GDALWMSMiniDriver_AGS::ImageRequest(CPLString *url, const GDALWMSImageRequestInfo &iri) {
    *url = m_base_url;

    if (m_base_url.ifind( "/export?") == std::string::npos)
        URLAppend(url, "/export?");

	URLAppendF(url, "&dpi=96");
	URLAppendF(url, "&transparent=%s", m_transparent.c_str());
	URLAppendF(url, "&format=%s", m_image_format.c_str());
	URLAppendF(url, "&bbox=%.8f,%.8f,%.8f,%.8f", 
		GetBBoxCoord(iri, m_bbox_order[0]), GetBBoxCoord(iri, m_bbox_order[1]), 
        GetBBoxCoord(iri, m_bbox_order[2]), GetBBoxCoord(iri, m_bbox_order[3]));
	URLAppendF(url, "&bboxSR=%s", m_irs.c_str());
	URLAppendF(url, "&imageSR=%s", m_irs.c_str());
	URLAppendF(url, "&size=%d,%d", iri.m_sx,iri.m_sy);
	URLAppendF(url, "&f=image");

    CPLDebug("AGS", "URL = %s", url->c_str());
}

void GDALWMSMiniDriver_AGS::TiledImageRequest(CPLString *url, const GDALWMSImageRequestInfo &iri, const GDALWMSTiledImageRequestInfo &tiri) {
	ImageRequest(url, iri);
}


void GDALWMSMiniDriver_AGS::GetTiledImageInfo(CPLString *url,
                                              const GDALWMSImageRequestInfo &iri,
                                              const GDALWMSTiledImageRequestInfo &tiri,
                                              int nXInBlock,
                                              int nYInBlock)
{
    *url = m_base_url;

    if (m_base_url.ifind( "/identify?") == std::string::npos)
        URLAppend(url, "/identify?");
	
	printf(">>> nXInBlock: %d\n", nXInBlock);
	printf(">>> nYInBlock: %d\n", nYInBlock);
	printf(">>> iri.m_sx: %d\n", iri.m_sx);
	printf(">>> iri.m_sy: %d\n", iri.m_sy);

	printf(">>> iri.m_x0: %f\n", iri.m_x0);
	printf(">>> iri.m_y0: %f\n", iri.m_y0);
	printf(">>> iri.m_x1: %f\n", iri.m_x1);
	printf(">>> iri.m_y1: %f\n", iri.m_y1);

	double fX = GetBBoxCoord(iri, 'x') + nXInBlock * (GetBBoxCoord(iri, 'X') - GetBBoxCoord(iri, 'x')) / iri.m_sx;
	double fY = GetBBoxCoord(iri, 'y') + (iri.m_sy - nYInBlock) * (GetBBoxCoord(iri, 'Y') - GetBBoxCoord(iri, 'y')) / iri.m_sy;

	printf(">>> fX: %f\n", fX);
	printf(">>> fY: %f\n", fY);

    URLAppendF(url, "&geometry=%f,%f", fX, fY);
	URLAppendF(url, "&geometryType=esriGeometryPoint");
	URLAppendF(url, "&tolerance=2");
	URLAppendF(url, "&imageDisplay=%d,%d,96", iri.m_sx,iri.m_sy);
	URLAppendF(url, "&mapExtent=%.8f,%.8f,%.8f,%.8f", 
		GetBBoxCoord(iri, m_bbox_order[0]), GetBBoxCoord(iri, m_bbox_order[1]), 
        GetBBoxCoord(iri, m_bbox_order[2]), GetBBoxCoord(iri, m_bbox_order[3]));
	URLAppendF(url, "&returnGeometry=true");
	URLAppendF(url, "&returnZ=false");
	URLAppendF(url, "&returnM=false");
    URLAppendF(url, "&f=json");

    CPLDebug("AGS", "URL = %s", url->c_str());
}


const char *GDALWMSMiniDriver_AGS::GetProjectionInWKT() {
    return m_projection_wkt.c_str();
}

double GDALWMSMiniDriver_AGS::GetBBoxCoord(const GDALWMSImageRequestInfo &iri, char what) {
    switch (what) {
        case 'x': return MIN(iri.m_x0, iri.m_x1);
        case 'y': return MIN(iri.m_y0, iri.m_y1);
        case 'X': return MAX(iri.m_x0, iri.m_x1);
        case 'Y': return MAX(iri.m_y0, iri.m_y1);
    }
    return 0.0;
}