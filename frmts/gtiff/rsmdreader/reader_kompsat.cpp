/******************************************************************************
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

#include <time.h>

#include "cplkeywordparser.h"

#include "reader_kompsat.h"

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


	int ReadIMDFile( const char *pszFilename, CPLStringList& oslRPC)
	{
        char **papszLines = CSLLoad( pszFilename );
        if(papszLines == NULL)
            return -1;
     
		CPLString osGroupName = "";

        for(int i = 0; papszLines[i] != NULL; i++ )
        {
            CPLString osLine(papszLines[i]);
            
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
			
			if (osGroupName.empty())
			{
				const char* value = CPLParseNameTabValue(osLine.c_str(), &ppszKey);
				oslRPC.AddNameValue(ppszKey, value);
			}
			else
			{
				int i;
				for( i = 0; osLine.c_str()[i] != '\0'; i++ )
				{
					if( osLine.c_str()[i] != '\t')
						break;
				}
				const char* value = CPLParseNameTabValue(osLine.c_str() + i, &ppszKey);
				oslRPC.AddNameValue(CPLString().Printf("%s.%s",osGroupName.c_str(), ppszKey).c_str(), value);
			}
			
			CPLFree( ppszKey ); 
        }
        
		return 0;
	}
}

Kompsat::Kompsat(const char* pszFilename)
	:RSMDReader(pszFilename, "Kompsat")
{
	osIMDSourceFilename = GDALFindAssociatedFile( pszFilename, "eph", NULL, 0 );
	osRPCSourceFilename = GDALFindAssociatedFile( pszFilename, "rpc", NULL, 0 );
	osIMDSourceFilenameTXT = GDALFindAssociatedFile( pszFilename, "txt", NULL, 0 );
};

const bool Kompsat::IsFullCompliense() const
{
	if (!osIMDSourceFilename.empty())
		return true;
        
	return false;
}

void Kompsat::ReadImageMetadata(CPLStringList& szrImageMetadata) const
{
	if (!osIMDSourceFilename.empty() || !osIMDSourceFilenameTXT.empty())
	{
		ReadImageMetadataFromWKT(szrImageMetadata);
	}
}

void Kompsat::ReadImageMetadataFromWKT(CPLStringList& szrImageMetadata) const
{
	if (!osIMDSourceFilename.empty())
	{
		ReadIMDFile( osIMDSourceFilename.c_str(), szrImageMetadata);
	}

	if (!osIMDSourceFilenameTXT.empty())
	{
		ReadIMDFile( osIMDSourceFilenameTXT.c_str(), szrImageMetadata);
	}
}

void Kompsat::GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const
{
	if( CSLFindName(szrImageMetadata.List(), "IMG_ACQISITION_START_TIME") != -1 &&
			CSLFindName(szrImageMetadata.List(), "IMG_ACQISITION_END_TIME") != -1 )
	{
		CPLString osTimeStart = CSLFetchNameValue(szrImageMetadata.List(), "IMG_ACQISITION_START_TIME");
		CPLString osTimeEnd = CSLFetchNameValue(szrImageMetadata.List(), "IMG_ACQISITION_END_TIME");
		CPLString osAcqisitionTime;

		if(GetAcqisitionTime(osTimeStart, osTimeEnd, CPLString("%d %d %d %d %d %d"), osAcqisitionTime) )
			szrCommonImageMetadata.SetNameValue(MDName_AcquisitionDateTime.c_str(), osAcqisitionTime.c_str());
		else
			szrCommonImageMetadata.SetNameValue(MDName_AcquisitionDateTime.c_str(), "unknown");
	}

	if( CSLFindName(szrImageMetadata.List(), "AUX_SATELLITE_NAME") != -1)
	{
		CPLString osSatName = CSLFetchNameValue(szrImageMetadata.List(), "AUX_SATELLITE_NAME");
		szrCommonImageMetadata.SetNameValue(MDName_SatelliteId.c_str(), osSatName.c_str());
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

	if(!osIMDSourceFilename.empty())
		papszFileList.AddString(osIMDSourceFilename.c_str());
		
	if(!osIMDSourceFilenameTXT.empty())
		papszFileList.AddString(osIMDSourceFilenameTXT.c_str());

	if(!osRPCSourceFilename.empty())
		papszFileList.AddString(osRPCSourceFilename.c_str());

	return papszFileList;
}
