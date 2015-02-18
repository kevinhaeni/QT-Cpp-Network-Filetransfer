#pragma once

namespace net {

/**
 * Parses TCP address string.
 * Address string must be in the following format: IP:PORT or FQDN:PORT
 */
class TcpAddress
{
public:
	TcpAddress(const std::string& address);

	/// Returns host string
	const char* host() const;

	/// Returns port number
	unsigned int port() const;

private:
	std::string m_host;
	unsigned int m_port;

	static std::string resolveName(const std::string& hostName);
};

} // namespace net
