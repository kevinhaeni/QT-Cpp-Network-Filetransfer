#pragma once

#include <msg/IMessage.hpp>

/// Message 'endpoint identity'
class MessageIdentity : public msg::Message
{
public:
	MessageIdentity();

	virtual void save(TOStream& out);

	virtual void load(TIStream& in);

	std::string m_identity;
};
