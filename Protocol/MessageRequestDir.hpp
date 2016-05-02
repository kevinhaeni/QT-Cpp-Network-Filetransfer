#pragma once

#include <msg/IMessage.hpp>

/// Message 'request dir contents'
class MessageRequestDir : public msg::Message
{
public:
	MessageRequestDir();

	virtual void save(TOStream& out);
	virtual void load(TIStream& in);

	std::wstring m_dir;
};
