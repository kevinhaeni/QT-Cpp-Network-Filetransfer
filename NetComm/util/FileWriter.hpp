#pragma once

#include <stdio.h>
#include <vector>
#include <string>

namespace util
{

class FileWriter
{
public:
	FileWriter();
	~FileWriter();

	bool open(const std::string& name);
	void close();
	bool write(const std::vector<char>& buf);
	__int64 size() const;

private:
	FILE* m_file;
	std::string m_name;
};

} // namespace util