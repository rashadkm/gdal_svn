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

#include "reader_spot.h"

#include "remote_sensing_metadata.h"
#include "utils.h"

Spot::Spot(const char* pszFilename)
	:RSMDReader(pszFilename, "Spot")
{
	CPLString osDirName = CPLGetDirname(pszFilename);
	osIMDSourceFilename = CPLFormFilename( osDirName.c_str(), "METADATA.DIM", NULL );

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
		CPLXMLNode* psNode = CPLParseXMLFile(osIMDSourceFilename.c_str());
		if(psNode == NULL)
			return false;

		CPLXMLNode* psDimapNode = psNode->psNext->psNext;
		if(psDimapNode == NULL)
			return false;

		CPLXMLNode* psMDProfileNode = CPLSearchXMLNode(psDimapNode, "METADATA_PROFILE");
		if(psMDProfileNode == NULL)
			return false;

		CPLXMLNode* psMDProfileValue = psMDProfileNode->psChild;
		if(psMDProfileValue == NULL && psMDProfileValue->eType != CXT_Text)
			return false;

		if( !EQUALN(psMDProfileValue->pszValue, "SPOT", 4) )
			return false;

		return true;
	}
	return false;
}

void Spot::ReadImageMetadata(CPLStringList& szrImageMetadata) const
{
	printf(">>> Spot::ReadImageMetadata\n");
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
			return;
		
		CPLStringList expulsionNodeNames;
		expulsionNodeNames.AddString("Sensor_Calibration");
		expulsionNodeNames.AddString("Ephemeris");
		expulsionNodeNames.AddString("Satellite_Attitudes");
		expulsionNodeNames.AddString("Instrument_Look_Angles_List");
		ReadXMLToStringList(psDimapNode, expulsionNodeNames, szrImageMetadata);
	}
}

void Spot::GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const
{
	if( CSLFindName(szrImageMetadata.List(), "Dataset_Sources.Source_Information.Scene_Source.MISSION") != -1 &&
		CSLFindName(szrImageMetadata.List(), "Dataset_Sources.Source_Information.Scene_Source.MISSION_INDEX") != -1)
	{
		CPLString MissionValue = CSLFetchNameValue(szrImageMetadata.List(), "Dataset_Sources.Source_Information.Scene_Source.MISSION");
		CPLString MissionIndexValue = CSLFetchNameValue(szrImageMetadata.List(), "Dataset_Sources.Source_Information.Scene_Source.MISSION_INDEX");
		CPLString SatelliteId = MissionValue + " " + MissionIndexValue;

		szrCommonImageMetadata.SetNameValue(MDName_SatelliteId.c_str(), SatelliteId.c_str());
	}

	if( CSLFindName(szrImageMetadata.List(), "Dataset_Sources.Source_Information.Scene_Source.IMAGING_DATE") != -1 &&
			CSLFindName(szrImageMetadata.List(), "Dataset_Sources.Source_Information.Scene_Source.IMAGING_TIME") != -1)
	{
		CPLString osAcqisitionTime = CSLFetchNameValue(szrImageMetadata.List(), "Dataset_Sources.Source_Information.Scene_Source.IMAGING_TIME");
		CPLString osAcqisitionDate = CSLFetchNameValue(szrImageMetadata.List(), "Dataset_Sources.Source_Information.Scene_Source.IMAGING_DATE");
		CPLString AcqisitionDateTime = osAcqisitionDate + " " + osAcqisitionTime;

		szrCommonImageMetadata.SetNameValue(MDName_AcquisitionDateTime.c_str(), AcqisitionDateTime.c_str());
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