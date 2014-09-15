#ifndef _READER_ABSTRACT_H_INCLUDED
#define _READER_ABSTRACT_H_INCLUDED

#include "cpl_string.h"

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
		return ReadMetadata();
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
	virtual const CPLStringList ReadMetadata() const = 0;
};

#endif /* _READER_ABSTRACT_H_INCLUDED */