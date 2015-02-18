#pragma once

#include <memory>
#include <map>
#include <vector>
#include <Windows.h>
#include <util/ThreadMutex.hpp>
#include <util/SharedPtr.hpp>
#include "IStream.hpp"

namespace net {

/// Base interface for stream event delegates
struct IStreamListenerDelegate
{
	virtual ~IStreamListenerDelegate() {}

	/**
	 * Is called when data are available on a stream.
	 * writeStream must be used to write().
	 */
	virtual void onDataReceived(
		::net::IStream::TId streamId,
		const unsigned char* buf,
		size_t bufSize) = 0;

	/// Is called when a stream has died
	virtual void onStreamDied(::net::IStream::TId streamId) = 0;
};

/**
 * Singleton reactor/dispatcher.
 * Global listener of stream incoming data.
 */
class StreamListener
{
	/// Explicit instantiation is forbidden
	StreamListener();

public:
	~StreamListener();

	/// Use this method to access global singleton instance
	static StreamListener& instance();

	/**
	 * Adds delegate_ as an observer of stream's incoming data events
	 */
	void addDelegate(TStreamPtr stream, IStreamListenerDelegate* delegate_);

	/**
	 * Runs listening loop.
	 * Call to this function blocks until stop (from some other thread or from within a delegate) is called.
	 */
	void run();

	/**
	 * Call this method to request run() to stop listening to events.
	 * This method is non-blocking and can be used from within the IStreamListenerDelegate's methods.
	 * Call joinRun() to wait for run() to complete (but not from IStreamListenerDelegate's methods).
	 */
	void cancelRun();

	/**
	 * Waits for the stop request to complete.
	 * Do not call this method from within of any IStreamListenerDelegate's methods, it will cause a deadlock.
	 */
	void joinRun();

	/**
	 * This method should be used when writing to watched stream instead of IStream::write()
	 *	in order to let the StreamListener instance handle stream errors.
	 * This call is blocking.
	 */
	void writeStream(::net::IStream::TId stream, const unsigned char* buf, size_t count);

	/**
	 * Explicitly closes specified stream in case some higher level error occurs.
	 */
	void closeStream(::net::IStream::TId streamId, const std::string& errorDescription);

private:
	static util::ThreadMutex s_sync;
	static std::auto_ptr<StreamListener> s_instance;

	/// StreamListener internal state
	enum {
		LISTENER_STOPPED = 0,
		LISTENER_RUNNING,
		LISTENER_STOPPING
	} m_state;

	// Flag and event to indicate that stop() is called
	bool m_stopped;
	HANDLE m_hEventStopping;
	HANDLE m_hEventStopped;

	/// Maximum allowed number of threads
	size_t m_maxWorkerThreadCount;

	/// Index of a next to check stream
	size_t m_streamIndex;

	typedef std::map<
		::net::IStream::TId,	// Stream ID
		bool					// Busy flag
	> TStreamsBusy;

	/**
	 * Busy flags for each stream,
	 * true indicates that this stream is being read,
	 *	thus cannot be read by another worker thread.
	 */
	TStreamsBusy m_streamsBusy;

	typedef std::vector<HANDLE> THandles;
	THandles m_workerThreads;

	typedef std::map<
		::net::IStream::TId,
		TStreamPtr
	> TStreams;
	
	TStreams m_streams;

	typedef std::vector<IStreamListenerDelegate*> TDelegates;

	typedef std::map<
		::net::IStream::TId,	// Stream ID
		TDelegates				// List of its delegates
	> TStreamDelegates;

	TStreamDelegates m_streamDelegates;

	/// Creates a worker thread. Must be executed under a sync.
	void spawnWorkerThread();

	/// Worker thread start routine
	static DWORD WINAPI workerThreadFunc(LPVOID param);

	/// Worker routine, runs on multiple threads
	void listenStreams();

	/**
	 * Must NOT be executed under a lock. Acquires a lock itself.
	 * Notifies all delegates of a stream error and removes the stream from a list of listened streams.
	 */
	void streamDied(::net::IStream::TId, const std::string& errorDescription);

	/**
	 * Must be executed under a lock.
	 * Gets next stream (as identified by m_strewamIndex) in a round-robin fashion.
	 * Returns NULL pointer if no streams available.
	 */
	TStreamPtr getNextStream(TDelegates& delegates, bool& indexReset);

	/**
	 * Must be executed under a lock.
	 * Looks up a stream by its ID.
	 */
	TStreamPtr getStreamById(::net::IStream::TId streamId) const;
};

} // namespace net
