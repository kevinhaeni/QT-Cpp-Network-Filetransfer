#pragma once

#include <msg/Messenger.hpp>
#include <net/BindingFactory.hpp>
#include <util/FileReader.hpp>
#include <util/FileWriter.hpp>

class SvcMsgFactory;
class MessageRequestFile;
class MessageUploadFile;

/// Service sending/receiving messages
class Service : public msg::IBindingDelegate, public msg::IMessengerDelegate
{
public:
	Service(const std::string& address);
	~Service();

	// Run client service in loop.
	void neverStop();

	// Connect to remote server
	void connect();

	// Release all allocated resources
	void reset();


	// msg::IBindingDelegate
	virtual void onStreamCreated(net::IStream::TId streamId);
	virtual void onStreamDied(net::IStream::TId streamId);

	// msg::IMessengerDelegate
	virtual void onMessageReceived(
		net::IStream::TId streamId,
		msg::TMessagePtr message);

private:
	void requestFile(net::IStream::TId streamId, const MessageRequestFile& msg);
	void uploadFile(net::IStream::TId streamId, const MessageUploadFile& msg);

private:
	typedef std::map<
		net::IStream::TId,	// stream ID
		std::string			// endpoint ID
	> TEndpoints;

private:
	static util::ThreadMutex s_sync;
	static Service* s_instance;

	mutable util::ThreadMutex m_sync;
	std::auto_ptr<SvcMsgFactory> m_msgFactory;
	net::TBindingPtr m_binding;
	std::string m_address;
	bool m_disconnected;
	util::FileReader m_fileReader;
	util::FileWriter m_fileWriter;

	TEndpoints m_endpoints;
};
