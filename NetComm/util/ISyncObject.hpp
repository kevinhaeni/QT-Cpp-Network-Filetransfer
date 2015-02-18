#pragma once

namespace util {

/**
 * Base interface for synchronization objects.
 */
struct ISyncObject
{
	virtual ~ISyncObject() {}

	/// Locks object
	virtual void lock() = 0;

	/// Unlocks object
	virtual void unlock() = 0;
};

} // namespace util
