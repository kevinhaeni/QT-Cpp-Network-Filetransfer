#pragma once

#include <msg/IMessage.hpp>
#include "DataTypes.hpp"

/// Message 'request file'
class MessageUploadFileReply : public msg::Message
{
public:
	MessageUploadFileReply();

	virtual void save(TOStream& out);

	virtual void load(TIStream& in);

	bool m_ok;
};
