#include "global.h"
#include "LightsDriver_SextetStream.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "RageUtil.h"

#include <cstring>

using namespace std;

// Number of printable characters used to encode lights
static const size_t CABINET_SEXTET_COUNT = 1;
static const size_t CONTROLLER_SEXTET_COUNT = 6;

// Number of bytes to contain the full pack and a trailing LF
static const size_t FULL_SEXTET_COUNT = CABINET_SEXTET_COUNT + (2 * CONTROLLER_SEXTET_COUNT) + 1;


// Serialization routines

// Encodes the low 6 bits of a byte as a printable, non-space ASCII
// character (i.e., within the range 0x21-0x7E) such that the low 6 bits of
// the character are the same as the input.
inline uint8_t printableSextet(uint8_t data)
{
	// Maps the 6-bit value into the range 0x30-0x6F, wrapped in such a way
	// that the low 6 bits of the result are the same as the data (so
	// decoding is trivial).
	//
	//	00nnnn	->	0100nnnn (0x4n)
	//	01nnnn	->	0101nnnn (0x5n)
	//	10nnnn	->	0110nnnn (0x6n)
	//	11nnnn	->	0011nnnn (0x3n)

	// Put another way, the top 4 bits H of the output are determined from
	// the top two bits T of the input like so:
	// 	H = ((T + 1) mod 4) + 3

	return ((data + (uint8_t)0x10) & (uint8_t)0x3F) + (uint8_t)0x30;
}

// Packs 6 booleans into a 6-bit value
inline uint8_t packPlainSextet(bool b0, bool b1, bool b2, bool b3, bool b4, bool b5)
{
	return (uint8_t)(
		(b0 ? 0x01 : 0) |
		(b1 ? 0x02 : 0) |
		(b2 ? 0x04 : 0) |
		(b3 ? 0x08 : 0) |
		(b4 ? 0x10 : 0) |
		(b5 ? 0x20 : 0));
}

// Packs 6 booleans into a printable sextet
inline uint8_t packPrintableSextet(bool b0, bool b1, bool b2, bool b3, bool b4, bool b5)
{
	return printableSextet(packPlainSextet(b0, b1, b2, b3, b4, b5));
}

// Packs the cabinet lights into a printable sextet and adds it to a buffer
inline size_t packCabinetLights(const LightsState *ls, uint8_t* buffer)
{
	buffer[0] = packPrintableSextet(
		ls->m_bCabinetLights[LIGHT_MARQUEE_UP_LEFT],
		ls->m_bCabinetLights[LIGHT_MARQUEE_UP_RIGHT],
		ls->m_bCabinetLights[LIGHT_MARQUEE_LR_LEFT],
		ls->m_bCabinetLights[LIGHT_MARQUEE_LR_RIGHT],
		ls->m_bCabinetLights[LIGHT_BASS_LEFT],
		ls->m_bCabinetLights[LIGHT_BASS_RIGHT]);
	return CABINET_SEXTET_COUNT;
}

// Packs the button lights for a controller into 6 printable sextets and
// adds them to a buffer
inline size_t packControllerLights(const LightsState *ls, GameController gc, uint8_t* buffer)
{
	// Menu buttons
	buffer[0] = packPrintableSextet(
		ls->m_bGameButtonLights[gc][GAME_BUTTON_MENULEFT],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_MENURIGHT],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_MENUUP],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_MENUDOWN],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_START],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_SELECT]);

	// Other non-sensors
	buffer[1] = packPrintableSextet(
		ls->m_bGameButtonLights[gc][GAME_BUTTON_BACK],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_COIN],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_OPERATOR],
		false, //Removed as SM3.95 does not have EFFECT_UP and EFFECT_DOWN
		false,
		false);

	// Sensors
	buffer[2] = packPrintableSextet(
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_01],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_02],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_03],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_04],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_05],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_06]);
	buffer[3] = packPrintableSextet(
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_07],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_08],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_09],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_10],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_11],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_12]);
	buffer[4] = packPrintableSextet(
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_13],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_14],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_15],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_16],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_17],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_18]);
	buffer[5] = packPrintableSextet(
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_19],
		false,
		false,
		false,
		false,
		false);

	return CONTROLLER_SEXTET_COUNT;
}

inline size_t packLine(uint8_t * buffer, const LightsState* ls)
{
	size_t index = 0;

	index += packCabinetLights(ls, &(buffer[index]));

	//Hardcoded to two players.
	FOREACH_ENUM(GameController, 2, gc)
	{
		index += packControllerLights(ls, gc, &(buffer[index]));
	}

	// Terminate with LF
	buffer[index++] = 0xA;

	return index;
}



// Private members/methods are kept out of the header using an opaque pointer `_impl`.
// Google "pimpl idiom" for an explanation of what's going on and why it is (or might be) useful.


// Implementation class

namespace
{
	class Impl
	{
	protected:
		uint8_t lastOutput[FULL_SEXTET_COUNT];
		RageFile * out;

	public:
		Impl(RageFile * file) {
			out = file;

			// Ensure a non-match the first time
			lastOutput[0] = 0;
		}

		virtual ~Impl() {
			if(out != NULL)
			{
				out->Flush();
				out->Close();
				SAFE_DELETE(out);
			}
		}

		void Set(const LightsState * ls)
		{
			uint8_t buffer[FULL_SEXTET_COUNT];

			packLine(buffer, ls);

			// Only write if the message has changed since the last write.
			if(memcmp(buffer, lastOutput, FULL_SEXTET_COUNT) != 0)
			{
				if(out != NULL)
				{
					out->Write(buffer, FULL_SEXTET_COUNT);
					out->Flush();
				}

				// Remember last message
				memcpy(lastOutput, buffer, FULL_SEXTET_COUNT);
			}
		}
	};
}


// LightsDriver_SextetStream interface
// (Wrapper for Impl)

#define IMPL ((Impl*)_impl)

LightsDriver_SextetStream::LightsDriver_SextetStream()
{
	_impl = NULL;
}

LightsDriver_SextetStream::~LightsDriver_SextetStream()
{
	if(IMPL != NULL)
	{
		delete IMPL;
	}
}

void LightsDriver_SextetStream::Set(const LightsState *ls)
{
	if(IMPL != NULL)
	{
		IMPL->Set(ls);
	}
}


// LightsDriver_SextetStreamToFile implementation

REGISTER_LIGHTS_DRIVER(SextetStreamToFile);

#if defined(_WINDOWS)
	#define DEFAULT_OUTPUT_FILENAME "\\\\.\\pipe\\StepMania-Lights-SextetStream"
#else
	#define DEFAULT_OUTPUT_FILENAME "Data/StepMania-Lights-SextetStream.out"
#endif
static Preference<CString> g_sSextetStreamOutputFilename("SextetStreamOutputFilename", DEFAULT_OUTPUT_FILENAME);

inline RageFile * openOutputStream(const CString& filename)
{
	RageFile * file = new RageFile;

	if(!file->Open(filename, RageFile::WRITE|RageFile::STREAMED))
	{
		LOG->Warn("Error opening file '%s' for output: %s", filename.c_str(), file->GetError().c_str());
		SAFE_DELETE(file);
		file = NULL;
	}

	return file;
}

LightsDriver_SextetStreamToFile::LightsDriver_SextetStreamToFile(RageFile * file)
{
	_impl = new Impl(file);
}

LightsDriver_SextetStreamToFile::LightsDriver_SextetStreamToFile(const CString& filename)
{
	_impl = new Impl(openOutputStream(filename));
}

LightsDriver_SextetStreamToFile::LightsDriver_SextetStreamToFile()
{
	_impl = new Impl(openOutputStream(g_sSextetStreamOutputFilename));
}

/*
 * Copyright © 2014 Peter S. May
 * Backported by dinsfire 2017
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
