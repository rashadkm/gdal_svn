#include "cplkeywordparser.h"

#include "reader_spot.h"

#include "remote_sensing_metadata.h"
#include "utils.h"

Spot::Spot(const char* pszFilename)
	:RSMDReader(pszFilename, "Spot")
{
	CPLString osDirName = CPLGetDirname(pszFilename);

	osIMDSourceFilename = CPLString().Printf("%s/METADATA.DIM",osDirName.c_str());

	VSIStatBufL sStatBuf;
	if( VSIStatExL( osIMDSourceFilename.c_str(), &sStatBuf, VSI_STAT_EXISTS_FLAG ) != 0 )
    {
		osIMDSourceFilename = "";
	}
};

const bool Spot::IsFullCompliense() const
{
	if( !osIMDSourceFilename.empty() )
	{
		return true;
	}
	return false;
}

void Spot::ReadImageMetadata(CPLStringList& szrImageMetadata) const
{
	ReadImageMetadataFromXML(szrImageMetadata);
}

void Spot::ReadImageMetadataFromXML(CPLStringList& szrImageMetadata) const
{
	
	if(osIMDSourceFilename != "")
	{
		CPLXMLNode* psNode = CPLParseXMLFile(osIMDSourceFilename.c_str());
		if(psNode == NULL)
			return;

		CPLXMLNode* psDimapNode = psNode->psNext->psNext;
		if(psDimapNode == NULL)
			printf("Dimap_Document node not found\n");

		ReadXML(CPLSearchXMLNode(psDimapNode,"Dataset_Sources"), szrImageMetadata);
	}
	
}

void Spot::GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const
{
	if( CSLFindName(szrImageMetadata.List(), "Source_Information.Scene_Source.MISSION") != -1 &&
		CSLFindName(szrImageMetadata.List(), "Source_Information.Scene_Source.MISSION_INDEX") != -1)
	{
		CPLString MissionValue = CSLFetchNameValue(szrImageMetadata.List(), "Source_Information.Scene_Source.MISSION");
		CPLString MissionIndexValue = CSLFetchNameValue(szrImageMetadata.List(), "Source_Information.Scene_Source.MISSION_INDEX");
		CPLString pszMD = MDName_SatelliteId + "=" + MissionValue + " " + MissionIndexValue;
		szrCommonImageMetadata.AddString(pszMD.c_str());
	}

	if( CSLFindName(szrImageMetadata.List(), "Source_Information.Scene_Source.IMAGING_DATE") != -1 &&
			CSLFindName(szrImageMetadata.List(), "Source_Information.Scene_Source.IMAGING_TIME") != -1)
	{
		CPLString osAcqisitionTime = CSLFetchNameValue(szrImageMetadata.List(), "Source_Information.Scene_Source.IMAGING_TIME");
		CPLString osAcqisitionDate = CSLFetchNameValue(szrImageMetadata.List(), "Source_Information.Scene_Source.IMAGING_DATE");
		
		CPLString pszMD = MDName_AcquisitionDateTime + "=" + osAcqisitionDate + " " + osAcqisitionTime;
		szrCommonImageMetadata.AddString(pszMD.c_str());
	}
}

void Spot::ReadRPC(RSMDRPC& rRPC) const
{
}

const CPLStringList Spot::DefineSourceFiles() const
{
	CPLStringList papszFileList;
	if(osIMDSourceFilename != "")
	{
		papszFileList.AddString(osIMDSourceFilename.c_str());
	}
	return papszFileList;
}