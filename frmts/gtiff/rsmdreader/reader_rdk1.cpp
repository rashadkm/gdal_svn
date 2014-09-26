#include <sstream>
#include "cplkeywordparser.h"

#include "reader_rdk1.h"

#include "remote_sensing_metadata.h"
#include "utils.h"

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
			
		ReadXML(psRootNode, szrBadXMLMetadata);
	}

	for(int i = 0; i < szrBadXMLMetadata.size(); i++)
	{
		char *ppszRootKey = NULL;				
		const char* value = CPLParseNameValue(szrBadXMLMetadata[i], &ppszRootKey);

		//printf(">>> ppszRootKey: %s\n", ppszRootKey);
		
		std::stringstream ss(value);

		while( ss.good() )
		{
			
			CPLString osLine = "";
			std::getline(ss, osLine);
			
			if(osLine.empty())
				continue;

			//printf(">>> osLine: %s\n", osLine.c_str());

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
		CPLString pszMD = MDName_SatelliteId + "=" + SatelliteIdValue;
		szrCommonImageMetadata.AddString(pszMD.c_str());
	}

	if( CSLFindName(szrImageMetadata.List(), "Acquisition.dDateRoute") != -1 &&
			CSLFindName(szrImageMetadata.List(), "Acquisition.tTimeRoutePlan") != -1)
	{
		CPLString osAcqisitionTime = CSLFetchNameValue(szrImageMetadata.List(), "Acquisition.tTimeRoutePlan");
		CPLString osAcqisitionDate = CSLFetchNameValue(szrImageMetadata.List(), "Acquisition.dDateRoute");
		
		CPLString pszMD = MDName_AcquisitionDateTime + "=" + osAcqisitionDate + " " + osAcqisitionTime;
		szrCommonImageMetadata.AddString(pszMD.c_str());
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
			
		if( strstr(psNode->pszValue, "MSP_ROOT") != NULL)
			return true;
	}
	return false;
}