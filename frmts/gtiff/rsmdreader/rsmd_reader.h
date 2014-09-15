#ifndef _RSMD_READER_H_INCLUDED
#define _RSMD_READER_H_INCLUDED

#include "reader_abstract.h"

/**
@mainpage
@code
#include "rsmd_reader.h"
@endcode

End use it!
*/

/**
@brief Remote sensing metadata providers
*/
typedef enum
{
	RSD_DigitalGlobe = 1,

} RSDProvider;

/**
@brief Return object - metadata reader for given type of data provider
@code
RSMetadata::RSMDReader *mdReader = RSMetadata::GetRSMDReader(pszFilename, RSMetadata::RSD_DigitalGlobe);	
CPLStringList sourseMDFileList = mdReader->GetSourceFileList();
for( int i = 0; i < sourseMDFileList.size(); i++ )
{
printf( "\t\t >>> source file: %s\n", sourseMDFileList[i] );
}

CPLStringList metadata = mdReader->GetMetadata();
for( int i = 0; i < metadata.size(); i++ )
{
printf( "\t\t >>> metadata: %s\n", metadata[i] );
}
@endcode
*/
RSMDReader* GetRSMDReader(const char* pszFilename, RSDProvider rsdProvider);

/**
@brief Return object - metadata reader for suitable type of data provider
@code
RSMetadata::RSMDReader *mdReader = RSMetadata::GetRSMDReader(pszFilename);	
CPLStringList sourseMDFileList = mdReader->GetSourceFileList();
for( int i = 0; i < sourseMDFileList.size(); i++ )
{
printf( "\t\t >>> source file: %s\n", sourseMDFileList[i] );
}

CPLStringList metadata = mdReader->GetMetadata();
for( int i = 0; i < metadata.size(); i++ )
{
printf( "\t\t >>> metadata: %s\n", metadata[i] );
}
@endcode
*/
RSMDReader* GetRSMDReader(const char* pszFilename);

#endif /* _RSMD_READER_H_INCLUDED */