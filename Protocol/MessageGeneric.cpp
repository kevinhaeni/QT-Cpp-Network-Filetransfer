#include "MessageGeneric.hpp"

#include <windows.h>
#include <cstdlib>
#include <ctime>
#include <util/ScopedArray.hpp>

#include "SvcMsgFactory.hpp"


MessageGeneric::MessageGeneric()
	: Message(SvcMsgFactory::MSG_GENERIC)
{
}

void
MessageGeneric::save(TOStream& out)
{

	util::T_UI4 val = m_commandType;
	out << val;

	unsigned int count = m_params.size();
	out << count;

	for (auto i : m_params) {
		const char* sz = i.c_str();
		unsigned int len = i.length();
		out << len;
		out.write(reinterpret_cast<const unsigned char*>(sz), len);
	}
}

void
MessageGeneric::load(TIStream& in)
{
	util::T_UI4 val;
	in >> val;
	m_commandType = val;

	unsigned int count = 0;
	in >> count;

	for (int i = 1; i <= count; i++){
		unsigned int len = 0;
		in >> len;

		util::ScopedArray<char> buf(new char[len + 1]);
		memset(buf.get(), 0, len + 1);

		in.read(reinterpret_cast<unsigned char*>(buf.get()), len);

		m_params.push_back(buf.get());
	}
}
