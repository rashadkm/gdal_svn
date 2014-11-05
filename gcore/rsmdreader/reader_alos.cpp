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

#include <fstream>
#include <stdio.h>

#include "cplkeywordparser.h"

#include "reader_alos.h"
#include "remote_sensing_metadata.h"
#include "utils.h"

namespace
{
	const time_t GetAcqisitionTimeFromString(const CPLString& rsAcqisitionTime, CPLString& rsAcqisitionTimeFormatted)
	{
		size_t iYear;
		size_t iMonth;
		size_t iDay;
		size_t iHours = 0;
		size_t iMin = 0;
		size_t iSec = 0;

		int r = sscanf ( rsAcqisitionTime, "%4ul%2ul%2ul %ul:%ul:%ul.%*s", &iYear, &iMonth, &iDay, &iHours, &iMin, &iSec);

		// For Img_SceneCenterDateTime r = 20090428 07:53:50.116 
		// For Lbi_ObservationDate r = 3 (20100815)
		if (r != 6 && r != 3)
			return -1;
		
		tm tmDateTime;
		tmDateTime.tm_sec = iSec;
		tmDateTime.tm_min = iMin;
		tmDateTime.tm_hour = iHours;
		tmDateTime.tm_mday = iDay;
		tmDateTime.tm_mon = iMonth - 1;
		tmDateTime.tm_year = iYear - 1900;

		char buffer [80];
		/* size_t dCharsCount = */ strftime (buffer,80,AcquisitionDateTimeFormat,&tmDateTime);
		rsAcqisitionTimeFormatted.assign(&buffer[0]);

		return mktime(&tmDateTime);
	}
}

ALOS::ALOS(const char* pszFilename)
:RSMDReader(pszFilename, "ALOS")
{
    CPLString osDirName(CPLGetDirname(pszFilename));
    CPLString osFileName(CPLGetBasename(pszFilename));

    if (CPLStrnlen(osFileName, 255) > 4)
    {
        // Without IMG_ prefix
        CPLString osSceneProductInfo(osFileName.substr(4, osFileName.size() - 4));

        osIMDSourceFilename = CPLFormFilename(osDirName, "summary", ".txt");
        osRPCSourceFilename = CPLFormFilename(osDirName, CPLSPrintf("RPC-%s", osSceneProductInfo.c_str()), ".txt");
        
        if (!CPLCheckForFile((char*)osIMDSourceFilename.c_str(), NULL))
        {
            osIMDSourceFilename.clear();
        }

        if (!CPLCheckForFile((char*)osRPCSourceFilename.c_str(), NULL))
        {
            osRPCSourceFilename.clear();
        }
    }
}

const bool ALOS::IsFullCompliense() const
{
	if (!osIMDSourceFilename.empty())
		return true;
        
	return false;
}

void ALOS::ReadImageMetadata(CPLStringList& szrImageMetadata) const
{
	char **papszLines = CSLLoad( osIMDSourceFilename );
    if( papszLines == NULL )
        return;
     
    for( int i = 0; papszLines[i] != NULL; i++ )
    {
        CPLString osLine(papszLines[i]);
        
        char *ppszKey = NULL;				
		const char* value = CPLParseNameValue(osLine, &ppszKey);

		szrImageMetadata.AddNameValue(ppszKey, value);
		
		CPLFree( ppszKey );
    }
     
    CSLDestroy( papszLines );
}

void ALOS::GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const
{
	if( CSLFindName(szrImageMetadata.List(), "Img_SceneCenterDateTime") != -1)
	{
		CPLString osAcqisitionTime = CSLFetchNameValue(szrImageMetadata.List(), "Img_SceneCenterDateTime");
		CPLString osAcqisitionTimeF = "";
		GetAcqisitionTimeFromString(CPLStripQuotes(osAcqisitionTime), osAcqisitionTimeF);
		szrCommonImageMetadata.SetNameValue(MDName_AcquisitionDateTime, osAcqisitionTimeF);
	}
	else if ( CSLFindName(szrImageMetadata.List(), "Lbi_ObservationDate") != -1)
	{
		CPLString osAcqisitionTime = CSLFetchNameValue(szrImageMetadata.List(), "Lbi_ObservationDate");
		CPLString osAcqisitionTimeF = "";
		GetAcqisitionTimeFromString(CPLStripQuotes(osAcqisitionTime), osAcqisitionTimeF);
		szrCommonImageMetadata.SetNameValue(MDName_AcquisitionDateTime, osAcqisitionTimeF);
	}

	if( CSLFindName(szrImageMetadata.List(), "Lbi_Satellite") != -1)
	{
		CPLString SatelliteIdValue = CSLFetchNameValue(szrImageMetadata.List(), "Lbi_Satellite");
		szrCommonImageMetadata.SetNameValue(MDName_SatelliteId, CPLStripQuotes(SatelliteIdValue));
	}

	if( CSLFindName(szrImageMetadata.List(), "Img_CloudQuantityOfAllImage") != -1)
	{
		CPLString CloudCover = CSLFetchNameValue(szrImageMetadata.List(), "Img_CloudQuantityOfAllImage");
		szrCommonImageMetadata.SetNameValue(MDName_CloudCover, CPLStripQuotes(CloudCover));
	}
}

void ALOS::ReadRPC(RSMDRPC& rRPC) const
{
	if (osRPCSourceFilename != "")
	{
		
		std::ifstream ifs(osRPCSourceFilename);
		
		if ( !ifs.is_open() )
			return;
		
		char lineOffset[6];
		ifs.read(lineOffset,6);
		rRPC.lineOffset.Printf("%d", atoi(lineOffset));

		char sampOffset[5];
		ifs.read(sampOffset,5);
		rRPC.sampOffset.Printf("%d", atoi(sampOffset));

		char latOffset[8];
		ifs.read(latOffset,8);
		rRPC.latOffset.Printf("%f", atof(latOffset));
		
		char longOffset[9];
		ifs.read(longOffset,9);
		rRPC.longOffset.Printf("%f", atof(longOffset));

		char heightOffset[5];
		ifs.read(heightOffset,5);
		rRPC.heightOffset.Printf("%d", atoi(heightOffset));

		char lineScale[6];
		ifs.read(lineScale,6);
		rRPC.lineScale.Printf("%d", atoi(lineScale));

		char sampScale[5];
		ifs.read(sampScale,5);
		rRPC.sampScale.Printf("%d", atoi(sampScale));

		char latScale[8];
		ifs.read(latScale,8);
		rRPC.latScale.Printf("%f", atof(latScale));

		char longScale[9];
		ifs.read(longScale,9);
		rRPC.longScale.Printf("%f", atof(longScale));

		char heightScale[5];
		ifs.read(heightScale,5);
		rRPC.heightScale.Printf("%d", atoi(heightScale));
		
		char coeff[12];
		for(size_t i = 0; i < 20; i++)
		{
			ifs.read(coeff,12);
			rRPC.lineNumCoef += CPLString().Printf("%f ",atof(coeff));
		}

		for(size_t i = 0; i < 20; i++)
		{
			ifs.read(coeff,12);
			rRPC.lineDenCoef += CPLString().Printf("%f ",atof(coeff));
		}

		for(size_t i = 0; i < 20; i++)
		{
			ifs.read(coeff,12);
			rRPC.sampNumCoef += CPLString().Printf("%f ",atof(coeff));
		}

		for(size_t i = 0; i < 20; i++)
		{
			ifs.read(coeff,12);
			rRPC.sampDenCoef += CPLString().Printf("%f ",atof(coeff));
		}
	}
}

const CPLStringList ALOS::DefineSourceFiles() const
{
	CPLStringList papszFileList;

	if(!osIMDSourceFilename.empty())
		papszFileList.AddString(osIMDSourceFilename);
	if(!osRPCSourceFilename.empty())
		papszFileList.AddString(osRPCSourceFilename);


	return papszFileList;
}

