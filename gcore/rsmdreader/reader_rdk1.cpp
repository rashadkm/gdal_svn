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

#include <sstream>
#include <time.h>
#include <stdio.h>

#include "cplkeywordparser.h"

#include "reader_rdk1.h"

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

		int r = sscanf ( rsAcqisitionTime.c_str(), "%lu/%lu/%lu %lu:%lu:%lu.%*s", &iDay, &iMonth, &iYear, &iHours, &iMin, &iSec);
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

RDK1::RDK1(const char* pszFilename)
	:RSMDReader(pszFilename, "RDK1")
{
	CPLString osFilename = GDALFindAssociatedFile( pszFilename, "XML", NULL, 0 );

    osIMDSourceFilename = "";
    if( IsIMDValid(osFilename) )
        osIMDSourceFilename = osFilename;
};

const bool RDK1::IsFullCompliense() const
{
	if (osIMDSourceFilename != "")
		return true;

    return false;
}

void RDK1::ReadImageMetadata(CPLStringList& szrImageMetadata) const
{
	CPLStringList szrBadXMLMetadata;
	if(osIMDSourceFilename != "")
	{
		CPLXMLNode* psNode = CPLParseXMLFile(osIMDSourceFilename.c_str());
		if(psNode == NULL)
		{
			return;
		}

		CPLXMLNode* psRootNode = psNode->psNext;
		if(psRootNode == NULL)
		{
			return;
		}
		
		CPLStringList expulsionNodeNames;
		ReadXMLToStringList(psRootNode, expulsionNodeNames, szrBadXMLMetadata);
	}
	
	for(int i = 0; i < szrBadXMLMetadata.size(); i++)
	{
		char *ppszRootKey = NULL;				
		const char* value = CPLParseNameValue(szrBadXMLMetadata[i], &ppszRootKey);
		
		std::stringstream ss(value);

		while( ss.good() )
		{
			
			CPLString osLine = "";
			std::getline(ss, osLine);
			
			if(osLine.empty())
				continue;

			char *ppszKey = NULL;	

			/*
			* TODO Change CPLParseNameValue function for delete spaces before char '='
			*/
			const char* value = CPLParseNameValue(osLine.c_str(), &ppszKey);
			
			CPLString osKey(ppszKey);
			int i = osKey.size();
			while(osKey[i-1] == ' ')
			{
				osKey[i-1] = '\0';
				i--;
			}

			CPLString osFullKey = CPLString(ppszRootKey) + "." + osKey;
			szrImageMetadata.AddNameValue(osFullKey.c_str(), value);
		}
	}
}

void RDK1::GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const
{
	if( CSLFindName(szrImageMetadata.List(), "MSP_ROOT.cCodeKA") != -1)
	{
		CPLString SatelliteIdValue = CSLFetchNameValue(szrImageMetadata.List(), "MSP_ROOT.cCodeKA");
		szrCommonImageMetadata.SetNameValue(MDName_SatelliteId.c_str(), CPLStripQuotes(SatelliteIdValue).c_str());
	}

	if( CSLFindName(szrImageMetadata.List(), "Acquisition.dDateRoute") != -1 &&
			CSLFindName(szrImageMetadata.List(), "Acquisition.tTimeRoutePlan") != -1)
	{
		CPLString osAcqisitionTime = CSLFetchNameValue(szrImageMetadata.List(), "Acquisition.tTimeRoutePlan");
		CPLString osAcqisitionDate = CSLFetchNameValue(szrImageMetadata.List(), "Acquisition.dDateRoute");
		CPLString AcqisitionDateTime;

		GetAcqisitionTimeFromString(osAcqisitionDate + " " + osAcqisitionTime, AcqisitionDateTime);		
		szrCommonImageMetadata.SetNameValue(MDName_AcquisitionDateTime.c_str(), AcqisitionDateTime.c_str());
	}
}

void RDK1::ReadRPC(RSMDRPC& rRPC) const
{
	
}

const CPLStringList RDK1::DefineSourceFiles() const
{
	CPLStringList papszFileList;

	if(osIMDSourceFilename != "")
	{
		papszFileList.AddString(osIMDSourceFilename.c_str());
	}

	return papszFileList;
}

bool RDK1::IsIMDValid(const CPLString& psFilename) const
{
	if(psFilename != "")
	{
		CPLXMLNode* psNode = CPLParseXMLFile(psFilename.c_str());
		if(psNode == NULL)
			return false;

		psNode = psNode->psNext;
		if(psNode == NULL)
			return false;
			
		if( EQUAL(psNode->pszValue, "MSP_ROOT") )
			return true;
	}
	return false;
}
