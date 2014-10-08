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

#ifndef _READER_ABSTRACT_H_INCLUDED
#define _READER_ABSTRACT_H_INCLUDED

#include "remote_sensing_metadata.h"

#include "cpl_string.h"

const CPLString const MDPrefix_IMD= "IMD";
const CPLString const MDPrefix_RPC= "RPC";
const CPLString const MDPrefix_Common_IMD= "IMAGERY";

typedef struct
{
	CPLString lineOffset;
	CPLString sampOffset;
	CPLString latOffset;
	CPLString longOffset;
	CPLString heightOffset;
	CPLString lineScale;
	CPLString sampScale;
	CPLString latScale;
	CPLString longScale;
	CPLString heightScale;
	CPLString lineNumCoef;
	CPLString lineDenCoef;
	CPLString sampNumCoef;
	CPLString sampDenCoef;
} RSMDRPC;

/**
@brief Metadata reader base class
*/
class RSMDReader
{
public:
/**
@brief Constructor
@param pszFilename
@param pszProviderName
*/
	RSMDReader(const char* pszFilename, const char* pszProviderName):pszFilename(pszFilename), pszProviderName(pszProviderName)
	{
	};

/**
@brief Return list of metadata
*/
	const CPLStringList GetMetadata() const
	{
		CPLStringList szMetadata;
		
		CPLStringList szImageMetadata;
		ReadImageMetadata(szImageMetadata);
		for(int i = 0; i < szImageMetadata.size(); ++i)
		{
			szMetadata.AddString(CPLString().Printf("%s.%s",MDPrefix_IMD.c_str(), szImageMetadata[i]).c_str());
		}

		CPLStringList szCommonImageMetadata;
		GetCommonImageMetadata(szImageMetadata, szCommonImageMetadata);
		if( CSLFindName(szCommonImageMetadata.List(), MDName_CloudCover.c_str()) == -1)
		{
			szCommonImageMetadata.SetNameValue(MDName_CloudCover.c_str(), "0");
		}
		for(int i = 0; i < szCommonImageMetadata.size(); ++i)
		{
			szMetadata.AddString(CPLString().Printf("%s.%s",MDPrefix_Common_IMD.c_str(), szCommonImageMetadata[i]).c_str());
		}

		RSMDRPC szRPC;
		ReadRPC(szRPC);
		if( !szRPC.lineOffset.empty() )
			szMetadata.AddString(CPLString().Printf("%s.LINE_OFF=%s",MDPrefix_RPC.c_str(), szRPC.lineOffset.c_str()).c_str());
		if( !szRPC.sampOffset.empty() )
			szMetadata.AddString(CPLString().Printf("%s.SAMP_OFF=%s",MDPrefix_RPC.c_str(), szRPC.sampOffset.c_str()).c_str());
		if( !szRPC.latOffset.empty() )
			szMetadata.AddString(CPLString().Printf("%s.LAT_OFF=%s",MDPrefix_RPC.c_str(), szRPC.latOffset.c_str()).c_str());
		if( !szRPC.longOffset.empty() )
			szMetadata.AddString(CPLString().Printf("%s.LONG_OFF=%s",MDPrefix_RPC.c_str(), szRPC.longOffset.c_str()).c_str());
		if( !szRPC.heightOffset.empty() )
			szMetadata.AddString(CPLString().Printf("%s.HEIGHT_OFF=%s",MDPrefix_RPC.c_str(), szRPC.heightOffset.c_str()).c_str());
		if( !szRPC.lineScale.empty() )
			szMetadata.AddString(CPLString().Printf("%s.LINE_SCALE=%s",MDPrefix_RPC.c_str(), szRPC.lineScale.c_str()).c_str());
		if( !szRPC.sampScale.empty() )
			szMetadata.AddString(CPLString().Printf("%s.SAMP_SCALE=%s",MDPrefix_RPC.c_str(), szRPC.sampScale.c_str()).c_str());
		if( !szRPC.latScale.empty() )
			szMetadata.AddString(CPLString().Printf("%s.LAT_SCALE=%s",MDPrefix_RPC.c_str(), szRPC.latScale.c_str()).c_str());
		if( !szRPC.longScale.empty() )
			szMetadata.AddString(CPLString().Printf("%s.LONG_SCALE=%s",MDPrefix_RPC.c_str(), szRPC.longScale.c_str()).c_str());
		if( !szRPC.heightScale.empty() )
			szMetadata.AddString(CPLString().Printf("%s.HEIGHT_SCALE=%s",MDPrefix_RPC.c_str(), szRPC.heightScale.c_str()).c_str());
		if( !szRPC.lineNumCoef.empty() )
			szMetadata.AddString(CPLString().Printf("%s.LINE_NUM_COEFF=%s",MDPrefix_RPC.c_str(), szRPC.lineNumCoef.c_str()).c_str());
		if( !szRPC.lineDenCoef.empty() )
			szMetadata.AddString(CPLString().Printf("%s.LINE_DEN_COEFF=%s",MDPrefix_RPC.c_str(), szRPC.lineDenCoef.c_str()).c_str());
		if( !szRPC.sampNumCoef.empty() )
			szMetadata.AddString(CPLString().Printf("%s.SAMP_NUM_COEFF=%s",MDPrefix_RPC.c_str(), szRPC.sampNumCoef.c_str()).c_str());
		if( !szRPC.sampDenCoef.empty() )
			szMetadata.AddString(CPLString().Printf("%s.SAMP_DEN_COEFF=%s",MDPrefix_RPC.c_str(), szRPC.sampDenCoef.c_str()).c_str());
		return szMetadata;
	};

    
/**
@brief Return list of sources files
*/
	const CPLStringList GetSourceFileList() const
	{
		return DefineSourceFiles();
	}

/**
@brief Determine whether the input parameter correspond to the particular provider of remote sensing data completely
@return True if all sources files found
*/
	virtual const bool IsFullCompliense() const = 0;

/**
@brief Return remote sensing metadata provider name 
@return metadata provider name
*/
	const char* ProviderName() const
	{
		return pszProviderName;
	};

protected:
	const char* pszFilename;
	const char* pszProviderName;

private:
/**
@brief Search all specific to the provider sources files
*/
	virtual const CPLStringList DefineSourceFiles() const = 0;

/**
@brief Read metadata from sources files
*/
	virtual void ReadImageMetadata(CPLStringList& szrImageMetadata) const = 0;

/**
@brief Find and return common image metadata (common image metadata described in remote_sensing_metadata.h)
*/
	virtual void GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const = 0;

/**
@brief Read metadata from sources files
*/
	virtual void ReadRPC(RSMDRPC& rRPC) const = 0;

};

#endif /* _READER_ABSTRACT_H_INCLUDED */