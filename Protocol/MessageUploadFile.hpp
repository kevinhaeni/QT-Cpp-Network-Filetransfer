#pragma once

#include <msg/IMessage.hpp>
#include "DataTypes.hpp"

/// Message 'Response file'
class MessageUploadFile : public msg::Message
{
public:
	MessageUploadFile();

	virtual void save(TOStream& out);
	virtual void load(TIStream& in);

	FileChunk m_chunk;
};
