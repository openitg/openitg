#include "global.h"
#include "RageUtil.h"
#include "RageLog.h"

#include "io/USBDevice.h"
#include "io/PIUIO.h"
#include "io/ITGIO.h"

#include <map>
#include <usb.h>

static CString sClassDescriptions[] = {
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
	"Data" }; // 10?  Shows up for my Motorola Razr

USBDevice::USBDevice() {}

USBDevice::~USBDevice() {}

CString USBDevice::GetClassDescription( unsigned iClass )
{
	if ( iClass == 255 ) return "Vendor";
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

bool USBDevice::GetDeviceProperty( const CString &sProperty, CString &out )
{
	CString sTargetFile = "/rootfs/sys/bus/usb/devices/" + m_sDeviceDir + "/" + sProperty;
	return GetFileContents(sTargetFile, out, true);
}

bool USBDevice::GetInterfaceProperty( const CString &sProperty, const unsigned iInterface, CString &out)
{
	if (iInterface > m_sInterfaceDeviceDirs.size() - 1)
	{
		LOG->Warn( "Cannot access interface %i with USBDevice interface count %i", iInterface, m_sInterfaceDeviceDirs.size() );
		return false;
	}
	CString sTargetFile = "/rootfs/sys/bus/usb/devices/" + m_sDeviceDir + ":" + m_sInterfaceDeviceDirs[iInterface] + "/" + sProperty;
	return GetFileContents( sTargetFile, out, true );
}

/* CRASH: Somehow, m_iInterfaceClasses.size is getting corrupted and
 * returning larger amounts than exist, causing segfaults. -- Vyhd */
bool USBDevice::IsHub()
{
	LOG->Trace( "USBDevice::IsHub()" );
	CHECKPOINT_M( ssprintf("Total interfaces: %i", m_iInterfaceClasses.size()) );

	int iClasses = m_iInterfaceClasses.size();
	LOG->Trace( "iClasses: %i; vector size: %i", iClasses, m_iInterfaceClasses.size() );
	for (unsigned i = 0; i < iClasses; i++)
	{
//		for( unsigned j = 0; j < m_iInterfaceClasses.size(); j++ )
//			LOG->Trace( "Part %i: %i", j, m_iInterfaceClasses[j] );

		CHECKPOINT_M( ssprintf("Interface %i of %i", i, m_iInterfaceClasses.size()) );

		if (m_iInterfaceClasses[i] == 9)
			return true;
	}
	CHECKPOINT;
	return false;
}

bool USBDevice::IsITGIO()
{
//	return ITGIO::DeviceMatches( m_iIdVendor, m_iIdProduct );
	if ( m_iIdVendor == 0x7c0 )
	{
		if (m_iIdProduct == 0x1501 || m_iIdProduct == 0x1582 || m_iIdProduct == 0x1584)
			return true;
	}
	return false;
}

bool USBDevice::IsPIUIO()
{
	// return PIUIO::DeviceMatches( m_iIdVendor, m_iIdProduct );
	if ( m_iIdVendor == 0x547 && m_iIdProduct == 0x1002 ) return true;
	return false;
}

bool USBDevice::Load(const CString &nDeviceDir, const vector<CString> &interfaces)
{
	m_sDeviceDir = nDeviceDir;
	m_sInterfaceDeviceDirs = interfaces;
	CString buf;

	if (GetDeviceProperty("idVendor", buf))
		sscanf(buf, "%x", &m_iIdVendor);
	else
		m_iIdVendor = -1;

	if (GetDeviceProperty("idProduct", buf))
		sscanf(buf, "%x", &m_iIdProduct);
	else
		m_iIdProduct = -1;

	if (GetDeviceProperty("bMaxPower", buf))
		sscanf(buf, "%imA", &m_iMaxPower);
	else
		m_iMaxPower = -1;

	if (m_iIdVendor == -1 || m_iIdProduct == -1 || m_iMaxPower == -1)
	{
		LOG->Warn( "Could not load USBDevice %s", nDeviceDir.c_str() );
		return false;
	}

	for (unsigned i = 0; i < m_sInterfaceDeviceDirs.size(); i++)
	{
		int iClass;
		if ( GetInterfaceProperty( "bInterfaceClass", i, buf ) )
		{
			sscanf( buf, "%x", &iClass );
		}
		else
		{
			LOG->Warn("Could not read interface %i for %s:%s", i, m_sDeviceDir.c_str(), m_sInterfaceDeviceDirs[i].c_str() );
			iClass = -1;
		}
		m_iInterfaceClasses.push_back(iClass);
	}
	return true;
}

// this is the diary of a mad man
bool GetUSBDeviceList(vector<USBDevice> &pDevList)
{
	FlushDirCache();

	std::map< CString, vector<CString> > sDevInterfaceList;
	vector<CString> sDirList;
	GetDirListing( "/rootfs/sys/bus/usb/devices/", sDirList, true, false );
	for (unsigned i = 0; i < sDirList.size(); i++)
	{
		CString sDirEntry = sDirList[i];
		vector<CString> components;

		if (sDirEntry.substr(0, 3) == "usb") continue;

		split( sDirEntry, ":", components, true );
		if ( components.size() < 2 ) continue;

		if ( ! IsADirectory( "/rootfs/sys/bus/usb/devices/" + components[0] ) ) continue;

		//LOG->Info("GetUSBDeviceList(): Found USB Device Entry %s", sDirEntry.c_str() );

		CHECKPOINT;
		// ugly and will most likely cause a crash
		sDevInterfaceList[components[0]].push_back(components[1]);
		CHECKPOINT;

	}

	map< CString, vector<CString> >::iterator iter;
 
	for(iter = sDevInterfaceList.begin(); iter != sDevInterfaceList.end(); iter++)
	{
		CHECKPOINT;
		USBDevice newDev;
		CString sDevName = iter->first;
		CHECKPOINT;
		vector<CString> sDevChildren = iter->second;
		CHECKPOINT;
		
		//LOG->Info("Loading USB device %s", sDevName.c_str() );
		//for (unsigned j = 0; j < sDevChildren.size(); j++)
		//{
		//	LOG->Info( "\t%s :: %s", sDevName.c_str(), sDevChildren[j].c_str() );
		//}

		if ( newDev.Load(sDevName, sDevChildren) ) pDevList.push_back(newDev);
		CHECKPOINT;
	}
	return true;
}
