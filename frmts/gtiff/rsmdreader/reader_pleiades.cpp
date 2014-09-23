#include "cplkeywordparser.h"

#include "reader_pleiades.h"

#include "remote_sensing_metadata.h"
#include "utils.h"

Pleiades::Pleiades(const char* pszFilename)
	:RSMDReader(pszFilename, "Pleiades")
{
	CPLString osDirName = CPLGetDirname(pszFilename);
	CPLString osBaseName = CPLGetBasename(pszFilename);
	osBaseName.replace(0,4,"");

	osXMLIMDSourceFilename = CPLString().Printf("%s/DIM_%s.XML",osDirName.c_str(), osBaseName.c_str());
	osXMLRPCSourceFilename = CPLString().Printf("%s/RPC_%s.XML",osDirName.c_str(), osBaseName.c_str());

	VSIStatBufL sStatBuf;
	if( VSIStatExL( osXMLIMDSourceFilename.c_str(), &sStatBuf, VSI_STAT_EXISTS_FLAG ) != 0 )
    {
		osXMLIMDSourceFilename = "";
	}
	if( VSIStatExL( osXMLRPCSourceFilename.c_str(), &sStatBuf, VSI_STAT_EXISTS_FLAG ) != 0 )
    {
		osXMLRPCSourceFilename = "";
	}
};

const bool Pleiades::IsFullCompliense() const
{
	if (!osXMLIMDSourceFilename.empty() && !osXMLRPCSourceFilename.empty())
	{
		printf("	>>> return true\n");
		return true;
	}

	return false;
}

void Pleiades::ReadImageMetadata(CPLStringList& szrImageMetadata) const
{
	ReadImageMetadataFromXML(szrImageMetadata);
}

void Pleiades::ReadImageMetadataFromXML(CPLStringList& szrImageMetadata) const
{
	
	if(osXMLIMDSourceFilename != "")
	{
		CPLXMLNode* psNode = CPLParseXMLFile(osXMLIMDSourceFilename.c_str());
		if(psNode == NULL)
			return;

		CPLXMLNode* psDimapNode = psNode->psNext->psNext;
		if(psDimapNode == NULL)
			printf("Dimap_Document node not found\n");

		ReadXML(CPLSearchXMLNode(psDimapNode,"Dataset_Sources"), szrImageMetadata);
	}
	
}

void Pleiades::GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const
{
	//if( CSLFindName(szrImageMetadata.List(), "Dataset_Sources.Source_Identification.Strip_Source.INSTRUMENT_INDEX") != -1)
	if( CSLFindName(szrImageMetadata.List(), "Source_Identification.Strip_Source.INSTRUMENT_INDEX") != -1)
	{
		//CPLString SatelliteIdValue = CSLFetchNameValue(szrImageMetadata.List(), "Dataset_Sources.Source_Identification.Strip_Source.INSTRUMENT_INDEX");
		CPLString SatelliteIdValue = CSLFetchNameValue(szrImageMetadata.List(), "Source_Identification.Strip_Source.INSTRUMENT_INDEX");
		CPLString pszMD = MDName_SatelliteId + "=" + SatelliteIdValue;
		szrCommonImageMetadata.AddString(pszMD.c_str());
	}
}

void Pleiades::ReadRPC(RSMDRPC& rRPC) const
{
	ReadRPCFromXML(rRPC);
}
void Pleiades::ReadRPCFromXML(RSMDRPC& rRPC) const
{
	
	CPLStringList szrRPC;
	if(osXMLRPCSourceFilename != "")
	{
		CPLXMLNode* psNode = CPLParseXMLFile(osXMLRPCSourceFilename.c_str());
		if(psNode == NULL)
			return;

		CPLXMLNode* psRootNode = psNode->psNext;
		if(psRootNode == NULL)
			printf("Dimap_Document node not found\n");

		CPLStringList szRPCdata;
		ReadXML(CPLSearchXMLNode(psRootNode, "Global_RFM"), szrRPC);
	}

	const char* szpLineOffset = szrRPC.FetchNameValue("RFM_Validity.LINE_OFF");
	if (szpLineOffset != NULL)
		rRPC.lineOffset = CPLString(szpLineOffset);

	const char* szpSampOffset = szrRPC.FetchNameValue("RFM_Validity.SAMP_OFF");
	if (szpSampOffset != NULL) 
		rRPC.sampOffset = szpSampOffset;

	const char* szpLatOffset = szrRPC.FetchNameValue("RFM_Validity.LAT_OFF");
	if (szpLatOffset != NULL) 
		rRPC.latOffset = CPLString(szpLatOffset);

	const char* szpLongOffset = szrRPC.FetchNameValue("RFM_Validity.LONG_OFF");
	if (szpLongOffset != NULL) 
		rRPC.longOffset = CPLString(szpLongOffset);

	const char* szpHeightOffset = szrRPC.FetchNameValue("RFM_Validity.HEIGHT_OFF");
	if (szpHeightOffset != NULL) 
		rRPC.heightOffset = CPLString(szpHeightOffset);

	const char* szpLineScale = szrRPC.FetchNameValue("RFM_Validity.LINE_SCALE");
	if (szpLineScale != NULL) 
		rRPC.lineScale = CPLString(szpLineScale);

	const char* szpSampScale = szrRPC.FetchNameValue("RFM_Validity.SAMP_SCALE");
	if (szpSampScale != NULL) 
		rRPC.sampScale = CPLString(szpSampScale);

	const char* szpLatScale = szrRPC.FetchNameValue("RFM_Validity.LAT_SCALE");
	if (szpLatScale != NULL) 
		rRPC.latScale = CPLString(szpLatScale);

	const char* szpLongScale = szrRPC.FetchNameValue("RFM_Validity.LONG_SCALE");
	if (szpLongScale != NULL) 
		rRPC.longScale = CPLString(szpLongScale);

	const char* szpHeightScale = szrRPC.FetchNameValue("RFM_Validity.HEIGHT_SCALE");
	if (szpHeightScale != NULL) 
		rRPC.heightScale = CPLString(szpHeightScale);

	CPLString lineNumCoef;
	for(int i = 1; i < 21; ++i)
	{
		CPLString coefName = CPLString().Printf("Direct_Model.LINE_NUM_COEFF_%d",i);
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
		CPLString coefName = CPLString().Printf("Direct_Model.LINE_DEN_COEFF_%d",i);
		const char* szpCoef = szrRPC.FetchNameValue(coefName.c_str());
		if (szpCoef != NULL)
			 lineDenCoef = lineDenCoef + " " + CPLString(szpCoef);
		else
			return;
	}
	rRPC.lineDenCoef = lineDenCoef.c_str();

	CPLString sampNumCoef;
	for(int i = 1; i < 21; ++i)
	{
		CPLString coefName = CPLString().Printf("Direct_Model.SAMP_NUM_COEFF_%d",i);
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
		CPLString coefName = CPLString().Printf("Direct_Model.SAMP_DEN_COEFF_%d",i);
		const char* szpCoef = szrRPC.FetchNameValue(coefName.c_str());
		if (szpCoef != NULL)
			 sampDenCoef = sampDenCoef + " " + CPLString(szpCoef);
		else
			return;
	}
	rRPC.sampDenCoef = sampDenCoef.c_str();
}

const CPLStringList Pleiades::DefineSourceFiles() const
{
	CPLStringList papszFileList;
	if(osXMLIMDSourceFilename != "")
	{
		papszFileList.AddString(osXMLIMDSourceFilename.c_str());
	}
	if(osXMLRPCSourceFilename != "")
	{
		papszFileList.AddString(osXMLRPCSourceFilename.c_str());
	}
	return papszFileList;
}