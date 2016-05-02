#include "SvcMsgFactory.hpp"

#include <util/Error.hpp>

#include "MessageIdentity.hpp"
#include "MessageRequestDir.hpp"
#include "MessageResponseDir.hpp"
#include "MessageRequestFile.hpp"
#include "MessageResponseFile.hpp"
#include "MessageResponseSysInfo.hpp"
#include "MessageUploadFile.hpp"
#include "MessageUploadFileReply.hpp"
#include "MessageGeneric.hpp"

::msg::TMessagePtr
SvcMsgFactory::createMessage(util::T_UI4 messageType)
{
	::msg::TMessagePtr message;

	switch (messageType)
	{
	case MSG_IDENTITY:
		message = std::make_shared<MessageIdentity>();
		break;
	case MSG_GENERIC:
		message = std::make_shared<MessageGeneric>();
		break;
	case MSG_REQUEST_DIR:
		message = std::make_shared<MessageRequestDir>();		
		break;
	case MSG_RESPONSE_DIR:
		message = std::make_shared<MessageResponseDir>();
		break;
	case MSG_REQUEST_FILE:
		message = std::make_shared<MessageRequestFile>();
		break;
	case MSG_RESPONSE_FILE:
		message = std::make_shared<MessageResponseFile>();
		break;
	case MSG_RESPONSE_SYSINFO:
		message = std::make_shared<MessageResponseSysInfo>();
		break;
	case MSG_UPLOAD_FILE:
		message = std::make_shared<MessageUploadFile>();
		break;
	case MSG_UPLOAD_FILE_REPLY:
		message = std::make_shared<MessageUploadFileReply>();
		break;
	default:
		assert(!"Unsupported message type");
		throw util::Error("Unsupported mesage typ");
	}

	return message;
}
