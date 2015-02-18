#pragma once

namespace util {

template<typename T>
struct ScopedArray
{
	ScopedArray() : m_arr(0)
	{
	}

	ScopedArray(T* arr) : m_arr(arr)
	{
		chkptr(m_arr);
	}

	~ScopedArray()
	{
		if (m_arr)
		{
			delete[] m_arr;
			m_arr = 0;
		}
	}

	void reset(T* arr)
	{
		if (m_arr)
		{
			delete[] m_arr;
			m_arr = 0;
		}

		m_arr = arr;
		chkptr(m_arr);
	}

	T* get()
	{
		return m_arr;
	}

	const T* get() const
	{
		return m_arr;
	}

private:
	T* m_arr;
};

} // namespace util
