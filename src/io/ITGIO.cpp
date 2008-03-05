#include "global.h"
#include "RageLog.h"

#include "io/ITGIO.h"

CString ITGIO::m_sInputError;
int ITGIO::m_iInputErrorCount = 0;

bool ITGIO::Matches( int idVendor, int idProduct ) const
{
	if ( idVendor == 0x7c0 )
	{
		if ( idProduct == 0x1501 || idProduct == 0x1582 || idProduct == 0x1584 )
			return true;
	}

	return false;
}

bool ITGIO::Read( u_int32_t *pData )
{
	int iResult;

	while( 1 )
	{
		/* XXX: I hate magic values. What do these mean? -- Vyhd */
		iResult = usb_control_msg(m_pHandle, 161, 1, 256, 0, (char *)pData, 4, 1000);
		if( iResult == 4 ) // all data read
			break;

		// all data not read
		Reconnect();
		//Write( [obj+12] );
	}

	return true;
}

bool ITGIO::Write( u_int32_t iData )
{
	int iResult;

	while( 1 )
	{
		iResult = usb_control_msg(m_pHandle, 33, 9, 512, 0, (char *)&iData, 4, 1000 );
		
		if( iResult == 4 ) // all data read
			break;
		
		Reconnect();
	}

	return true;
}

void ITGIO::Reconnect()
{
	//if( ??? )
	//	m_sInputError = "I/O timeout";
	
	if( g_bIgnoreNextITGIOError )
	{
		g_bIgnoreNextITGIOError = false;
		return;
	}

	m_sInputError = "I/O error";
	m_iInputErrorCount++;

	Close();

	// attempt to reconnect every 0.1 seconds
	while( !Open() )
		usleep(100000);

	m_sInputError = "";
}
