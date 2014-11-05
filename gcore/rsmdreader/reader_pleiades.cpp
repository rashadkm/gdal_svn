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

#include "cplkeywordparser.h"

#include "reader_pleiades.h"

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

		int r = sscanf ( rsAcqisitionTime, "%lu-%lu-%lu %lu:%lu:%lu.%*s", &iYear, &iMonth, &iDay, &iHours, &iMin, &iSec);
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
		/*size_t dCharsCount = */strftime (buffer,80,AcquisitionDateTimeFormat,&tmDateTime);
		rsAcqisitionTimeFormatted.assign(&buffer[0]);

		return mktime(&tmDateTime);
	}
}

Pleiades::Pleiades(const char* pszFilename)
	:RSMDReader(pszFilename, "Pleiades")
{
	CPLString osDirName = CPLGetDirname(pszFilename);
	CPLString osBaseName = CPLGetBasename(pszFilename);
	osBaseName.replace(0,4,"");

	osXMLIMDSourceFilename = CPLFormFilename( osDirName, CPLSPrintf("DIM_%s", osBaseName.c_str()), ".XML" );
	osXMLRPCSourceFilename = CPLFormFilename( osDirName, CPLSPrintf("RPC_%s", osBaseName.c_str()), ".XML" );

	if (!CPLCheckForFile((char*)osXMLIMDSourceFilename.c_str(), NULL))
    {
        osXMLIMDSourceFilename.clear();
	}
	
    if (!CPLCheckForFile((char*)osXMLRPCSourceFilename.c_str(), NULL))
    {
        osXMLRPCSourceFilename.clear();
	}
}

const bool Pleiades::IsFullCompliense() const
{
	if (!osXMLIMDSourceFilename.empty() && !osXMLRPCSourceFilename.empty())
	{
		return true;
	}

	return false;
}

void Pleiades::ReadImageMetadata(CPLStringList& szrImageMetadata) const
{
	ReadImageMetadataFromXML(szrImageMetadata);
}

void Pleiades::ReadImageMetadataFromXML(CPLStringList& szrImageMetadata) const
{
	
	if(!osXMLIMDSourceFilename.empty())
	{
		CPLXMLNode* psNode = CPLParseXMLFile(osXMLIMDSourceFilename);
		if(psNode == NULL)
			return;

		CPLXMLNode* psDimapNode = psNode->psNext->psNext;
		if(psDimapNode == NULL)
			return;

		CPLStringList expulsionNodeNames;
		//ReadXMLToStringList(CPLSearchXMLNode(psDimapNode,"Dataset_Sources"), expulsionNodeNames, szrImageMetadata);
		ReadXMLToStringList(psDimapNode, expulsionNodeNames, szrImageMetadata);
	}
	
}

void Pleiades::GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const
{
	if( CSLFindName(szrImageMetadata.List(), "Dataset_Sources.Source_Identification.Strip_Source.MISSION") != -1 &&
		CSLFindName(szrImageMetadata.List(), "Dataset_Sources.Source_Identification.Strip_Source.MISSION_INDEX") != -1)
	{
		CPLString SatelliteIdValue = CPLSPrintf( "%s.%s",
			CSLFetchNameValue(szrImageMetadata.List(), "Dataset_Sources.Source_Identification.Strip_Source.MISSION"),
			CSLFetchNameValue(szrImageMetadata.List(), "Dataset_Sources.Source_Identification.Strip_Source.MISSION_INDEX") );
		szrCommonImageMetadata.SetNameValue(MDName_SatelliteId, SatelliteIdValue);
	}

	if( CSLFindName(szrImageMetadata.List(), "Dataset_Sources.Source_Identification.Strip_Source.IMAGING_DATE") != -1 &&
			CSLFindName(szrImageMetadata.List(), "Dataset_Sources.Source_Identification.Strip_Source.IMAGING_TIME") != -1)
	{
		CPLString osAcqisitionTime = CSLFetchNameValue(szrImageMetadata.List(), "Dataset_Sources.Source_Identification.Strip_Source.IMAGING_TIME");
		CPLString osAcqisitionDate = CSLFetchNameValue(szrImageMetadata.List(), "Dataset_Sources.Source_Identification.Strip_Source.IMAGING_DATE");
		CPLString AcqisitionDateTime;
		GetAcqisitionTimeFromString( osAcqisitionDate + " " + osAcqisitionTime, AcqisitionDateTime);
		szrCommonImageMetadata.SetNameValue(MDName_AcquisitionDateTime, AcqisitionDateTime);
	}


}

void Pleiades::ReadRPC(RSMDRPC& rRPC) const
{
	ReadRPCFromXML(rRPC);
}
void Pleiades::ReadRPCFromXML(RSMDRPC& rRPC) const
{
	
	CPLStringList szrRPC;
	if(!osXMLRPCSourceFilename.empty())
	{
		CPLXMLNode* psNode = CPLParseXMLFile(osXMLRPCSourceFilename);
		if(psNode == NULL)
			return;

		CPLXMLNode* psRootNode = psNode->psNext;
		if(psRootNode == NULL)
			return;

		CPLStringList szRPCdata;
		CPLStringList expulsionNodeNames;
		ReadXMLToStringList(CPLSearchXMLNode(psRootNode, "Global_RFM"), expulsionNodeNames, szrRPC);
	}

	const char* szpLineOffset = szrRPC.FetchNameValue("RFM_Validity.LINE_OFF");
	if (szpLineOffset != NULL)
		rRPC.lineOffset = CPLString(szpLineOffset);

	const char* szpSampOffset = szrRPC.FetchNameValue("RFM_Validity.SAMP_OFF");
	if (szpSampOffset != NULL) 
		rRPC.sampOffset = szpSampOffset;

	const char* szpLatOffset = szrRPC.FetchNameValue("RFM_Validity.LAT_OFF");
	if (szpLatOffset != NULL) 
		rRPC.latOffset = CPLString(szpLatOffset);

	const char* szpLongOffset = szrRPC.FetchNameValue("RFM_Validity.LONG_OFF");
	if (szpLongOffset != NULL) 
		rRPC.longOffset = CPLString(szpLongOffset);

	const char* szpHeightOffset = szrRPC.FetchNameValue("RFM_Validity.HEIGHT_OFF");
	if (szpHeightOffset != NULL) 
		rRPC.heightOffset = CPLString(szpHeightOffset);

	const char* szpLineScale = szrRPC.FetchNameValue("RFM_Validity.LINE_SCALE");
	if (szpLineScale != NULL) 
		rRPC.lineScale = CPLString(szpLineScale);

	const char* szpSampScale = szrRPC.FetchNameValue("RFM_Validity.SAMP_SCALE");
	if (szpSampScale != NULL) 
		rRPC.sampScale = CPLString(szpSampScale);

	const char* szpLatScale = szrRPC.FetchNameValue("RFM_Validity.LAT_SCALE");
	if (szpLatScale != NULL) 
		rRPC.latScale = CPLString(szpLatScale);

	const char* szpLongScale = szrRPC.FetchNameValue("RFM_Validity.LONG_SCALE");
	if (szpLongScale != NULL) 
		rRPC.longScale = CPLString(szpLongScale);

	const char* szpHeightScale = szrRPC.FetchNameValue("RFM_Validity.HEIGHT_SCALE");
	if (szpHeightScale != NULL) 
		rRPC.heightScale = CPLString(szpHeightScale);

	CPLString lineNumCoef;
	for(int i = 1; i < 21; ++i)
	{
		CPLString coefName = CPLString().Printf("Direct_Model.LINE_NUM_COEFF_%d",i);
		const char* szpCoef = szrRPC.FetchNameValue(coefName);
		if (szpCoef != NULL)
			 lineNumCoef = lineNumCoef + " " + CPLString(szpCoef);
		else
			return;
	}
	rRPC.lineNumCoef = lineNumCoef;
	
	CPLString lineDenCoef;
	for(int i = 1; i < 21; ++i)
	{
		CPLString coefName = CPLString().Printf("Direct_Model.LINE_DEN_COEFF_%d",i);
		const char* szpCoef = szrRPC.FetchNameValue(coefName);
		if (szpCoef != NULL)
			 lineDenCoef = lineDenCoef + " " + CPLString(szpCoef);
		else
			return;
	}
	rRPC.lineDenCoef = lineDenCoef;

	CPLString sampNumCoef;
	for(int i = 1; i < 21; ++i)
	{
		CPLString coefName = CPLString().Printf("Direct_Model.SAMP_NUM_COEFF_%d",i);
		const char* szpCoef = szrRPC.FetchNameValue(coefName);
		if (szpCoef != NULL)
			 sampNumCoef = sampNumCoef + " " + CPLString(szpCoef);
		else
			return;
	}
	rRPC.sampNumCoef = sampNumCoef;

	CPLString sampDenCoef;
	for(int i = 1; i < 21; ++i)
	{
		CPLString coefName = CPLString().Printf("Direct_Model.SAMP_DEN_COEFF_%d",i);
		const char* szpCoef = szrRPC.FetchNameValue(coefName);
		if (szpCoef != NULL)
			 sampDenCoef = sampDenCoef + " " + CPLString(szpCoef);
		else
			return;
	}
	rRPC.sampDenCoef = sampDenCoef;
}

const CPLStringList Pleiades::DefineSourceFiles() const
{
	CPLStringList papszFileList;
	if(!osXMLIMDSourceFilename.empty())
	{
		papszFileList.AddString(osXMLIMDSourceFilename);
	}
	if(!osXMLRPCSourceFilename.empty())
	{
		papszFileList.AddString(osXMLRPCSourceFilename);
	}
	return papszFileList;
}