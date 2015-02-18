#include "MessageUploadFileReply.hpp"
#include "SvcMsgFactory.hpp"

MessageUploadFileReply::MessageUploadFileReply()
	: Message(SvcMsgFactory::MSG_UPLOAD_FILE_REPLY)
	, m_ok(false)
{
}

void MessageUploadFileReply::save(TOStream& out)
{
	out << m_ok;
}

void MessageUploadFileReply::load(TIStream& in)
{
	in >> m_ok;
}
