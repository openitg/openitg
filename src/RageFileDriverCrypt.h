#ifndef RAGE_FILE_DRIVER_CRYPT_H
#define RAGE_FILE_DRIVER_CRYPT_H

#include "RageFile.h"
#include "RageFileBasic.h"
#include "RageFileDriverDirect.h"

class RageFileObjCrypt: public RageFileObjDirect
{
public:
	RageFileObjCrypt( const RageFileObjCrypt &cpy ) : RageFileObjDirect(cpy) { }

	RageFileObjCrypt() { }
	virtual ~RageFileObjCrypt() { }

	virtual bool OpenInternal( const CString &sPath, int iMode, int &iError ) = 0;
	virtual int ReadInternal( void *pBuffer, size_t iBytes ) = 0;
	virtual RageFileObjDirect *Copy() const = 0;

	// call this in order to read non-cryptographically
	virtual int ReadDirect( void *pBuffer, size_t iBytes );

	virtual int WriteInternal( const void *pBuffer, size_t iBytes );
	virtual int GetFileSize() const;
};

class RageFileDriverCrypt: public RageFileDriverDirectReadOnly
{
public:
	RageFileDriverCrypt( const CString &root ) : RageFileDriverDirectReadOnly(root) { }

protected:
	// attempts to open and return a file of a derivative type
	virtual RageFileObjDirect *CreateInternal( const CString &sPath ) { return NULL; }
};

#endif

/*
 * Copyright (c) 2009 Marc Cannon ("Vyhd")
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

