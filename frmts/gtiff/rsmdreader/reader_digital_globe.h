#ifndef _READER_DIGITAL_GLOBE_H_INCLUDED
#define _READER_DIGITAL_GLOBE_H_INCLUDED

#include "cpl_string.h"
#include "gdal_priv.h"

#include "rsmd_reader.h"

/**
@brief Metadata reader for DigitalGlobe
*/
class DigitalGlobe: public RSMDReader
{
public:
	DigitalGlobe(const char* pszFilename);
    
	const bool IsFullCompliense() const;

private:
	CPLString osXMLSourceFilename;
	CPLString osIMDSourceFilename;
	CPLString osRPBSourceFilename;

private:
	const CPLStringList DefineSourceFiles() const;

	void ReadImageMetadata(CPLStringList& szrImageMetadata) const;
	void ReadImageMetadataFromWKT(CPLStringList& szrImageMetadata) const;
	void ReadImageMetadataFromXML(CPLStringList& szrImageMetadata) const;

	void GetCommonImageMetadata(CPLStringList& szrImageMetadata, CPLStringList& szrCommonImageMetadata) const;

	void ReadRPC(RSMDRPC& rRPC) const;
	void ReadRPCFromWKT(RSMDRPC& rRPC) const;
	void ReadRPCFromXML(RSMDRPC& rRPC) const;

};

#endif /* _READER_DIGITAL_GLOBE_H_INCLUDED */