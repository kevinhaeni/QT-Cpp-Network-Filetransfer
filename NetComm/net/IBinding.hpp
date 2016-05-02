#pragma once

#include <string>
#include <memory>
#include "IStream.hpp"

namespace net {

/**
 * Delegate for binding events
 */
struct IBindingDelegate
{
	virtual ~IBindingDelegate() {}

	/**
	 * Stream connection event.
	 */
	virtual void onStreamCreated(::net::TStreamPtr stream) = 0;
};

/**
 * Base interface for binding implementations.
 */
struct IBinding
{
	virtual ~IBinding() {}

	/// Binds to an address specified. Address string is specific to a binding implementation.
	virtual void bind(
		const std::string& address,
		IBindingDelegate* delegate_) = 0;
};

//typedef ::util::SharedPtr<IBinding> TBindingPtr;
typedef std::shared_ptr<IBinding> TBindingPtr;

} // namespace net
