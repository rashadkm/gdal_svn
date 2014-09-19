#ifndef _PLEIADES_GLOBE_H_INCLUDED
#define _PLEIADES_GLOBE_H_INCLUDED

#include "cpl_string.h"
#include "gdal_priv.h"

#include "rsmd_reader.h"

/**
@brief Metadata reader for Pleiades
*/
class Pleiades: public RSMDReader
{
public:
	Pleiades(const char* pszFilename);
    
	const bool IsFullCompliense() const;

private:
	CPLString osXMLIMDSourceFilename;
	CPLString osXMLRPCSourceFilename;

private:
	const CPLStringList DefineSourceFiles() const;

	void ReadImageMetadata(CPLStringList& szrImageMetadata) const;
	void ReadImageMetadataFromXML(CPLStringList& szrImageMetadata) const;

	void GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const;

	void ReadRPC(RSMDRPC& rRPC) const;
	void ReadRPCFromXML(RSMDRPC& rRPC) const;

};

#endif /* _PLEIADES_H_INCLUDED */