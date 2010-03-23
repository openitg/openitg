#include "global.h"
#include "ErrorStrings.h"
#include "RageUtil.h"

#if !defined(XBOX)
#include <windows.h>
#endif

CString ConvertWstringToCodepage( wstring s, int iCodePage )
{
	if( s.empty() )
		return CString();

	int iBytes = WideCharToMultiByte( iCodePage, 0, s.data(), s.size(), 
					NULL, 0, NULL, FALSE );
	ASSERT_M( iBytes > 0, werr_ssprintf( GetLastError(), "WideCharToMultiByte" ).c_str() );

	CString ret;
	WideCharToMultiByte( CP_ACP, 0, s.data(), s.size(), 
					ret.GetBuffer( iBytes ), iBytes, NULL, FALSE );
	ret.ReleaseBuffer( iBytes );

	return ret;
}

CString ConvertUTF8ToACP( const CString &s )
{
	return ConvertWstringToCodepage( CStringToWstring(s), CP_ACP );
}

wstring ConvertCodepageToWString( CString s, int iCodePage )
{
	if( s.empty() )
		return wstring();

	int iBytes = MultiByteToWideChar( iCodePage, 0, s.data(), s.size(), NULL, 0 );
	ASSERT_M( iBytes > 0, werr_ssprintf( GetLastError(), "MultiByteToWideChar" ).c_str() );

	wchar_t *pTemp = new wchar_t[iBytes];
	MultiByteToWideChar( iCodePage, 0, s.data(), s.size(), pTemp, iBytes );
	wstring sRet( pTemp, iBytes );
	delete [] pTemp;

	return sRet;
}

CString ConvertACPToUTF8( const CString &s )
{
	return WStringToCString( ConvertCodepageToWString(s, CP_ACP) );
}

/*
 * Copyright (c) 2001-2005 Chris Danford, Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
