/******************************************************************************
 * $Id$
 *
 * Project:  RSMDReader - Remote Sensing MetaData Reader
 * Purpose:  Read remote sensing metadata from files from different providers like as DigitalGlobe, GeoEye et al.
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

#include "rsmd_reader.h"

#include "reader_digital_globe.h"
#include "reader_orb_view.h"
#include "reader_pleiades.h"
#include "reader_geo_eye.h"
#include "reader_kompsat.h"
#include "reader_rdk1.h"
#include "reader_spot.h"
#include "reader_alos.h"

RSMDReader* GetRSMDReader(const char* pszFilename, RSMDProvider rsdProvider)
{
	if(rsdProvider == RSMD_DigitalGlobe)
		return new DigitalGlobe(pszFilename);

	return NULL;
};

RSMDReader* GetRSMDReader(const CPLString pszFilename)
{
	if(DigitalGlobe(pszFilename.c_str()).IsFullCompliense())
		return new DigitalGlobe(pszFilename.c_str());
    if(OrbView(pszFilename.c_str()).IsFullCompliense())
		return new OrbView(pszFilename.c_str());
	if(Pleiades(pszFilename.c_str()).IsFullCompliense())
		return new Pleiades(pszFilename.c_str());
	if(GeoEye(pszFilename.c_str()).IsFullCompliense())
		return new GeoEye(pszFilename.c_str());
	if(Kompsat(pszFilename.c_str()).IsFullCompliense())
		return new Kompsat(pszFilename.c_str());
	if(RDK1(pszFilename.c_str()).IsFullCompliense())
		return new RDK1(pszFilename.c_str());
	if(Spot(pszFilename.c_str()).IsFullCompliense())
		return new Spot(pszFilename.c_str());
	if(ALOS(pszFilename.c_str()).IsFullCompliense())
		return new ALOS(pszFilename.c_str());
	
	return NULL;
};