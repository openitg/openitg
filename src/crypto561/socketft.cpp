// socketft.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "socketft.h"

#ifdef SOCKETS_AVAILABLE

#include "wait.h"

#ifdef USE_BERKELEY_STYLE_SOCKETS
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#endif

NAMESPACE_BEGIN(CryptoPP)

#ifdef USE_WINDOWS_STYLE_SOCKETS
const int SOCKET_EINVAL = WSAEINVAL;
const int SOCKET_EWOULDBLOCK = WSAEWOULDBLOCK;
typedef int socklen_t;
#else
const int SOCKET_EINVAL = EINVAL;
const int SOCKET_EWOULDBLOCK = EWOULDBLOCK;
#endif

Socket::Err::Err(socket_t s, const std::string& operation, int error)
	: OS_Error(IO_ERROR, "Socket: " + operation + " operation failed with error " + IntToString(error), operation, error)
	, m_s(s)
{
}

Socket::~Socket()
{
	if (m_own)
	{
		try
		{
			CloseSocket();
		}
		catch (...)
		{
		}
	}
}

void Socket::AttachSocket(socket_t s, bool own)
{
	if (m_own)
		CloseSocket();

	m_s = s;
	m_own = own;
	SocketChanged();
}

socket_t Socket::DetachSocket()
{
	socket_t s = m_s;
	m_s = INVALID_SOCKET;
	SocketChanged();
	return s;
}

void Socket::Create(int nType)
{
	assert(m_s == INVALID_SOCKET);
	m_s = socket(AF_INET, nType, 0);
	CheckAndHandleError("socket", m_s);
	m_own = true;
	SocketChanged();
}

void Socket::CloseSocket()
{
	if (m_s != INVALID_SOCKET)
	{
#ifdef USE_WINDOWS_STYLE_SOCKETS
		CancelIo((HANDLE) m_s);
		CheckAndHandleError_int("closesocket", closesocket(m_s));
#else
		CheckAndHandleError_int("close", close(m_s));
#endif
		m_s = INVALID_SOCKET;
		SocketChanged();
	}
}

void Socket::Bind(unsigned int port, const char *addr)
{
	sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;

	if (addr == NULL)
		sa.sin_addr.s_addr = htonl(INADDR_ANY);
	else
	{
		unsigned long result = inet_addr(addr);
		if (result == -1)	// Solaris doesn't have INADDR_NONE
		{
			SetLastError(SOCKET_EINVAL);
			CheckAndHandleError_int("inet_addr", SOCKET_ERROR);
		}
		sa.sin_addr.s_addr = result;
	}

	sa.sin_port = htons((u_short)port);

	Bind((sockaddr *)&sa, sizeof(sa));
}

void Socket::Bind(const sockaddr *psa, socklen_t saLen)
{
	assert(m_s != INVALID_SOCKET);
	// cygwin workaround: needs const_cast
	CheckAndHandleError_int("bind", bind(m_s, const_cast<sockaddr *>(psa), saLen));
}

void Socket::Listen(int backlog)
{
	assert(m_s != INVALID_SOCKET);
	CheckAndHandleError_int("listen", listen(m_s, backlog));
}

bool Socket::Connect(const char *addr, unsigned int port)
{
	assert(addr != NULL);

	sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(addr);

	if (sa.sin_addr.s_addr == -1)	// Solaris doesn't have INADDR_NONE
	{
		hostent *lphost = gethostbyname(addr);
		if (lphost == NULL)
		{
			SetLastError(SOCKET_EINVAL);
			CheckAndHandleError_int("gethostbyname", SOCKET_ERROR);
		}

		sa.sin_addr.s_addr = ((in_addr *)lphost->h_addr)->s_addr;
	}

	sa.sin_port = htons((u_short)port);

	return Connect((const sockaddr *)&sa, sizeof(sa));
}

bool Socket::Connect(const sockaddr* psa, socklen_t saLen)
{
	assert(m_s != INVALID_SOCKET);
	int result = connect(m_s, const_cast<sockaddr*>(psa), saLen);
	if (result == SOCKET_ERROR && GetLastError() == SOCKET_EWOULDBLOCK)
		return false;
	CheckAndHandleError_int("connect", result);
	return true;
}

bool Socket::Accept(Socket& target, sockaddr *psa, socklen_t *psaLen)
{
	assert(m_s != INVALID_SOCKET);
	socket_t s = accept(m_s, psa, psaLen);
	if (s == INVALID_SOCKET && GetLastError() == SOCKET_EWOULDBLOCK)
		return false;
	CheckAndHandleError("accept", s);
	target.AttachSocket(s, true);
	return true;
}

void Socket::GetSockName(sockaddr *psa, socklen_t *psaLen)
{
	assert(m_s != INVALID_SOCKET);
	CheckAndHandleError_int("getsockname", getsockname(m_s, psa, psaLen));
}

void Socket::GetPeerName(sockaddr *psa, socklen_t *psaLen)
{
	assert(m_s != INVALID_SOCKET);
	CheckAndHandleError_int("getpeername", getpeername(m_s, psa, psaLen));
}

unsigned int Socket::Send(const byte* buf, size_t bufLen, int flags)
{
	assert(m_s != INVALID_SOCKET);
	int result = send(m_s, (const char *)buf, UnsignedMin(INT_MAX, bufLen), flags);
	CheckAndHandleError_int("send", result);
	return result;
}

unsigned int Socket::Receive(byte* buf, size_t bufLen, int flags)
{
	assert(m_s != INVALID_SOCKET);
	int result = recv(m_s, (char *)buf, UnsignedMin(INT_MAX, bufLen), flags);
	CheckAndHandleError_int("recv", result);
	return result;
}

void Socket::ShutDown(int how)
{
	assert(m_s != INVALID_SOCKET);
	int result = shutdown(m_s, how);
	CheckAndHandleError_int("shutdown", result);
}

void Socket::IOCtl(long cmd, unsigned long *argp)
{
	assert(m_s != INVALID_SOCKET);
#ifdef USE_WINDOWS_STYLE_SOCKETS
	CheckAndHandleError_int("ioctlsocket", ioctlsocket(m_s, cmd, argp));
#else
	CheckAndHandleError_int("ioctl", ioctl(m_s, cmd, argp));
#endif
}

bool Socket::SendReady(const timeval *timeout)
{
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(m_s, &fds);
	int ready;
	if (timeout == NULL)
		ready = select((int)m_s+1, NULL, &fds, NULL, NULL);
	else
	{
		timeval timeoutCopy = *timeout;	// select() modified timeout on Linux
		ready = select((int)m_s+1, NULL, &fds, NULL, &timeoutCopy);
	}
	CheckAndHandleError_int("select", ready);
	return ready > 0;
}

bool Socket::ReceiveReady(const timeval *timeout)
{
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(m_s, &fds);
	int ready;
	if (timeout == NULL)
		ready = select((int)m_s+1, &fds, NULL, NULL, NULL);
	else
	{
		timeval timeoutCopy = *timeout;	// select() modified timeout on Linux
		ready = select((int)m_s+1, &fds, NULL, NULL, &timeoutCopy);
	}
	CheckAndHandleError_int("select", ready);
	return ready > 0;
}

unsigned int Socket::PortNameToNumber(const char *name, const char *protocol)
{
	int port = atoi(name);
	if (IntToString(port) == name)
		return port;

	servent *se = getservbyname(name, protocol);
	if (!se)
		throw Err(INVALID_SOCKET, "getservbyname", SOCKET_EINVAL);
	return ntohs(se->s_port);
}

void Socket::StartSockets()
{
#ifdef USE_WINDOWS_STYLE_SOCKETS
	WSADATA wsd;
	int result = WSAStartup(0x0202, &wsd);
	if (result != 0)
		throw Err(INVALID_SOCKET, "WSAStartup", result);
#endif
}

void Socket::ShutdownSockets()
{
#ifdef USE_WINDOWS_STYLE_SOCKETS
	int result = WSACleanup();
	if (result != 0)
		throw Err(INVALID_SOCKET, "WSACleanup", result);
#endif
}

int Socket::GetLastError()
{
#ifdef USE_WINDOWS_STYLE_SOCKETS
	return WSAGetLastError();
#else
	return errno;
#endif
}

void Socket::SetLastError(int errorCode)
{
#ifdef USE_WINDOWS_STYLE_SOCKETS
	WSASetLastError(errorCode);
#else
	errno = errorCode;
#endif
}

void Socket::HandleError(const char *operation) const
{
	int err = GetLastError();
	throw Err(m_s, operation, err);
}

#ifdef USE_WINDOWS_STYLE_SOCKETS

SocketReceiver::SocketReceiver(Socket &s)
	: m_s(s), m_resultPending(false), m_eofReceived(false)
{
	m_event.AttachHandle(CreateEvent(NULL, true, false, NULL), true);
	m_s.CheckAndHandleError("CreateEvent", m_event.HandleValid());
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_overlapped.hEvent = m_event;
}

SocketReceiver::~SocketReceiver()
{
#ifdef USE_WINDOWS_STYLE_SOCKETS
	CancelIo((HANDLE) m_s.GetSocket());
#endif
}

bool SocketReceiver::Receive(byte* buf, size_t bufLen)
{
	assert(!m_resultPending && !m_eofReceived);

	DWORD flags = 0;
	// don't queue too much at once, or we might use up non-paged memory
	WSABUF wsabuf = {UnsignedMin((u_long)128*1024, bufLen), (char *)buf};
	if (WSARecv(m_s, &wsabuf, 1, &m_lastResult, &flags, &m_overlapped, NULL) == 0)
	{
		if (m_lastResult == 0)
			m_eofReceived = true;
	}
	else
	{
		switch (WSAGetLastError())
		{
		default:
			m_s.CheckAndHandleError_int("WSARecv", SOCKET_ERROR);
		case WSAEDISCON:
			m_lastResult = 0;
			m_eofReceived = true;
			break;
		case WSA_IO_PENDING:
			m_resultPending = true;
		}
	}
	return !m_resultPending;
}

void SocketReceiver::GetWaitObjects(WaitObjectContainer &container, CallStack const& callStack)
{
	if (m_resultPending)
		container.AddHandle(m_event, CallStack("SocketReceiver::GetWaitObjects() - result pending", &callStack));
	else if (!m_eofReceived)
		container.SetNoWait(CallStack("SocketReceiver::GetWaitObjects() - result ready", &callStack));
}

unsigned int SocketReceiver::GetReceiveResult()
{
	if (m_resultPending)
	{
		DWORD flags = 0;
		if (WSAGetOverlappedResult(m_s, &m_overlapped, &m_lastResult, false, &flags))
		{
			if (m_lastResult == 0)
				m_eofReceived = true;
		}
		else
		{
			switch (WSAGetLastError())
			{
			default:
				m_s.CheckAndHandleError("WSAGetOverlappedResult", FALSE);
			case WSAEDISCON:
				m_lastResult = 0;
				m_eofReceived = true;
			}
		}
		m_resultPending = false;
	}
	return m_lastResult;
}

// *************************************************************

SocketSender::SocketSender(Socket &s)
	: m_s(s), m_resultPending(false), m_lastResult(0)
{
	m_event.AttachHandle(CreateEvent(NULL, true, false, NULL), true);
	m_s.CheckAndHandleError("CreateEvent", m_event.HandleValid());
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_overlapped.hEvent = m_event;
}


SocketSender::~SocketSender()
{
#ifdef USE_WINDOWS_STYLE_SOCKETS
	CancelIo((HANDLE) m_s.GetSocket());
#endif
}

void SocketSender::Send(const byte* buf, size_t bufLen)
{
	assert(!m_resultPending);
	DWORD written = 0;
	// don't queue too much at once, or we might use up non-paged memory
	WSABUF wsabuf = {UnsignedMin((u_long)128*1024, bufLen), (char *)buf};
	if (WSASend(m_s, &wsabuf, 1, &written, 0, &m_overlapped, NULL) == 0)
	{
		m_resultPending = false;
		m_lastResult = written;
	}
	else
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
			m_s.CheckAndHandleError_int("WSASend", SOCKET_ERROR);

		m_resultPending = true;
	}
}

void SocketSender::SendEof()
{
	assert(!m_resultPending);
	m_s.ShutDown(SD_SEND);
	m_s.CheckAndHandleError("ResetEvent", ResetEvent(m_event));
	m_s.CheckAndHandleError_int("WSAEventSelect", WSAEventSelect(m_s, m_event, FD_CLOSE));
	m_resultPending = true;
}

bool SocketSender::EofSent()
{
	if (m_resultPending)
	{
		WSANETWORKEVENTS events;
		m_s.CheckAndHandleError_int("WSAEnumNetworkEvents", WSAEnumNetworkEvents(m_s, m_event, &events));
		if ((events.lNetworkEvents & FD_CLOSE) != FD_CLOSE)
			throw Socket::Err(m_s, "WSAEnumNetworkEvents (FD_CLOSE not present)", E_FAIL);
		if (events.iErrorCode[FD_CLOSE_BIT] != 0)
			throw Socket::Err(m_s, "FD_CLOSE (via WSAEnumNetworkEvents)", events.iErrorCode[FD_CLOSE_BIT]);
		m_resultPending = false;
	}
	return m_lastResult != 0;
}

void SocketSender::GetWaitObjects(WaitObjectContainer &container, CallStack const& callStack)
{
	if (m_resultPending)
		container.AddHandle(m_event, CallStack("SocketSender::GetWaitObjects() - result pending", &callStack));
	else
		container.SetNoWait(CallStack("SocketSender::GetWaitObjects() - result ready", &callStack));
}

unsigned int SocketSender::GetSendResult()
{
	if (m_resultPending)
	{
		DWORD flags = 0;
		BOOL result = WSAGetOverlappedResult(m_s, &m_overlapped, &m_lastResult, false, &flags);
		m_s.CheckAndHandleError("WSAGetOverlappedResult", result);
		m_resultPending = false;
	}
	return m_lastResult;
}

#endif

#ifdef USE_BERKELEY_STYLE_SOCKETS

SocketReceiver::SocketReceiver(Socket &s)
	: m_s(s), m_lastResult(0), m_eofReceived(false)
{
}

void SocketReceiver::GetWaitObjects(WaitObjectContainer &container, CallStack const& callStack)
{
	if (!m_eofReceived)
		container.AddReadFd(m_s, CallStack("SocketReceiver::GetWaitObjects()", &callStack));
}

bool SocketReceiver::Receive(byte* buf, size_t bufLen)
{
	m_lastResult = m_s.Receive(buf, bufLen);
	if (bufLen > 0 && m_lastResult == 0)
		m_eofReceived = true;
	return true;
}

unsigned int SocketReceiver::GetReceiveResult()
{
	return m_lastResult;
}

SocketSender::SocketSender(Socket &s)
	: m_s(s), m_lastResult(0)
{
}

void SocketSender::Send(const byte* buf, size_t bufLen)
{
	m_lastResult = m_s.Send(buf, bufLen);
}

void SocketSender::SendEof()
{
	m_s.ShutDown(SD_SEND);
}

unsigned int SocketSender::GetSendResult()
{
	return m_lastResult;
}

void SocketSender::GetWaitObjects(WaitObjectContainer &container, CallStack const& callStack)
{
	container.AddWriteFd(m_s, CallStack("SocketSender::GetWaitObjects()", &callStack));
}

#endif

NAMESPACE_END

#endif	// #ifdef SOCKETS_AVAILABLE
