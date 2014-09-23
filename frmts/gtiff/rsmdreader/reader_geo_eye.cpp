#include "cplkeywordparser.h"

#include "reader_geo_eye.h"

#include "remote_sensing_metadata.h"
#include "utils.h"

GeoEye::GeoEye(const char* pszFilename)
	:RSMDReader(pszFilename, "GeoEye")
{
	CPLString osDirName = CPLGetDirname(pszFilename);
	CPLString osBaseName = CPLGetBasename(pszFilename);
	osWKTRPCSourceFilename = CPLString().Printf("%s/%s_rpc.txt",osDirName.c_str(), osBaseName.c_str());

	VSIStatBufL sStatBuf;
	if( VSIStatExL( osWKTRPCSourceFilename.c_str(), &sStatBuf, VSI_STAT_EXISTS_FLAG ) != 0 )
    {
		osWKTRPCSourceFilename = "";
	}
};

const bool GeoEye::IsFullCompliense() const
{
	if (!osWKTRPCSourceFilename.empty())
		return true;

	return false;
}

void GeoEye::ReadImageMetadata(CPLStringList& szrImageMetadata) const
{

}

void GeoEye::GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const
{

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

	if(!osWKTRPCSourceFilename.empty())
	{
		papszFileList.AddString(osWKTRPCSourceFilename.c_str());
	}
	
	return papszFileList;
}