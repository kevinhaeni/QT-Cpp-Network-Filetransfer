#include "stdafx.h"
#include "Messenger.hpp"
#include <util/Error.hpp>
#include <util/ScopedArray.hpp>

#pragma warning(disable: 4996)

#define KMSG_INITIAL_DATA_BUF_SIZE (1024UL * 1024UL * 128UL)

#ifndef NDEBUG

#define ODS(s) { std::stringstream ss; ss << s << "\n"; OutputDebugStringA(ss.str().c_str()); }

namespace {

char
valueToChar(unsigned char value)
{
		if (10 > value)
			return '0' + value;
		else if (16 > value)
			return 'A' + value - 10;
		else
		{
			assert(!"invalid code");
			return '?';
		}
	}

void
dumpBinBuffer(const unsigned char* buf, size_t bufSize)
{
#if 0
		std::string s;
		s.reserve(bufSize * 3 + 2);
		for (size_t i = 0; i < bufSize; ++i)
		{
			unsigned char ch = buf[i];
			unsigned char h = ch >> 4;
			unsigned char l = ch & 0xF;
			s += valueToChar(h);
			s += valueToChar(l);
			s += ' ';
		}

		s += '\n';
		OutputDebugStringA(s.c_str());
#endif
	}

} // namespace

#else // NDEBUG

#define ODS(s)

#endif // NDEBUG

namespace msg {

namespace {

/// Message header sent over a stream
struct MessageHeader
{
	util::T_UI4 messageType;
	util::T_UI4 payloadSize;
};

} // namespace

///////////////////////////////////////////////////////////////////////////////////////////////////
// Messenger

util::ThreadMutex Messenger::s_sync;
std::auto_ptr<Messenger> Messenger::s_instance;

Messenger::Messenger()
: m_messageFactory(0),
  m_bindingDelegate(0)
{
}

Messenger&
Messenger::instance()
{
	util::ScopedLock lock(&s_sync);
	if (0 == s_instance.get())
	{
		s_instance.reset(new Messenger);
		chkptr(s_instance.get());
	}

	return *s_instance;
}

void
Messenger::setMessageFactory(IMessageFactory* messageFactory)
{
	util::ScopedLock lock(&s_sync);
	m_messageFactory = messageFactory;

	if (m_messageFactory)
	{
		chkptr(m_messageFactory);
	}
}

void
Messenger::setBindingDelegate(::msg::IBindingDelegate* bindingDelegate)
{
	util::ScopedLock lock(&s_sync);
	m_bindingDelegate = bindingDelegate;

	if (m_bindingDelegate)
	{
		chkptr(m_bindingDelegate);
	}
}

void
Messenger::onStreamCreated(net::TStreamPtr stream)
{
	net::StreamListener::instance().addDelegate(stream, this);

	::msg::IBindingDelegate* bindingDelegate = 0;
	{
		util::ScopedLock lock(&s_sync);
		bindingDelegate = m_bindingDelegate;
	}

	if (bindingDelegate)
		bindingDelegate->onStreamCreated(stream->id());
}

void
Messenger::onDataReceived(
	::net::IStream::TId streamId,
	const unsigned char* buf,
	size_t bufSize)
{
	util::ScopedLock lock(&s_sync);

	try
	{
		// Append data to corresponding data buffer

		TStreamData::iterator dd = m_streamData.find(streamId);
		if (dd == m_streamData.end())
		{
			dd = m_streamData.insert(std::make_pair(streamId, TData(0))).first;
		}

		TData& data = dd->second;

		// Required data length
		size_t dataSize = data.size();
		size_t requiredLen = dataSize + bufSize;
		size_t availableLen = data.capacity();

#ifndef NDEBUG
		{
			char buf2[128];
			memset(buf2, 0, sizeof(buf2));
			sprintf(buf2, "%p data before: ", reinterpret_cast<void*>(streamId));
			OutputDebugStringA(buf2);

			if (0 < data.size())
				dumpBinBuffer(&data[0], data.size());
			else
				OutputDebugStringA("<empty>\n");
		}
#endif // !NDEBUG

		// Resize data buffer if it is not large enough
		if (availableLen < requiredLen)
		{
			// Reserve 20% more
			size_t reserve = static_cast<size_t>(requiredLen * 1.2);

			if (reserve < KMSG_INITIAL_DATA_BUF_SIZE)
				reserve = KMSG_INITIAL_DATA_BUF_SIZE;

			data.reserve(reserve);
		}

		data.resize(requiredLen);

		// Copy data to buffer
		memcpy(&data[dataSize], buf, bufSize);

		// Try to extract message from data
		while (sizeof(MessageHeader) <= (dataSize = data.size()))
		{
			MessageHeader header;
			memset(&header, 0, sizeof(header));

			assert(data.size() == dataSize);

			unsigned char* pData = &data[0];
			memcpy(&header, pData, sizeof(header));

			util::T_UI4 messageSize = header.payloadSize + sizeof(MessageHeader); // two bytes at the beginning are message type and payload length
			if (messageSize <= dataSize)
			{
				TMessagePtr message;

				try
				{
					if (!m_messageFactory)
					{
						assert(!"Message factory must be set before receiving messages");
						throw std::logic_error("Internal error");
					}

					chkptr(m_messageFactory);
					message = m_messageFactory->createMessage(header.messageType);

					{
						util::ScopedLock lock(&m_memStreamSync);
						util::MemoryStream memstream(pData + sizeof(MessageHeader), pData + messageSize);
						message->load(memstream);
					}

					// Remove message bytes from data
					{
						TData::iterator bb = data.begin();
						std::advance(bb, messageSize);

#ifndef NDEBUG
						size_t dataSizeBefore = data.size();
						assert(dataSizeBefore >= messageSize);
#endif // !NDEBUG

						data.erase(data.begin(), bb);

#ifndef NDEBUG
						size_t dataSizeAfter = data.size();
						assert(dataSizeAfter + messageSize == dataSizeBefore);
						assert(dd->second.size() == dataSizeAfter);

						{
							char buf2[128];
							memset(buf2, 0, sizeof(buf2));
							sprintf(buf2, "%p data after: ", reinterpret_cast<void*>(streamId));
							OutputDebugStringA(buf2);

							if (0 < data.size())
								dumpBinBuffer(&data[0], data.size());
							else
								OutputDebugStringA("<empty>\n");
						}
#endif // !NDEBUG
					}

					TMessengerDelegates::iterator ii = m_messengerDelegates.find(streamId);
					if (ii == m_messengerDelegates.end())
					{
						assert(0);
						throw util::Error("Stream not found");
					}

					IMessengerDelegate* delegate_ = ii->second;
					chkptr(delegate_);

					delegate_->onMessageReceived(streamId, message);
				}
				catch (const std::exception& x)
				{
#ifndef NDEBUG
					const char* szMsg = x.what();
#endif
					// Unknown messages and message handling errors are simple discarded
					ignore_unused(x);
					assert(!"Unknown message type or message handling error");
				}
				catch (...)
				{
					// Unknown messages and message handling errors are simple discarded
					assert(!"Unknown message type or message handling error");
				}
			}
			else
			{
				// Not enough data
				break;
			}
		}
	}
	catch (const std::exception& x)
	{
		// Some error (probably bad_alloc) occured while processing stream data.
		// In this case stream is assumed to be in incosistent state and thus is not used any more.
		net::StreamListener::instance().closeStream(streamId, x.what());
	}
	catch (...)
	{
		// Some unknown error occured while processing stream data.
		// In this case stream is assumed to be in incosistent state and thus is not used any more.
		net::StreamListener::instance().closeStream(streamId, "Unknown error");
	}
}

void
Messenger::onStreamDied(::net::IStream::TId streamId)
{
	IMessengerDelegate* delegate_ = 0;

	{
		util::ScopedLock lock(&s_sync);

		TMessengerDelegates::iterator ii = m_messengerDelegates.find(streamId);
		if (ii != m_messengerDelegates.end())
		{
			delegate_ = ii->second;

			ODS("Removing delegates for stream: " << ii->first);

			m_messengerDelegates.erase(ii);
		}
		else
		{
			assert(!"Stream not found");
		}

		// Clean data associated with the stream
		m_streamData.erase(streamId);
	}

	if (delegate_)
	{
		chkptr(delegate_);
		delegate_->onStreamDied(streamId);
	}
}

void
Messenger::addDelegate(
	::net::IStream::TId streamId,
	IMessengerDelegate* delegate_)
{
	util::ScopedLock lock(&s_sync);

	TMessengerDelegates::iterator ii = m_messengerDelegates.find(streamId);
	if (ii == m_messengerDelegates.end())
	{
		ii = m_messengerDelegates.insert(std::make_pair(streamId, delegate_)).first;
	}
	else
	{
		assert(!"Delegate for this stream is already defined");
		ii->second = delegate_;
	}
}

void
Messenger::sendMessage(
	::net::IStream::TId streamId,
	TMessagePtr message)
{
	util::ScopedArray<unsigned char> buf;
	size_t bufSize = 0;

	const size_t headerSize = sizeof(MessageHeader);

	{
		util::ScopedLock lock(&m_memStreamSync);

		std::basic_string<unsigned char> membuf;
		util::MemoryStream memstream;
		memstream.exceptions(std::ios::badbit);
		message->save(memstream);

		membuf = memstream.str();
		bufSize = membuf.size();

		buf.reset(new unsigned char[bufSize + headerSize]);
		memcpy(buf.get() + headerSize, membuf.data(), bufSize);
	}

	MessageHeader* header = reinterpret_cast<MessageHeader*>(buf.get());
	memset(header, 0, headerSize);

	header->messageType = message->typeId();
	header->payloadSize = bufSize;

	net::StreamListener& streamListener = net::StreamListener::instance();

	// IMPORTANT !!!
	// Since Messenger::sendMessage() function can be called from different threads and
	//	Messenger::sendMessage() does not sync while calling writeStream(),
	//	it is important that all data are sent at once, otherwise data from different threads could interfere.
	streamListener.writeStream(streamId, buf.get(), bufSize + headerSize);
}

} // namespace msg
