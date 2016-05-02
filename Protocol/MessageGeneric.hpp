#pragma once

#include <msg/IMessage.hpp>
#include <vector>
#include <string>

/// Message 'endpoint identity'
class MessageGeneric : public msg::Message
{
public:
	enum cmdType
	{
		REQSYSINFO = 0, REQFILEEXEC
	};

	MessageGeneric();

	virtual void save(TOStream& out);

	virtual void load(TIStream& in);

	util::T_UI4 m_commandType;
	std::vector<std::string> m_params;
};
