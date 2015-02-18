#pragma once

#include "IStream.hpp"

namespace net {

class TcpServer;
class TcpClient;

/// TCP-connection based IStream implementation.
class TcpStream : public IStream
{
	friend class TcpServer;
	friend class TcpClient;

	/// Must not be created by a client code
	explicit TcpStream(int socket);

public:
	~TcpStream();

	virtual size_t read(unsigned char* buf, size_t bufSize);
	virtual void write(const unsigned char* buf, size_t count);

private:
	int m_socket;
};

} // namespace net
