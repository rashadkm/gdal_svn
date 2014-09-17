#include "cplkeywordparser.h"

#include "reader_digital_globe.h"

#include "remote_sensing_metadata.h"
#include "CPLXML_utils.h"

DigitalGlobe::DigitalGlobe(const char* pszFilename)
	:RSMDReader(pszFilename, "DigitalGlobe")
{
	
	osIMDSourceFilename = GDALFindAssociatedFile( pszFilename, "IMD", NULL, 0 );
	osRPBSourceFilename = GDALFindAssociatedFile( pszFilename, "RPB", NULL, 0 );
	if (osIMDSourceFilename == "" && osRPBSourceFilename == "")
		osXMLSourceFilename = GDALFindAssociatedFile( pszFilename, "XML", NULL, 0 );
	else
		osXMLSourceFilename = "";
		
	//osXMLSourceFilename = GDALFindAssociatedFile( pszFilename, "XML", NULL, 0 );
};

const bool DigitalGlobe::IsFullCompliense() const
{
	if (osIMDSourceFilename != "" && osRPBSourceFilename != "")
		return true;
	else if(osXMLSourceFilename != "")
		return true;

	return false;
}

void DigitalGlobe::ReadImageMetadata(CPLStringList& szrImageMetadata) const
{
	if (osIMDSourceFilename != "" && osRPBSourceFilename != "")
	{
		ReadImageMetadataFromWKT(szrImageMetadata);
	}
	else if(osXMLSourceFilename != "")
	{
		ReadImageMetadataFromXML(szrImageMetadata);
	}
}

void DigitalGlobe::ReadImageMetadataFromWKT(CPLStringList& szrImageMetadata) const
{
	if (osIMDSourceFilename != "")
	{
		char **papszIMDMD = GDALLoadIMDFile( osIMDSourceFilename.c_str(), NULL );

		if( papszIMDMD != NULL )
        {
			for(int i = 0; papszIMDMD[i] != NULL; i++ )
				szrImageMetadata.AddString(papszIMDMD[i]);

            CSLDestroy( papszIMDMD );
        }
	}
}

void DigitalGlobe::ReadImageMetadataFromXML(CPLStringList& szrImageMetadata) const
{
	if(osXMLSourceFilename != "")
	{
		CPLXMLNode* psNode = CPLParseXMLFile(osXMLSourceFilename.c_str());
		if(psNode == NULL)
			return;

		CPLXMLNode* psisdNode = psNode->psNext;
		if(psisdNode == NULL)
			printf("isd not found\n");

		CPLStringList szIMDdata;
		ReadXML(CPLSearchXMLNode(psisdNode, "IMD"), szrImageMetadata);
	}
}

void DigitalGlobe::GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const
{
	if( CSLFindName(szrImageMetadata.List(), "IMAGE.SATID") != -1)
	{
		CPLString SatelliteIdValue = CSLFetchNameValue(szrImageMetadata.List(), "IMAGE.SATID");
		CPLString pszMD = MDName_SatelliteId + "=" + SatelliteIdValue;
		szrCommonImageMetadata.AddString(pszMD.c_str());
	}
	if( CSLFindName(szrImageMetadata.List(), "IMAGE_1.SATID") != -1)
	{
		CPLString SatelliteIdValue = CSLFetchNameValue(szrImageMetadata.List(), "IMAGE_1.SATID");
		CPLString pszMD = MDName_SatelliteId + "=" + SatelliteIdValue;
		szrCommonImageMetadata.AddString(pszMD.c_str());
	}
}

void DigitalGlobe::ReadRPC(RSMDRPC& rRPC) const
{
	if (osIMDSourceFilename != "" && osRPBSourceFilename != "")
	{
		ReadRPCFromWKT(rRPC);
	}
	else if(osXMLSourceFilename != "")
	{
		ReadRPCFromXML(rRPC);
	}
}
void DigitalGlobe::ReadRPCFromWKT(RSMDRPC& rRPC) const
{
	CPLStringList szrRPC;
	if (osRPBSourceFilename != "")
	{
		char **papszRPCMD = GDALLoadRPBFile( osRPBSourceFilename.c_str(), NULL );

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
void DigitalGlobe::ReadRPCFromXML(RSMDRPC& rRPC) const
{
	CPLStringList szrRPC;
	if(osXMLSourceFilename != "")
	{
		CPLXMLNode* psNode = CPLParseXMLFile(osXMLSourceFilename.c_str());
		if(psNode == NULL)
			return;

		CPLXMLNode* psisdNode = psNode->psNext;
		if(psisdNode == NULL)
			printf("isd not found\n");

		CPLStringList szRPCdata;
		ReadXML(CPLSearchXMLNode(psisdNode, "RPB"), szrRPC);
	}

	const char* szpLineOffset = szrRPC.FetchNameValue("image.lineoffset");
	if (szpLineOffset != NULL)
		rRPC.lineOffset = CPLString(szpLineOffset);

	const char* szpSampOffset = szrRPC.FetchNameValue("image.sampOffset");
	if (szpSampOffset != NULL) 
		rRPC.sampOffset = szpSampOffset;

	const char* szpLatOffset = szrRPC.FetchNameValue("image.latOffset");
	if (szpLatOffset != NULL) 
		rRPC.latOffset = CPLString(szpLatOffset);

	const char* szpLongOffset = szrRPC.FetchNameValue("image.longOffset");
	if (szpLongOffset != NULL) 
		rRPC.longOffset = CPLString(szpLongOffset);

	const char* szpHeightOffset = szrRPC.FetchNameValue("image.heightOffset");
	if (szpHeightOffset != NULL) 
		rRPC.heightOffset = CPLString(szpHeightOffset);

	const char* szpLineScale = szrRPC.FetchNameValue("image.lineScale");
	if (szpLineScale != NULL) 
		rRPC.lineScale = CPLString(szpLineScale);

	const char* szpSampScale = szrRPC.FetchNameValue("image.sampScale");
	if (szpSampScale != NULL) 
		rRPC.sampScale = CPLString(szpSampScale);

	const char* szpLatScale = szrRPC.FetchNameValue("image.latScale");
	if (szpLatScale != NULL) 
		rRPC.latScale = CPLString(szpLatScale);

	const char* szpLongScale = szrRPC.FetchNameValue("image.longScale");
	if (szpLongScale != NULL) 
		rRPC.longScale = CPLString(szpLongScale);

	const char* szpHeightScale = szrRPC.FetchNameValue("image.heightScale");
	if (szpHeightScale != NULL) 
		rRPC.heightScale = CPLString(szpHeightScale);

	const char* szpLineNumCoef = szrRPC.FetchNameValue("image.lineNumCoefList.lineNumCoef");
	if (szpLineNumCoef != NULL)
		rRPC.lineNumCoef = CPLString(szpLineNumCoef);

	const char* szpLineDenCoef = szrRPC.FetchNameValue("image.lineDenCoefList.lineDenCoef");
	if (szpLineDenCoef != NULL)
		rRPC.lineDenCoef = CPLString(szpLineDenCoef);

	const char* szpSampNumCoef = szrRPC.FetchNameValue("image.sampNumCoefList.sampNumCoef");
	if (szpSampNumCoef != NULL)
		rRPC.sampNumCoef = CPLString(szpSampNumCoef);

	const char* szpSampDenCoef = szrRPC.FetchNameValue("image.sampDenCoefList.sampDenCoef");
	if (szpSampDenCoef != NULL)
		rRPC.sampDenCoef = CPLString(szpSampDenCoef);
}

const CPLStringList DigitalGlobe::DefineSourceFiles() const
{
	CPLStringList papszFileList;

	if(osIMDSourceFilename != "" && osRPBSourceFilename != "")
	{
		papszFileList.AddString(osIMDSourceFilename.c_str());
		papszFileList.AddString(osRPBSourceFilename.c_str());
	}
	else if(osXMLSourceFilename != "")
		papszFileList.AddString(osXMLSourceFilename.c_str());

	return papszFileList;
}