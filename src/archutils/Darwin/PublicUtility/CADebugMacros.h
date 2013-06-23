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
	CADebugMacros.h

=============================================================================*/
#if !defined(__CADebugMacros_h__)
#define __CADebugMacros_h__

//=============================================================================
//	CADebugMacros
//=============================================================================

//#define	CoreAudio_StopOnFailure			1
//#define	CoreAudio_TimeStampMessages		1
//#define	CoreAudio_ThreadStampMessages	1
//#define	CoreAudio_FlushDebugMessages	1

#define	CA4CCToCString(the4CC)	{ ((char*)&the4CC)[0], ((char*)&the4CC)[1], ((char*)&the4CC)[2], ((char*)&the4CC)[3], 0 }

#if	DEBUG || CoreAudio_Debug
	
	// can be used to break into debugger immediately, also see CADebugger
	#define BusError()		(*(long *)0 = 0)
	
	//	basic debugging print routines
	#if	TARGET_OS_MAC && !TARGET_API_MAC_CARBON
		extern pascal void DebugStr(const unsigned char* debuggerMsg);
		#define	DebugMessage(msg)	DebugStr("\p"msg)
		#define DebugMessageN1(msg, N1)
		#define DebugMessageN2(msg, N1, N2)
		#define DebugMessageN3(msg, N1, N2, N3)
	#else
		#include "CADebugPrintf.h"
		
		#if	CoreAudio_FlushDebugMessages && !CoreAudio_UseSysLog
			#define	FlushRtn	;fflush(DebugPrintfFile)
		#else
			#define	FlushRtn
		#endif
		
		#if		CoreAudio_ThreadStampMessages
			#include <pthread.h>
			extern "C" {
				#include <mach/mach_time.h>
			}
			#define	DebugMessage(msg)										DebugPrintfRtn(DebugPrintfFile, "%p %qd: %s\n", pthread_self(), mach_absolute_time(), msg) FlushRtn
			#define DebugMessageN1(msg, N1)									DebugPrintfRtn(DebugPrintfFile, "%p %.qd: "msg"\n", pthread_self(), mach_absolute_time(), N1) FlushRtn
			#define DebugMessageN2(msg, N1, N2)								DebugPrintfRtn(DebugPrintfFile, "%p %.qd: "msg"\n", pthread_self(), mach_absolute_time(), N1, N2) FlushRtn
			#define DebugMessageN3(msg, N1, N2, N3)							DebugPrintfRtn(DebugPrintfFile, "%p %.qd: "msg"\n", pthread_self(), mach_absolute_time(), N1, N2, N3) FlushRtn
			#define DebugMessageN4(msg, N1, N2, N3, N4)						DebugPrintfRtn(DebugPrintfFile, "%p %.qd: "msg"\n", pthread_self(), mach_absolute_time(), N1, N2, N3, N4) FlushRtn
			#define DebugMessageN5(msg, N1, N2, N3, N4, N5)					DebugPrintfRtn(DebugPrintfFile, "%p %.qd: "msg"\n", pthread_self(), mach_absolute_time(), N1, N2, N3, N4, N5) FlushRtn
			#define DebugMessageN6(msg, N1, N2, N3, N4, N5, N6)				DebugPrintfRtn(DebugPrintfFile, "%p %.qd: "msg"\n", pthread_self(), mach_absolute_time(), N1, N2, N3, N4, N5, N6) FlushRtn
			#define DebugMessageN7(msg, N1, N2, N3, N4, N5, N6, N7)			DebugPrintfRtn(DebugPrintfFile, "%p %.qd: "msg"\n", pthread_self(), mach_absolute_time(), N1, N2, N3, N4, N5, N6, N7) FlushRtn
			#define DebugMessageN8(msg, N1, N2, N3, N4, N5, N6, N7, N8)		DebugPrintfRtn(DebugPrintfFile, "%p %.qd: "msg"\n", pthread_self(), mach_absolute_time(), N1, N2, N3, N4, N5, N6, N7, N8) FlushRtn
			#define DebugMessageN9(msg, N1, N2, N3, N4, N5, N6, N7, N8, N9)	DebugPrintfRtn(DebugPrintfFile, "%p %.qd: "msg"\n", pthread_self(), mach_absolute_time(), N1, N2, N3, N4, N5, N6, N7, N8, N9) FlushRtn
		#elif	CoreAudio_TimeStampMessages
			extern "C" {
				#include <mach/mach_time.h>
			}
			#define	DebugMessage(msg)										DebugPrintfRtn(DebugPrintfFile, "%qd: %s\n", pthread_self(), mach_absolute_time(), msg) FlushRtn
			#define DebugMessageN1(msg, N1)									DebugPrintfRtn(DebugPrintfFile, "%qd: "msg"\n", mach_absolute_time(), N1) FlushRtn
			#define DebugMessageN2(msg, N1, N2)								DebugPrintfRtn(DebugPrintfFile, "%qd: "msg"\n", mach_absolute_time(), N1, N2) FlushRtn
			#define DebugMessageN3(msg, N1, N2, N3)							DebugPrintfRtn(DebugPrintfFile, "%qd: "msg"\n", mach_absolute_time(), N1, N2, N3) FlushRtn
			#define DebugMessageN4(msg, N1, N2, N3, N4)						DebugPrintfRtn(DebugPrintfFile, "%qd: "msg"\n", mach_absolute_time(), N1, N2, N3, N4) FlushRtn
			#define DebugMessageN5(msg, N1, N2, N3, N4, N5)					DebugPrintfRtn(DebugPrintfFile, "%qd: "msg"\n", mach_absolute_time(), N1, N2, N3, N4, N5) FlushRtn
			#define DebugMessageN6(msg, N1, N2, N3, N4, N5, N6)				DebugPrintfRtn(DebugPrintfFile, "%qd: "msg"\n", mach_absolute_time(), N1, N2, N3, N4, N5, N6) FlushRtn
			#define DebugMessageN7(msg, N1, N2, N3, N4, N5, N6, N7)			DebugPrintfRtn(DebugPrintfFile, "%qd: "msg"\n", mach_absolute_time(), N1, N2, N3, N4, N5, N6, N7) FlushRtn
			#define DebugMessageN8(msg, N1, N2, N3, N4, N5, N6, N7, N8)		DebugPrintfRtn(DebugPrintfFile, "%qd: "msg"\n", mach_absolute_time(), N1, N2, N3, N4, N5, N6, N7, N8) FlushRtn
			#define DebugMessageN9(msg, N1, N2, N3, N4, N5, N6, N7, N8, N9)	DebugPrintfRtn(DebugPrintfFile, "%qd: "msg"\n", mach_absolute_time(), N1, N2, N3, N4, N5, N6, N7, N8, N9) FlushRtn
		#else
			#define	DebugMessage(msg)										DebugPrintfRtn(DebugPrintfFile, "%s\n", msg) FlushRtn
			#define DebugMessageN1(msg, N1)									DebugPrintfRtn(DebugPrintfFile, msg"\n", N1) FlushRtn
			#define DebugMessageN2(msg, N1, N2)								DebugPrintfRtn(DebugPrintfFile, msg"\n", N1, N2) FlushRtn
			#define DebugMessageN3(msg, N1, N2, N3)							DebugPrintfRtn(DebugPrintfFile, msg"\n", N1, N2, N3) FlushRtn
			#define DebugMessageN4(msg, N1, N2, N3, N4)						DebugPrintfRtn(DebugPrintfFile, msg"\n", N1, N2, N3, N4) FlushRtn
			#define DebugMessageN5(msg, N1, N2, N3, N4, N5)					DebugPrintfRtn(DebugPrintfFile, msg"\n", N1, N2, N3, N4, N5) FlushRtn
			#define DebugMessageN6(msg, N1, N2, N3, N4, N5, N6)				DebugPrintfRtn(DebugPrintfFile, msg"\n", N1, N2, N3, N4, N5, N6) FlushRtn
			#define DebugMessageN7(msg, N1, N2, N3, N4, N5, N6, N7)			DebugPrintfRtn(DebugPrintfFile, msg"\n", N1, N2, N3, N4, N5, N6, N7) FlushRtn
			#define DebugMessageN8(msg, N1, N2, N3, N4, N5, N6, N7, N8)		DebugPrintfRtn(DebugPrintfFile, msg"\n", N1, N2, N3, N4, N5, N6, N7, N8) FlushRtn
			#define DebugMessageN9(msg, N1, N2, N3, N4, N5, N6, N7, N8, N9)	DebugPrintfRtn(DebugPrintfFile, msg"\n", N1, N2, N3, N4, N5, N6, N7, N8, N9) FlushRtn
		#endif
	#endif
	void	DebugPrint(const char *fmt, ...);	// can be used like printf
	#define DEBUGPRINT(msg) DebugPrint msg		// have to double-parenthesize arglist (see Debugging.h)
	#if VERBOSE
		#define vprint(msg) DEBUGPRINT(msg)
	#else
		#define vprint(msg)
	#endif
	
	#if	CoreAudio_StopOnFailure
		#include "CADebugger.h"
		#define STOP	CADebuggerStop()
	#else
		#define	STOP
	#endif

#else
	#define	DebugMessage(msg)
	#define DebugMessageN1(msg, N1)
	#define DebugMessageN2(msg, N1, N2)
	#define DebugMessageN3(msg, N1, N2, N3)
	#define DebugMessageN4(msg, N1, N2, N3, N4)
	#define DebugMessageN5(msg, N1, N2, N3, N4, N5)
	#define DebugMessageN6(msg, N1, N2, N3, N4, N5, N6)
	#define DebugMessageN7(msg, N1, N2, N3, N4, N5, N6, N7)
	#define DebugMessageN8(msg, N1, N2, N3, N4, N5, N6, N7, N8)
	#define DebugMessageN9(msg, N1, N2, N3, N4, N5, N6, N7, N8, N9)
	#define DEBUGPRINT(msg)
	#define vprint(msg)
	#define	STOP
#endif

void	LogError(const char *fmt, ...);			// writes to syslog (and stderr if debugging)

#define	Assert(inCondition, inMessage)													\
			if(!(inCondition))															\
			{																			\
				DebugMessage(inMessage);												\
				STOP;																	\
			}

#define	AssertNoError(inError, inMessage)												\
			{																			\
				SInt32 __E__err = (inError);											\
				if(__E__err != 0)														\
				{																		\
					DebugMessageN1(inMessage ", Error: %ld", __E__err);					\
					STOP;																\
				}																		\
			}

#define	AssertNoKernelError(inError, inMessage)											\
			{																			\
				unsigned int __E__err = (unsigned int)(inError);						\
				if(__E__err != 0)														\
				{																		\
					DebugMessageN1(inMessage ", Error: 0x%X", __E__err);				\
					STOP;																\
				}																		\
			}

#define	FailIf(inCondition, inHandler, inMessage)										\
			if(inCondition)																\
			{																			\
				DebugMessage(inMessage);												\
				STOP;																	\
				goto inHandler;															\
			}

#define	FailWithAction(inCondition, inAction, inHandler, inMessage)						\
			if(inCondition)																\
			{																			\
				DebugMessage(inMessage);												\
				STOP;																	\
				{ inAction; }															\
				goto inHandler;															\
			}

#if defined(__cplusplus)

#define	ThrowIf(inCondition, inException, inMessage)									\
			if(inCondition)																\
			{																			\
				DebugMessage(inMessage);												\
				STOP;																	\
				throw (inException);													\
			}

#define	ThrowIfNULL(inPointer, inException, inMessage)									\
			if((inPointer) == NULL)														\
			{																			\
				DebugMessage(inMessage);												\
				STOP;																	\
				throw (inException);													\
			}

#define	ThrowIfKernelError(inKernelError, inException, inMessage)						\
			{																			\
				kern_return_t __E__err = (inKernelError);								\
				if(__E__err != 0)														\
				{																		\
					DebugMessageN1(inMessage ", Error: 0x%X", __E__err);				\
					STOP;																\
					throw (inException);												\
				}																		\
			}

#define	ThrowIfError(inError, inException, inMessage)									\
			{																			\
				SInt32 __E__err = (inError);											\
				if(__E__err != 0)														\
				{																		\
					char __4CC_string[5];												\
					*((SInt32*)__4CC_string) = __E__err;								\
					__4CC_string[4] = 0;												\
					DebugMessageN2(inMessage ", Error: %ld (%s)", __E__err, __4CC_string);		\
					STOP;																\
					throw (inException);												\
				}																		\
			}

#define	SubclassResponsibility(inMethodName, inException)								\
			{																			\
				DebugMessage(inMethodName": Subclasses must implement this method");	\
				throw (inException);													\
			}

#endif	//	defined(__cplusplus)

#endif
