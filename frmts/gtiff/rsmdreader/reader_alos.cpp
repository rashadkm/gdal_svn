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

#include "cplkeywordparser.h"

#include "reader_alos.h"
#include "remote_sensing_metadata.h"
#include "utils.h"

ALOS::ALOS(const char* pszFilename)
	:RSMDReader(pszFilename, "ALOS")
{
	CPLString osDirName(CPLGetDirname(pszFilename));
	CPLString osFileName(CPLGetBasename(pszFilename));
	
	// Without IMG_ prefix
	CPLString osSceneProductInfo(osFileName.substr(4, osFileName.size() - 4));
	
	osIMDSourceFilename = CPLFormFilename( osDirName.c_str(), "summary", ".txt" );
	osRPCSourceFilename = CPLFormFilename( osDirName.c_str(), CPLSPrintf("RPC-%s", osSceneProductInfo.c_str()), ".txt" );

	VSIStatBufL sStatBuf;
	if( VSIStatExL( osIMDSourceFilename.c_str(), &sStatBuf, VSI_STAT_EXISTS_FLAG ) != 0 )
    {
		osIMDSourceFilename = "";
	}	

	if( VSIStatExL( osRPCSourceFilename.c_str(), &sStatBuf, VSI_STAT_EXISTS_FLAG ) != 0 )
    {
		osRPCSourceFilename = "";
	}

};

const bool ALOS::IsFullCompliense() const
{
	if (!osIMDSourceFilename.empty())
		return true;
        
	return false;
}

void ALOS::ReadImageMetadata(CPLStringList& szrImageMetadata) const
{
	char **papszLines = CSLLoad( osIMDSourceFilename.c_str() );
    if( papszLines == NULL )
        return;
     
    for( int i = 0; papszLines[i] != NULL; i++ )
    {
        CPLString osLine(papszLines[i]);
        
        char *ppszKey = NULL;				
		const char* value = CPLParseNameValue(osLine.c_str(), &ppszKey);

		szrImageMetadata.AddNameValue(ppszKey, value);
		
		CPLFree( ppszKey );
    }
     
    CSLDestroy( papszLines );
}

void ALOS::GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const
{
	if( CSLFindName(szrImageMetadata.List(), "Lbi_Satellite") != -1)
	{
		CPLString SatelliteIdValue = CSLFetchNameValue(szrImageMetadata.List(), "Lbi_Satellite");
		szrCommonImageMetadata.SetNameValue(MDName_SatelliteId.c_str(), SatelliteIdValue.c_str());
	}

	if( CSLFindName(szrImageMetadata.List(), "Img_CloudQuantityOfAllImage") != -1)
	{
		CPLString CloudCover = CSLFetchNameValue(szrImageMetadata.List(), "Img_CloudQuantityOfAllImage");
		szrCommonImageMetadata.SetNameValue(MDName_CloudCover.c_str(), CloudCover.c_str());
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
		papszFileList.AddString(osIMDSourceFilename.c_str());
	if(!osRPCSourceFilename.empty())
		papszFileList.AddString(osRPCSourceFilename.c_str());


	return papszFileList;
}

