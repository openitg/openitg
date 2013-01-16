/*	Copyright: 	� Copyright 2003 Apple Computer, Inc. All rights reserved.

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
			("Apple") in consideration of your agreement to the following terms, and your
			use, installation, modification or redistribution of this Apple software
			constitutes acceptance of these terms.  If you do not agree with these terms,
			please do not use, install, modify or redistribute this Apple software.

			In consideration of your agreement to abide by the following terms, and subject
			to these terms, Apple grants you a personal, non-exclusive license, under Apple�s
			copyrights in this original Apple software (the "Apple Software"), to use,
			reproduce, modify and redistribute the Apple Software, with or without
			modifications, in source and/or binary forms; provided that if you redistribute
			the Apple Software in its entirety and without modifications, you must retain
			this notice and the following text and disclaimers in all such redistributions of
			the Apple Software.  Neither the name, trademarks, service marks or logos of
			Apple Computer, Inc. may be used to endorse or promote products derived from the
			Apple Software without specific prior written permission from Apple.  Except as
			expressly stated in this notice, no other rights or licenses, express or implied,
			are granted by Apple herein, including but not limited to any patent rights that
			may be infringed by your derivative works or by other works in which the Apple
			Software may be incorporated.

			The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
			WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
			WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
			PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
			COMBINATION WITH YOUR PRODUCTS.

			IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
			CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
			GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
			ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
			OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
			(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
			ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*=============================================================================
	CAStreamBasicDescription.cpp
 
=============================================================================*/

#include "CAConditionalMacros.h"

#include "CAStreamBasicDescription.h"

#if	TARGET_OS_MAC && TARGET_RT_MAC_MACHO
#include <CoreServices/CoreServices.h>
#else
#include <Endian.h>
#endif

const AudioStreamBasicDescription	CAStreamBasicDescription::sEmpty = { 0.0, 0, 0, 0, 0, 0, 0, 0, 0 };

CAStreamBasicDescription::CAStreamBasicDescription(double inSampleRate,		UInt32 inFormatID,
									UInt32 inBytesPerPacket,	UInt32 inFramesPerPacket,
									UInt32 inBytesPerFrame,		UInt32 inChannelsPerFrame,
									UInt32 inBitsPerChannel,	UInt32 inFormatFlags)
{
	mSampleRate = inSampleRate;
	mFormatID = inFormatID;
	mBytesPerPacket = inBytesPerPacket;
	mFramesPerPacket = inFramesPerPacket;
	mBytesPerFrame = inBytesPerFrame;
	mChannelsPerFrame = inChannelsPerFrame;
	mBitsPerChannel = inBitsPerChannel;
	mFormatFlags = inFormatFlags;
}

void CAStreamBasicDescription::PrintFormat(FILE *f, const char *indent, const char *name) const
{
	fprintf(f, "%s%s ", indent, name);
	char formatID[5];
	*(UInt32 *)formatID = EndianU32_NtoB(mFormatID);
	formatID[4] = '\0';
	fprintf(f, "%2ld ch, %6.0f Hz, '%-4.4s' (0x%08lX) ",		
				NumberChannels(), mSampleRate, formatID,
				mFormatFlags);
	if (mFormatID == kAudioFormatLinearPCM) {
		bool isInt = !(mFormatFlags & kLinearPCMFormatFlagIsFloat);
		int wordSize = SampleWordSize();
		const char *endian = (wordSize > 1) ? 
			((mFormatFlags & kLinearPCMFormatFlagIsBigEndian) ? " big-endian" : " little-endian" ) : "";
		const char *sign = isInt ? 
			((mFormatFlags & kLinearPCMFormatFlagIsSignedInteger) ? " signed" : " unsigned") : "";
		const char *floatInt = isInt ? "integer" : "float";
		char packed[32];
		if (PackednessIsSignificant()) {
			if (mFormatFlags & kLinearPCMFormatFlagIsPacked)
				sprintf(packed, "packed in %d bytes", wordSize);
			else
				sprintf(packed, "unpacked in %d bytes", wordSize);
		} else
			packed[0] = '\0';
		const char *align = AlignmentIsSignificant() ?
			((mFormatFlags & kLinearPCMFormatFlagIsAlignedHigh) ? " high-aligned" : " low-aligned") : "";
		const char *deinter = (mFormatFlags & kAudioFormatFlagIsNonInterleaved) ? ", deinterleaved" : "";
		const char *commaSpace = (packed[0]!='\0') || (align[0]!='\0') ? ", " : "";
		
		fprintf(f, "%ld-bit%s%s %s%s%s%s%s\n",
			mBitsPerChannel, endian, sign, floatInt, 
			commaSpace, packed, align, deinter);
	} else
		fprintf(f, "%ld bits/channel, %ld bytes/packet, %ld frames/packet, %ld bytes/frame\n", 
			mBitsPerChannel, mBytesPerPacket, mFramesPerPacket, mBytesPerFrame);
}

void	CAStreamBasicDescription::NormalizeLinearPCMFormat(AudioStreamBasicDescription& ioDescription)
{
	if(ioDescription.mFormatID == kAudioFormatLinearPCM)
	{
		//	we have to doctor the linear PCM format because we always
		//	only export 32 bit, native endian, fully packed floats
		ioDescription.mBitsPerChannel = sizeof(Float32) * 8;
		ioDescription.mBytesPerFrame = sizeof(Float32) * ioDescription.mChannelsPerFrame;
		ioDescription.mFramesPerPacket = 1;
		ioDescription.mBytesPerPacket = ioDescription.mFramesPerPacket * ioDescription.mBytesPerFrame;
		
		ioDescription.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
		ioDescription.mFormatFlags |= kAudioFormatFlagsNativeEndian;
	}
}

void	CAStreamBasicDescription::ResetFormat(AudioStreamBasicDescription& ioDescription)
{
	ioDescription.mSampleRate = 0;
	ioDescription.mFormatID = 0;
	ioDescription.mBytesPerPacket = 0;
	ioDescription.mFramesPerPacket = 0;
	ioDescription.mBytesPerFrame = 0;
	ioDescription.mChannelsPerFrame = 0;
	ioDescription.mBitsPerChannel = 0;
	ioDescription.mFormatFlags = 0;
}

void	CAStreamBasicDescription::FillOutFormat(AudioStreamBasicDescription& ioDescription, const AudioStreamBasicDescription& inTemplateDescription)
{
	if(ioDescription.mSampleRate == 0)
	{
		ioDescription.mSampleRate = inTemplateDescription.mSampleRate;
	}
	if(ioDescription.mFormatID == 0)
	{
		ioDescription.mFormatID = inTemplateDescription.mFormatID;
	}
	if(ioDescription.mFormatFlags == 0)
	{
		ioDescription.mFormatFlags = inTemplateDescription.mFormatFlags;
	}
	if(ioDescription.mBytesPerPacket == 0)
	{
		ioDescription.mBytesPerPacket = inTemplateDescription.mBytesPerPacket;
	}
	if(ioDescription.mFramesPerPacket == 0)
	{
		ioDescription.mFramesPerPacket = inTemplateDescription.mFramesPerPacket;
	}
	if(ioDescription.mBytesPerFrame == 0)
	{
		ioDescription.mBytesPerFrame = inTemplateDescription.mBytesPerFrame;
	}
	if(ioDescription.mChannelsPerFrame == 0)
	{
		ioDescription.mChannelsPerFrame = inTemplateDescription.mChannelsPerFrame;
	}
	if(ioDescription.mBitsPerChannel == 0)
	{
		ioDescription.mBitsPerChannel = inTemplateDescription.mBitsPerChannel;
	}
}

void	CAStreamBasicDescription::GetSimpleName(const AudioStreamBasicDescription& inDescription, char* outName, bool inAbbreviate)
{
	switch(inDescription.mFormatID)
	{
		case kAudioFormatLinearPCM:
			{
				const char* theEndianString = NULL;
				if((inDescription.mFormatFlags & kAudioFormatFlagIsBigEndian) != 0)
				{
					#if	TARGET_RT_LITTLE_ENDIAN
						theEndianString = "Big Endian";
					#endif
				}
				else
				{
					#if	TARGET_RT_BIG_ENDIAN
						theEndianString = "Little Endian";
					#endif
				}
				
				const char* theKindString = NULL;
				if((inDescription.mFormatFlags & kAudioFormatFlagIsFloat) != 0)
				{
					theKindString = (inAbbreviate ? "Float" : "Floating Point");
				}
				else if((inDescription.mFormatFlags & kAudioFormatFlagIsSignedInteger) != 0)
				{
					theKindString = (inAbbreviate ? "SInt" : "Signed Integer");
				}
				else
				{
					theKindString = (inAbbreviate ? "UInt" : "Unsigned Integer");
				}
				
				const char* thePackingString = NULL;
				if((inDescription.mFormatFlags & kAudioFormatFlagIsPacked) == 0)
				{
					if((inDescription.mFormatFlags & kAudioFormatFlagIsAlignedHigh) != 0)
					{
						thePackingString = "High";
					}
					else
					{
						thePackingString = "Low";
					}
				}
				
				if(inAbbreviate)
				{
					if(theEndianString != NULL)
					{
						if(thePackingString != NULL)
						{
							sprintf(outName, "%d Ch %s %s %s%d/%s%d", (int)inDescription.mChannelsPerFrame, theEndianString, thePackingString, theKindString, (int)inDescription.mBitsPerChannel, theKindString, (int)(inDescription.mBytesPerFrame / inDescription.mChannelsPerFrame) * 8);
						}
						else
						{
							sprintf(outName, "%d Ch %s %s%d", (int)inDescription.mChannelsPerFrame, theEndianString, theKindString, (int)inDescription.mBitsPerChannel);
						}
					}
					else
					{
						if(thePackingString != NULL)
						{
							sprintf(outName, "%d Ch %s %s%d/%s%d", (int)inDescription.mChannelsPerFrame, thePackingString, theKindString, (int)inDescription.mBitsPerChannel, theKindString, (int)((inDescription.mBytesPerFrame / inDescription.mChannelsPerFrame) * 8));
						}
						else
						{
							sprintf(outName, "%d Ch %s%d", (int)inDescription.mChannelsPerFrame, theKindString, (int)inDescription.mBitsPerChannel);
						}
					}
				}
				else
				{
					if(theEndianString != NULL)
					{
						if(thePackingString != NULL)
						{
							sprintf(outName, "%d Channel %d Bit %s %s Aligned %s in %d Bits", (int)inDescription.mChannelsPerFrame, (int)inDescription.mBitsPerChannel, theEndianString, theKindString, thePackingString, (int)(inDescription.mBytesPerFrame / inDescription.mChannelsPerFrame) * 8);
						}
						else
						{
							sprintf(outName, "%d Channel %d Bit %s %s", (int)inDescription.mChannelsPerFrame, (int)inDescription.mBitsPerChannel, theEndianString, theKindString);
						}
					}
					else
					{
						sprintf(outName, "%d Channel %d Bit %s", (int)inDescription.mChannelsPerFrame, (int)inDescription.mBitsPerChannel, theKindString);
						if(thePackingString != NULL)
						{
							sprintf(outName, "%d Channel %d Bit %s Aligned %s in %d Bits", (int)inDescription.mChannelsPerFrame, (int)inDescription.mBitsPerChannel, theKindString, thePackingString, (int)(inDescription.mBytesPerFrame / inDescription.mChannelsPerFrame) * 8);
						}
						else
						{
							sprintf(outName, "%d Channel %d Bit %s", (int)inDescription.mChannelsPerFrame, (int)inDescription.mBitsPerChannel, theKindString);
						}
					}
				}
			}
			break;
		
		case kAudioFormatAC3:
			strcpy(outName, "AC-3");
			break;
		
		case kAudioFormat60958AC3:
			strcpy(outName, "AC-3 for SPDIF");
			break;
		
		default:
			{
				char* the4CCString = (char*)&inDescription.mFormatID;
				outName[0] = the4CCString[0];
				outName[1] = the4CCString[1];
				outName[2] = the4CCString[2];
				outName[3] = the4CCString[3];
				outName[4] = 0;
			}
			break;
	};
}

#if CoreAudio_Debug
#include "CALogMacros.h"

void	CAStreamBasicDescription::PrintToLog(const AudioStreamBasicDescription& inDesc)
{
	PrintFloat		("  Sample Rate:        ", inDesc.mSampleRate);
	Print4CharCode	("  Format ID:          ", inDesc.mFormatID);
	PrintHex		("  Format Flags:       ", inDesc.mFormatFlags);
	PrintInt		("  Bytes per Packet:   ", inDesc.mBytesPerPacket);
	PrintInt		("  Frames per Packet:  ", inDesc.mFramesPerPacket);
	PrintInt		("  Bytes per Frame:    ", inDesc.mBytesPerFrame);
	PrintInt		("  Channels per Frame: ", inDesc.mChannelsPerFrame);
	PrintInt		("  Bits per Channel:   ", inDesc.mBitsPerChannel);
}
#endif

bool	operator<(const AudioStreamBasicDescription& x, const AudioStreamBasicDescription& y)
{
	bool theAnswer = false;
	bool isDone = false;
	
	//	note that if either side is 0, that field is skipped
	
	//	format ID is the first order sort
	if((!isDone) && ((x.mFormatID != 0) && (y.mFormatID != 0)))
	{
		if(x.mFormatID != y.mFormatID)
		{
			//	formats are sorted numerically except that linear
			//	PCM is always first
			if(x.mFormatID == kAudioFormatLinearPCM)
			{
				theAnswer = true;
			}
			else if(y.mFormatID == kAudioFormatLinearPCM)
			{
				theAnswer = false;
			}
			else
			{
				theAnswer = x.mFormatID < y.mFormatID;
			}
			isDone = true;
		}
	}
	
	//	floating point vs integer for linear PCM only
	if((!isDone) && ((x.mFormatID == kAudioFormatLinearPCM) && (y.mFormatID == kAudioFormatLinearPCM)))
	{
		if((x.mFormatFlags & kAudioFormatFlagIsFloat) != (y.mFormatFlags & kAudioFormatFlagIsFloat))
		{
			//	floating point is better than integer
			theAnswer = y.mFormatFlags & kAudioFormatFlagIsFloat;
			isDone = true;
		}
	}
	
	//	bit depth
	if((!isDone) && ((x.mBitsPerChannel != 0) && (y.mBitsPerChannel != 0)))
	{
		if(x.mBitsPerChannel != y.mBitsPerChannel)
		{
			//	deeper bit depths are higher quality
			theAnswer = x.mBitsPerChannel < y.mBitsPerChannel;
			isDone = true;
		}
	}
	
	//	sample rate
	if((!isDone) && ((x.mSampleRate != 0) && (y.mSampleRate != 0)))
	{
		if(x.mSampleRate != y.mSampleRate)
		{
			//	higher sample rates are higher quality
			theAnswer = x.mSampleRate < y.mSampleRate;
			isDone = true;
		}
	}
	
	//	number of channels
	if((!isDone) && ((x.mChannelsPerFrame != 0) && (y.mChannelsPerFrame != 0)))
	{
		if(x.mChannelsPerFrame != y.mChannelsPerFrame)
		{
			//	more channels is higher quality
			theAnswer = x.mChannelsPerFrame < y.mChannelsPerFrame;
			isDone = true;
		}
	}
	
	return theAnswer;
}

static bool MatchFormatFlags(const AudioStreamBasicDescription& x, const AudioStreamBasicDescription& y)
{
	UInt32 xFlags = x.mFormatFlags;
	UInt32 yFlags = y.mFormatFlags;
	
	// match wildcards
	if (x.mFormatID == 0 || y.mFormatID == 0 || xFlags == 0 || yFlags == 0) 
		return true;
	
	if (x.mFormatID == kAudioFormatLinearPCM)
	{		 		
		// knock off the all clear flag
		xFlags = xFlags & ~kAudioFormatFlagsAreAllClear;
		yFlags = yFlags & ~kAudioFormatFlagsAreAllClear;
	
		// if both kAudioFormatFlagIsPacked bits are set, then we don't care about the kAudioFormatFlagIsAlignedHigh bit.
		if (xFlags & yFlags & kAudioFormatFlagIsPacked) {
			xFlags = xFlags & ~kAudioFormatFlagIsAlignedHigh;
			yFlags = yFlags & ~kAudioFormatFlagIsAlignedHigh;
		}
		
		// if both kAudioFormatFlagIsFloat bits are set, then we don't care about the kAudioFormatFlagIsSignedInteger bit.
		if (xFlags & yFlags & kAudioFormatFlagIsFloat) {
			xFlags = xFlags & ~kAudioFormatFlagIsSignedInteger;
			yFlags = yFlags & ~kAudioFormatFlagIsSignedInteger;
		}
		
		//	if the bit depth is 8 bits or less and the format is packed, we don't care about endianness
		if((x.mBitsPerChannel <= 8) && ((xFlags & kAudioFormatFlagIsPacked) == kAudioFormatFlagIsPacked))
		{
			xFlags = xFlags & ~kAudioFormatFlagIsBigEndian;
		}
		if((y.mBitsPerChannel <= 8) && ((yFlags & kAudioFormatFlagIsPacked) == kAudioFormatFlagIsPacked))
		{
			yFlags = yFlags & ~kAudioFormatFlagIsBigEndian;
		}
	}
	return xFlags == yFlags;
}

bool	operator==(const AudioStreamBasicDescription& x, const AudioStreamBasicDescription& y)
{
	//	the semantics for equality are:
	//		1) Values must match exactly
	//		2) wildcard's are ignored in the comparison
	
#define MATCH(name) ((x.name) == 0 || (y.name) == 0 || (x.name) == (y.name))
	
	return 
			//	check the sample rate
		MATCH(mSampleRate)
		
			//	check the format ids
		&& MATCH(mFormatID)
		
			//	check the format flags
		&& MatchFormatFlags(x, y)  
			
			//	check the bytes per packet
		&& MATCH(mBytesPerPacket) 
		
			//	check the frames per packet
		&& MATCH(mFramesPerPacket) 
		
			//	check the bytes per frame
		&& MATCH(mBytesPerFrame) 
		
			//	check the channels per frame
		&& MATCH(mChannelsPerFrame) 
		
			//	check the channels per frame
		&& MATCH(mBitsPerChannel) ;
}

bool SanityCheck(const AudioStreamBasicDescription& x)
{
	// This function returns false if there are sufficiently insane values in any field.
	// It is very conservative so even some very unlikely values will pass.
	// This is just meant to catch the case where the data from a file is corrupted.

	return 
		(x.mSampleRate >= 0. && x.mSampleRate < 1e6)	
		&& (x.mBytesPerPacket < 1000000)
		&& (x.mFramesPerPacket < 1000000)
		&& (x.mBytesPerFrame < 1000000)
		&& (x.mChannelsPerFrame <= 1024)
		&& (x.mBitsPerChannel <= 1024);
}
