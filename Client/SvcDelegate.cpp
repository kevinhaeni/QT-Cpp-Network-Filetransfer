#include "SvcDelegate.h"


//#define K_RECONNECT_INTERVAL_MS 1000


SvcDelegate::SvcDelegate(const std::string& address)
	: m_disconnected(false)
	, m_address(address)
{
	if (m_address.empty())
		m_address = "127.0.0.1:7777";
}

void SvcDelegate::onEndpointConnected(const std::string& endpointId)
{
	logger::out("Connected!");
}

/// Fires when an endpoint is disconnected
void SvcDelegate::onEndpointDisconnected(const std::string& endpointId)
{
	logger::out("Disconnected!");

	util::ScopedLock lock(&m_sync);
	m_disconnected = true;
}

void SvcDelegate::onResponseDir(
	const std::string& endpointId,
	const std::string& content)
{
	assert(!"We don't expect it to be called on the client side");
}

void SvcDelegate::neverStop()
{
	resetService();
	while (true)
	{
		::Sleep(K_RECONNECT_INTERVAL_MS);

		bool disconnected = false;
		{
			util::ScopedLock lock(&m_sync);
			disconnected = m_disconnected;
		}

		if (disconnected)
		{
			resetService();
		}
	}
}

void SvcDelegate::resetService()
{
	try
	{
		{
			util::ScopedLock lock(&m_sync);
			m_disconnected = true;
		}

		logger::out("Connecting...");

		// Important !!! First delete previous
		m_svc.reset(0);

		// Only then create another instance
		//m_svc.reset(new Service(m_address));

		logger::out("Connect initiated");

		util::ScopedLock lock(&m_sync);
		m_disconnected = false;
	}
	catch (const std::exception& x)
	{
		ignore_unused(x);
		logger::out("Connect error");
		logger::out(x.what());
	}
	catch (...)
	{
		logger::out("Unknown connect error");
	}
}
