#pragma once

#include "ScopedLock.hpp"
#include "ThreadMutex.hpp"
#include "utils.h"

namespace util {

namespace detail {

/**
 * Counts references in a thread-safe way.
 */
template<class T>
class SharedPtrHolder
{
public:
	SharedPtrHolder(T* p)
	: m_refCount(0),
	  m_p(p)
	{
		chkptr(m_p);
		addref();
	}

	~SharedPtrHolder()
	{
		chkptr(m_p);
		delete m_p;
	}

	/// Increments reference count
	void addref()
	{
		ScopedLock lock(&m_sync);
		++m_refCount;
	}

	/**
	 * Decrements reference count.
	 * Deletes holder object when ref count drops to zero.
	 */
	void release()
	{
		// Store ref count in a var because m_sync should be released befoe being deleted
		int refCount = 0;
		{
			ScopedLock lock(&m_sync);
			refCount = --m_refCount;
		}

		assert(0 <= refCount);
		if (0 >= refCount)
			delete this;
	}

	/// Returnes untyped pointer
	T* get()
	{
		ScopedLock lock(&m_sync);
		return m_p;
	}

	/// Returns untyped const-pointer
	const T* get() const
	{
		ScopedLock lock(&m_sync);
		return m_p;
	}

protected:
	mutable ThreadMutex m_sync;
	T* m_p;
	int m_refCount;
};

} // namespace detail

/**
 * Basic shared pointer implementation
 */
template<class T>
class SharedPtr
{
public:
	SharedPtr()
	: m_holder(0)
	{
	}

	SharedPtr(T* p)
	: m_holder(p ? new detail::SharedPtrHolder<T>(p) : 0)
	{
		if (p)
		{
			chkptr(m_holder);
		}
	}

	SharedPtr(const SharedPtr<T>& rhs)
	: m_holder(rhs.m_holder)
	{
		if (0 != m_holder)
		{
			m_holder->addref();
		}
	}

	~SharedPtr()
	{
		ScopedLock lock(&m_sync);
		if (0 != m_holder)
			m_holder->release();
	}

	SharedPtr<T> operator=(T* p)
	{
		ScopedLock lock(&m_sync);
		if (m_holder)
		{
			m_holder->release();
			m_holder = 0;
		}

		if (p)
		{
			m_holder = new detail::SharedPtrHolder<T>(p);
			chkptr(m_holder);
		}

		return *this;
	}

	SharedPtr<T> operator=(const SharedPtr<T>& rhs)
	{
		ScopedLock lock(&m_sync);
		if (m_holder &&
			m_holder != rhs.m_holder)
		{
			m_holder->release();
			m_holder = 0;
		}

		m_holder = rhs.m_holder;
		if (0 != m_holder)
		{
			m_holder->addref();
		}

		return *this;
	}

	/// Returns a pointer to an object
	T* get()
	{
		ScopedLock lock(&m_sync);
		return m_holder ? m_holder->get() : 0;
	}

	/// Returns a constant pointer to an object
	const T* get() const
	{
		ScopedLock lock(&m_sync);
		return m_holder ? m_holder->get() : 0;
	}

	/// Dereference for convenient access of members
	T* operator ->()
	{
		return get();
	}

	/// Dereference as a const pointer for convenient access of members
	const T* operator ->() const
	{
		return get();
	}

	operator bool() const
	{
		return 0 != get();
	}

	/// Checks if this instance is equal to another instance of SharedPtr
	bool operator ==(const SharedPtr<T>& rhs) const
	{
		ScopedLock lock(&m_sync);
		return m_holder == rhs.m_holder;
	}

	/// Checks if this instance is not equal to another instance of SharedPtr
	bool operator !=(const SharedPtr<T>& rhs) const
	{
		return !operator==(rhs);
	}

	/// Compares this instance to another instance of SharedPtr
	bool operator <(const SharedPtr<T>& rhs) const
	{
		ScopedLock lock(&m_sync);
		return m_holder < rhs.m_holder;
	}

private:
	mutable ThreadMutex m_sync;
	detail::SharedPtrHolder<T>* m_holder;
};

} // namespace util
