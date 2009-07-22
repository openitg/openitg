/*
 * RageFileDriverProbe - RageFileDriverCrypt hack to auto-probe for and handle both encrypted or decrypted ITG2 data
 */

#ifndef RAGE_FILE_DRIVER_PROBE_H
#define RAGE_FILE_DRIVER_PROBE_H

#include "RageFile.h"
#include "RageFileBasic.h"
#include "RageFileDriver.h"
#include "RageFileDriverCrypt_ITG2.h"

class RageFileDriverProbe: public RageFileDriverCrypt_ITG2
{
public:
	RageFileDriverProbe( ): RageFileDriverCrypt_ITG2( CRYPT_KEY ) {}
	RageFileDriverProbe( const CString &sRoot ): RageFileDriverCrypt_ITG2( sRoot, CRYPT_KEY ) {}

protected:
	RageFileObjDirect *CreateInternal( const CString &sPath );
};


#endif

/*
 * Copyright (c) 2009 Pat McIlroy
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

