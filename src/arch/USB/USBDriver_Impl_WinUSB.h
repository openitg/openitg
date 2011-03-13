#ifndef USB_DRIVER_IMPL_WINUSB_H
#define USB_DRIVER_IMPL_WINUSB_H

#include "USBDriver_Impl.h"
struct WINUSB_INTERFACE_HANDLE;

class USBDriver_Impl_WinUSB : public USBDriver_Impl
{
public:
	USBDriver_Impl();
	~USBDriver_Impl();

	bool Open( int iVendorID, int iProductID );
	void Close();

	int ControlMessage( int iType, int iRequest, int iValue, int iIndex, char *pData, int iSize, int iTimeout );

	int BulkRead( int iEndpoint, char *pData, int iSize, int iTimeout );
	int BulkWrite( int iEndpoint, char *pData, int iSize, int iTimeout );

	int InterruptRead( int iEndpoint, char *pData, int iSize, int iTimeout );
	int InterruptWrite( int iEndpoint, char *pData, int iSize, int iTimeout );

protected:
	bool SetConfiguration( int iConfig );

	bool ClaimInterface( int iInterface );
	bool ReleaseInterface( int iInterface );

private:
	WINUSB_INTERFACE_HANDLE m_hDevice;
};

#endif // USBDRIVER_IMPL_WINUSB_H
