#include "stdafx.h"
#include "StreamListener.hpp"
#include <util/Error.hpp>

#define K_DATA_CHUNK_SIZE (1024 * 1024)

namespace net {

///////////////////////////////////////////////////////////////////////////////////////////////////
// StreamListener

util::ThreadMutex StreamListener::s_sync;
std::auto_ptr<StreamListener> StreamListener::s_instance;

StreamListener::StreamListener()
: m_stopped(true),
  m_state(LISTENER_STOPPED),
  m_hEventStopping(INVALID_HANDLE_VALUE),
  m_hEventStopped(INVALID_HANDLE_VALUE),
  m_maxWorkerThreadCount(1), // At least one worker thread
  m_streamIndex(0)
{
	m_hEventStopping = ::CreateEvent(0, FALSE, FALSE, 0);
	if (NULL == m_hEventStopping)
		throw util::Error("Failed to create an event");

	m_hEventStopped = ::CreateEvent(0, FALSE, FALSE, 0);
	if (NULL == m_hEventStopping)
		throw util::Error("Failed to create an event");

	// Set maximum number of worker threads to number of processors
	char* szProcCount = getenv("NUMBER_OF_PROCESSORS");
	if (szProcCount)
	{
		m_maxWorkerThreadCount = atoi(szProcCount);
		if (m_maxWorkerThreadCount < 1)
			m_maxWorkerThreadCount = 1;
		else if (m_maxWorkerThreadCount > 64)
			m_maxWorkerThreadCount = 64;
	}
}

StreamListener::~StreamListener()
{
	::CloseHandle(m_hEventStopping);
	::CloseHandle(m_hEventStopped);
}

StreamListener&
StreamListener::instance()
{
	util::ScopedLock lock(&s_sync);
	if (0 == s_instance.get())
	{
		s_instance.reset(new StreamListener);
		chkptr(s_instance.get());
	}

	return *s_instance;
}

void
StreamListener::addDelegate(TStreamPtr stream, IStreamListenerDelegate* delegate_)
{
	util::ScopedLock lock(&s_sync);

	if (LISTENER_STOPPING == m_state)
		throw util::Error("Cannot register a delegate while stream listener is being stopped");

	::net::IStream::TId streamId = stream->id();

	// Add to a collection of streams
	assert(m_streams.find(streamId) == m_streams.end());
	m_streams[streamId] = stream;

	assert(m_streamsBusy.find(streamId) == m_streamsBusy.end());
	m_streamsBusy[streamId] = false;

	// Check if this stream already has delegates and if not create en empty list of delegates for it
	TStreamDelegates::iterator ii = m_streamDelegates.find(streamId);
	if (ii == m_streamDelegates.end())
	{
		ii = m_streamDelegates.insert(std::make_pair(streamId, TDelegates())).first;
	}

	// Add delegate_ to a list of delegates
	TDelegates& delegates = ii->second;
	assert(std::find(delegates.begin(), delegates.end(), delegate_) == delegates.end() &&
			"This delegate is already present in the list");
	delegates.push_back(delegate_);

	size_t workerCount = m_workerThreads.size();
	if (workerCount < m_maxWorkerThreadCount &&
		m_streamDelegates.size() > workerCount)
	{
		spawnWorkerThread();
	}
}

void
StreamListener::spawnWorkerThread()
{
	DWORD dwThread = 0;
	HANDLE h = ::CreateThread(0, 0, workerThreadFunc, this, 0, &dwThread);
	if (NULL == h)
		throw util::Error("Failed to create a stream listener thread");

	// Save handle in a list of thread handles
	m_workerThreads.push_back(h);
}

DWORD WINAPI
StreamListener::workerThreadFunc(LPVOID param)
{
	StreamListener* self = static_cast<StreamListener*>(param);
	chkptr(self);

	try
	{
		self->listenStreams();
	}
	catch (const std::exception& x)
	{
		ignore_unused(x);

		// Suppress exception
#ifndef NDEBUG
		const char* szMsg = x.what();
		ignore_unused(szMsg);
#endif

		assert(0);
		return 1;
	}
	catch (...)
	{
		// Suppress unknown exception
		assert(0);
		return 2;
	}

	return 0;
}

void
StreamListener::cancelRun()
{
	// Request stop
	util::ScopedLock lock(&s_sync);

	if (LISTENER_RUNNING == m_state)
	{
		m_state = LISTENER_STOPPING;
		::SetEvent(m_hEventStopping);
	}
}

void
StreamListener::joinRun()
{
	bool alreadyStopped = false;

	// Check state before, stop must be requested
	{
		util::ScopedLock lock(&s_sync);
		assert(LISTENER_STOPPING == m_state ||
			   LISTENER_STOPPED == m_state);

		alreadyStopped = LISTENER_STOPPED == m_state;
	}

	// Wait for completion
	if (!alreadyStopped)
		::WaitForSingleObject(m_hEventStopped, INFINITE);
	else
		::ResetEvent(m_hEventStopped);

	// Check state after, stop must complete
	{
		util::ScopedLock lock(&s_sync);
		assert(LISTENER_STOPPED == m_state);
	}
}

void
StreamListener::run()
{
	// Reset stopped flag
	{
		util::ScopedLock lock(&s_sync);
		m_state = LISTENER_RUNNING;
	}

	// Suspend execute until stop() is called
	::WaitForSingleObject(m_hEventStopping, INFINITE);

	// Wait for all threads to exit

	THandles handles;
	{
		util::ScopedLock lock(&s_sync);
		handles = m_workerThreads;
	}

	if (0 < handles.size())
		::WaitForMultipleObjects(handles.size(), &handles[0], TRUE, INFINITE);

	TStreamDelegates streamDelegates;
	{
		util::ScopedLock lock(&s_sync);

		m_workerThreads.clear();
		m_streams.clear();
		m_streamsBusy.clear();
		m_streamIndex = 0;

		std::copy(m_streamDelegates.begin(), m_streamDelegates.end(), std::inserter(streamDelegates, streamDelegates.begin()));
		m_streamDelegates.clear();
	}

	// Notify all delegates (not under lock to avoid deadlocks)
	for (TStreamDelegates::const_iterator dd = streamDelegates.begin();
		 dd != streamDelegates.end();
		 ++dd)
	{
		::net::IStream::TId streamId = dd->first;
		const TDelegates& delegates = dd->second;

		for (TDelegates::const_iterator ii = delegates.begin();
			 ii != delegates.end();
			 ++ii)
		{
			IStreamListenerDelegate* delegate_ = *ii;
			chkptr(delegate_);

			delegate_->onStreamDied(streamId);
		}
	}

	// At the set state to stopped
	{
		util::ScopedLock lock(&s_sync);

		assert(LISTENER_STOPPING == m_state);
		m_state = LISTENER_STOPPED; // Allow the same instance to re-run()

		// Notify joinRun() that run() has stopped.
		::SetEvent(m_hEventStopped);
	}
}

TStreamPtr
StreamListener::getNextStream(TDelegates& delegates, bool& indexReset)
{
	TStreamPtr stream;

	size_t count = m_streamDelegates.size();

	// Find first non-busy stream (assume same order as in m_streamDelegates)
	size_t streamIndex = 0;
	if (0 < count)
	{
		indexReset = false;
		for (streamIndex = m_streamIndex % count;
			 streamIndex != m_streamIndex || !indexReset;
			 ++streamIndex)
		{
			if (count <= streamIndex)
			{
				indexReset = true;
				streamIndex = streamIndex % count;
			}

			// Find stream ID by indexing m_streamDelegates (since it will be indexed later)
			TStreamDelegates::const_iterator ii = m_streamDelegates.begin();
			std::advance(ii, streamIndex);
			::net::IStream::TId streamId = ii->first;

			// With this stream ID check its busy flag
			TStreamsBusy::const_iterator jj = m_streamsBusy.find(streamId);
			if (jj != m_streamsBusy.end())
			{
				bool busy = jj->second;
				if (!busy)
					break;
			}
			else
			{
				assert(!"Stream ID not found");
			}
		}
	}
	else
	{
		indexReset = true;
	}

	// If not all streams are busy
	if (m_streamIndex != streamIndex || !indexReset)
	{
		m_streamIndex = streamIndex;

		if (m_streamIndex >= count)
		{
			m_streamIndex = 0;
			indexReset = true;
		}

		if (m_streamIndex < count)
		{
			TStreamDelegates::const_iterator ii = m_streamDelegates.begin();
			std::advance(ii, m_streamIndex);
			::net::IStream::TId streamId = ii->first;
			delegates = TDelegates(ii->second.begin(), ii->second.end());

			stream = getStreamById(streamId);
			m_streamsBusy[streamId] = true;

			assert(!!stream);
			++m_streamIndex;
		}
	}

	return stream;
}

TStreamPtr
StreamListener::getStreamById(::net::IStream::TId streamId) const
{
	TStreams::const_iterator tt = m_streams.find(streamId);
	if (tt == m_streams.end())
	{
		assert(0);
		throw std::logic_error("Stream not found");
	}

	TStreamPtr stream = tt->second;
	return stream;
}

void
StreamListener::listenStreams()
{
	std::vector<unsigned char> dataBuf(K_DATA_CHUNK_SIZE, 0);

	bool dataReceived = false;
	while (true)
	{
		dataReceived = false;

		TStreamPtr stream;
		TDelegates delegates;

		{
			util::ScopedLock lock(&s_sync);
			if (LISTENER_STOPPING == m_state)
				return;

			bool indexReset = false;
			stream = getNextStream(delegates, indexReset);

			// If no data received on any stream, sleep a little bit
			if (indexReset || !dataReceived)
				::Sleep(1);
		}

		if (!stream)
		{
			::Sleep(1); //10
			continue;
		}

		size_t cb = 0;
		do
		{
			cb = 0;

			// Wrap reading operation with try/catch in order to handle stream errors
			try
			{
				cb = stream->read(&dataBuf[0], K_DATA_CHUNK_SIZE);
			}
			catch (const std::exception& x)
			{
				streamDied(stream->id(), x.what());

				cb = 0;
				stream = 0;
			}
			catch (...)
			{
				streamDied(stream->id(), "Unknown error");

				cb = 0;
				stream = 0;
			}

			if (0 < cb)
			{
				for (TDelegates::iterator ii = delegates.begin(); ii != delegates.end(); ++ii)
				{
					IStreamListenerDelegate*& delegate_ = *ii;
					delegate_->onDataReceived(stream->id(), &dataBuf[0], cb);
				}

				dataReceived = true;
			}
		} while (0 < cb);

		// Reset busy flag for this stream
		if (!!stream)
		{
			util::ScopedLock lock(&s_sync);
			m_streamsBusy[stream->id()] = false;
		}
	}
}

void
StreamListener::writeStream(::net::IStream::TId streamId, const unsigned char* buf, size_t count)
{
	TStreamPtr stream;
	{
		util::ScopedLock lock(&s_sync);
		stream = getStreamById(streamId);
	}

	try
	{
		stream->write(buf, count);
	}
	catch (const std::exception& x)
	{
		streamDied(stream->id(), x.what());
	}
	catch (...)
	{
		streamDied(stream->id(), "Unknown error");
	}
}

void
StreamListener::closeStream(::net::IStream::TId streamId, const std::string& errorDescription)
{
	streamDied(streamId, errorDescription);
}

void
StreamListener::streamDied(::net::IStream::TId streamId, const std::string& errorDescription)
{
	util::ScopedLock lock(&s_sync);

	TStreamDelegates::iterator ii = m_streamDelegates.find(streamId);
	if (ii != m_streamDelegates.end())
	{
		// Notify all delgates
		TDelegates& delegates = ii->second;
		for (TDelegates::iterator jj = delegates.begin();
			 jj != delegates.end();
			 ++jj)
		{
			IStreamListenerDelegate* delegate_ = *jj;
			delegate_->onStreamDied(streamId);
		}

		m_streamDelegates.erase(ii);
	}
	else
	{
		assert(!"Stream not found in the list of watched streams");
	}
}

} // namespace net
