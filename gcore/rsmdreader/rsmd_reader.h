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

#ifndef _RSMD_READER_H_INCLUDED
#define _RSMD_READER_H_INCLUDED

#include "reader_abstract.h"

/**
@mainpage
@code
#include "rsmd_reader.h"
@endcode

End use it!
*/

/**
@brief Remote sensing metadata providers
*/
typedef enum
{
	RSMD_DigitalGlobe = 1,
	RSMD_OrbView,
    RSMD_Pleiades,
    RSMD_GeoEye,
    RSMD_Kompsat,
    RSMD_RDK1,
    RSMD_Spot,
    RSMD_ALOS,
    RSMD_EROS,
    RSMD_Formosat,
    RSMD_Landsat,
    RSMD_RapidEye

} RSMDProvider;

/**
@brief Return object - metadata reader for given type of data provider
@code
RSMetadata::RSMDReader *mdReader = RSMetadata::GetRSMDReader(pszFilename, RSMetadata::RSMD_DigitalGlobe);	
CPLStringList sourseMDFileList = mdReader->GetSourceFileList();
for( int i = 0; i < sourseMDFileList.size(); i++ )
{
printf( "\t\t >>> source file: %s\n", sourseMDFileList[i] );
}

CPLStringList metadata = mdReader->GetMetadata();
for( int i = 0; i < metadata.size(); i++ )
{
printf( "\t\t >>> metadata: %s\n", metadata[i] );
}
@endcode
*/
RSMDReader* GetRSMDReader(const char* pszFilename, RSMDProvider rsdProvider);

/**
@brief Return object - metadata reader for suitable type of data provider
@code
RSMetadata::RSMDReader *mdReader = RSMetadata::GetRSMDReader(pszFilename);	
CPLStringList sourseMDFileList = mdReader->GetSourceFileList();
for( int i = 0; i < sourseMDFileList.size(); i++ )
{
printf( "\t\t >>> source file: %s\n", sourseMDFileList[i] );
}

CPLStringList metadata = mdReader->GetMetadata();
for( int i = 0; i < metadata.size(); i++ )
{
printf( "\t\t >>> metadata: %s\n", metadata[i] );
}
@endcode
*/
RSMDReader* GetRSMDReader(const CPLString pszFilename);

#endif /* _RSMD_READER_H_INCLUDED */