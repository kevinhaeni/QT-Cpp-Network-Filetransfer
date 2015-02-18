#pragma once

#include <util/utils.h>
#include "IMessage.hpp"

namespace msg {

/**
 * Basic interface for message factory implementation.
 */
struct IMessageFactory
{
	virtual ~IMessageFactory() {}

	/// Creates a message instance by its type
	virtual ::msg::TMessagePtr createMessage(util::T_UI4 messageType) = 0 {
		util::StaticAssert<(sizeof(util::T_UI1) == 1)>();
	}
};

} // namespace msg
