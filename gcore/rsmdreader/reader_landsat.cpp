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

#include "reader_landsat.h"

#include "remote_sensing_metadata.h"
#include "utils.h"

namespace
{
	int ReadIMDFile( const char *pszFilename, CPLStringList& oslIMD)
	{
        char **papszLines = CSLLoad( pszFilename );
        if(papszLines == NULL)
            return -1;
     
		std::vector<CPLString> osGroupNames;
		CPLString osGroupName;

        for( int i = 0; papszLines[i] != NULL; i++ )
        {
            CPLString osLine(papszLines[i]);

			if( EQUAL(osLine.c_str(), "END") )
				break;		

			char *ppszKey = NULL;
			const char* value = CPLGoodParseNameValue(osLine, &ppszKey, '=');

			if( value == NULL)
			{
				CPLFree(ppszKey);
				break;
			}

			if( EQUALN(ppszKey, "GROUP", 5) )
			{
				osGroupNames.push_back( CPLString(value) );

				osGroupName.clear();
				for(size_t i = 0; i < osGroupNames.size(); i++)
				{
					osGroupName += osGroupNames[i] + ".";
				}

			}
			else if( EQUALN(ppszKey, "END_GROUP", 9) )
			{
				osGroupNames.pop_back();

				osGroupName.clear();
				for(size_t i = 0; i < osGroupNames.size(); i++)
				{
					osGroupName += osGroupNames[i] + ".";
				}

			}
			else
			{
				if (osGroupName.empty())
					oslIMD.AddNameValue(ppszKey, value);
				else
				{
					oslIMD.AddNameValue( CPLSPrintf("%s%s", osGroupName.c_str(), ppszKey), value );
				}
			}

			CPLFree(ppszKey);
        }

        CSLDestroy( papszLines );

		return 0;
	}

	const time_t GetAcqisitionTimeFromString(const CPLString& rsAcqisitionTime, CPLString& rsAcqisitionTimeFormatted)
	{
		size_t iYear;
		size_t iMonth;
		size_t iDay;
		size_t iHours;
		size_t iMin;
		size_t iSec;
		
		int r = sscanf ( rsAcqisitionTime, "%ul-%ul-%ul %ul:%ul:%ul.%*s", &iYear, &iMonth, &iDay, &iHours, &iMin, &iSec);
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

Landsat::Landsat(const char* pszFilename)
	:RSMDReader(pszFilename, "Landsat")
{
	CPLString osDirName = CPLGetDirname(pszFilename);
	CPLString osBaseName = CPLGetBasename(pszFilename);
	
	CPLString sBase(osBaseName.substr(0, osBaseName.size()-4));

	osIMDSourceFilename = CPLFormFilename( osDirName, CPLSPrintf("%s_MTL", sBase.c_str()), ".txt" );

    if (!CPLCheckForFile((char*)osIMDSourceFilename.c_str(), NULL))
    {
        osIMDSourceFilename.clear();
	}
}

const bool Landsat::IsFullCompliense() const
{
	if( !osIMDSourceFilename.empty() )
	{
		return true;
	}

	return false;
}

void Landsat::ReadImageMetadata(CPLStringList& szrImageMetadata) const
{
	if( !osIMDSourceFilename.empty() )
	{
		ReadIMDFile(osIMDSourceFilename, szrImageMetadata);
	}
}

void Landsat::GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const
{
	if( CSLFindName(szrImageMetadata.List(), "L1_METADATA_FILE.PRODUCT_METADATA.SPACECRAFT_ID") != -1 )
	{
		CPLString SatelliteIdValue = CSLFetchNameValue(szrImageMetadata.List(), "L1_METADATA_FILE.PRODUCT_METADATA.SPACECRAFT_ID");
		szrCommonImageMetadata.SetNameValue(MDName_SatelliteId, CPLStripQuotes(SatelliteIdValue));
	}

	if( CSLFindName(szrImageMetadata.List(), "L1_METADATA_FILE.IMAGE_ATTRIBUTES.CLOUD_COVER") != -1 )
	{
		CPLString CloudCover = CSLFetchNameValue(szrImageMetadata.List(), "L1_METADATA_FILE.IMAGE_ATTRIBUTES.CLOUD_COVER");
		szrCommonImageMetadata.SetNameValue(MDName_CloudCover, CPLStripQuotes(CloudCover));
	}
	
	if( CSLFindName(szrImageMetadata.List(), "L1_METADATA_FILE.PRODUCT_METADATA.ACQUISITION_DATE") != -1 &&
			CSLFindName(szrImageMetadata.List(), "L1_METADATA_FILE.PRODUCT_METADATA.SCENE_CENTER_SCAN_TIME") != -1)
	{
		CPLString osAcqisitionTime = CSLFetchNameValue(szrImageMetadata.List(), "L1_METADATA_FILE.PRODUCT_METADATA.SCENE_CENTER_SCAN_TIME");
		CPLString osAcqisitionDate = CSLFetchNameValue(szrImageMetadata.List(), "L1_METADATA_FILE.PRODUCT_METADATA.ACQUISITION_DATE");
		
		CPLString AcqisitionDateTime;
		GetAcqisitionTimeFromString(CPLStripQuotes(osAcqisitionDate) + " " + CPLStripQuotes(osAcqisitionTime), AcqisitionDateTime);
		szrCommonImageMetadata.SetNameValue(MDName_AcquisitionDateTime, AcqisitionDateTime);
	}
	else if( CSLFindName(szrImageMetadata.List(), "L1_METADATA_FILE.PRODUCT_METADATA.DATE_ACQUIRED") != -1 &&
			CSLFindName(szrImageMetadata.List(), "L1_METADATA_FILE.PRODUCT_METADATA.SCENE_CENTER_TIME") != -1)
	{
		CPLString osAcqisitionTime = CSLFetchNameValue(szrImageMetadata.List(), "L1_METADATA_FILE.PRODUCT_METADATA.SCENE_CENTER_TIME");
		CPLString osAcqisitionDate = CSLFetchNameValue(szrImageMetadata.List(), "L1_METADATA_FILE.PRODUCT_METADATA.DATE_ACQUIRED");
		
		CPLString AcqisitionDateTime;
		GetAcqisitionTimeFromString(CPLStripQuotes(osAcqisitionDate) + " " + CPLStripQuotes(osAcqisitionTime), AcqisitionDateTime);
		szrCommonImageMetadata.SetNameValue(MDName_AcquisitionDateTime, AcqisitionDateTime);
	}
}

void Landsat::ReadRPC(RSMDRPC& rRPC) const
{

}

const CPLStringList Landsat::DefineSourceFiles() const
{
	CPLStringList papszFileList;
	if(!osIMDSourceFilename.empty())
	{
		papszFileList.AddString(osIMDSourceFilename);
	}

	return papszFileList;
}