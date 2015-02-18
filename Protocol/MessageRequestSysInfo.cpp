#include "MessageRequestSysInfo.hpp"

#include <util/ScopedArray.hpp>

#include "SvcMsgFactory.hpp"

#pragma warning(disable: 4996)

MessageRequestSysInfo::MessageRequestSysInfo()
	: Message(SvcMsgFactory::MSG_REQUEST_SYSINFO)
{
}

void
MessageRequestSysInfo::save(TOStream& out)
{
	const char* szReq = m_sysinfo.c_str();
	unsigned int lenReq = m_sysinfo.length();
	out << lenReq;
	out.write(reinterpret_cast<const unsigned char*>(szReq), lenReq);
}

void MessageRequestSysInfo::load(TIStream& in)
{
	unsigned int len = 0;
	in >> len;

	util::ScopedArray<char> buf(new char[len + 1]);
	memset(buf.get(), 0, len + 1);

	in.read(reinterpret_cast<unsigned char*>(buf.get()), len);

	m_sysinfo = buf.get();
}
