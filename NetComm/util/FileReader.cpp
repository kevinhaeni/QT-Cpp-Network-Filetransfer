#include "FileReader.hpp"

namespace util
{

FileReader::FileReader()
	: m_file(NULL)
	, m_size(0)
{
}

FileReader::~FileReader()
{
	close();
}

bool FileReader::open(const std::string& name)
{
	if (!name.empty() && (m_name != name))
	{
		close();
		if (fopen_s(&m_file, name.c_str(), "rb") == 0)
		{
			m_name = name;
			// Get the file size
			_fseeki64(m_file, 0, SEEK_END);
			m_size = _ftelli64(m_file);
			rewind(m_file);
		}
		else
		{
			m_file = NULL;
		}
	}
	return (m_file != NULL);
}

void FileReader::close()
{
	if (m_file)
	{
		fclose(m_file);
		m_file = NULL;
		m_name.clear();
	}
}

bool FileReader::read(std::vector<char>& buf, __int64 startFrom, int size)
{
	if (!m_file || (startFrom + size) > m_size)
		return false;

	// Seek to position
	if (startFrom != _ftelli64(m_file))
	{
		_fseeki64(m_file, startFrom, SEEK_SET);
	}

	// Read data
	bool result = true;
	buf.clear();
	buf.resize(size);
	if (size != 0)
	{
		result = (fread_s(&buf.front(), size, 1, size, m_file) == size);
		if (!result)
			buf.clear();
	}

	// Close file if end was reached
	if (_ftelli64(m_file) == m_size)
		close();

	return result;
}

__int64 FileReader::size() const
{
	return m_size;
}

} //namespace util