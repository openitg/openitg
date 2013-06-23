#ifndef RAGE_SOUND_DRIVER_CA_H
#define RAGE_SOUND_DRIVER_CA_H

#include "RageSoundDriver_Generic_Software.h"

struct AudioTimeStamp;
struct AudioBufferList;
struct OpaqueAudioConverter;
typedef struct OpaqueAudioConverter *AudioConverterRef;
typedef unsigned long UInt32;
typedef UInt32 AudioDeviceID;
typedef UInt32 AudioDevicePropertyID;
typedef unsigned char Boolean;
typedef long OSStatus;
class CAAudioHardwareDevice;
class RageSoundBase;

class RageSoundDriver_CA : public RageSoundDriver_Generic_Software
{
private:
    int64_t mDecodePos;
    float mLatency;
    CAAudioHardwareDevice *mOutputDevice;
	AudioConverterRef mConverter;
	
	static OSStatus GetData(AudioDeviceID inDevice,
							const AudioTimeStamp *inNow,
							const AudioBufferList *inInputData,
							const AudioTimeStamp *inInputTime,
							AudioBufferList *outOutputData,
							const AudioTimeStamp *inOutputTime,
							void *inClientData);	
    
    static OSStatus OverloadListener(AudioDeviceID inDevice,
                                     UInt32 inChannel,
                                     Boolean isInput,
                                     AudioDevicePropertyID inPropertyID,
                                     void *inData);
                              
public:
    RageSoundDriver_CA();
    CString Init();
    ~RageSoundDriver_CA();
    float GetPlayLatency() const { return mLatency; }
    int64_t GetPosition(const RageSoundBase *sound) const;
    void SetupDecodingThread();
};

#endif

/*
 * (c) 2004 Steve Checkoway
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
