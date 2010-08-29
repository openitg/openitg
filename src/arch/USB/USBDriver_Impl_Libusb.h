#ifndef USB_DRIVER_IMPL_LIBUSB_H
#define USB_DRIVER_IMPL_LIBUSB_H

#include "USBDriver_Impl.h"
#include <usb.h>

class USBDriver_Impl_Libusb : public USBDriver_Impl
{
public:
	USBDriver_Impl_Libusb();
	~USBDriver_Impl_Libusb();

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
	usb_dev_handle *m_pHandle;
};

#endif // USB_DRIVER_LIBUSB_H

