#ifndef _READER_ABSTRACT_H_INCLUDED
#define _READER_ABSTRACT_H_INCLUDED

#include "cpl_string.h"

typedef struct
{
	CPLString lineOffset;
	CPLString sampOffset;
	CPLString latOffset;
	CPLString longOffset;
	CPLString heightOffset;
	CPLString lineScale;
	CPLString sampScale;
	CPLString latScale;
	CPLString longScale;
	CPLString heightScale;
	CPLString lineNumCoef;
	CPLString lineDenCoef;
	CPLString sampNumCoef;
	CPLString sampDenCoef;
} RSMDRPC;
/**
@brief Metadata reader base class
*/
class RSMDReader
{
public:
/**
@brief Constructor
@param pszFilename
@param pszProviderName
*/
	RSMDReader(const char* pszFilename, const char* pszProviderName):pszFilename(pszFilename), pszProviderName(pszProviderName)
	{
	};

/**
@brief Return list of metadata
*/
	const CPLStringList GetMetadata() const
	{
		CPLStringList szMetadata;
		
		CPLStringList szImageMetadata;
		ReadImageMetadata(szImageMetadata);
		for(int i = 0; i < szImageMetadata.size(); ++i)
		{
			szMetadata.AddString(CPLString().Printf("IMD.%s",szImageMetadata[i]).c_str());
		}

		CPLStringList szCommonImageMetadata;
		GetCommonImageMetadata(szImageMetadata, szCommonImageMetadata);
		for(int i = 0; i < szCommonImageMetadata.size(); ++i)
		{
			szMetadata.AddString(CPLString().Printf("IMAGERY.%s",szCommonImageMetadata[i]).c_str());
		}

		RSMDRPC szRPC;
		ReadRPC(szRPC);
		szMetadata.AddString(CPLString().Printf("RPC.LINE_OFF=%s",szRPC.lineOffset.c_str()).c_str());
		szMetadata.AddString(CPLString().Printf("RPC.SAMP_OFF=%s",szRPC.sampOffset.c_str()).c_str());
		szMetadata.AddString(CPLString().Printf("RPC.LAT_OFF=%s",szRPC.latOffset.c_str()).c_str());
		szMetadata.AddString(CPLString().Printf("RPC.LONG_OFF=%s",szRPC.longOffset.c_str()).c_str());
		szMetadata.AddString(CPLString().Printf("RPC.HEIGHT_OFF=%s",szRPC.heightOffset.c_str()).c_str());
		szMetadata.AddString(CPLString().Printf("RPC.LINE_SCALE=%s",szRPC.lineScale.c_str()).c_str());
		szMetadata.AddString(CPLString().Printf("RPC.SAMP_SCALE=%s",szRPC.sampScale.c_str()).c_str());
		szMetadata.AddString(CPLString().Printf("RPC.LAT_SCALE=%s",szRPC.latScale.c_str()).c_str());
		szMetadata.AddString(CPLString().Printf("RPC.LONG_SCALE=%s",szRPC.longScale.c_str()).c_str());
		szMetadata.AddString(CPLString().Printf("RPC.HEIGHT_SCALE=%s",szRPC.heightScale.c_str()).c_str());
		szMetadata.AddString(CPLString().Printf("RPC.LINE_NUM_COEFF=%s",szRPC.lineNumCoef.c_str()).c_str());
		szMetadata.AddString(CPLString().Printf("RPC.LINE_DEN_COEFF=%s",szRPC.lineDenCoef.c_str()).c_str());
		szMetadata.AddString(CPLString().Printf("RPC.SAMP_NUM_COEFF=%s",szRPC.sampNumCoef.c_str()).c_str());
		szMetadata.AddString(CPLString().Printf("RPC.SAMP_DEN_COEFF=%s",szRPC.sampDenCoef.c_str()).c_str());

		return szMetadata;
	};

    
/**
@brief Return list of sources files
*/
	const CPLStringList GetSourceFileList() const
	{
		return DefineSourceFiles();
	}

/**
@brief Determine whether the input parameter correspond to the particular provider of remote sensing data completely
@return True if all sources files found
*/
	virtual const bool IsFullCompliense() const = 0;

/**
@brief Return remote sensing metadata provider name 
@return metadata provider name
*/
	const char* ProviderName() const
	{
		return pszProviderName;
	};

protected:
	const char* pszFilename;
	const char* pszProviderName;

private:
/**
@brief Search all specific to the provider sources files
*/
	virtual const CPLStringList DefineSourceFiles() const = 0;

/**
@brief Read metadata from sources files
*/
	virtual void ReadImageMetadata(CPLStringList& szrImageMetadata) const = 0;

/**
@brief Find and return common image metadata (common image metadata described in remote_sensing_metadata.h)
*/
	virtual void GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const = 0;

/**
@brief Read metadata from sources files
*/
	virtual void ReadRPC(RSMDRPC& rRPC) const = 0;

};

#endif /* _READER_ABSTRACT_H_INCLUDED */