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
	const char* szDir = m_dir.c_str();
	unsigned int lenDir = m_dir.length();
	out << lenDir;
	out.write(reinterpret_cast<const unsigned char*>(szDir), lenDir);
}

void
MessageRequestDir::load(TIStream& in)
{
	unsigned int lenDir = 0;
	in >> lenDir;

	util::ScopedArray<char> buf(new char[lenDir + 1]);
	memset(buf.get(), 0, lenDir + 1);

	in.read(reinterpret_cast<unsigned char*>(buf.get()), lenDir);

	m_dir = buf.get();
}
