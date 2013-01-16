#include "global.h"
#include "RageFileDriverProbe.h"
#include "RageLog.h"
#include "RageUtil.h"

REGISTER_FILE_DRIVER( Probe, "PRB" );

// this function is to be error free
//
// if something fails, return RageFileObjDirect as fallback since it's going
//  to fail anyway
RageFileObjDirect *RageFileDriverProbe::CreateInternal( const CString &sPath )
{
	if (LOG)
		LOG->Debug( "%s: sPath = %s", __FUNCTION__, sPath.c_str() );


	RageFileObjDirect *rfod = new RageFileObjDirect;

	int iErr;
	if ( !rfod->OpenInternal( sPath, RageFile::READ, iErr ) )
		return rfod;
	int got = 0;
	CString sHeader;
	got = rfod->Read( sHeader, 2 );
	SAFE_DELETE( rfod );

	if ( got == 2 && ( sHeader == ":|" || sHeader == "8O" ))
	{
		LOG->Debug( "%s: returning %s as RageFileObjCrypt_ITG2", __FUNCTION__, sPath.c_str() );
		return new RageFileObjCrypt_ITG2( "" );
	}
	else
	{
		LOG->Debug( "%s: returning %s as RageFileObjDirect", __FUNCTION__, sPath.c_str() );
		return new RageFileObjDirect;
	}
}
