#pragma once

#include <util/ThreadMutex.hpp>
#include "IBinding.hpp"

namespace net {

class BindingFactory;

/**
 * TCP server binding implementation
 */
class TcpServer : public IBinding
{
	friend class BindingFactory;

	/// Must be created only by means of BindingFactory
	TcpServer();

public:
	~TcpServer();

	/**
	 * Starts listening for incoming connections on a specified address.
	 * Address string must be in the following format: IP:PORT or FQDN:PORT
	 */
	virtual void bind(
		const std::string& address,
		IBindingDelegate* delegate_);

private:
	mutable util::ThreadMutex m_sync;
	bool m_cancelled;
	bool m_acceptorStopped;
	int m_listenSocket;
	IBindingDelegate* m_delegate;

	bool cancelled() const;

	static DWORD WINAPI acceptorThreadFunc(LPVOID param);

	void runAcceptor();
};

} // namespace net
