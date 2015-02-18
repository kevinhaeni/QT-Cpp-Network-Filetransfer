#include "MessageRequestFile.hpp"
#include "SvcMsgFactory.hpp"

MessageRequestFile::MessageRequestFile()
	: Message(SvcMsgFactory::MSG_REQUEST_FILE)
{
}

void
MessageRequestFile::save(TOStream& out)
{
	m_request.save(out);
}

void
MessageRequestFile::load(TIStream& in)
{
	m_request.load(in);
}
