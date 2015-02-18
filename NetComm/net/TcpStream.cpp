#include "stdafx.h"
#include "TcpStream.hpp"
#include "WSAError.hpp"

#ifndef NDEBUG

namespace {

util::ThreadMutex g_sync;

char
valueToChar(unsigned char value)
{
	if (10 > value)
		return '0' + value;
	else if (16 > value)
		return 'A' + value - 10;
	else
	{
		assert(!"invalid code");
		return '?';
	}
}

void
dumpBinBuffer(const unsigned char* buf, size_t bufSize)
{
#if 0
	std::string s;
	s.reserve(bufSize * 3 + 2);
	for (size_t i = 0; i < bufSize; ++i)
	{
		unsigned char ch = buf[i];
		unsigned char h = ch >> 4;
		unsigned char l = ch & 0xF;
		s += valueToChar(h);
		s += valueToChar(l);
		s += ' ';
	}

	s += '\n';
	OutputDebugStringA(s.c_str());
#endif
}

} // namespace

#endif // !NDEBUG

namespace net {

TcpStream::TcpStream(int socket)
	: m_socket(socket)
{
	// Switch to non-blocking mode
	u_long iMode = FIONBIO;
	::ioctlsocket(m_socket, FIONBIO, &iMode);
}

TcpStream::~TcpStream()
{
	::closesocket(m_socket);
}

size_t
TcpStream::read(unsigned char* buf, size_t bufSize)
{
	int n = ::recv(m_socket, reinterpret_cast<char*>(buf), bufSize, 0);

	if (SOCKET_ERROR == n)
	{
		int code = ::WSAGetLastError();
		if (WSAEWOULDBLOCK == code)
		{
			return 0;
		}
		else
		{
			throw WSAError();
		}
	}
	else if (0 == n) // The connection was closed
	{
		throw util::ConnectionClosedError();
	}

#ifndef NDEBUG
	if (0 < n)
	{
		util::ScopedLock lock(&g_sync);

		char buf2[128];
		memset(buf2, 0, sizeof(buf2));
		sprintf_s(buf2, sizeof(buf2), "%p Received bytes: ", this);
		OutputDebugStringA(buf2);

		dumpBinBuffer(buf, n);
	}
#endif // !NDEBUG


	return n;
}

void
TcpStream::write(const unsigned char* buf, size_t count)
{
#ifndef NDEBUG
	{
		util::ScopedLock lock(&g_sync);

		char buf2[128];
		memset(buf2, 0, sizeof(buf2));
		sprintf_s(buf2, sizeof(buf2), "%p Sending bytes: ", this);
		OutputDebugStringA(buf2);

		dumpBinBuffer(buf, count);
	}
#endif // !NDEBUG

	size_t totalSent = 0;
	while (totalSent < count)
	{
		int n = ::send(m_socket, reinterpret_cast<const char*>(buf) + totalSent, count - totalSent, 0);
		if (SOCKET_ERROR == n)
		{
			int wsaError = ::WSAGetLastError();
			if (WSAEWOULDBLOCK == wsaError) // Writing faster then WSA can send
			{
				::Sleep(50);
				continue;
			}

			throw net::WSAError();
		}
		else if (0 == n && 0 < count)
		{
			throw util::Error("TCP connection was closed");
		}

		totalSent += n;
	}
}

} // namespace net
