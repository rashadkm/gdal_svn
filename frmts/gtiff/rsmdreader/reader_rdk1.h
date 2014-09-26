#ifndef _READER_RDK1_H_INCLUDED
#define _READER_RDK1_H_INCLUDED

#include "cpl_string.h"
#include "gdal_priv.h"

#include "rsmd_reader.h"

/**
@brief Metadata reader for RDK1
*/
class RDK1: public RSMDReader
{
public:
	RDK1(const char* pszFilename);
    
	const bool IsFullCompliense() const;

private:
	CPLString osIMDSourceFilename;

private:
	const CPLStringList DefineSourceFiles() const;

	void ReadImageMetadata(CPLStringList& szrImageMetadata) const;

	void GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const;

	void ReadRPC(RSMDRPC& rRPC) const;

	bool IsIMDValid(const CPLString& psFilename) const;
};

#endif /* _READER_RDK1_H_INCLUDED */