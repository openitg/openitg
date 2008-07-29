#ifndef INPUT_HANDLER_PIUIO_H
#define INPUT_HANDLER_PIUIO_H

#ifdef WIN32
#include <windows.h>
#else
#include <stdint.h>
#endif

#include "InputHandler.h"
#include "RageThreads.h"
#include "RageTimer.h"
#include "io/PIUIO.h"

class CoinQueue
{
public:
	CoinQueue() { m_iCoins = 0; m_bShutdown = false; }
	void AddCoin() { m_iCoins++; }

	void Run();
	void Stop();

private:
	int m_iCoins;
	bool m_bShutdown;
	RageThread CQThread;

	static int CoinQueue_Start( void *p );
	void CoinQueue_ThreadMain();
};

class InputHandler_PIUIO: public InputHandler
{
public:
	InputHandler_PIUIO();
	~InputHandler_PIUIO();

	void GetDevicesAndDescriptions( vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut );
	void ReloadSensorReports();
private:
	
	PIUIO Board;
	RageThread InputThread;
	CoinQueue m_CoinQueue;

	void HandleInput();

	void HandleInputNormal();
	void HandleInputKernel();

	// a function pointer to which of the two input handlers we use
	void (InputHandler_PIUIO::*InternalInputHandler)();

	// allow this driver to update lights with "ext"
	void UpdateLights();

	static int InputThread_Start( void *p );
	void InputThreadMain();

	// keeps track of which sensors are on for each input
	bool m_bInputs[32][4];

	// used to determine which inputs have sensor comments
	bool m_bReportSensor[32];

	/* the fully combined bit field that input is read from */
	uint32_t m_iInputField;

	/* used for normal reads - one uint32_t per sensor set */
	uint32_t m_iInputData[4];

	/* array used for bulk read/write sessions */
	uint32_t m_iBulkReadData[8];

	/* data that will be written to PIUIO */
	uint32_t m_iLightData;

	bool m_bFoundDevice;
	bool m_bShutdown;
	bool m_bCoinEvent;

	/* debug code */
	void RunTimingCode();
	RageTimer m_InputTimer;
	RageTimer m_USBTimer;
	unsigned int m_iReadCount;
	float m_fTotalReadTime;

};




#define USE_INPUT_HANDLER_PIUIO

#endif

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

