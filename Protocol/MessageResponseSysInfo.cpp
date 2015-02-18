#include "MessageResponseSysInfo.hpp"

#include <util/ScopedArray.hpp>

#include "SvcMsgFactory.hpp"

#pragma warning(disable: 4996)

MessageResponseSysInfo::MessageResponseSysInfo()
	: Message(SvcMsgFactory::MSG_RESPONSE_SYSINFO)
{
}


MessageResponseSysInfo::MessageResponseSysInfo(const std::vector<std::string>& info)
	: Message(SvcMsgFactory::MSG_RESPONSE_SYSINFO)
	, m_sysinfo(info)
{
}

void MessageResponseSysInfo::save(TOStream& out)
{
	if (m_sysinfo.empty())
	{
		m_sysinfo.push_back("No information about endpoint available");
	}

	unsigned int count = m_sysinfo.size();
	out << count;

	for(auto i : m_sysinfo) {
		const char* sz = i.c_str();
		unsigned int len = i.length();
		out << len;
		out.write(reinterpret_cast<const unsigned char*>(sz), len);
	}
}

void MessageResponseSysInfo::load(TIStream& in)
{
	unsigned int count = 0;
	in >> count;

	for(int i = 1; i <= count; i++){
		unsigned int len = 0;
		in >> len;

		util::ScopedArray<char> buf(new char[len + 1]);
		memset(buf.get(), 0, len + 1);

		in.read(reinterpret_cast<unsigned char*>(buf.get()), len);

		m_sysinfo.push_back(buf.get());
	}
}
