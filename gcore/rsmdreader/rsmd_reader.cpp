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
#include "reader_eros.h"
#include "reader_formosat.h"
#include "reader_landsat.h"
#include "reader_rapid_eye.h"

RSMDReader* GetRSMDReader(const char* pszFilename, RSMDProvider rsdProvider)
{
	if(rsdProvider == RSMD_DigitalGlobe)
		return new DigitalGlobe(pszFilename);
    else if(rsdProvider == RSMD_OrbView)
        return new OrbView(pszFilename);
    else if(rsdProvider == RSMD_Pleiades)
        return new Pleiades(pszFilename);
    else if(rsdProvider == RSMD_GeoEye)
        return new GeoEye(pszFilename);
    else if(rsdProvider == RSMD_Kompsat)
        return new Kompsat(pszFilename);
    else if(rsdProvider == RSMD_RDK1)
        return new RDK1(pszFilename);
    else if(rsdProvider == RSMD_Spot)
        return new Spot(pszFilename);
    else if(rsdProvider == RSMD_ALOS)
        return new ALOS(pszFilename);
    else if(rsdProvider == RSMD_EROS)
        return new EROS(pszFilename);
    else if(rsdProvider == RSMD_Formosat)
        return new Formosat(pszFilename);
    else if(rsdProvider == RSMD_Landsat)
        return new Landsat(pszFilename);
    else if(rsdProvider == RSMD_RapidEye)
        return new RapidEye(pszFilename);

	return NULL;
};

RSMDReader* GetRSMDReader(const CPLString &pszFilename)
{
	if(DigitalGlobe(pszFilename).IsFullCompliense())
		return new DigitalGlobe(pszFilename);
    if(OrbView(pszFilename).IsFullCompliense())
		return new OrbView(pszFilename);
	if(Pleiades(pszFilename).IsFullCompliense())
		return new Pleiades(pszFilename);
	if(GeoEye(pszFilename).IsFullCompliense())
		return new GeoEye(pszFilename);
	if(Kompsat(pszFilename).IsFullCompliense())
		return new Kompsat(pszFilename);
	if(RDK1(pszFilename).IsFullCompliense())
		return new RDK1(pszFilename);
	if(Spot(pszFilename).IsFullCompliense())
		return new Spot(pszFilename);
	if(ALOS(pszFilename).IsFullCompliense())
		return new ALOS(pszFilename);
	if(EROS(pszFilename).IsFullCompliense())
		return new EROS(pszFilename);
	if(Formosat(pszFilename).IsFullCompliense())
		return new Formosat(pszFilename);
	if(Landsat(pszFilename).IsFullCompliense())
		return new Landsat(pszFilename);
	if(RapidEye(pszFilename).IsFullCompliense())
		return new RapidEye(pszFilename);
	
	return NULL;
};