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

#include <map>

#include "remote_sensing_metadata.h"

#include "cpl_string.h"

const CPLString MDPrefix_IMD= "IMD";
const CPLString MDPrefix_RPC= "RPC";
const CPLString MDPrefix_Common_IMD= "IMAGERY";

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
	//const CPLStringList GetMetadata() const
	const std::map<CPLString, CPLStringList>* GetMetadata() const
	{
		std::map<CPLString, CPLStringList>* pmMetadata = new std::map<CPLString, CPLStringList>();
		//CPLStringList szMetadata;
		
		CPLStringList szImageMetadata;
		ReadImageMetadata(szImageMetadata);
		//for(int i = 0; i < szImageMetadata.size(); ++i)
		//{
		//	szMetadata.AddString(CPLString().Printf("%s.%s",MDPrefix_IMD.c_str(), szImageMetadata[i]).c_str());
		//}
		(*pmMetadata)[MDPrefix_IMD] = szImageMetadata;

		CPLStringList szCommonImageMetadata;
		GetCommonImageMetadata(szImageMetadata, szCommonImageMetadata);
		if( CSLFindName(szCommonImageMetadata.List(), MDName_CloudCover.c_str()) == -1)
		{
			szCommonImageMetadata.SetNameValue(MDName_CloudCover.c_str(), "0");
		}
		//for(int i = 0; i < szCommonImageMetadata.size(); ++i)
		//{
		//	szMetadata.AddString(CPLString().Printf("%s.%s",MDPrefix_Common_IMD.c_str(), szCommonImageMetadata[i]).c_str());
		//}
		(*pmMetadata)[MDPrefix_Common_IMD] = szCommonImageMetadata;
			
		RSMDRPC szRPC;
		ReadRPC(szRPC);
		CPLStringList szRPCMetadata;
		if( !szRPC.lineOffset.empty() )
			szRPCMetadata.AddNameValue("LINE_OFF", szRPC.lineOffset.c_str());
		if( !szRPC.sampOffset.empty() )
			szRPCMetadata.AddNameValue("SAMP_OFF", szRPC.sampOffset.c_str());
		if( !szRPC.latOffset.empty() )
			szRPCMetadata.AddNameValue("LAT_OFF", szRPC.latOffset.c_str());
		if( !szRPC.longOffset.empty() )
			szRPCMetadata.AddNameValue("LONG_OFF", szRPC.longOffset.c_str());
		if( !szRPC.heightOffset.empty() )
			szRPCMetadata.AddNameValue("HEIGHT_OFF", szRPC.heightOffset.c_str());
		if( !szRPC.lineScale.empty() )
			szRPCMetadata.AddNameValue("LINE_SCALE", szRPC.lineScale.c_str());
		if( !szRPC.sampScale.empty() )
			szRPCMetadata.AddNameValue("SAMP_SCALE", szRPC.sampScale.c_str());
		if( !szRPC.latScale.empty() )
			szRPCMetadata.AddNameValue("LAT_SCALE", szRPC.latScale.c_str());
		if( !szRPC.longScale.empty() )
			szRPCMetadata.AddNameValue("LONG_SCALE", szRPC.longScale.c_str());
		if( !szRPC.heightScale.empty() )
			szRPCMetadata.AddNameValue("HEIGHT_SCALE", szRPC.heightScale.c_str());
		if( !szRPC.lineNumCoef.empty() )
			szRPCMetadata.AddNameValue("LINE_NUM_COEFF", szRPC.lineNumCoef.c_str());
		if( !szRPC.lineDenCoef.empty() )
			szRPCMetadata.AddNameValue("LINE_DEN_COEFF", szRPC.lineDenCoef.c_str());
		if( !szRPC.sampNumCoef.empty() )
			szRPCMetadata.AddNameValue("SAMP_NUM_COEFF", szRPC.sampNumCoef.c_str());
		if( !szRPC.sampDenCoef.empty() )
			szRPCMetadata.AddNameValue("SAMP_DEN_COEFF", szRPC.sampDenCoef.c_str());

		(*pmMetadata)[MDPrefix_RPC] = szRPCMetadata;

		return pmMetadata;
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