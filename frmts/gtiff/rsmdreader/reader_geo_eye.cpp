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
#include "cpl_vsi_virtual.h"

#include "reader_geo_eye.h"

#include "remote_sensing_metadata.h"
#include "utils.h"

GeoEye::GeoEye(const char* pszFilename)
	:RSMDReader(pszFilename, "GeoEye")
{
	CPLString osDirName = CPLGetDirname(pszFilename);
	CPLString osBaseName = CPLGetBasename(pszFilename);
	
	osWKTRPCSourceFilename = CPLFormFilename( osDirName.c_str(), CPLSPrintf("%s_rpc", osBaseName.c_str()), ".txt" );

	VSIStatBufL sStatBuf;
	if( VSIStatExL( osWKTRPCSourceFilename.c_str(), &sStatBuf, VSI_STAT_EXISTS_FLAG ) != 0 )
    {
		osWKTRPCSourceFilename = "";
	}


	osIMDSourceFilename = "";
	VSIFilesystemHandler *poFSHandler = VSIFileManager::GetHandler( pszFilename );
	char **papszFiles = NULL;
	papszFiles = poFSHandler->ReadDir("./");
	for( int i = 0; papszFiles[i] != NULL; i++ )
    {
		CPLString osFileName(papszFiles[i]);
		int iFound = osFileName.find("_metadata");
		if( iFound != -1)
		{
			osIMDSourceFilename = CPLFormFilename( osDirName.c_str(), osFileName.c_str(), NULL );
			break;
		}
    }
	
};

const bool GeoEye::IsFullCompliense() const
{
	if (!osWKTRPCSourceFilename.empty() && !osIMDSourceFilename.empty())
		return true;

	return false;
}

void GeoEye::ReadImageMetadata(CPLStringList& szrImageMetadata) const
{	
    char **papszLines = CSLLoad( osIMDSourceFilename.c_str() );
    if( papszLines == NULL )
        return;
     
    for( int i = 0; papszLines[i] != NULL; i++ )
    {
        CPLString osLine(papszLines[i]);
        
        if( EQUALN( osLine, "Sensor:", 7) ||
            EQUALN( osLine, "Percent Cloud Cover:", 20) ||
            EQUALN( osLine, "Acquisition Date/Time:", 22) )
		{
			char *ppszKey = NULL;
			const char* value = CPLParseNameValue(osLine.c_str(), &ppszKey);

			szrImageMetadata.AddNameValue(ppszKey, value);
			
			CPLFree( ppszKey ); 
		}
    }
     
    CSLDestroy( papszLines );
}

void GeoEye::GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const
{
	if( CSLFindName(szrImageMetadata.List(), "Sensor") != -1)
	{
		CPLString osSatName = CSLFetchNameValue(szrImageMetadata.List(), "Sensor");
		szrCommonImageMetadata.SetNameValue(MDName_SatelliteId.c_str(), osSatName.c_str());
	}

	if( CSLFindName(szrImageMetadata.List(), "Percent Cloud Cover") != -1)
	{
		CPLString osCloudCover = CSLFetchNameValue(szrImageMetadata.List(), "Percent Cloud Cover");
		szrCommonImageMetadata.SetNameValue(MDName_CloudCover.c_str(), osCloudCover.c_str());
	}

	if( CSLFindName(szrImageMetadata.List(), "Acquisition Date/Time") != -1)
	{
		CPLString osAcqisitionTime = CSLFetchNameValue(szrImageMetadata.List(), "Acquisition Date/Time");
		szrCommonImageMetadata.SetNameValue(MDName_AcquisitionDateTime.c_str(), osAcqisitionTime.c_str());
	}
}

void GeoEye::ReadRPC(RSMDRPC& rRPC) const
{
	if (!osWKTRPCSourceFilename.empty())
	{
		ReadRPCFromWKT(rRPC);
	}
}
void GeoEye::ReadRPCFromWKT(RSMDRPC& rRPC) const
{
	CPLStringList szrRPC;
	if (osWKTRPCSourceFilename != "")
	{
		char **papszRPCMD = GDALLoadRPCFile( osWKTRPCSourceFilename.c_str(), NULL );

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
const CPLStringList GeoEye::DefineSourceFiles() const
{
	CPLStringList papszFileList;

	if(!osWKTRPCSourceFilename.empty() && !osIMDSourceFilename.empty())
	{
		papszFileList.AddString(osWKTRPCSourceFilename.c_str());
		papszFileList.AddString(osIMDSourceFilename.c_str());
	}
	
	return papszFileList;
}