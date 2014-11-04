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

#include "reader_eros.h"
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

		int r = sscanf ( rsAcqisitionTime.c_str(), "%lu-%lu-%lu,%lu:%lu:%lu.%*d", &iYear, &iMonth, &iDay, &iHours, &iMin, &iSec);

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

EROS::EROS(const char* pszFilename)
	:RSMDReader(pszFilename, "EROS")
{
	CPLString osDirName(CPLGetDirname(pszFilename));
	CPLString osFileName(CPLGetBasename(pszFilename));
	
	CPLString osBaseName( osFileName.substr(0, osFileName.find(".")) );
	
	osIMDSourceFilename = CPLFormFilename( osDirName.c_str(), osBaseName.c_str(), ".pass" );

	VSIStatBufL sStatBuf;
	if( VSIStatExL( osIMDSourceFilename.c_str(), &sStatBuf, VSI_STAT_EXISTS_FLAG ) != 0 )
    {
		osIMDSourceFilename = "";
	}
};

const bool EROS::IsFullCompliense() const
{
	if ( !osIMDSourceFilename.empty() )
		return true;
        
	return false;
}

void EROS::ReadImageMetadata(CPLStringList& szrImageMetadata) const
{
	char **papszLines = CSLLoad( osIMDSourceFilename.c_str() );
    if( papszLines == NULL )
        return;
     
    for( int i = 0; papszLines[i] != NULL; i++ )
    {
        CPLString osLine(papszLines[i]);
        
        char *ppszKey = NULL;
		const char* value = CPLGoodParseNameValue(osLine.c_str(), &ppszKey, ' ');

		szrImageMetadata.AddNameValue(ppszKey, value);
		
		CPLFree( ppszKey );
    }
     
    CSLDestroy( papszLines );
}

void EROS::GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const
{
	if( CSLFindName(szrImageMetadata.List(), "satellite") != -1)
	{
		CPLString SatelliteIdValue = CSLFetchNameValue(szrImageMetadata.List(), "satellite");
		szrCommonImageMetadata.SetNameValue(MDName_SatelliteId.c_str(), SatelliteIdValue.c_str());
	}

	if( CSLFindName(szrImageMetadata.List(), "sweep_start_utc") != -1 &&
			CSLFindName(szrImageMetadata.List(), "sweep_end_utc") != -1)
	{
		CPLString osTimeStart = CSLFetchNameValue(szrImageMetadata.List(), "sweep_start_utc");
		CPLString osTimeEnd = CSLFetchNameValue(szrImageMetadata.List(), "sweep_end_utc");
		CPLString osAcqisitionTime;
		
		if(GetAcqisitionMidTime(osTimeStart, osTimeEnd, osAcqisitionTime))
			szrCommonImageMetadata.SetNameValue(MDName_AcquisitionDateTime.c_str(), osAcqisitionTime.c_str());
		else
			szrCommonImageMetadata.SetNameValue(MDName_AcquisitionDateTime.c_str(), "unknown");
	}
}

void EROS::ReadRPC(RSMDRPC& rRPC) const
{
	
}

const CPLStringList EROS::DefineSourceFiles() const
{
	CPLStringList papszFileList;

	if(!osIMDSourceFilename.empty())
	{
		papszFileList.AddString(osIMDSourceFilename.c_str());
	}

	return papszFileList;
}
