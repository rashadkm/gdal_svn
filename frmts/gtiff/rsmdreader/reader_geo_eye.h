#ifndef _READER_GEO_EYE_H_INCLUDED
#define _READER_GEO_EYE_H_INCLUDED

#include "cpl_string.h"
#include "gdal_priv.h"

#include "rsmd_reader.h"

/**
@brief Metadata reader for DigitalGlobe
*/
class GeoEye: public RSMDReader
{
public:
	GeoEye(const char* pszFilename);
    
	const bool IsFullCompliense() const;

private:
	CPLString osWKTRPCSourceFilename;
	CPLString osIMDSourceFilename;

private:
	const CPLStringList DefineSourceFiles() const;

	void ReadImageMetadata(CPLStringList& szrImageMetadata) const;

	void GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const;

	void ReadRPC(RSMDRPC& rRPC) const;
	void ReadRPCFromWKT(RSMDRPC& rRPC) const;

};

#endif /* _READER_GEO_EYE_H_INCLUDED */