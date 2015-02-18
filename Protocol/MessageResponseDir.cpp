#include "MessageResponseDir.hpp"

#include <util/ScopedArray.hpp>

#include "SvcMsgFactory.hpp"

MessageResponseDir::MessageResponseDir()
	: Message(SvcMsgFactory::MSG_RESPONSE_DIR)
{
}

void
MessageResponseDir::save(TOStream& out)
{
	size_t sz = m_content.size();
	out << sz;

	for(auto item : m_content) {
		item.save(out);
	} 
}

void
MessageResponseDir::load(TIStream& in)
{
	size_t count;
	in >> count;
	m_content.clear();
	m_content.reserve(count);

	for(size_t i = 0; i < count; ++i) {
		DirItem item;
		item.load(in);
		m_content.push_back(item);
	}
}
