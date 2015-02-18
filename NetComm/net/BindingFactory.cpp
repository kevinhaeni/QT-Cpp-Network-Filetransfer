#include "stdafx.h"
#include "BindingFactory.hpp"
#include "TcpClient.hpp"
#include "TcpServer.hpp"
#include <util/Error.hpp>

namespace net {

BindingFactory::BindingFactory()
{
}

BindingFactory::~BindingFactory()
{
}

net::TBindingPtr
BindingFactory::createBinding(
	BindingType bindingType)
{
	net::TBindingPtr binding = 0;

	switch (bindingType)
	{
	case BINDING_TCP_SERVER:
		binding = new TcpServer;
		break;
	case BINDING_TCP_CLIENT:
		binding = new TcpClient;
		break;
	default:
		assert(!"Unknown binding type");
		throw util::Error("Unknown binding type");
	}

	chkptr(binding.get());
	return binding;
}

} // namespace net
