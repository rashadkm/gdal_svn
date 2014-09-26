#include "cplkeywordparser.h"

#include "reader_digital_globe.h"

#include "remote_sensing_metadata.h"
#include "utils.h"

namespace
{
	bool GetAcqisitionTime(const CPLString& rsAcqisitionStartTime, const CPLString& rsAcqisitionEndTime, CPLString& osAcqisitionTime)
	{
		size_t iYearStart;
		size_t iYearEnd;

		size_t iMonthStart;
		size_t iMonthEnd;

		size_t iDayStart;
		size_t iDayEnd;
		
		size_t iHoursStart;
		size_t iHoursEnd;

		size_t iMinStart;
		size_t iMinEnd;

		size_t iSecStart;
		size_t iSecEnd;

		int r1 = sscanf ( rsAcqisitionStartTime.c_str(), "%d-%d-%dT%d:%d:%d.%*dZ", &iYearStart, &iMonthStart, &iDayStart, &iHoursStart, &iMinStart, &iSecStart);
		int r2 = sscanf ( rsAcqisitionEndTime.c_str(), "%d-%d-%dT%d:%d:%d.%*dZ", &iYearEnd, &iMonthEnd, &iDayEnd, &iHoursEnd, &iMinEnd, &iSecEnd);
		
		if (r1 != 6 || r2 != 6)
			return false;

		tm timeptrStart;
		timeptrStart.tm_sec = iSecStart;
		timeptrStart.tm_min = iMinStart;
		timeptrStart.tm_hour = iHoursStart;
		timeptrStart.tm_mday = iDayStart;
		timeptrStart.tm_mon = iMonthStart-1;
		timeptrStart.tm_year = iYearStart-1900;

		time_t timeStart = mktime(&timeptrStart);

		tm timeptrEnd;
		timeptrEnd.tm_sec = iSecEnd;
		timeptrEnd.tm_min = iMinEnd;
		timeptrEnd.tm_hour = iHoursEnd;
		timeptrEnd.tm_mday = iDayEnd;
		timeptrEnd.tm_mon = iMonthEnd-1;
		timeptrEnd.tm_year = iYearEnd-1900;
		
		time_t timeEnd = mktime(&timeptrEnd);

		time_t timeAverage = timeStart + (timeEnd - timeStart)/2;

		tm * acqisitionTime = localtime(&timeAverage);
		
		osAcqisitionTime.Printf("%d %d %d %d %d %d", 
			1900 + acqisitionTime->tm_year,
			1 + acqisitionTime->tm_mon,
			acqisitionTime->tm_mday,
			acqisitionTime->tm_hour,
			acqisitionTime->tm_min,
			acqisitionTime->tm_sec);

		return true;
	}
}

DigitalGlobe::DigitalGlobe(const char* pszFilename)
	:RSMDReader(pszFilename, "DigitalGlobe")
{
	osIMDSourceFilename = GDALFindAssociatedFile( pszFilename, "IMD", NULL, 0 );
	osRPBSourceFilename = GDALFindAssociatedFile( pszFilename, "RPB", NULL, 0 );
	osXMLSourceFilename = "";

	if (osIMDSourceFilename == "" || osRPBSourceFilename == "" )
	{
		CPLString osFilename = GDALFindAssociatedFile( pszFilename, "XML", NULL, 0 );
		if( IsXMLValid(osFilename) )
			osXMLSourceFilename = osFilename;
	}	
		
	//osXMLSourceFilename = GDALFindAssociatedFile( pszFilename, "XML", NULL, 0 );
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
			
		ReadXML(CPLSearchXMLNode(psisdNode, "IMD"), szrImageMetadata);
	}
}

void DigitalGlobe::GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const
{
	if( CSLFindName(szrImageMetadata.List(), "IMAGE.SATID") != -1)
	{
		CPLString SatelliteIdValue = CSLFetchNameValue(szrImageMetadata.List(), "IMAGE.SATID");
		CPLString pszMD = MDName_SatelliteId + "=" + SatelliteIdValue;
		szrCommonImageMetadata.AddString(pszMD.c_str());
	}
	if( CSLFindName(szrImageMetadata.List(), "IMAGE_1.SATID") != -1)
	{
		CPLString SatelliteIdValue = CSLFetchNameValue(szrImageMetadata.List(), "IMAGE_1.SATID");
		CPLString pszMD = MDName_SatelliteId + "=" + SatelliteIdValue;
		szrCommonImageMetadata.AddString(pszMD.c_str());
	}
	if( CSLFindName(szrImageMetadata.List(), "IMAGE_1.cloudCover") != -1)
	{
		CPLString osCloudCoverValue = CSLFetchNameValue(szrImageMetadata.List(), "IMAGE_1.cloudCover");
		CPLString pszMD = MDName_CloudCover + "=" + osCloudCoverValue;
		szrCommonImageMetadata.AddString(pszMD.c_str());
	}
	if( CSLFindName(szrImageMetadata.List(), "IMAGE.CLOUDCOVER") != -1)
	{
		CPLString osCloudCoverValue = CSLFetchNameValue(szrImageMetadata.List(), "IMAGE.CLOUDCOVER");
		CPLString pszMD = MDName_CloudCover + "=" + osCloudCoverValue;
		szrCommonImageMetadata.AddString(pszMD.c_str());
	}
	
	if( CSLFindName(szrImageMetadata.List(), "MAP_PROJECTED_PRODUCT.earliestAcqTime") != -1 &&
			CSLFindName(szrImageMetadata.List(), "MAP_PROJECTED_PRODUCT.latestAcqTime") != -1)
	{
		CPLString osTimeStart = CSLFetchNameValue(szrImageMetadata.List(), "MAP_PROJECTED_PRODUCT.earliestAcqTime");
		CPLString osTimeEnd = CSLFetchNameValue(szrImageMetadata.List(), "MAP_PROJECTED_PRODUCT.latestAcqTime");


		CPLString osAcqisitionTime;
		CPLString pszMD;
		if(GetAcqisitionTime(osTimeStart, osTimeEnd, osAcqisitionTime))
		{
			pszMD.Printf("%s=%s",MDName_AcquisitionDateTime.c_str(), osAcqisitionTime.c_str());
			
		}
		else
		{
			pszMD.Printf("%s=unknown", MDName_AcquisitionDateTime.c_str());
		}
		szrCommonImageMetadata.AddString(pszMD.c_str());
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
			printf("isd not found\n");

		CPLStringList szRPCdata;
		ReadXML(CPLSearchXMLNode(psisdNode, "RPB"), szrRPC);
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
			printf("psNode == NULL\n");
			return false;
		}

		CPLXMLNode* psisdNode = psNode->psNext;
		if(psisdNode == NULL)
		{
			printf("psisdNode == NULL\n");
			return false;
		}
			
		CPLXMLNode* psSatIdNode = CPLSearchXMLNode(psisdNode, "SATID");
		if(psSatIdNode != NULL)
			return true;
	}

	return false;
}