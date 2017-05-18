#ifndef HTTPHelper_H
#define HTTPHelper_H

#include "global.h" // StepMania only includes
#include "ezsockets.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "RageThreads.h"
#include <sstream>
#define HTTP_CHUNK_SIZE 1024 //matches EZSockets
#define SSTR( x ) static_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

class HTTPHelper
{
public:

	HTTPHelper();
	~HTTPHelper();
	//True if proper string, false if improper
	static bool ParseHTTPAddress( const CString & URL, CString & Proto, CString & Server, int & Port, CString & Addy );
	static CString URLEncode( const CString &URL, bool force=false );			//Encode any string in URL-style
	static CString StripOutContainers( const CString & In );	//Strip off "'s and ''s
	static CString SubmitPostRequest( const CString &URL, const CString &PostData ); // Sends a URL Post Request
	void Threaded_SubmitPostRequest( const CString &URL, const CString &PostData ); // Sends a URL Post Request
	CString GetThreadedResult();

	
private:
	static CString HTTPThread_SubmitPostRequest( const CString &URL, const CString &PostData ); // Sends a URL Post Request
	void Threaded_SubmitPostRequest_Thread_Wrapper(); //wrapper for mutex use
	static int Threaded_SubmitPostRequest_Start(void *p) { ((HTTPHelper *)p)->Threaded_SubmitPostRequest_Thread_Wrapper(); return 0; };

	CString _URL;
	CString _PostData;
	CString _retBuffer;
	RageMutex *m_ResultLock;
};

#endif