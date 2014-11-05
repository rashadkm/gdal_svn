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

#include "reader_orb_view.h"

#include "remote_sensing_metadata.h"
#include "utils.h"

namespace
{
	int ReadRPCFile( const char *pszFilename, CPLStringList& oslIMD)
	{
        
        char **papszLines = CSLLoad( pszFilename );
        if(papszLines == NULL)
            return -1;
     
        for(int i = 0; papszLines[i] != NULL; i++ )
        {
            CPLString osLine(papszLines[i]);
            
            char *ppszKey = NULL;				
			const char* value = CPLParseNameValue(osLine, &ppszKey);

			oslIMD.AddNameValue(ppszKey, value);
			
			CPLFree( ppszKey ); 
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

		int r = sscanf ( rsAcqisitionTime, "%ul-%ul-%ulT%ul:%ul:%ul.%*s", &iYear, &iMonth, &iDay, &iHours, &iMin, &iSec);
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

OrbView::OrbView(const char* pszFilename)
	:RSMDReader(pszFilename, "OrbView")
{
	osIMDSourceFilename = GDALFindAssociatedFile( pszFilename, "pvl", NULL, 0 );
	
	CPLString osDirName = CPLGetDirname(pszFilename);
	CPLString osBaseName = CPLGetBasename(pszFilename);
	osRPCSourceFilename = CPLFormFilename( osDirName, CPLSPrintf("%s_rpc", osBaseName.c_str()), ".txt" );

    if (!CPLCheckForFile((char*)osRPCSourceFilename.c_str(), NULL))
    {
		osRPCSourceFilename = "";
	}
};

const bool OrbView::IsFullCompliense() const
{
	if (osIMDSourceFilename != "" && osRPCSourceFilename != "")
		return true;

	return false;
}

void OrbView::ReadImageMetadata(CPLStringList& szrImageMetadata) const
{
	if (osIMDSourceFilename != "")
	{
		ReadImageMetadataFromWKT(szrImageMetadata);
	}
}

void OrbView::ReadImageMetadataFromWKT(CPLStringList& szrImageMetadata) const
{
	if (osIMDSourceFilename != "")
	{
		CPLKeywordParser oParser;

		VSILFILE *fp = VSIFOpenL( osIMDSourceFilename, "r" );

		if( fp == NULL )
			return;

		if( !oParser.Ingest( fp ) )
		{
			VSIFCloseL( fp );
			return;
		}

		VSIFCloseL( fp );

		/* -------------------------------------------------------------------- */
		/*      Consider version changing.                                      */
		/* -------------------------------------------------------------------- */
		char **papszPVLMD = CSLDuplicate( oParser.GetAllKeywords() );

		if( papszPVLMD != NULL )
		{
			papszPVLMD = CSLSetNameValue( papszPVLMD, 
											"md_type", "pvl" );
	        
			for(int i = 0; papszPVLMD[i] != NULL; i++ )
				szrImageMetadata.AddString(papszPVLMD[i]);

			CSLDestroy( papszPVLMD );
		}
	}
}

void OrbView::GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const
{
	if( CSLFindName(szrImageMetadata.List(), "sensorInfo.satelliteName") != -1 )
	{
		CPLString SatelliteIdValue = CSLFetchNameValue(szrImageMetadata.List(), "sensorInfo.satelliteName");
		szrCommonImageMetadata.SetNameValue(MDName_SatelliteId, CPLStripQuotes(SatelliteIdValue));
	}

	if( CSLFindName(szrImageMetadata.List(), "productInfo.productCloudCoverPercentage") != -1 )
	{
		CPLString osCloudCoverValue = CSLFetchNameValue(szrImageMetadata.List(), "productInfo.productCloudCoverPercentage");
		szrCommonImageMetadata.SetNameValue(MDName_CloudCover, CPLStripQuotes(osCloudCoverValue));
	}
	
	if( CSLFindName(szrImageMetadata.List(), "inputImageInfo.firstLineAcquisitionDateTime") != -1 )
	{
		CPLString osAcqisitionTime;
		GetAcqisitionTimeFromString(CSLFetchNameValue(szrImageMetadata.List(), "inputImageInfo.firstLineAcquisitionDateTime"), osAcqisitionTime);
		szrCommonImageMetadata.SetNameValue(MDName_AcquisitionDateTime, osAcqisitionTime);
	}
}

void OrbView::ReadRPC(RSMDRPC& rRPC) const
{
	if (osRPCSourceFilename != "")
	{
		ReadRPCFromWKT(rRPC);
	}
}
void OrbView::ReadRPCFromWKT(RSMDRPC& rRPC) const
{
	CPLStringList szrRPC;
	if (osRPCSourceFilename != "")
	{
        ReadRPCFile(osRPCSourceFilename, szrRPC);
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
		CPLString coefName = CPLString().Printf("LINE_DEN_COEFF_%d",i);
		const char* szpCoef = szrRPC.FetchNameValue(coefName);
		if (szpCoef != NULL)
		{
			lineDenCoef = lineDenCoef + " " + CPLString(szpCoef);
		}
		else
			return;
	}
	rRPC.lineDenCoef = lineDenCoef;

	CPLString sampNumCoef;
	for(int i = 1; i < 21; ++i)
	{
		CPLString coefName = CPLString().Printf("SAMP_NUM_COEFF_%d",i);
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
		CPLString coefName = CPLString().Printf("SAMP_DEN_COEFF_%d",i);
		const char* szpCoef = szrRPC.FetchNameValue(coefName);
		if (szpCoef != NULL)
			 sampDenCoef = sampDenCoef + " " + CPLString(szpCoef);
		else
			return;
	}
	rRPC.sampDenCoef = sampDenCoef;
}

const CPLStringList OrbView::DefineSourceFiles() const
{
	CPLStringList papszFileList;

	if(!osIMDSourceFilename.empty() && !osRPCSourceFilename.empty())
	{
		papszFileList.AddString(osIMDSourceFilename);
		papszFileList.AddString(osRPCSourceFilename);
	}

	return papszFileList;
}
