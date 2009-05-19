#ifndef INPUT_HANDLER_MK3HELPER_H
#define INPUT_HANDLER_MK3HELPER_H

/* All ports are 16 bits wide. */
const short MK3_INPUT_PORT_1 = 0x2A4;
const short MK3_INPUT_PORT_2 = 0x2A6;

const short MK3_OUTPUT_PORT_1 = 0x2A0;
const short MK3_OUTPUT_PORT_2 = 0x2A2;

#ifdef LINUX
#include <sys/io.h>
namespace MK3
{
	inline void Write( uint32_t iData )
	{
		outw_p( (uint16_t)(iData>>16),	MK3_OUTPUT_PORT_1 );
		outw_p( (uint16_t)(iData),	MK3_OUTPUT_PORT_2 );
	}

	inline void Read( uint32_t *pData )
	{
		*pData = inw_p(MK3_INPUT_PORT_1) << 16 | inw_p(MK3_INPUT_PORT_2);
	}
};
#endif

/* Assembler written for the sake of speed here, since it's easy and
 * I'm too lazy to look for a good Windows MMIO API. Please note that
 * this requires another program to grant I/O permissions until we
 * implement InpOut32 or something into the game binary, or the game
 * will crash with an "Illegal Instruction" error. -- Vyhd
 */
#ifdef WINDOWS
namespace MK3
{
	inline void __fastcall Write( uint32_t iData )
	{
		__asm
		{
			mov eax,ecx			; load 'data' into eax
			mov dx,MK3_OUTPUT_PORT_2	; set the second port
			out dx,ax			; write the lower word first
			shr eax,16			; move the high word over
			mov dx,MK3_OUTPUT_PORT_1	; set the first port
			out dx,ax			; write the high word
		}
	}

	inline void __fastcall Read( uint32_t *pData )
	{
		__asm
		{
			mov dx,MK3_INPUT_PORT_1		; set the first port
			in ax, dx			; read in one 16-bit port
			shl eax,16			; shift eax left 16 to clear ax
			mov dx,MK3_INPUT_PORT_2		; set the second port
			in ax, dx			; read the second 16-bit port
			mov dword ptr [ecx], eax	; assign our value
		}
	}
#endif

#endif // INPUT_HANDLER_MK3HELPER_H
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
