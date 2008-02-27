#include "global.h"
#include "io/USBDevice.h"
#include "RageUtil.h"
#include "RageLog.h"
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

	for (unsigned i = 0; i < iInterfaceClasses.size(); i++)
		sInterfaceDescriptions.push_back( GetClassDescription(iInterfaceClasses[i]) );

	return join( ", ", sInterfaceDescriptions );
}

bool USBDevice::GetDeviceProperty( const CString &sProperty, CString &out )
{
	CString sTargetFile = "/rootfs/sys/bus/usb/devices/" + sDeviceDir + "/" + sProperty;
	return GetFileContents(sTargetFile, out, true);
}

bool USBDevice::GetInterfaceProperty( const CString &sProperty, const unsigned iInterface, CString &out)
{
	if (iInterface > sInterfaceDeviceDirs.size() - 1)
	{
		LOG->Warn( "Cannot access interface %i with USBDevice interface count %i", iInterface, sInterfaceDeviceDirs.size() );
		return false;
	}
	CString sTargetFile = "/rootfs/sys/bus/usb/devices/" + sDeviceDir + ":" + sInterfaceDeviceDirs[iInterface] + "/" + sProperty;
	return GetFileContents( sTargetFile, out, true );
}

bool USBDevice::IsHub()
{
	for (unsigned i = 0; i < iInterfaceClasses.size(); i++)
		if (iInterfaceClasses[i] == 9) return true;
	return false;
}

bool USBDevice::IsITGIO()
{
	// return ITGIO::DeviceMatches( iIdVendor, iIdProduct );
	if ( iIdVendor == 0x7c0 )
	{
		if (iIdProduct == 0x1501 || iIdProduct == 0x1582 || iIdProduct == 0x1584)
			return true;
	}
	return false;
}

bool USBDevice::IsPIUIO()
{
	// return PIUIO::DeviceMatches( iIdVendor, iIdProduct );
	if ( iIdVendor == 0x547 && iIdProduct == 0x1002 ) return true;
	return false;
}

bool USBDevice::Load(const CString &nDeviceDir, const vector<CString> &interfaces)
{
	sDeviceDir = nDeviceDir;
	sInterfaceDeviceDirs = interfaces;
	CString buf;

	if (GetDeviceProperty("idVendor", buf))
		sscanf(buf, "%x", &iIdVendor);
	else
		iIdVendor = -1;

	if (GetDeviceProperty("idProduct", buf))
		sscanf(buf, "%x", &iIdProduct);
	else
		iIdProduct = -1;

	if (GetDeviceProperty("bMaxPower", buf))
		sscanf(buf, "%imA", &iMaxPower);
	else
		iMaxPower = -1;

	if (iIdVendor == -1 || iIdProduct == -1 || iMaxPower == -1)
	{
		LOG->Warn( "Could not load USBDevice %s", nDeviceDir.c_str() );
		return false;
	}

	for (unsigned i = 0; i < sInterfaceDeviceDirs.size(); i++)
	{
		int iClass;
		if ( GetInterfaceProperty( "bInterfaceClass", i, buf ) )
		{
			sscanf( buf, "%x", &iClass );
		}
		else
		{
			LOG->Warn("Could not read interface %i for %s:%s", i, sDeviceDir.c_str(), sInterfaceDeviceDirs[i].c_str() );
			iClass = -1;
		}
		iInterfaceClasses.push_back(iClass);
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
		USBDevice newDev;
		CString sDevName = iter->first;
		vector<CString> sDevChildren = iter->second;
		
		//LOG->Info("Loading USB device %s", sDevName.c_str() );
		//for (unsigned j = 0; j < sDevChildren.size(); j++)
		//{
		//	LOG->Info( "\t%s :: %s", sDevName.c_str(), sDevChildren[j].c_str() );
		//}

		if ( newDev.Load(sDevName, sDevChildren) ) pDevList.push_back(newDev);
	}
	return true;
}
