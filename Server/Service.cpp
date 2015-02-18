#include "Service.hpp"

#include <net/BindingFactory.hpp>
#include <protocol/MessageIdentity.hpp>
#include <protocol/MessageRequestDir.hpp>
#include <protocol/MessageResponseDir.hpp>
#include <protocol/MessageRequestFile.hpp>
#include <protocol/MessageResponseFile.hpp>
#include <protocol/MessageRequestSysInfo.hpp>
#include <protocol/MessageResponseSysInfo.hpp>
#include <protocol/MessageUploadFile.hpp>
#include <protocol/MessageUploadFileReply.hpp>
#include <protocol/SvcMsgFactory.hpp>
#include <util/Error.hpp>

util::ThreadMutex Service::s_sync;
Service* Service::s_instance = 0;

static DWORD WINAPI listenerWorkerProc(LPVOID param);

Service::Service(net::BindingFactory::BindingType bindingType, const std::string& address, IServiceDelegate* delegate_) : m_hWorker(NULL)
{
	m_msgFactory.reset(new SvcMsgFactory);

	msg::Messenger& messenger = msg::Messenger::instance();
	messenger.setMessageFactory(m_msgFactory.get());
	messenger.setBindingDelegate(this); // Messanger will notify of new connections

	m_binding = net::BindingFactory::createBinding(bindingType);
	m_binding->bind(address, &(msg::Messenger::instance())); // Notifications of new streams are managed by Messanger

	this->m_delegate.push_back(delegate_);

	DWORD dwThreadId = 0;
	m_hWorker = ::CreateThread(0, 0, listenerWorkerProc, this, 0, &dwThreadId);
	if (NULL == m_hWorker)
		throw util::Error("Failed to create worker");

	// Check that there's always only 1 instance (not implemented as singleton, though should be)
	{
		util::ScopedLock lock(&s_sync);
		assert(0 == s_instance);
		s_instance = this;
	}
}

Service::~Service()
{
	OutputDebugStringA("\n>> Service::dtor\n");

	net::StreamListener::instance().cancelRun();

	// Wait for the worker to complete
	DWORD exitCode = 0;
	if (!::GetExitCodeThread(m_hWorker, &exitCode) &&
		STILL_ACTIVE == exitCode)
	{
		::WaitForSingleObject(m_hWorker, INFINITE);
	}

	// Don't need this binding any more
	{
		util::ScopedLock lock(&m_sync);
		m_binding = 0;
	}

	// Wait for the listener to stop running
	//	otherwise Service will get callbacks after it is destroyed.
	net::StreamListener::instance().joinRun();

	msg::Messenger& messenger = msg::Messenger::instance();
	messenger.setMessageFactory(0);
	messenger.setBindingDelegate(0);

	// Reset instance pointer
	{
		util::ScopedLock lock(&s_sync);
		assert(0 != s_instance);
		s_instance = 0;
	}

	OutputDebugStringA("\n<< Service::dtor\n");
}

void Service::addDelegate(IServiceDelegate* delegate_)
{
	if (std::find(m_delegate.begin(), m_delegate.end(), delegate_) == m_delegate.end())
		m_delegate.push_back(delegate_);
}

void Service::deleteDelegate(IServiceDelegate* delegate_)
{
	std::vector<IServiceDelegate*>::iterator position = std::find(m_delegate.begin(), m_delegate.end(), delegate_);
	if (position != m_delegate.end())
		m_delegate.erase(position);
}


void Service::onStreamCreated(net::IStream::TId streamId)
{
	msg::Messenger& messenger = msg::Messenger::instance();

	messenger.addDelegate(streamId, this);
	messenger.sendMessage(streamId, new MessageIdentity);
}

void Service::onStreamDied(net::IStream::TId streamId)
{
	std::string endpointId;
	IServiceDelegate* delegate_ = 0;
	{
		util::ScopedLock lock(&m_sync);

		TEndpoints::iterator ii = m_endpoints.find(streamId);
		if (ii == m_endpoints.end())
		{
			assert(!"Stream either not found or died while processing MessageIdentity");
		}
		else
		{
			endpointId = ii->second;
			assert(!endpointId.empty());
		}
	}

	for(IServiceDelegate* i: m_delegate){
		if (i)
		{
			chkptr(i);

			util::ScopedLock lock(&m_sync);

			i->onEndpointDisconnected(endpointId);
		}
	}
}

void Service::onMessageReceived(::net::IStream::TId streamId, ::msg::TMessagePtr message)
{
	msg::IMessage* m = message.get();

	if (MessageIdentity* msgIdentity = dynamic_cast<MessageIdentity*>(m))
	{
		// First message after connect, remember client's endpoint
		for(IServiceDelegate* delegate: m_delegate) {
			if (delegate)
			{
				{
					util::ScopedLock lock(&m_sync);
					m_endpoints[streamId] = msgIdentity->m_identity;
				}
				delegate->onEndpointConnected(msgIdentity->m_identity);
			}
		}
		return;
	}

	// Find client's endpoint id
	TEndpoints::const_iterator it = m_endpoints.find(streamId);
	if (it == m_endpoints.end())
	{
		assert(!"Stream not found");
		return;
	}
	const std::string& endpointId = it->second;

	if (MessageResponseDir* msgResponseDir = dynamic_cast<MessageResponseDir*>(m))
	{
		for(IServiceDelegate* delegate: m_delegate) {
			if (delegate)
			{
				util::ScopedLock lock(&m_sync);
				delegate->onResponseDir(endpointId, msgResponseDir->m_content);
			}
		}
	}
	else if (MessageResponseFile* msgResponseFile = dynamic_cast<MessageResponseFile*>(m))
	{
		for(IServiceDelegate* delegate: m_delegate) {
			if (delegate)
			{
				util::ScopedLock lock(&m_sync);
				delegate->onResponseFile(endpointId, msgResponseFile->m_response);
			}
		}
	}
	else if (MessageResponseSysInfo* msgResponseSysInfo = dynamic_cast<MessageResponseSysInfo*>(m))
	{
		for(IServiceDelegate* delegate: m_delegate) {
			if (delegate)
			{
				util::ScopedLock lock(&m_sync);
				delegate->onResponseSysInfo(endpointId, msgResponseSysInfo->m_sysinfo);
			}
		}
	}
	else if (MessageUploadFileReply* msgUploadFileReply = dynamic_cast<MessageUploadFileReply*>(m))
	{
		for(IServiceDelegate* delegate: m_delegate) {
			if (delegate)
			{
				util::ScopedLock lock(&m_sync);
				delegate->onUploadFileReply(endpointId, msgUploadFileReply->m_ok);
			}
		}
	}
	else
	{
		assert(!"Unknown message");
	}
}

void Service::requestDir(const std::string& endpointId, const std::string& dir)
{
	util::ScopedLock lock(&m_sync);

	MessageRequestDir* msgRequest = new MessageRequestDir;
	msgRequest->m_dir = dir;
	msg::Messenger::instance().sendMessage(findStream(endpointId), msgRequest);
}


void Service::requestFile(const std::string& endpointId, const FileRequest &request)
{
	util::ScopedLock lock(&m_sync);

	MessageRequestFile* msgRequest = new MessageRequestFile;
	msgRequest->m_request = request;
	msg::Messenger::instance().sendMessage(findStream(endpointId), msgRequest);
}

void Service::uploadFile(const std::string& endpointId, const FileChunk& chunk)
{
	util::ScopedLock lock(&m_sync);

	MessageUploadFile* msgUpload = new MessageUploadFile;
	msgUpload->m_chunk = chunk;
	msg::Messenger::instance().sendMessage(findStream(endpointId), msgUpload);
}

void Service::requestSysInfo(const std::string& endpointId)
{
	util::ScopedLock lock(&m_sync);

	MessageRequestSysInfo* msgRequest = new MessageRequestSysInfo;
	msgRequest->m_sysinfo = "Default Request";
	msg::Messenger::instance().sendMessage(findStream(endpointId), msgRequest);
}


DWORD WINAPI listenerWorkerProc(LPVOID param)
{
	try
	{
		net::StreamListener::instance().run();
	}
	catch (const std::exception& x)
	{
		// Suppress exception
#ifndef NDEBUG
		const char* szMsg = x.what();
		OutputDebugStringA("\n*** Error in service worker thread: ");
		OutputDebugStringA(szMsg);
		OutputDebugStringA("\n");
		ignore_unused(szMsg);
#endif

		assert(0);
		return 1;
	}
	catch (...)
	{
		OutputDebugStringA("\n*** Unknown error in service worker thread\n");

		// Suppress unknown exception
		assert(0);
		return 2;
	}

	return 0;
}

::net::IStream::TId Service::findStream(const std::string& endpointId)
{
	// Lookup corresponding stream ID
	::net::IStream::TId streamId = 0;
	for (TEndpoints::const_iterator ii = m_endpoints.begin(); ii != m_endpoints.end(); ++ii)
	{
		if (ii->second == endpointId)
		{
			streamId = ii->first;
			break;
		}
	}

	if (0 == streamId)
		throw util::Error("Invalid endpoint name: " + endpointId);
	return streamId;
}
