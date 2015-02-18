#include "MessageResponseFile.hpp"
#include "SvcMsgFactory.hpp"
#include <fstream>

MessageResponseFile::MessageResponseFile()
	: Message(SvcMsgFactory::MSG_RESPONSE_FILE)
{
}

void MessageResponseFile::save(TOStream& out)
{
	m_response.save(out);
}

void MessageResponseFile::load(TIStream& in)
{
	m_response.load(in);
}
