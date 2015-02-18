#pragma once

#include "IBinding.hpp"

namespace net {

/**
 * Bindings factory. Create bindings by calling createBinding() method.
 */
class BindingFactory
{
	BindingFactory();

public:
	~BindingFactory();

	/// Supported binding types
	enum BindingType
	{
		UNDEFINED = 0,
		BINDING_TCP_SERVER,
		BINDING_TCP_CLIENT
	};

	/// Creates bindings by a type specified
	static net::TBindingPtr createBinding(
		BindingType bindingType);
};

} // namespace net
