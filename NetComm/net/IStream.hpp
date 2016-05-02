#pragma once

#include <memory>
#include "util/utils.h"
#include "util/ThreadMutex.hpp"
#include "util/ScopedLock.hpp"

namespace net {

/**
 * Base interface for client bindings implementations.
 */
struct IStream
{
	virtual ~IStream() {}

	typedef void* TId;

	/// Unique stream identifier
	virtual TId id() const
	{
		return (TId)(this);
	}

	/// Implementation must not block if no data available, rather it must return 0
	virtual size_t read(unsigned char* buf, size_t bufSize) = 0;

	/// Implementation is expected to write() to a stream synchroniosly, but this is not mandatory.
	virtual void write(const unsigned char* buf, size_t count) = 0;
};

typedef std::shared_ptr<IStream> TStreamPtr;

} // namespace net
