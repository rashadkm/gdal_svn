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

#ifndef _READER_EROS_H_INCLUDED
#define _READER_EROS_H_INCLUDED

#include "cpl_string.h"
#include "gdal_priv.h"

#include "rsmd_reader.h"

/**
@brief Metadata reader for EROS
*/
class EROS: public RSMDReader
{
public:
	EROS(const char* pszFilename);
    
	const bool IsFullCompliense() const;

private:
	CPLString osIMDSourceFilename;

private:
	const CPLStringList DefineSourceFiles() const;

	void ReadImageMetadata(CPLStringList& szrImageMetadata) const;

	void GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const;

	void ReadRPC(RSMDRPC& rRPC) const;
};

#endif /* _READER_EROS_H_INCLUDED */