#pragma once

#include "IBinding.hpp"

namespace net {

class BindingFactory;

/**
 * TCP client binding implementation
 */
class TcpClient : public IBinding
{

public:
	/// Must be created only by means of BindingFactory
	TcpClient();
	~TcpClient();

	/**
	 * Connects to an address specified.
	 * Address string must be in the following format: IP:PORT or FQDN:PORT
	 */
	virtual void bind(
		const std::string& address,
		IBindingDelegate* delegate_);
};

} // namespace net
