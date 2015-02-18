#include "MessageIdentity.hpp"

#include <windows.h>
#include <cstdlib>
#include <ctime>
#include <util/ScopedArray.hpp>

#include "SvcMsgFactory.hpp"


MessageIdentity::MessageIdentity()
	: Message(SvcMsgFactory::MSG_IDENTITY)
{
}

void
MessageIdentity::save(TOStream& out)
{
	if (m_identity.empty())
	{
		std::stringstream sid;

		// try to get computer name as identity
		const char* szIdentity = getenv("COMPUTERNAME");
		if (szIdentity && *szIdentity)
		{
			sid << szIdentity;
		}
		else
		{
			// Some random identity
			srand(time(0) && 0xFFFF);
			sid << rand();
		}

		// Append process ID
		sid << "-" << (int)::GetCurrentProcessId();

		m_identity = sid.str();
	}

	const char* sz = m_identity.c_str();
	unsigned int len = m_identity.length();
	out << len;
	out.write(reinterpret_cast<const unsigned char*>(sz), len);
}

void
MessageIdentity::load(TIStream& in)
{
	unsigned int len = 0;
	in >> len;

	util::ScopedArray<char> buf(new char[len + 1]);
	memset(buf.get(), 0, len + 1);

	in.read(reinterpret_cast<unsigned char*>(buf.get()), len);

	m_identity = buf.get();
}
