#include "stdafx.h"
#include "TcpServer.hpp"
#include "TcpAddress.hpp"
#include "TcpStream.hpp"
#include "WSAError.hpp"

namespace net {

TcpServer::TcpServer()
: m_cancelled(false),
  m_acceptorStopped(true),
  m_listenSocket(0),
  m_delegate(0)
{
}

TcpServer::~TcpServer()
{
	// request acceptor thread to exit
	{
		util::ScopedLock lock(&m_sync);
		m_cancelled = true;
	}

	// Wait for acceptor thread to exit
	while (true)
	{
		// Check it has exited
		{
			util::ScopedLock lock(&m_sync);
			if (m_acceptorStopped)
				break;
		}

		::Sleep(1);
	}

	::closesocket(m_listenSocket);
}

void
TcpServer::bind(
	const std::string& address,
	IBindingDelegate* delegate_)
{
	TcpAddress tcpAddress(address);
	m_delegate = delegate_;

    struct sockaddr_in serv_addr; 

    char sendBuff[1025];

    m_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(sendBuff, 0, sizeof(sendBuff)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(tcpAddress.host());
    serv_addr.sin_port = htons(tcpAddress.port());

	assert(serv_addr.sin_addr.s_addr != INADDR_NONE);

    if (SOCKET_ERROR == ::bind(m_listenSocket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)))
	{
		if (::WSAGetLastError() == WSAEADDRINUSE)
			throw util::Error("Address: \"" + address + "\" is already in use");
		else
			throw WSAError();
	}

	if (SOCKET_ERROR == ::listen(m_listenSocket, SOMAXCONN))
		throw WSAError();

	// Switch to non-blocking mode
	u_long iMode = FIONBIO;
	::ioctlsocket(m_listenSocket, FIONBIO, &iMode);

	{
		util::ScopedLock lock(&m_sync);
		m_acceptorStopped = false;
	}

	DWORD threadId = 0;
	HANDLE h = ::CreateThread(0, 0, acceptorThreadFunc, this, 0, &threadId);
	if (NULL == h)
		throw util::Error("Failed to create acceptor thread");
}

DWORD WINAPI
TcpServer::acceptorThreadFunc(LPVOID param)
{
	TcpServer* self = static_cast<TcpServer*>(param);
	chkptr(self);

	try
	{
		self->runAcceptor();
	}
	catch (const std::exception& x)
	{
		// Suppress exception
#ifndef NDEBUG
		const char* szMsg = x.what();
		ignore_unused(szMsg);
#endif

		assert(0);
		return 1;
	}
	catch (...)
	{
		// Suppress unknown exception
		assert(0);
		return 2;
	}

	return 0;
}

void
TcpServer::runAcceptor()
{
    while (!cancelled())
    {
        int connfd = ::accept(m_listenSocket, (struct sockaddr*)NULL, NULL);
		if (SOCKET_ERROR != connfd)
		{
			TStreamPtr stream = std::make_shared<TcpStream>(connfd);

			if (m_delegate)
				m_delegate->onStreamCreated(stream);
		}
		else
		{
			::Sleep(1);
		}
	}

	util::ScopedLock lock(&m_sync);
	m_acceptorStopped = true;
}

bool
TcpServer::cancelled() const
{
	util::ScopedLock lock(&m_sync);
	return m_cancelled;
}

} // namespace net
