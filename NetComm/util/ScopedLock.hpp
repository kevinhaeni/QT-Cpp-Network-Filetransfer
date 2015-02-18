#pragma once

#include "ISyncObject.hpp"

namespace util {

/// Provides scoped locking for synchronization objects.
class ScopedLock
{
public:
	ScopedLock(util::ISyncObject* sync);
	~ScopedLock();

private:
	util::ISyncObject* m_sync;
};

} // namespace util
