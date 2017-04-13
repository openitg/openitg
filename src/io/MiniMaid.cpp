#include "global.h"
#include "MiniMaid.h"
#include "RageLog.h"
#include "arch/USB/USBDriver_Impl.h"

const uint16_t MINIMAID_VENDOR_ID = 0xbeef;
const uint16_t MINIMAID_PRODUCT_ID = 0x5730;

// From the HID spec:
static const int HID_REPORT_TYPE_INPUT = 0x01;
static const int HID_REPORT_TYPE_OUTPUT = 0x02;
static const int HID_REPORT_TYPE_FEATURE = 0x03;

static const int MINIMAID_INPUT_INTERFACE = 0;
static const int MINIMAID_LIGHTS_INTERFACE = 1;

/* I/O request timeout, in microseconds (so, 10 ms) */
const unsigned REQ_TIMEOUT = 10000;


bool MiniMaid::DeviceMatches( int iVID, int iPID )
{
	if( iVID != MINIMAID_VENDOR_ID ) 
		return false;

	if( iPID == MINIMAID_PRODUCT_ID )
		return true;
		
	return false;
}

bool MiniMaid::Open()
{
	if( OpenInternal(MINIMAID_VENDOR_ID, MINIMAID_PRODUCT_ID) )
	{
		m_bInitialized = true;
		return true;
	}
	else
	{
		m_bInitialized = false;
		LOG->Warn("Could not open a connection to the MiniMaid device!");
	}

	return false;
}

bool MiniMaid::Write( uint64_t data )
{
	int iType = (USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE);
	int iRequest = HID_SET_REPORT;
	int iValue = (HID_REPORT_TYPE_OUTPUT << 8) | 0x00;
	
	// Short pins on P1 and P2 side. This is necessary for some reason to prevent some
	// panels from sticking.
	static const uint64_t uHiPins = 0x10;
	data |= (uHiPins << (2 * 8)); // P1
	data |= (uHiPins << (3 * 8)); // P2
	data |= (uHiPins << (6 * 8)); // ??
	
	int iResult = m_pDriver->ControlMessage(
		iType,
		iRequest,
		iValue, 
		MINIMAID_LIGHTS_INTERFACE,
		(char *)&data,
		sizeof(data),
		REQ_TIMEOUT
	);
	
	bool bSuccess = ( iResult == sizeof(data) );

	if( !bSuccess ) {
		LOG->Warn("MiniMaid writing failed: %i (%s)", 
			iResult, m_pDriver->GetError());
	}

	return bSuccess;
}

bool MiniMaid::Read( uint64_t *pData )
{
	static const int iExpectedNumBytes = 6;
	
	int iType = (USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE);
	int iRequest = HID_GET_REPORT;
	int iValue = (HID_REPORT_TYPE_INPUT << 8) | 0x00;
	
	int iResult = m_pDriver->ControlMessage(iType, 
		iRequest, 
		iValue,
		MINIMAID_INPUT_INTERFACE, 
		(char *)pData, 
		sizeof(*pData), 
		REQ_TIMEOUT
	);
	
	bool bSuccess = ( iResult == iExpectedNumBytes );
	
	if ( !bSuccess )
	{
		LOG->Warn("MiniMaid reading failed: %i (%s)",
			iResult, m_pDriver->GetError());
	}
	
	return bSuccess;
}
