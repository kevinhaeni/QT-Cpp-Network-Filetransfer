#pragma once

#include <util/Error.hpp>

namespace net {

class WSAError : public util::Error
{
public:
	WSAError();
};

} // namespace net
