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

#include "reader_digital_globe.h"

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

		int r = sscanf ( rsAcqisitionTime.c_str(), "%lu-%lu-%luT%lu:%lu:%lu.%*ulZ", &iYear, &iMonth, &iDay, &iHours, &iMin, &iSec);

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

	bool GetAcqisitionMidTime(const CPLString& rsAcqisitionStartTime, const CPLString& rsAcqisitionEndTime, CPLString& osAcqisitionTime)
	{
		CPLString sTimeStart;
		CPLString sTimeEnd;
		time_t timeStart = GetAcqisitionTimeFromString(rsAcqisitionStartTime, sTimeStart);
		time_t timeEnd = GetAcqisitionTimeFromString(rsAcqisitionEndTime, sTimeEnd);

		time_t timeMid = timeStart + (timeEnd - timeStart)/2;

		char buffer [80];
		size_t dCharsCount = strftime (buffer,80,AcquisitionDateTimeFormat.c_str(), localtime(&timeMid));
		osAcqisitionTime.assign(&buffer[0]);

		return true;
	}
}

DigitalGlobe::DigitalGlobe(const char* pszFilename)
	:RSMDReader(pszFilename, "DigitalGlobe")
{
	osIMDSourceFilename = GDALFindAssociatedFile( pszFilename, "IMD", NULL, 0 );
	osRPBSourceFilename = GDALFindAssociatedFile( pszFilename, "RPB", NULL, 0 );
	osXMLSourceFilename = "";

	if (osIMDSourceFilename.empty() || osRPBSourceFilename.empty() )
	{
		CPLString osFilename = GDALFindAssociatedFile( pszFilename, "XML", NULL, 0 );
		if( IsXMLValid(osFilename) )
			osXMLSourceFilename = osFilename;
	}
};

const bool DigitalGlobe::IsFullCompliense() const
{
	if (osIMDSourceFilename != "" && osRPBSourceFilename != "")
		return true;
	else if(osXMLSourceFilename != "")
		return true;

	return false;
}

void DigitalGlobe::ReadImageMetadata(CPLStringList& szrImageMetadata) const
{
	if (osIMDSourceFilename != "" && osRPBSourceFilename != "")
	{
		ReadImageMetadataFromWKT(szrImageMetadata);
	}
	else if(osXMLSourceFilename != "")
	{
		ReadImageMetadataFromXML(szrImageMetadata);
	}
}

void DigitalGlobe::ReadImageMetadataFromWKT(CPLStringList& szrImageMetadata) const
{
	if (osIMDSourceFilename != "")
	{
		char **papszIMDMD = GDALLoadIMDFile( osIMDSourceFilename.c_str(), NULL );

		if( papszIMDMD != NULL )
        {
			for(int i = 0; papszIMDMD[i] != NULL; i++ )
				szrImageMetadata.AddString(papszIMDMD[i]);

            CSLDestroy( papszIMDMD );
        }
	}
}

void DigitalGlobe::ReadImageMetadataFromXML(CPLStringList& szrImageMetadata) const
{
	if(osXMLSourceFilename != "")
	{
		CPLXMLNode* psNode = CPLParseXMLFile(osXMLSourceFilename.c_str());
		if(psNode == NULL)
		{
			return;
		}

		CPLXMLNode* psisdNode = psNode->psNext;
		if(psisdNode == NULL)
		{
			return;
		}
		
		CPLStringList expulsionNodeNames;
		ReadXMLToStringList(CPLSearchXMLNode(psisdNode, "IMD"), expulsionNodeNames, szrImageMetadata);
	}
}

void DigitalGlobe::GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const
{
	if( CSLFindName(szrImageMetadata.List(), "IMAGE.SATID") != -1)
	{
		CPLString SatelliteIdValue = CSLFetchNameValue(szrImageMetadata.List(), "IMAGE.SATID");
		szrCommonImageMetadata.SetNameValue(MDName_SatelliteId, CPLStripQuotes(SatelliteIdValue));
	}
	if( CSLFindName(szrImageMetadata.List(), "IMAGE_1.SATID") != -1)
	{
		CPLString SatelliteIdValue = CSLFetchNameValue(szrImageMetadata.List(), "IMAGE_1.SATID");
		szrCommonImageMetadata.SetNameValue(MDName_SatelliteId, CPLStripQuotes(SatelliteIdValue));
	}
	if( CSLFindName(szrImageMetadata.List(), "IMAGE_1.cloudCover") != -1)
	{
		CPLString osCloudCoverValue = CSLFetchNameValue(szrImageMetadata.List(), "IMAGE_1.cloudCover");
		szrCommonImageMetadata.SetNameValue(MDName_CloudCover, CPLStripQuotes(osCloudCoverValue));
	}
	if( CSLFindName(szrImageMetadata.List(), "IMAGE.CLOUDCOVER") != -1)
	{
		CPLString osCloudCoverValue = CSLFetchNameValue(szrImageMetadata.List(), "IMAGE.CLOUDCOVER");
		szrCommonImageMetadata.SetNameValue(MDName_CloudCover, CPLStripQuotes(osCloudCoverValue));
	}
	
	if( CSLFindName(szrImageMetadata.List(), "MAP_PROJECTED_PRODUCT.earliestAcqTime") != -1 &&
			CSLFindName(szrImageMetadata.List(), "MAP_PROJECTED_PRODUCT.latestAcqTime") != -1)
	{
		CPLString osTimeStart = CSLFetchNameValue(szrImageMetadata.List(), "MAP_PROJECTED_PRODUCT.earliestAcqTime");
		CPLString osTimeEnd = CSLFetchNameValue(szrImageMetadata.List(), "MAP_PROJECTED_PRODUCT.latestAcqTime");
		CPLString osAcqisitionTime;
		
		if(GetAcqisitionMidTime(osTimeStart, osTimeEnd, osAcqisitionTime))
			szrCommonImageMetadata.SetNameValue(MDName_AcquisitionDateTime, osAcqisitionTime);
		else
			szrCommonImageMetadata.SetNameValue(MDName_AcquisitionDateTime, "unknown");
	}
}

void DigitalGlobe::ReadRPC(RSMDRPC& rRPC) const
{
	if (osRPBSourceFilename != "")
	{
		ReadRPCFromWKT(rRPC);
	}
	else if(osXMLSourceFilename != "")
	{
		ReadRPCFromXML(rRPC);
	}
}
void DigitalGlobe::ReadRPCFromWKT(RSMDRPC& rRPC) const
{
	CPLStringList szrRPC;
	if (osRPBSourceFilename != "")
	{
		char **papszRPCMD = GDALLoadRPBFile( osRPBSourceFilename.c_str(), NULL );

		if( papszRPCMD != NULL )
        {
			for(int i = 0; papszRPCMD[i] != NULL; i++ )
				szrRPC.AddString(papszRPCMD[i]);
            CSLDestroy( papszRPCMD );
        }
	}

	const char* szpLineOffset = szrRPC.FetchNameValue("LINE_OFF");
	if (szpLineOffset != NULL)
		rRPC.lineOffset = CPLString(szpLineOffset);

	const char* szpSampOffset = szrRPC.FetchNameValue("SAMP_OFF");
	if (szpSampOffset != NULL) 
		rRPC.sampOffset = szpSampOffset;

	const char* szpLatOffset = szrRPC.FetchNameValue("LAT_OFF");
	if (szpLatOffset != NULL) 
		rRPC.latOffset = CPLString(szpLatOffset);

	const char* szpLongOffset = szrRPC.FetchNameValue("LONG_OFF");
	if (szpLongOffset != NULL) 
		rRPC.longOffset = CPLString(szpLongOffset);

	const char* szpHeightOffset = szrRPC.FetchNameValue("HEIGHT_OFF");
	if (szpHeightOffset != NULL) 
		rRPC.heightOffset = CPLString(szpHeightOffset);

	const char* szpLineScale = szrRPC.FetchNameValue("LINE_SCALE");
	if (szpLineScale != NULL) 
		rRPC.lineScale = CPLString(szpLineScale);

	const char* szpSampScale = szrRPC.FetchNameValue("SAMP_SCALE");
	if (szpSampScale != NULL) 
		rRPC.sampScale = CPLString(szpSampScale);

	const char* szpLatScale = szrRPC.FetchNameValue("LAT_SCALE");
	if (szpLatScale != NULL) 
		rRPC.latScale = CPLString(szpLatScale);

	const char* szpLongScale = szrRPC.FetchNameValue("LONG_SCALE");
	if (szpLongScale != NULL) 
		rRPC.longScale = CPLString(szpLongScale);

	const char* szpHeightScale = szrRPC.FetchNameValue("HEIGHT_SCALE");
	if (szpHeightScale != NULL) 
		rRPC.heightScale = CPLString(szpHeightScale);

	const char* szpLineNumCoef = szrRPC.FetchNameValue("LINE_NUM_COEFF");
	if (szpLineNumCoef != NULL)
		rRPC.lineNumCoef = CPLString(szpLineNumCoef);

	const char* szpLineDenCoef = szrRPC.FetchNameValue("LINE_DEN_COEFF");
	if (szpLineDenCoef != NULL)
		rRPC.lineDenCoef = CPLString(szpLineDenCoef);

	const char* szpSampNumCoef = szrRPC.FetchNameValue("SAMP_NUM_COEFF");
	if (szpSampNumCoef != NULL)
		rRPC.sampNumCoef = CPLString(szpSampNumCoef);

	const char* szpSampDenCoef = szrRPC.FetchNameValue("SAMP_DEN_COEFF");
	if (szpSampDenCoef != NULL)
		rRPC.sampDenCoef = CPLString(szpSampDenCoef);
}
void DigitalGlobe::ReadRPCFromXML(RSMDRPC& rRPC) const
{
	CPLStringList szrRPC;
	if(osXMLSourceFilename != "")
	{
		CPLXMLNode* psNode = CPLParseXMLFile(osXMLSourceFilename.c_str());
		if(psNode == NULL)
			return;

		CPLXMLNode* psisdNode = psNode->psNext;
		if(psisdNode == NULL)
			return;

		CPLStringList szRPCdata;
		CPLStringList expulsionNodeNames;
		ReadXMLToStringList(CPLSearchXMLNode(psisdNode, "RPB"), expulsionNodeNames, szrRPC);
	}

	const char* szpLineOffset = szrRPC.FetchNameValue("image.lineoffset");
	if (szpLineOffset != NULL)
		rRPC.lineOffset = CPLString(szpLineOffset);

	const char* szpSampOffset = szrRPC.FetchNameValue("image.sampOffset");
	if (szpSampOffset != NULL) 
		rRPC.sampOffset = szpSampOffset;

	const char* szpLatOffset = szrRPC.FetchNameValue("image.latOffset");
	if (szpLatOffset != NULL) 
		rRPC.latOffset = CPLString(szpLatOffset);

	const char* szpLongOffset = szrRPC.FetchNameValue("image.longOffset");
	if (szpLongOffset != NULL) 
		rRPC.longOffset = CPLString(szpLongOffset);

	const char* szpHeightOffset = szrRPC.FetchNameValue("image.heightOffset");
	if (szpHeightOffset != NULL) 
		rRPC.heightOffset = CPLString(szpHeightOffset);

	const char* szpLineScale = szrRPC.FetchNameValue("image.lineScale");
	if (szpLineScale != NULL) 
		rRPC.lineScale = CPLString(szpLineScale);

	const char* szpSampScale = szrRPC.FetchNameValue("image.sampScale");
	if (szpSampScale != NULL) 
		rRPC.sampScale = CPLString(szpSampScale);

	const char* szpLatScale = szrRPC.FetchNameValue("image.latScale");
	if (szpLatScale != NULL) 
		rRPC.latScale = CPLString(szpLatScale);

	const char* szpLongScale = szrRPC.FetchNameValue("image.longScale");
	if (szpLongScale != NULL) 
		rRPC.longScale = CPLString(szpLongScale);

	const char* szpHeightScale = szrRPC.FetchNameValue("image.heightScale");
	if (szpHeightScale != NULL) 
		rRPC.heightScale = CPLString(szpHeightScale);

	const char* szpLineNumCoef = szrRPC.FetchNameValue("image.lineNumCoefList.lineNumCoef");
	if (szpLineNumCoef != NULL)
		rRPC.lineNumCoef = CPLString(szpLineNumCoef);

	const char* szpLineDenCoef = szrRPC.FetchNameValue("image.lineDenCoefList.lineDenCoef");
	if (szpLineDenCoef != NULL)
		rRPC.lineDenCoef = CPLString(szpLineDenCoef);

	const char* szpSampNumCoef = szrRPC.FetchNameValue("image.sampNumCoefList.sampNumCoef");
	if (szpSampNumCoef != NULL)
		rRPC.sampNumCoef = CPLString(szpSampNumCoef);

	const char* szpSampDenCoef = szrRPC.FetchNameValue("image.sampDenCoefList.sampDenCoef");
	if (szpSampDenCoef != NULL)
		rRPC.sampDenCoef = CPLString(szpSampDenCoef);
}

const CPLStringList DigitalGlobe::DefineSourceFiles() const
{
	CPLStringList papszFileList;

	if(osIMDSourceFilename != "" && osRPBSourceFilename != "")
	{
		papszFileList.AddString(osIMDSourceFilename.c_str());
		papszFileList.AddString(osRPBSourceFilename.c_str());
	}
	else if(osXMLSourceFilename != "")
		papszFileList.AddString(osXMLSourceFilename.c_str());

	return papszFileList;
}

bool DigitalGlobe::IsXMLValid(const CPLString& psFilename) const
{
	if(psFilename != "")
	{
		CPLXMLNode* psNode = CPLParseXMLFile(psFilename.c_str());
		if(psNode == NULL)
		{
			return false;
		}

		CPLXMLNode* psisdNode = psNode->psNext;
		if(psisdNode == NULL)
		{
			return false;
		}
			
		CPLXMLNode* psSatIdNode = CPLSearchXMLNode(psisdNode, "SATID");
		if(psSatIdNode != NULL)
			return true;
	}

	return false;
}
