#pragma once

#include <msg/IMessage.hpp>
#include "DataTypes.hpp"

/// Message 'request file'
class MessageRequestFile : public msg::Message
{
public:
	MessageRequestFile();

	virtual void save(TOStream& out);

	virtual void load(TIStream& in);

	FileRequest m_request;
};
