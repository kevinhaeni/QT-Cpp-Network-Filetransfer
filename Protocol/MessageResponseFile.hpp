#pragma once

#include <msg/IMessage.hpp>
#include "DataTypes.hpp"

/// Message 'Response file'
class MessageResponseFile : public msg::Message
{
public:
	MessageResponseFile();

	virtual void save(TOStream& out);
	virtual void load(TIStream& in);

	FileChunk m_response;
};
