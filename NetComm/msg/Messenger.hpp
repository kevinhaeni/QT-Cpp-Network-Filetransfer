#pragma once

#include <net/IBinding.hpp>
#include <net/StreamListener.hpp>
#include "IMessage.hpp"
#include "IMessageFactory.hpp"

namespace msg {

/**
 * Base interface for messenger event delegates.
 */
struct IMessengerDelegate
{
	virtual ~IMessengerDelegate() {}

	/// Is called when a new message is received
	virtual void onMessageReceived(
		::net::IStream::TId streamId,
		::msg::TMessagePtr message) = 0;

	/// Is propagated from StreamListener when net::IStreamListenerDelegate::onStreamDied() is called
	virtual void onStreamDied(::net::IStream::TId streamId) = 0;
};

/**
 * Delegate for stream creation events
 */
struct IBindingDelegate
{
	virtual ~IBindingDelegate() {}

	/**
	 * Stream connection event.
	 */
	virtual void onStreamCreated(::net::IStream::TId streamId) = 0;
};

/**
 * Messenger built on top of net library.
 * Allows sending/receiving messages of fixed (yet arbitrary) size.
 */
class Messenger : public net::IBindingDelegate, public net::IStreamListenerDelegate
{
	/// Explicit instantiation is forbidden
	Messenger();

public:

	/// Use this method to access global singleton instance
	static Messenger& instance();

	/// Sets message factory which will be responsible for creating messages
	void setMessageFactory(IMessageFactory* messageFactory);

	/// Sets new stream created event handler
	void setBindingDelegate(::msg::IBindingDelegate* bindingDelegate);

	/**
	 * Adds delegate_ as an observer of stream's incoming data events
	 */
	void addDelegate(
		::net::IStream::TId streamId,
		IMessengerDelegate* delegate_);

	/// Sends a message over the specified stream
	void sendMessage(
		::net::IStream::TId streamId,
		TMessagePtr message);

	//
	// net::IBindingDelegate
	//

	virtual void onStreamCreated(net::TStreamPtr stream);

	//
	// net::IStreamListenerDelegate
	//

	virtual void onDataReceived(
		::net::IStream::TId streamId,
		const unsigned char* buf,
		size_t bufSize);

	virtual void onStreamDied(::net::IStream::TId streamId);

private:
	static util::ThreadMutex s_sync;
	static std::auto_ptr<Messenger> s_instance;

	/**
	 * Special mutex for MemoryStream access.
	 * underlyting string_stream implementation is not thread safe even when different
	 *	threads access different unrelated instances.
	 */
	util::ThreadMutex m_memStreamSync;

	IMessageFactory* m_messageFactory;
	::msg::IBindingDelegate* m_bindingDelegate;

	typedef std::map<
		::net::IStream::TId,	// Stream ID
		IMessengerDelegate*		// Delegate
	> TMessengerDelegates;

	/// A collection of delegates for each stream
	TMessengerDelegates m_messengerDelegates;

	typedef std::vector<unsigned char> TData;

	typedef std::map<
		::net::IStream::TId,	// Stream ID
		TData
	> TStreamData;

	/// This is where data from each stream are collected until a full message is received
	TStreamData m_streamData;
};

} // namespace msg
