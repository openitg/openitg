#ifndef RAGE_FILE_DRIVER_CRYPT_ITG2_H
#define RAGE_FILE_DRIVER_CRYPT_ITG2_H

#include "RageFileDriverCrypt.h"
#include "aes/aes.h"
#include <map>

#define REGISTER_ITG2_FILE_DRIVER(name,type,secret) \
static struct FileDriverEntry_##name: public FileDriverEntry \
{ \
	FileDriverEntry_##name(): FileDriverEntry( type ) { } \
	RageFileDriver *Create( const CString &sRoot ) const \
	{ \
		return new RageFileDriverCrypt_ITG2( sRoot, secret ); \
	} \
} const g_RegisterDriver_##name;

class RageFileObjCrypt_ITG2: public RageFileObjCrypt
{
public:
	RageFileObjCrypt_ITG2( const CString &secret ): RageFileObjCrypt() { m_sSecret = secret; }
	RageFileObjCrypt_ITG2( const RageFileObjCrypt_ITG2 &cpy );

	bool OpenInternal( const CString &sPath, int iMode, int &iError );
	int ReadInternal( void *buffer, size_t bytes );
	int GetFileSize() { return m_iFileSize; }

	RageFileObjDirect *Copy() const { return new RageFileObjCrypt_ITG2((*this)); }
private:
	size_t m_iFileSize;
	size_t m_iHeaderSize;
	CString m_sSecret;

	aes_decrypt_ctx m_ctx[1];
};

class RageFileDriverCrypt_ITG2: public RageFileDriverCrypt
{
public:
	RageFileDriverCrypt_ITG2( const CString &sRoot, const CString &secret = "" );
private:
	RageFileObjDirect *CreateInternal() { return new RageFileObjCrypt_ITG2(m_sSecret); }

	CString m_sSecret;
};

#endif // RAGE_FILE_DRIVER_CRYPT_ITG2_H

/*
 * (c) 2009 BoXoRRoXoRs
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