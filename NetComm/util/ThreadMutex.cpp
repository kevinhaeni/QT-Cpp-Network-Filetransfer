#include "ThreadMutex.hpp"
#include <windows.h>

namespace util {

ThreadMutex::ThreadMutex()
: m_cs(0)
{
	m_cs = new CRITICAL_SECTION;
	::InitializeCriticalSection(static_cast<LPCRITICAL_SECTION>(m_cs));
}

ThreadMutex::~ThreadMutex()
{
	::DeleteCriticalSection(static_cast<LPCRITICAL_SECTION>(m_cs));

	delete static_cast<LPCRITICAL_SECTION>(m_cs);
	m_cs = 0;
}

void
ThreadMutex::lock()
{
	::EnterCriticalSection(static_cast<LPCRITICAL_SECTION>(m_cs));
}

void
ThreadMutex::unlock()
{
	::LeaveCriticalSection(static_cast<LPCRITICAL_SECTION>(m_cs));
}

} // namespace util
