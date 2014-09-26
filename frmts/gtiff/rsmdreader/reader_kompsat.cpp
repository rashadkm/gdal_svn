#include <iostream>
#include <fstream>
#include <time.h>
#include <string>

#include "cplkeywordparser.h"

#include "reader_kompsat.h"

#include "remote_sensing_metadata.h"
#include "utils.h"

namespace
{
	int ReadRPCFile( const char *pszFilename, CPLStringList& oslIMD)
	{
		std::ifstream ifs(pszFilename);
		
		if ( !ifs.is_open() )
			return -1;

		while( ifs.good() )
		{
			CPLString osLine = "";
			std::getline(ifs, osLine);
			
			char *ppszKey = NULL;				
			const char* value = CPLParseNameValue(osLine.c_str(), &ppszKey);

			oslIMD.AddNameValue(ppszKey, value);
			
			CPLFree( ppszKey ); 
		}

		ifs.close();

		return 0;
	}


	int ReadIMDFile( const char *pszFilename, CPLStringList& oslRPC)
	{
		std::ifstream ifs(pszFilename);
		
		if ( !ifs.is_open() )
			return -1;

		CPLString osGroupName = "";

		while( ifs.good() )
		{
			CPLString osLine = "";
			std::getline(ifs, osLine);

			char *ppszKey = NULL;				

			if(strstr(osLine.c_str(),"BEGIN_") != NULL && strstr(osLine.c_str(),"_BEGIN") == NULL)
			{
				int iFound = osLine.find("_BLOCK");
				
				char* ppszGroupName = (char *) CPLMalloc(iFound - 6);
				strncpy(ppszGroupName, osLine.c_str() + 6, iFound - 6 );
				osGroupName = ppszGroupName;
				osLine.Clear();
				continue;
			}

			if(strstr(osLine.c_str(),"END_") != NULL && strstr(osLine.c_str(),"_END") == NULL)
			{
				osGroupName = "";
				osLine.Clear();
				continue;
			}
			
			if (osGroupName == "")
			{
				const char* value = CPLParseNameTabValue(osLine.c_str(), &ppszKey);
				oslRPC.AddNameValue(ppszKey, value);
			}
			else
			{
				/*
					Not parse the parameters in groups
				*/

				/*
				int i;
				for( i = 0; osLine.c_str()[i] != '\0'; i++ )
				{
					if( osLine.c_str()[i] != '\t')
						break;
				}
				const char* value = CPLParseIMDNameValue(osLine.c_str() + i, &ppszKey);
				oslRPC.AddNameValue(CPLString().Printf("%s.%s",osGroupName.c_str(), ppszKey).c_str(), value);
				*/
			}
		}

		ifs.close();

		return 0;
	}

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

		int r1 = sscanf ( rsAcqisitionStartTime.c_str(), "%d %d %d %d %d %d", &iYearStart, &iMonthStart, &iDayStart, &iHoursStart, &iMinStart, &iSecStart);
		int r2 = sscanf ( rsAcqisitionEndTime.c_str(), "%d %d %d %d %d %d", &iYearEnd, &iMonthEnd, &iDayEnd, &iHoursEnd, &iMinEnd, &iSecEnd);
		
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

Kompsat::Kompsat(const char* pszFilename)
	:RSMDReader(pszFilename, "Kompsat")
{
	osIMDSourceFilename = GDALFindAssociatedFile( pszFilename, "eph", NULL, 0 );
	osRPCSourceFilename = GDALFindAssociatedFile( pszFilename, "rpc", NULL, 0 );
};

const bool Kompsat::IsFullCompliense() const
{
	if (osIMDSourceFilename != "" && osRPCSourceFilename != "")
		return true;
        
	return false;
}

void Kompsat::ReadImageMetadata(CPLStringList& szrImageMetadata) const
{
	if (osIMDSourceFilename != "" && osRPCSourceFilename != "")
	{
		ReadImageMetadataFromWKT(szrImageMetadata);
	}
}

void Kompsat::ReadImageMetadataFromWKT(CPLStringList& szrImageMetadata) const
{
	if (osIMDSourceFilename != "")
	{
		ReadIMDFile( osIMDSourceFilename.c_str(), szrImageMetadata);
	}
}

void Kompsat::GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const
{
	if( CSLFindName(szrImageMetadata.List(), "IMG_ACQISITION_START_TIME") != -1 &&
			CSLFindName(szrImageMetadata.List(), "IMG_ACQISITION_END_TIME") != -1)
	{
		CPLString osTimeStart = CSLFetchNameValue(szrImageMetadata.List(), "IMG_ACQISITION_START_TIME");
		CPLString osTimeEnd = CSLFetchNameValue(szrImageMetadata.List(), "IMG_ACQISITION_END_TIME");


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

	if( CSLFindName(szrImageMetadata.List(), "AUX_SATELLITE_NAME") != -1)
	{
		CPLString osSatName = CSLFetchNameValue(szrImageMetadata.List(), "AUX_SATELLITE_NAME");
		CPLString pszMD;
		pszMD.Printf("%s=%s",MDName_SatelliteId.c_str(), osSatName.c_str());
		szrCommonImageMetadata.AddString(pszMD.c_str());
	}
}

void Kompsat::ReadRPC(RSMDRPC& rRPC) const
{
	if (osRPCSourceFilename != "")
	{
		ReadRPCFromWKT(rRPC);
	}
}
void Kompsat::ReadRPCFromWKT(RSMDRPC& rRPC) const
{
	CPLStringList szrRPC;
	if (osRPCSourceFilename != "")
	{
        ReadRPCFile(osRPCSourceFilename.c_str(), szrRPC);
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

	CPLString lineNumCoef;
	for(int i = 1; i < 21; ++i)
	{
		CPLString coefName = CPLString().Printf("LINE_NUM_COEFF_%d",i);
		const char* szpCoef = szrRPC.FetchNameValue(coefName.c_str());
		if (szpCoef != NULL)
			 lineNumCoef = lineNumCoef + " " + CPLString(szpCoef);
		else
			return;
	}
	rRPC.lineNumCoef = lineNumCoef.c_str();
	
	CPLString lineDenCoef;
	for(int i = 1; i < 21; ++i)
	{
		CPLString coefName = CPLString().Printf("LINE_DEN_COEFF_%d",i);
		const char* szpCoef = szrRPC.FetchNameValue(coefName.c_str());
		if (szpCoef != NULL)
		{
			lineDenCoef = lineDenCoef + " " + CPLString(szpCoef);
		}
		else
			return;
	}
	rRPC.lineDenCoef = lineDenCoef.c_str();

	CPLString sampNumCoef;
	for(int i = 1; i < 21; ++i)
	{
		CPLString coefName = CPLString().Printf("SAMP_NUM_COEFF_%d",i);
		const char* szpCoef = szrRPC.FetchNameValue(coefName.c_str());
		if (szpCoef != NULL)
			 sampNumCoef = sampNumCoef + " " + CPLString(szpCoef);
		else
			return;
	}
	rRPC.sampNumCoef = sampNumCoef.c_str();

	CPLString sampDenCoef;
	for(int i = 1; i < 21; ++i)
	{
		CPLString coefName = CPLString().Printf("SAMP_DEN_COEFF_%d",i);
		const char* szpCoef = szrRPC.FetchNameValue(coefName.c_str());
		if (szpCoef != NULL)
			 sampDenCoef = sampDenCoef + " " + CPLString(szpCoef);
		else
			return;
	}
	rRPC.sampDenCoef = sampDenCoef.c_str();
}

const CPLStringList Kompsat::DefineSourceFiles() const
{
	CPLStringList papszFileList;

	if(osIMDSourceFilename != "" && osRPCSourceFilename != "")
	{
		papszFileList.AddString(osIMDSourceFilename.c_str());
		papszFileList.AddString(osRPCSourceFilename.c_str());
	}

	return papszFileList;
}
