#ifndef USBIO_H
#define USBIO_H

#include <usb.h>

#define HID_GET_REPORT	0x01
#define HID_SET_REPORT	0x09

// For HIDs, these addresses are used for interface-level I/O.
#define HID_IFACE_IN	256
#define HID_IFACE_OUT	512

class USBDriver
{
public:
	USBDriver();
	virtual ~USBDriver();

	bool Open();
	void Close();

	virtual bool Read( uint32_t *pData ) = 0;
	virtual bool Write( uint32_t iData ) = 0;
protected:
	virtual bool Matches(int idVendor, int idProduct) const;
	virtual void Reconnect() { return; }

	struct usb_device *FindDevice();
	usb_dev_handle *m_pHandle;
};

class ITGIO: public USBDriver
{
public:
	static bool DeviceMatches( int idVendor, int idProduct );
	bool Read( uint32_t *pData );
	bool Write( uint32_t iData );

protected:
	bool Matches( int idVendor, int idProduct ) const;
	void Reconnect();
};


class PIUIO: public USBDriver
{
public:
	static bool DeviceMatches( int idVendor, int idProduct );
	bool Read( uint32_t *pData );
	bool Write( uint32_t iData );
	bool BulkReadWrite( uint32_t *pArray );

protected:
	bool Matches( int idVendor, int idProduct ) const;
	void Reconnect();
};

class PacDrive: public USBDriver
{
public:
	bool Read( uint32_t *pData ) { return false; }
	bool Write( uint32_t iData );

protected:
	bool Matches( int idVendor, int idProduct ) const;
};

enum Board
{
	BOARD_ITGIO,
	BOARD_PIUIO,
	BOARD_PACDRIVE,
	NUM_BOARDS,
	BOARD_INVALID
};

// Global functions that may be useful.
namespace USBIO
{
	// tries to open the given board type
	USBDriver *TryBoardType( Board type );

	// probes for boards and sets type to a match
	USBDriver *FindBoard( Board &type );

	// sets bitmasks specific to each board
	void MaskOutput( Board type, uint32_t *pData );
};

#endif /* USBIO_H */


/*
 * Copyright (c) 2008 BoXoRRoXoRs
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
