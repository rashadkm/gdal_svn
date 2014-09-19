#include "rsmd_reader.h"

#include "reader_digital_globe.h"
#include "reader_pleiades.h"
#include "reader_geo_eye.h"

RSMDReader* GetRSMDReader(const char* pszFilename, RSDProvider rsdProvider)
{
	if(rsdProvider == RSD_DigitalGlobe)
		return new DigitalGlobe(pszFilename);

	return NULL;
};

RSMDReader* GetRSMDReader(const char* pszFilename)
{
	if(DigitalGlobe(pszFilename).IsFullCompliense())
		return new DigitalGlobe(pszFilename);
	if(Pleiades(pszFilename).IsFullCompliense())
		return new Pleiades(pszFilename);
	if(GeoEye(pszFilename).IsFullCompliense())
		return new GeoEye(pszFilename);

	return NULL;
};