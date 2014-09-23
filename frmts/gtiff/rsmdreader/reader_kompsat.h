#ifndef _READER_KOMPSAT_H_INCLUDED
#define _READER_KOMPSAT_H_INCLUDED

#include "cpl_string.h"
#include "gdal_priv.h"

#include "rsmd_reader.h"

/**
@brief Metadata reader for Kompsat
*/
class Kompsat: public RSMDReader
{
public:
	Kompsat(const char* pszFilename);
    
	const bool IsFullCompliense() const;

private:
	CPLString osIMDSourceFilename;
	CPLString osRPCSourceFilename;

private:
	const CPLStringList DefineSourceFiles() const;

	void ReadImageMetadata(CPLStringList& szrImageMetadata) const;
	void ReadImageMetadataFromWKT(CPLStringList& szrImageMetadata) const;

	void GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const;

	void ReadRPC(RSMDRPC& rRPC) const;
	void ReadRPCFromWKT(RSMDRPC& rRPC) const;
};

#endif /* _READER_KOMPSAT_H_INCLUDED */