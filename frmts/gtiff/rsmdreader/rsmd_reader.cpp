#include "rsmd_reader.h"

#include "reader_digital_globe.h"

RSMDReader* GetRSMDReader(const char* pszFilename, RSDProvider rsdProvider)
{
	if(rsdProvider == RSD_DigitalGlobe)
		return new DigitalGlobe(pszFilename);

	return NULL;
};

RSMDReader* GetRSMDReader(const char* pszFilename)
{
	RSMDReader* pReader = NULL;

	if(DigitalGlobe(pszFilename).IsFullCompliense())
		return new DigitalGlobe(pszFilename);


	return NULL;
};