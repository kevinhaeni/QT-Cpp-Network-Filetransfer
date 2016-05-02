#pragma once

#include <QSharedPointer>
#include <msg/Messenger.hpp>
#include <net/BindingFactory.hpp>
#include <protocol/DataTypes.hpp>

class SvcMsgFactory;

/// Base interface for service events handler
struct IServiceDelegate
{
	virtual ~IServiceDelegate() {}

	/// Fires when a new endpoint is connected and sent its identity
	virtual void onEndpointConnected(const std::string& endpointId) {}

	/// Fires when an endpoint is disconnected
	virtual void onEndpointDisconnected(const std::string& endpointId) {}

	/// Fires when directory content reponse is received
	virtual void onResponseDir(const std::string& endpointId, const TDirItems& content) {}

	/// Fires when directory content reponse is received
	virtual void onResponseFile(const std::string& endpointId, const FileChunk& chunk) {}

	/// Fires when sysinfo reponse is received
	virtual void onResponseSysInfo(const std::string& endpointId, const std::vector<std::string>& sysinfo) {}

	/// Fires when upload file result is received
	virtual void onUploadFileReply(const std::string& endpointId, bool ok) {}
};

/// Service sending/receiving messages
class Service : public msg::IBindingDelegate, public msg::IMessengerDelegate
{
public:
	Service(
		net::BindingFactory::BindingType bindingType,
		const std::string& address,
		IServiceDelegate* delegate_);
	~Service();

	void addDelegate(IServiceDelegate* delegate_);
	void deleteDelegate(IServiceDelegate* delegate_);

	/// Sends dir content request to the specified endpoint
	void requestDir(const std::string& endpointId, const std::wstring& dir);

	/// Sends file request to the specified endpoint
	void requestFile(const std::string& endpointId, const FileRequest& request);

	/// Send chunk of file to upload
	void uploadFile(const std::string& endpointId, const FileChunk& chunk);

	/// Requests the execution of a remote file
	void executeFile(const std::string& endpointId, const std::wstring& remoteFile);

	/// Sends sys info request to the specified endpoint
	void requestSysInfo(const std::string& endpointId);


	//
	// msg::IBindingDelegate
	//
	virtual void onStreamCreated(net::IStream::TId streamId);
	virtual void onStreamDied(net::IStream::TId streamId);

	//
	// msg::IMessengerDelegate
	//
	virtual void onMessageReceived(
		::net::IStream::TId streamId,
		::msg::TMessagePtr message);

private:
	::net::IStream::TId findStream(const std::string& endpointId);

private:
	static util::ThreadMutex s_sync;
	static Service* s_instance;

	mutable util::ThreadMutex m_sync;
	std::auto_ptr<SvcMsgFactory> m_msgFactory;
	std::vector<IServiceDelegate*> m_delegate;
	HANDLE m_hWorker;
	net::TBindingPtr m_binding;

	typedef std::map<
		::net::IStream::TId,	// stream ID
		std::string				// endpoint ID
	> TEndpoints;
	TEndpoints m_endpoints;
};

typedef QSharedPointer<Service> ServicePtr;
