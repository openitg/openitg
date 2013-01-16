/* InputMapper - Holds user-chosen input preferences and saves it between sessions. */

#ifndef INPUTMAPPER_H
#define INPUTMAPPER_H

#include "RageInputDevice.h"
#include "GameInput.h"
#include "MenuInput.h"
#include "StyleInput.h"
#include "GameConstantsAndTypes.h"



const int NUM_GAME_TO_DEVICE_SLOTS	= 5;	// five device inputs may map to one game input


class InputMapper
{
public:
	InputMapper();
	~InputMapper();

	void ReadMappingsFromDisk();
	void SaveMappingsToDisk();

	void ClearAllMappings();

	void SetInputMap( DeviceInput DeviceI, GameInput GameI, int iSlotIndex );
	void ClearFromInputMap( DeviceInput DeviceI );
	void ClearFromInputMap( GameInput GameI, int iSlotIndex );

	void AddDefaultMappingsForCurrentGameIfUnmapped();
	void AutoMapJoysticksForCurrentGame();

	bool IsMapped( DeviceInput DeviceI );
	bool IsMapped( GameInput GameI );
	bool IsMapped( StyleInput StyleI );

	bool DeviceToGame( DeviceInput DeviceI, GameInput& GameI );	// return true if there is a mapping from device to pad
	bool GameToDevice( GameInput GameI, int iSoltNum, DeviceInput& DeviceI );	// return true if there is a mapping from pad to device

	void GameToStyle( GameInput GameI, StyleInput &StyleI );
	void StyleToGame( StyleInput StyleI, GameInput &GameI );

	void GameToMenu( GameInput GameI, MenuInput &MenuI );
	void MenuToGame( MenuInput MenuI, GameInput GameIout[4] );

	float GetSecsHeld( GameInput GameI );
	float GetSecsHeld( MenuInput MenuI );
	float GetSecsHeld( StyleInput StyleI );

	bool IsButtonDown( GameInput GameI );
	bool IsButtonDown( MenuInput MenuI );
	bool IsButtonDown( StyleInput StyleI );

	void ResetKeyRepeat( GameInput GameI );
	void ResetKeyRepeat( MenuInput MenuI );
	void ResetKeyRepeat( StyleInput StyleI );

	struct Mapping {
		bool IsEndMarker() const { return iSlotIndex==-1; }

		int iSlotIndex;	// -1 == end marker
		int deviceButton;
		GameButton gb;
		/* If this is true, this is an auxilliary mapping assigned to the second
		* player.  If two of the same device are found, and the device has secondary
		* entries, the later entries take precedence.  This way, if a Pump pad is
		* found, it'll map P1 to the primary pad and P2 to the secondary pad.
		* (We can't tell if a slave pad is actually there.)  Then, if a second primary
		* is found (DEVICE_PUMP2), 2P will be mapped to it. 
		*
		* This isn't well-tested; I only have one Pump pad. */
		bool SecondController;
	};

	void ApplyMapping( const Mapping *maps, GameController gc, InputDevice device );

protected:
	// all the DeviceInputs that map to a GameInput
	DeviceInput m_GItoDI[MAX_GAME_CONTROLLERS][MAX_GAME_BUTTONS][NUM_GAME_TO_DEVICE_SLOTS];

	// lookup for efficiency from a DeviceInput to a GameInput
	// This is repopulated every time m_PItoDI changes by calling UpdateTempDItoPI().
	GameInput m_tempDItoGI[NUM_INPUT_DEVICES][MAX_DEVICE_BUTTONS];
	void UpdateTempDItoGI();
};


extern InputMapper*	INPUTMAPPER;	// global and accessable from anywhere in our program


#endif

/*
 * (c) 2001-2003 Chris Danford
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
