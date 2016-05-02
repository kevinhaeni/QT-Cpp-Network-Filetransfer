#pragma once

#include <stdio.h>
#include <vector>
#include <string>

namespace util
{

class FileReader
{
public:
	FileReader();
	~FileReader();

	bool open(const std::wstring& name);
	void close();
	bool read(std::vector<char>& buf, __int64 startFrom, int size);
	__int64 size() const;

private:
	FILE* m_file;
	std::wstring m_name;
	__int64 m_size;
};

} //namespace util
