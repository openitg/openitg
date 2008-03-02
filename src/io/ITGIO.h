#ifndef IO_ITGIO_H
#define IO_ITGIO_H

#include <usb.h>
#include "USBDriver.h"

class ITGIO: public USBDriver
{
public:

//	I'd like to implement this, so our "IsITGIO" declarations are
//	all in one place. It's not important right now, though. - Vyhd
//	static bool DeviceMatches( int idVendor, int idProduct );

	bool Read( uint32_t *pData );
	bool Write( uint32_t iData );

	/* Globally accessible. */
	static int m_iInputErrorCount;
	static CString m_sInputError;
protected:
	bool Matches( int idVendor, int idProduct ) const;

	void Reconnect();

	/* Until we know where this is called, I'd appreciate
	 * not having all the unnecessary linker errors. -- Vyhd */
	bool g_bIgnoreNextITGIOError; 
};

//extern bool g_bIgnoreNextITGIOError;

#endif /* IO_ITGIO_H */

/*
 * (c) 2008 BoXoRRoXoRs
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

