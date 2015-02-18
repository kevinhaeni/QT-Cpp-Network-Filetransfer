#pragma once

#include <msg/IMessage.hpp>

/// Message 'endpoint system info'
class MessageRequestSysInfo : public msg::Message
{
public:
	MessageRequestSysInfo();

	virtual void save(TOStream& out);

	virtual void load(TIStream& in);

	std::string m_sysinfo;
};
