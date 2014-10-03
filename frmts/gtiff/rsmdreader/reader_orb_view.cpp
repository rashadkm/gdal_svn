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
			const char* value = CPLParseNameValue(osLine.c_str(), &ppszKey);

			oslIMD.AddNameValue(ppszKey, value);
			
			CPLFree( ppszKey ); 
        }
         
        CSLDestroy( papszLines );
        
		return 0;
	}
}

OrbView::OrbView(const char* pszFilename)
	:RSMDReader(pszFilename, "OrbView")
{
	osIMDSourceFilename = GDALFindAssociatedFile( pszFilename, "pvl", NULL, 0 );
	CPLString osDirName = CPLGetDirname(pszFilename);
	CPLString osBaseName = CPLGetBasename(pszFilename);
	osRPCSourceFilename = CPLString().Printf( "%s/%s_rpc.txt", osDirName.c_str(), osBaseName.c_str() );	
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

		VSILFILE *fp = VSIFOpenL( osIMDSourceFilename.c_str(), "r" );

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
		szrCommonImageMetadata.SetNameValue(MDName_SatelliteId.c_str(), SatelliteIdValue);
	}

	if( CSLFindName(szrImageMetadata.List(), "productInfo.productCloudCoverPercentage") != -1 )
	{
		CPLString osCloudCoverValue = CSLFetchNameValue(szrImageMetadata.List(), "productInfo.productCloudCoverPercentage");
		szrCommonImageMetadata.SetNameValue(MDName_CloudCover.c_str(), osCloudCoverValue);
	}
	
	if( CSLFindName(szrImageMetadata.List(), "inputImageInfo.firstLineAcquisitionDateTime") != -1 )
	{
		CPLString osAcqisitionTime = CSLFetchNameValue(szrImageMetadata.List(), "inputImageInfo.firstLineAcquisitionDateTime");
		szrCommonImageMetadata.SetNameValue(MDName_AcquisitionDateTime.c_str(), osAcqisitionTime);
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

const CPLStringList OrbView::DefineSourceFiles() const
{
	CPLStringList papszFileList;

	if(osIMDSourceFilename != "" && osRPCSourceFilename != "")
	{
		papszFileList.AddString(osIMDSourceFilename.c_str());
		papszFileList.AddString(osRPCSourceFilename.c_str());
	}

	return papszFileList;
}
