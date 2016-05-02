#include "stdafx.h"
#include "TcpClient.hpp"
#include "TcpAddress.hpp"
#include "TcpStream.hpp"
#include "WSAError.hpp"

namespace net {

TcpClient::TcpClient()
{
}

TcpClient::~TcpClient()
{
}

void
TcpClient::bind(
	const std::string& address,
	IBindingDelegate* delegate_)
{
	TcpAddress tcpAddress(address);

    int sockfd = 0;
	int n = 0;
    char recvBuff[1024];
    struct sockaddr_in serv_addr; 

    memset(recvBuff, 0, sizeof(recvBuff));
	if (SOCKET_ERROR == (sockfd = ::socket(AF_INET, SOCK_STREAM, 0)))
		throw WSAError();

    memset(&serv_addr, 0, sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(tcpAddress.host());
	serv_addr.sin_port = htons(tcpAddress.port()); 

	assert(serv_addr.sin_addr.s_addr != INADDR_NONE);

	if (SOCKET_ERROR == ::connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)))
		throw WSAError();

	TStreamPtr stream = std::make_shared<TcpStream>(sockfd);

	if (delegate_)
		delegate_->onStreamCreated(stream);
}

} // namespace net
