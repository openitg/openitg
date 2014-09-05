#ifndef INPUT_HANDLER_MINIMAID_H
#define INPUT_HANDLER_MINIMAID_H

#include "InputHandler.h"
#include "RageThreads.h"
#include "RageTimer.h"

#include "LightsMapper.h"
#include "io/MiniMaid.h"

class InputHandler_MiniMaid: public InputHandler
{
public:
	InputHandler_MiniMaid();
	~InputHandler_MiniMaid();

	void GetDevicesAndDescriptions( vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut );

private:
	MiniMaid m_pBoard;
	RageThread InputThread;
	LightsMapping m_LightsMappings;
	
	bool m_bShouldStop;
	uint64_t m_iInputField;
	uint64_t m_iLastInputField;
	uint64_t m_iLightsField;
	
	void Reconnect();

	CString GetInputDescription( const uint64_t iInputData, short iBit );
	void HandleInput();
	
	void SetLightsMappings();
	void UpdateLights();
	
	int InputThreadMain();
	static int InputThread_Start( void *data ) { return ((InputHandler_MiniMaid *) data)->InputThreadMain(); }
};

#endif