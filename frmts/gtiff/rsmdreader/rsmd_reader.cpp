#include "rsmd_reader.h"

#include "reader_digital_globe.h"
#include "reader_pleiades.h"
#include "reader_geo_eye.h"
#include "reader_kompsat.h"

RSMDReader* GetRSMDReader(const char* pszFilename, RSDProvider rsdProvider)
{
	if(rsdProvider == RSD_DigitalGlobe)
		return new DigitalGlobe(pszFilename);

	return NULL;
};

RSMDReader* GetRSMDReader(const CPLString pszFilename)
{
	if(DigitalGlobe(pszFilename.c_str()).IsFullCompliense())
		return new DigitalGlobe(pszFilename.c_str());
	if(Pleiades(pszFilename.c_str()).IsFullCompliense())
		return new Pleiades(pszFilename.c_str());
	if(GeoEye(pszFilename.c_str()).IsFullCompliense())
		return new GeoEye(pszFilename.c_str());
	if(Kompsat(pszFilename.c_str()).IsFullCompliense())
		return new Kompsat(pszFilename.c_str());

	return NULL;
};