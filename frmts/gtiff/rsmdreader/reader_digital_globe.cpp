#include "reader_digital_globe.h"

#include "remote_sensing_metadata.h"
#include "CPLXML_utils.h"

DigitalGlobe::DigitalGlobe(const char* pszFilename)
	:RSMDReader(pszFilename, "DigitalGlobe")
{
	osXMLSourceFilename = GDALFindAssociatedFile( pszFilename, "XML", NULL, 0 );
};

const bool DigitalGlobe::IsFullCompliense() const
{
	if (osXMLSourceFilename != "")
		return true;

	return false;
}

const CPLStringList DigitalGlobe::ReadMetadata() const
{
	CPLStringList metadata;
	
	if(osXMLSourceFilename != "")
	{
		CPLXMLNode* psNode = CPLParseXMLFile(osXMLSourceFilename.c_str());
		if(psNode == NULL)
			return NULL;

		CPLXMLNode* psisdNode = psNode->psNext;
		if(psisdNode == NULL)
			printf("isd not found\n");

		CPLString sSatIdMetadata = MDName_SatelliteId+"=Unknown";
		CPLXMLNode* psSatIdNode = CPLSearchXMLNode(psisdNode, "SATID");
		
		const char* pszSatIdVal = GetCPLXMLNodeTextValue(psSatIdNode);
		if(pszSatIdVal != NULL)
				sSatIdMetadata = MDName_SatelliteId + "=" + pszSatIdVal;

		metadata.AddString(sSatIdMetadata.c_str());
	}
	
	return metadata;
}

const CPLStringList DigitalGlobe::DefineSourceFiles() const
{
	CPLStringList papszFileList;

	if(osXMLSourceFilename != "")
		papszFileList.AddString(osXMLSourceFilename.c_str());

	return papszFileList;
}