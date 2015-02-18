#pragma once

#include <sstream>

namespace util {

class MemoryStream : public std::basic_stringstream<unsigned char>
{
	typedef std::basic_stringstream<unsigned char> TBase;

public:
	MemoryStream()
	{
	}

	MemoryStream(const unsigned char* begin_, const unsigned char* end_)
	: TBase(std::basic_string<unsigned char>(begin_, end_))
	{
	}

	template<typename T>
	MemoryStream& operator <<(T i) {
		return writePrimitive(i);
	}

	template<typename T>
	MemoryStream& operator >>(T& i) {
		return readPrimitive(i);
	}

	template<typename T>
	MemoryStream& operator <<(std::basic_string<T> s) {
		size_t len = s.length();
		writePrimitive(len);
		write(reinterpret_cast<const unsigned char*>(s.c_str()), len * sizeof(T));
	}

	template<typename T>
	MemoryStream& operator >>(std::basic_string<T>& s) {
		size_t len = 0;
		readPrimitive(len);

		util::ScopedArray<T> buf(new T[len + 1]);
		memset(buf.get(), 0, (len + 1) * sizeof(T));

		read(reinterpret_cast<unsigned char*>(buf.get()), len * sizeof(T));

		s = buf.get();
	}

private:

	template<typename T>
	MemoryStream& writePrimitive(T t)
	{
		write(reinterpret_cast<const unsigned char*>(&t), sizeof(t));
		return *this;
	}

	template<typename T>
	MemoryStream& readPrimitive(T& t)
	{
		read(reinterpret_cast<unsigned char*>(&t), sizeof(t));
		return *this;
	}
};

template<typename T>
inline
MemoryStream& operator <<(MemoryStream& st, T t)
{
	return st.operator <<(t);
}

template<typename T>
inline
MemoryStream& operator >>(MemoryStream& st, T& t)
{
	return st.operator >>(t);
}

} // namespace msg
