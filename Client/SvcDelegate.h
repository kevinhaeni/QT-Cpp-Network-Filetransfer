#pragma once

#include "Service.hpp"

/// Dummy service event sink
struct SvcDelegate : IServiceDelegate
{
	SvcDelegate(const std::string& address);

	virtual void onEndpointConnected(const std::string& endpointId);

	/// Fires when an endpoint is disconnected
	virtual void onEndpointDisconnected(const std::string& endpointId);

	virtual void onResponseDir(const std::string& endpointId, const std::string& content);

	void neverStop();

	void resetService();

private:
	util::ThreadMutex m_sync;
	std::auto_ptr<Service> m_svc;
	bool m_disconnected;
	std::string m_address;
};
