#include "stdafx.h"
#include "WSAError.hpp"

namespace net {

WSAError::WSAError()
{
	std::stringstream msg;
	msg << "WSA error occured with code: " << ::WSAGetLastError();
	m_errorDescription = msg.str();
}

} // namespace net
