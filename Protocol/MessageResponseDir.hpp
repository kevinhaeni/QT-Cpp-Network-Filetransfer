#pragma once

#include <msg/IMessage.hpp>
#include "DataTypes.hpp"

/// Message 'response with dir contents'
class MessageResponseDir : public msg::Message
{
public:
	MessageResponseDir();

	virtual void save(TOStream& out);
	virtual void load(TIStream& in);

	TDirItems m_content;
};
