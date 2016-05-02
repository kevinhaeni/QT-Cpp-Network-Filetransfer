#pragma once

#include <iostream>
#include <util/utils.h>
#include <memory>
#include <util/MemoryStream.hpp>

namespace msg {

/**
 * Base interface for all messages.
 * Message is a packet of fixed (but arbitrary) width.
 * Messages are used to encapsulate entities as opposed to raw streams of bytes.
 */
struct IMessage
{
	virtual ~IMessage() {}

	virtual util::T_UI4 typeId() const = 0
	{
		util::StaticAssert<sizeof(util::T_UI4) == 4>();
	}

	typedef util::MemoryStream TOStream;
	typedef util::MemoryStream TIStream;

	/// Serializes message into a provided stream
	virtual void save(TOStream& out) = 0;

	/// Deserializes message from a provided stream
	virtual void load(TIStream& in) = 0;
};

class Message : public IMessage
{
public:
	Message(util::T_UI4 typeId)
		: m_typeId(typeId)
	{}

	virtual util::T_UI4 typeId() const
	{
		return m_typeId;
	}

private:
	util::T_UI4 m_typeId;
};

typedef std::shared_ptr<IMessage> TMessagePtr;

} // namespace msg
