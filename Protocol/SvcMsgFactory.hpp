#pragma once

#include <msg/IMessageFactory.hpp>

class SvcMsgFactory : public msg::IMessageFactory
{
public:
	enum
	{
		MSG_IDENTITY = 1,
		MSG_REQUEST_DIR,
		MSG_RESPONSE_DIR,
		MSG_REQUEST_FILE,
		MSG_RESPONSE_FILE,
		MSG_REQUEST_SYSINFO,
		MSG_RESPONSE_SYSINFO,
		MSG_UPLOAD_FILE,
		MSG_UPLOAD_FILE_REPLY,
		MSG_GENERIC
	};

	virtual ::msg::TMessagePtr createMessage(util::T_UI4 messageType);
};
