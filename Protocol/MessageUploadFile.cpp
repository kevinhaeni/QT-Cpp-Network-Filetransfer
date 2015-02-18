#include "MessageUploadFile.hpp"
#include "SvcMsgFactory.hpp"
#include <fstream>

MessageUploadFile::MessageUploadFile()
	: Message(SvcMsgFactory::MSG_UPLOAD_FILE)
{
}

void MessageUploadFile::save(TOStream& out)
{
	m_chunk.save(out);
}

void MessageUploadFile::load(TIStream& in)
{
	m_chunk.load(in);
}
