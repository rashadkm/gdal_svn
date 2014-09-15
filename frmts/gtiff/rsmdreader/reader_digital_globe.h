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

private:

	const CPLStringList ReadMetadata() const;

	const CPLStringList DefineSourceFiles() const;
};

#endif /* _READER_DIGITAL_GLOBE_H_INCLUDED */