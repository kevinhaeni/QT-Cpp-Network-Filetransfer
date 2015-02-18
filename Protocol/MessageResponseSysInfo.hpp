#pragma once

#include <vector>
#include <string>
#include <msg/IMessage.hpp>

/// Message 'endpoint system info'
class MessageResponseSysInfo : public msg::Message
{
public:
	MessageResponseSysInfo();
	MessageResponseSysInfo(const std::vector<std::string>& info);

	virtual void save(TOStream& out);

	virtual void load(TIStream& in);

	std::vector<std::string> m_sysinfo;
};
