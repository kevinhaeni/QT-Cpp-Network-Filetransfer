#include "FileWriter.hpp"

namespace util
{

FileWriter::FileWriter()
	: m_file(NULL)
{
}

FileWriter::~FileWriter()
{
	close();
}

bool FileWriter::open(const std::string& name)
{
	if (!name.empty() && (m_name != name))
	{
		close();
		if (fopen_s(&m_file, name.c_str(), "wb") == 0)
		{
			m_name = name;
		}
		else
		{
			m_file = NULL;
		}
	}
	return (m_file != NULL);
}

void FileWriter::close()
{
	if (m_file)
	{
		fclose(m_file);
		m_file = NULL;
		m_name.clear();
	}
}

bool FileWriter::write(const std::vector<char>& buf)
{
	if (!m_file)
		return false;

	// Write data
	bool result = true;
	size_t sz = buf.size();

	if (sz != 0)
	{
		result = (fwrite(&buf.front(), 1, sz, m_file) == sz);
	}

	return result;
}

__int64 FileWriter::size() const
{
	if (!m_file)
		return 0;
	return _ftelli64(m_file);
}

} //namespace util