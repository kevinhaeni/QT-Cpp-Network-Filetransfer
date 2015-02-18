#include "Service.hpp"

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

#include "Logger.hpp"

namespace {
	// Size of transmitting block of file
	const int kFileChunkSize = 1024*100;

	// Reconnect interval, ms
	const int kReconnectInterval = 1000;
}

util::ThreadMutex Service::s_sync;
Service* Service::s_instance = 0;

namespace
{

std::vector<std::string> getSysInfo()
{
	std::vector<std::string> result;
	result.push_back("Hostname:");
	result.push_back("IP-Address:");
	result.push_back("Operating-System:");

	return result;
}

TDirItems getDirectoryContent(const std::string& dir)
{
	TDirItems content;
	
	WIN32_FIND_DATAA fd;
	memset(&fd, 0, sizeof(fd));

	std::string mask = dir;
	size_t len = mask.length();
	if (0 < len &&
		mask[len - 1] != '\\' &&
		mask[len - 1] != '/')
	{
		mask += "\\";
	}
	mask += "*.*";

	HANDLE h = ::FindFirstFileA(mask.c_str(), &fd);
	if (INVALID_HANDLE_VALUE != h)
	{
		do {
			if (strcmp(fd.cFileName, ".") == 0)
				continue;

			DirItem item;
			item.m_isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			item.m_name = fd.cFileName;
			content.push_back(item);
		} while (::FindNextFileA(h, &fd));

		::FindClose(h);
	}
	return content;
}

} // namespace

Service::Service(const std::string& address)
	: m_address(address)
	, m_disconnected(false)
{
	if (m_address.empty())
		m_address = "127.0.0.1:7777";

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

	reset();

	// Reset instance pointer
	{
		util::ScopedLock lock(&s_sync);
		assert(0 != s_instance);
		s_instance = 0;
	}

	OutputDebugStringA("\n<< Service::dtor\n");
}

void Service::neverStop()
{
	connect();
	while (true)
	{
		::Sleep(kReconnectInterval);
		if (m_disconnected)
		{
			reset();
			connect();
		}
	}
}

void Service::connect()
{
	try
	{
		m_disconnected = false;
		m_msgFactory.reset(new SvcMsgFactory);

		msg::Messenger& messenger = msg::Messenger::instance();
		messenger.setMessageFactory(m_msgFactory.get());
		messenger.setBindingDelegate(this); // Messanger will notify of new connections

		m_binding = net::BindingFactory::createBinding(net::BindingFactory::BINDING_TCP_CLIENT);
		m_binding->bind(m_address, &messenger); // Notifications of new streams are managed by Messanger
		return;
	}
	catch (const std::exception& x)
	{
		logger::out("Connect error");
		logger::out(x.what());
	}
	catch (...)
	{
		logger::out("Unknown connect error");
	}
	m_disconnected = true;
}

void Service::reset()
{
	net::StreamListener::instance().cancelRun();

	// Don't need this binding any more
	{
		util::ScopedLock lock(&m_sync);
		m_binding = 0;
	}

	// Wait for the listener to stop running
	// otherwise Service will get callbacks after it is destroyed.
	net::StreamListener::instance().joinRun();

	msg::Messenger& messenger = msg::Messenger::instance();
	messenger.setMessageFactory(0);
	messenger.setBindingDelegate(0);
}

void
Service::onStreamCreated(net::IStream::TId streamId)
{
	msg::Messenger& messenger = msg::Messenger::instance();

	messenger.addDelegate(streamId, this);
	messenger.sendMessage(streamId, new MessageIdentity);
}

void
Service::onStreamDied(::net::IStream::TId streamId)
{
	std::string endpointId;
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

	if (!endpointId.empty())
	{
		logger::out("Disconnected!");
		m_disconnected = true;
	}
	m_fileReader.close();
}

void
Service::onMessageReceived(
	net::IStream::TId streamId,
	msg::TMessagePtr message)
{
	msg::IMessage* m = message.get();

	if (MessageIdentity* msgIdentity = dynamic_cast<MessageIdentity*>(m))
	{
		std::string endpointId = msgIdentity->m_identity;

		assert(m_endpoints.find(streamId) == m_endpoints.end());
		m_endpoints[streamId] = endpointId;
	}
	else if (MessageRequestDir* msgRequestDir = dynamic_cast<MessageRequestDir*>(m))
	{
		MessageResponseDir* msgResponse = new MessageResponseDir();
		msgResponse->m_content = getDirectoryContent(msgRequestDir->m_dir);

		msg::Messenger::instance().sendMessage(streamId, msgResponse);
	}
	else if (MessageRequestFile* msgRequestFile = dynamic_cast<MessageRequestFile*>(m))
	{
		requestFile(streamId, *msgRequestFile);
	}
	else if (MessageRequestSysInfo* msgRequestSysInfo = dynamic_cast<MessageRequestSysInfo*>(m))
	{
		MessageResponseSysInfo* msgResponse = new MessageResponseSysInfo(::getSysInfo());

		msg::Messenger::instance().sendMessage(streamId, msgResponse);
	}
	else if (MessageUploadFile* msgUploadFile = dynamic_cast<MessageUploadFile*>(m))
	{
		uploadFile(streamId, *msgUploadFile);
	}
	else
	{
		assert(!"Unknown message");
	}
}

void Service::requestFile(net::IStream::TId streamId, const MessageRequestFile& msg)
{
	MessageResponseFile* response = new MessageResponseFile();
	FileChunk& chunk = response->m_response;
	chunk.m_fileName = msg.m_request.m_fileName;

	// Try to open file
	if (m_fileReader.open(msg.m_request.m_fileName))
	{
		chunk.m_fileSize = m_fileReader.size();
		__int64 chunkSize = chunk.m_fileSize - msg.m_request.m_startFrom;
		if (chunkSize > kFileChunkSize)
			chunkSize = kFileChunkSize;

		chunk.m_valid = m_fileReader.read(chunk.m_fileData, msg.m_request.m_startFrom, static_cast<int>(chunkSize));
	}
	else
	{
		// File not found
		chunk.m_valid = false;
	}

	msg::Messenger::instance().sendMessage(streamId, response);
}

void Service::uploadFile(net::IStream::TId streamId, const MessageUploadFile& msg)
{
	const FileChunk& chunk = msg.m_chunk;
	if (!chunk.m_valid)
	{
		// This is the way to stop upload
		m_fileWriter.close();
		// TODO: delete partially uploaded file
		return;
	}

	bool ok = false;
	// Try to open file
	if (m_fileWriter.open(chunk.m_fileName))
	{
		// Write data to file
		ok = m_fileWriter.write(chunk.m_fileData);

		if (chunk.m_fileSize <= m_fileWriter.size())
			m_fileWriter.close();
	}
	MessageUploadFileReply* response = new MessageUploadFileReply();
	response->m_ok = ok;

	msg::Messenger::instance().sendMessage(streamId, response);
}

