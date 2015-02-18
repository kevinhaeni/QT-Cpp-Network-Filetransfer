#pragma once

#include "ISyncObject.hpp"

namespace util {

/// Thread mutex implemented by means of a critical section
class ThreadMutex : public ISyncObject
{
public:
	ThreadMutex();
	~ThreadMutex();

	virtual void lock();
	virtual void unlock();

private:
	void* m_cs;
};

} // namespace util
