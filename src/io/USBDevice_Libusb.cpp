#include "global.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "io/USBDevice.h"

#include <cstdlib>
#include <map>
#ifdef WIN32
#include "libusb/usb.h"
#else
#include <usb.h>
#endif


static CString sClassDescriptions[] =
{
	"Unknown", // 0
	"Audio", // 1
	"Communications", // 2
	"Input", // 3
	"Unknown", // 4
	"Unknown", // 5
	"Camera", // 6
	"Printer", // 7
	"Storage", // 8
	"Hub", // 9
	"Data" // 10?  Shows up for my Motorola Razr
};

//USBDevice::USBDevice() {}

//USBDevice::~USBDevice() {}

/* This isn't as easy to do with libusb as it is with Unix/Linux.
 * Format, from what I can tell:
 *	"x-x" = bus, port
 *	"x-x:x.x" bus, port, config, interface (multi-interface)
 *	"x-x.x" bus, port, sub-port (child device)
 *
 * Please note that isn't implemented yet...
 * 
 * XXX: I think "devnum" might be Linux-only. We need to check that out.
 */
CString USBDevice::GetDeviceDir()
{
	LOG->Warn( "Bus: %i, Device: %i", atoi(m_Device->bus->dirname), m_Device->devnum );

	return ssprintf( "%i-%i", atoi(m_Device->bus->dirname), m_Device->devnum );
}

CString USBDevice::GetClassDescription( unsigned iClass )
{
	if ( iClass == 255 )
		return "Vendor";
	if ( iClass > 10)
		return "Unknown";

	return sClassDescriptions[iClass];
}

CString USBDevice::GetDescription()
{
	if( IsITGIO() || IsPIUIO() )
		return "Input/lights controller";
	
	vector<CString> sInterfaceDescriptions;

	for (unsigned i = 0; i < m_iInterfaceClasses.size(); i++)
		sInterfaceDescriptions.push_back( GetClassDescription(m_iInterfaceClasses[i]) );

	return join( ", ", sInterfaceDescriptions );
}

/* Ugly... */
bool USBDevice::GetDeviceProperty( const CString &sProperty, CString &sOut )
{
	LOG->Trace("USBDevice::GetDeviceProperty(): %s", sProperty.c_str());
	if( sProperty == "idVendor" )
		sOut = ssprintf( "%x", m_Device->descriptor.idVendor );
	else if( sProperty == "idProduct" )
		sOut = ssprintf( "%x", m_Device->descriptor.idProduct );
	else if( sProperty == "bMaxPower" )
	/* HACK: for some reason, MaxPower is returning half the actual value... */
		sOut = ssprintf( "%i", m_Device->config->MaxPower*2 );
//	else if( sProperty == "bDeviceClass" )
//		sOut = m_Device->descriptor.bDeviceClass;
	else
		return false;

	return true;
}

/* XXX: doesn't get multiple interfaces like the Linux code does. */
bool USBDevice::GetInterfaceProperty( const CString &sProperty, const unsigned iInterface, CString &sOut )
{
	if( (signed)iInterface > m_Device->config->bNumInterfaces )
	{
		LOG->Warn( "Cannot access interface %i with USBDevice interface count %i",
			    iInterface, m_Device->config->bNumInterfaces );
		return false;
	}

	if( sProperty == "bInterfaceClass" )
		sOut = ssprintf( "%i", m_Device->config->interface->altsetting[iInterface].bInterfaceClass );
	else
		return false;

	return true;		
}

bool USBDevice::IsHub()
{
	CString sClass;

	if( GetDeviceProperty( "bDeviceClass", sClass ) && atoi(sClass) == 9 )
		return true;

	for (unsigned i = 0; i < m_iInterfaceClasses.size(); i++)
		if( m_iInterfaceClasses[i] == 9 )
			return true;

	return false;
}

bool USBDevice::IsITGIO()
{
	// return ITGIO::DeviceMatches( m_iIdVendor, m_iIdProduct );
	if ( m_iIdVendor == 0x7c0 )
		if ( m_iIdProduct == 0x1501 || m_iIdProduct == 0x1582 || m_iIdProduct == 0x1584)
			return true;

	return false;
}

bool USBDevice::IsPIUIO()
{
	// return PIUIO::DeviceMatches( m_iIdVendor, m_iIdProduct );
	if ( m_iIdVendor == 0x547 && m_iIdProduct == 0x1002 ) return true;

	return false;
}

bool USBDevice::Load( struct usb_device *dev )
{
	m_Device = dev;
	
	if( m_Device == NULL )
	{
		LOG->Warn( "Invalid usb_device passed to Load()." );
		return false;
	}

	
	/* Irrelevant USB objects... */
	/* if( m_iIdVendor == 0 || m_iIdProduct == 0 )
		return false; */

	CString buf;

	if( GetDeviceProperty("idVendor", buf) )
		sscanf(buf, "%x", &m_iIdVendor);
	else
		m_iIdVendor = -1;

	if( GetDeviceProperty("idProduct", buf) )

		sscanf(buf, "%x", &m_iIdProduct);
	else
		m_iIdProduct = -1;

	if( GetDeviceProperty("bMaxPower", buf) )
		sscanf(buf, "%imA", &m_iMaxPower);
	else
		m_iMaxPower = -1;

	if (m_iIdVendor == -1 || m_iIdProduct == -1 || m_iMaxPower == -1)
	{
		LOG->Warn( "Could not load USBDevice" );
		return false;
	}


	for (int i = 0; i < m_Device->config->interface->num_altsetting; i++)
	{
		int iClass;
		if ( GetInterfaceProperty( "bInterfaceClass", i, buf ) )
			sscanf( buf, "%i", &iClass );
		else
		{
			LOG->Warn("Could not read interface %i.", i );
			iClass = -1;
		}

		m_iInterfaceClasses.push_back( iClass );
	}

	return true;
}

// this is the diary of a mad man
/* I'm keeping the checkpoints for now, just to make sure. -- Vyhd */
bool GetUSBDeviceList(vector<USBDevice> &pDevList)
{
	usb_init();
	usb_find_busses();
	usb_find_devices();
	
	CHECKPOINT_M( "USB initiated" );

	vector<struct usb_device> vDevices;
	vector<CString>	vDeviceDirs;

	CHECKPOINT_M( "Device created" );

	/* get all devices on the system */
	for( struct usb_bus *bus = usb_get_busses(); bus; bus = bus->next )
	{
		if( bus->root_dev )
			vDevices.push_back( *bus->root_dev );
		else
			for( struct usb_device *dev = bus->devices; dev; dev->next )
				vDevices.push_back( *dev );

		CHECKPOINT_M( ssprintf( "%i devices set", vDevices.size()) );
	}

	/* get their children - in other cases, this could be an accidental
	 * infinite loop, but here, it works out nicely as a recursion process. */
	for( unsigned i = 0; i < vDevices.size(); i++ )
	{
		for( unsigned j = 0; j < vDevices[i].num_children; j++ )
		{
			/* Talk about a dumb oversight, huh. */
			if( vDevices[i].children[j] )
				vDevices.push_back( *vDevices[i].children[j] );

			CHECKPOINT_M( ssprintf("Child %i on %i set", j, i) );
		}
	}

	for( unsigned i = 0; i < vDevices.size(); i++ )
	{
		CHECKPOINT_M( ssprintf( "Assigning device %i of %i", i, vDevices.size()) );
		USBDevice newDev;

		if (vDevices[i].descriptor.idVendor == 0 || vDevices[i].descriptor.idProduct == 0) continue;

		LOG->Trace( "Attempting to load from idVendor %x, idProduct %x",
			vDevices[i].descriptor.idVendor, 
			vDevices[i].descriptor.idProduct );

		if ( newDev.Load( &vDevices[i] ) )
		{
			pDevList.push_back( newDev );
			CHECKPOINT_M( ssprintf( "Device %i of %i assigned", i, vDevices.size()) );
		}
		else
		{
			if( vDevices[i].descriptor.idVendor != 0 && vDevices[i].descriptor.idProduct != 0 )
				LOG->Warn( "Loading failed: %x, %x", vDevices[i].descriptor.idVendor, vDevices[i].descriptor.idProduct );
		}
	}

	return true;
}

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
