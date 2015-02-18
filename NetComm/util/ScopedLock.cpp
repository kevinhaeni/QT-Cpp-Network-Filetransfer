#include "ScopedLock.hpp"
#include "utils.h"

namespace util {

ScopedLock::ScopedLock(util::ISyncObject* sync)
: m_sync(sync)
{
	chkptr(m_sync);
	m_sync->lock();
}

ScopedLock::~ScopedLock()
{
	chkptr(m_sync);
	m_sync->unlock();
}

} // namespace util
