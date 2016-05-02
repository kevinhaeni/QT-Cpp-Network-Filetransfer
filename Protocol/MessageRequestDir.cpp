#include "MessageRequestDir.hpp"

#include <util/ScopedArray.hpp>

#include "SvcMsgFactory.hpp"

MessageRequestDir::MessageRequestDir()
	: Message(SvcMsgFactory::MSG_REQUEST_DIR)
{
}

void
MessageRequestDir::save(TOStream& out)
{
	
	size_t len = m_dir.size();
	out << len;

	size_t sz = m_dir.size()  * sizeof(wchar_t);
	out << sz;
	if (sz)
		out.write((const unsigned char*)m_dir.c_str(), sz);	
	/*const char* szDir = m_dir.c_str();
	unsigned int lenDir = m_dir.length();
	out << lenDir;
	out.write(reinterpret_cast<const unsigned char*>(szDir), lenDir);*/
}

void
MessageRequestDir::load(TIStream& in)
{
	size_t len;
	in >> len;
	m_dir.resize(len);

	size_t sz;
	in >> sz;

	if (sz)
		in.read((unsigned char*)&m_dir.front(), sz);
}
