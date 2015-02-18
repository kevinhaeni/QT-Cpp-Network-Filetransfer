#include "stdafx.h"
#include "TcpAddress.hpp"
#include <util/Error.hpp>
#include <util/utils.h>

namespace net {

TcpAddress::TcpAddress(const std::string& address)
: m_port(0)
{
	size_t len = address.length();
	const char* szAddress = address.c_str();

	size_t pos = address.find_last_of(":");
	if (std::string::npos == pos || pos >= len - 1)
		throw util::Error("Invalid TCP address string: " + address + ", port not specified");

	const char* szPort = szAddress + pos + 1;
	m_port = atoi(szPort);

	if (0 == pos)
		throw util::Error("Host name is not specified: " + address);

	m_host = std::string(szAddress, szAddress + pos);

	if (std::string::npos != m_host.find_first_not_of("0123456789."))
	{
		m_host = resolveName(m_host);
	}
}

const char*
TcpAddress::host() const
{
	return m_host.c_str();
}

unsigned int
TcpAddress::port() const
{
	return m_port;
}

std::string
TcpAddress::resolveName(const std::string& hostName)
{
	std::string ip;

    addrinfo* addrInfo = 0;
    int res = ::getaddrinfo(hostName.c_str(), NULL, NULL, &addrInfo);
	if (res)
		throw util::Error("Failed to resolve hostname: \"" + hostName + "\"");

    if (addrInfo)
    {
        sockaddr* socketAddress = addrInfo->ai_addr;
        chkptr(socketAddress);

        const unsigned char* tons = reinterpret_cast<const unsigned char*>(&(socketAddress->sa_data[2]));
        if (tons)
		{
			std::stringstream sip;
			sip << (int)tons[0] << "." << (int)tons[1] << "." << (int)tons[2] << "." << (int)tons[3];
			ip = sip.str();
		}
    }

	::freeaddrinfo(addrInfo);

	if (ip.empty())
		throw util::Error("Failed to resolve hostname: \"" + hostName + "\"");

	return ip;
}

} // namespace net
