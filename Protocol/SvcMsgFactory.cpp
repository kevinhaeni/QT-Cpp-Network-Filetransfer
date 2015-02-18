#include "SvcMsgFactory.hpp"

#include <util/Error.hpp>

#include "MessageIdentity.hpp"
#include "MessageRequestDir.hpp"
#include "MessageResponseDir.hpp"
#include "MessageRequestFile.hpp"
#include "MessageResponseFile.hpp"
#include "MessageRequestSysInfo.hpp"
#include "MessageResponseSysInfo.hpp"
#include "MessageUploadFile.hpp"
#include "MessageUploadFileReply.hpp"

::msg::TMessagePtr
SvcMsgFactory::createMessage(util::T_UI4 messageType)
{
	::msg::TMessagePtr message;

	switch (messageType)
	{
	case MSG_IDENTITY:
		message = new MessageIdentity;
		break;
	case MSG_REQUEST_DIR:
		message = new MessageRequestDir;
		break;
	case MSG_RESPONSE_DIR:
		message = new MessageResponseDir;
		break;
	case MSG_REQUEST_FILE:
		message = new MessageRequestFile;
		break;
	case MSG_RESPONSE_FILE:
		message = new MessageResponseFile;
		break;
	case MSG_REQUEST_SYSINFO:
		message = new MessageRequestSysInfo;
		break;
	case MSG_RESPONSE_SYSINFO:
		message = new MessageResponseSysInfo;
		break;
	case MSG_UPLOAD_FILE:
		message = new MessageUploadFile;
		break;
	case MSG_UPLOAD_FILE_REPLY:
		message = new MessageUploadFileReply;
		break;
	default:
		assert(!"Unsupported message type");
		throw util::Error("Unsupported mesage typ");
	}

	return message;
}
