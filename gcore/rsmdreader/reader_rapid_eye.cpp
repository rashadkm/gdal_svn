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
#include <vector>

#include "cplkeywordparser.h"

#include "reader_rapid_eye.h"

#include "remote_sensing_metadata.h"
#include "utils.h"

namespace
{
	const time_t GetAcqisitionTimeFromString(const CPLString& rsAcqisitionTime, CPLString& rsAcqisitionTimeFormatted)
	{
		size_t iYear;
		size_t iMonth;
		size_t iDay;
		size_t iHours;
		size_t iMin;
		size_t iSec;

		int r = sscanf ( rsAcqisitionTime.c_str(), "%lu-%lu-%luT%lu:%lu:%lu.%*s", &iYear, &iMonth, &iDay, &iHours, &iMin, &iSec);
		if (r != 6)
			return -1;
		
		tm tmDateTime;
		tmDateTime.tm_sec = iSec;
		tmDateTime.tm_min = iMin;
		tmDateTime.tm_hour = iHours;
		tmDateTime.tm_mday = iDay;
		tmDateTime.tm_mon = iMonth - 1;
		tmDateTime.tm_year = iYear - 1900;

		char buffer [80];
		size_t dCharsCount = strftime (buffer,80,AcquisitionDateTimeFormat.c_str(),&tmDateTime);
		rsAcqisitionTimeFormatted.assign(&buffer[0]);

		return mktime(&tmDateTime);
	}
}

RapidEye::RapidEye(const char* pszFilename)
	:RSMDReader(pszFilename, "RapidEye")
{
	CPLString osDirName = CPLGetDirname(pszFilename);
	CPLString osBaseName = CPLGetBasename(pszFilename);

	osIMDSourceFilename = CPLFormFilename( osDirName.c_str(), CPLSPrintf("%s_metadata", osBaseName.c_str()), ".xml" );

	VSIStatBufL sStatBuf;
	if( VSIStatExL( osIMDSourceFilename.c_str(), &sStatBuf, VSI_STAT_EXISTS_FLAG ) != 0 )
    {
		osIMDSourceFilename = "";
	}
};

const bool RapidEye::IsFullCompliense() const
{
	if( !osIMDSourceFilename.empty() )
	{
		return true;
	}

	return false;
}

void RapidEye::ReadImageMetadata(CPLStringList& szrImageMetadata) const
{
	if(!osIMDSourceFilename.empty())
	{
		CPLXMLNode* psNode = CPLParseXMLFile(osIMDSourceFilename.c_str());
		if(psNode == NULL)
			return;

		CPLXMLNode* psEarthObservationNode = psNode->psNext;
		if(psEarthObservationNode == NULL)
			return;

		CPLStringList expulsionNodeNames;
		ReadXMLToStringList(psEarthObservationNode, expulsionNodeNames, szrImageMetadata);
	}
}

void RapidEye::GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const
{
	if( CSLFindName(szrImageMetadata.List(), 
		"gml:using.eop:EarthObservationEquipment.eop:platform.eop:Platform.eop:serialIdentifier") != -1 )
	{
		CPLString SatelliteIdValue = CSLFetchNameValue(szrImageMetadata.List(), 
			"gml:using.eop:EarthObservationEquipment.eop:platform.eop:Platform.eop:serialIdentifier");
		szrCommonImageMetadata.SetNameValue(MDName_SatelliteId.c_str(), SatelliteIdValue.c_str());
	}

	if( CSLFindName(szrImageMetadata.List(), 
		"gml:resultOf.re:EarthObservationResult.opt:cloudCoverPercentage") != -1 )
	{
		CPLString CloudCover = CSLFetchNameValue(szrImageMetadata.List(), 
			"gml:resultOf.re:EarthObservationResult.opt:cloudCoverPercentage");
		szrCommonImageMetadata.SetNameValue(MDName_CloudCover.c_str(), CloudCover.c_str());
	}
	
	if( CSLFindName(szrImageMetadata.List(), 
		"gml:using.eop:EarthObservationEquipment.eop:acquisitionParameters.re:Acquisition.re:acquisitionDateTime") != -1)
	{
		CPLString AcqisitionDateTime;
		GetAcqisitionTimeFromString( CSLFetchNameValue(szrImageMetadata.List(), 
			"gml:using.eop:EarthObservationEquipment.eop:acquisitionParameters.re:Acquisition.re:acquisitionDateTime"), AcqisitionDateTime);
		szrCommonImageMetadata.SetNameValue(MDName_AcquisitionDateTime.c_str(), AcqisitionDateTime.c_str());
	}
}

void RapidEye::ReadRPC(RSMDRPC& rRPC) const
{

}

const CPLStringList RapidEye::DefineSourceFiles() const
{
	CPLStringList papszFileList;
	if(osIMDSourceFilename != "")
	{
		papszFileList.AddString(osIMDSourceFilename.c_str());
	}

	return papszFileList;
}
