#pragma once

#include <string>
#include <vector>

namespace util
{
class MemoryStream;
}

struct DirItem
{
	DirItem()
		: m_isDir(false)
	{}

	void save(util::MemoryStream& out);
	void load(util::MemoryStream& in);

	std::wstring m_name;
	bool m_isDir;
};

typedef std::vector<DirItem> TDirItems;

struct FileRequest
{
	FileRequest()
		: m_startFrom(0)
	{}

	void save(util::MemoryStream& out);
	void load(util::MemoryStream& in);

	std::wstring m_fileName;
	__int64 m_startFrom;
};

struct FileChunk
{
	FileChunk()
		: m_fileSize(0)
		, m_positionFrom(0)
		, m_valid(false)
	{}

	void save(util::MemoryStream& out);
	void load(util::MemoryStream& in);

	std::wstring m_fileName;
	__int64 m_fileSize;
	__int64 m_positionFrom;
	std::vector<char> m_fileData;
	bool m_valid;
};
