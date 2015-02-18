#include "DataTypes.hpp"

#include <util/MemoryStream.hpp>

void DirItem::save(util::MemoryStream& out)
{
	size_t sz = m_name.size();
	out << sz;
	if (sz)
		out.write((const unsigned char*)m_name.c_str(), sz);
	out << m_isDir;
}

void DirItem::load(util::MemoryStream& in)
{
	size_t sz;
	in >> sz;
	m_name.resize(sz);
	if (sz)
		in.read((unsigned char*)&m_name.front(), sz);
	in >> m_isDir;
}


void FileRequest::save(util::MemoryStream& out)
{
	size_t sz = m_fileName.size();
	out << sz;
	if (sz)
		out.write((const unsigned char*)m_fileName.c_str(), sz);

	out << m_startFrom;
}

void FileRequest::load(util::MemoryStream& in)
{
	size_t sz;
	in >> sz;
	m_fileName.resize(sz);
	if (sz)
		in.read((unsigned char*)&m_fileName.front(), sz);

	in >> m_startFrom;
}


void FileChunk::save(util::MemoryStream& out)
{
	size_t sz = m_fileName.size();
	out << sz;
	if (sz)
		out.write((const unsigned char*)m_fileName.c_str(), sz);
	out << m_fileSize;
	out << m_positionFrom;
	sz = m_fileData.size();
	out << sz;
	if (sz)
		out.write((const unsigned char*)&m_fileData.front(), sz);
	out << m_valid;
}

void FileChunk::load(util::MemoryStream& in)
{
	size_t sz;
	in >> sz;
	m_fileName.resize(sz);
	if (sz)
		in.read((unsigned char*)&m_fileName.front(), sz);

	in >> m_fileSize;
	in >> m_positionFrom;

	in >> sz;
	m_fileData.resize(sz);
	if (sz)
		in.read((unsigned char*)&m_fileData.front(), sz);

	in >> m_valid;
}