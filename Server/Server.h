#pragma once

#include <windows.h>

#include <stdexcept>
#include <string>
#include <iostream>
#include <sstream>
#include <limits>
#include <cassert>
#include <ctime>

#include <util/SharedPtr.hpp>
#include <util/Error.hpp>
#include <util/GetOpt.hpp>
#include <util/MemoryStream.hpp>
#include <util/ScopedArray.hpp>

#include <net/BindingFactory.hpp>
#include <net/StreamListener.hpp>

#include <msg/Messenger.hpp>

#include <QtWidgets/QMainWindow>
#include <QtWidgets/qmenu.h>
#include <QMouseEvent>

#include "ui_server.h"
#include "FileTransferWindow.h"
#include "Service.hpp"

class Server : public QMainWindow, public IServiceDelegate
{
	Q_OBJECT
public:
	Server(QWidget *parent = 0);

protected:
	//
	// IServiceDelegate interface
	//
	virtual void onEndpointConnected(const std::string& endpointId);
	virtual void onEndpointDisconnected(const std::string& endpointId);
	virtual void onResponseSysInfo(const std::string& endpointId, const std::vector<std::string>& sysinfo);

private:
	Ui::ServerClass ui;

	util::ThreadMutex m_sync;
	ServicePtr m_service;

	typedef std::vector<std::string> TStrings;
	typedef std::map<std::string, std::string> TStringMap;
	typedef std::map<std::string,std::vector<std::string>> TStringVectorMap;

	// Is used to pass connected endpoint IDs from a delegate thread to a main thread
	TStrings m_connectedEndpoints;
	// Is used to pass disconnected endpoint IDs from a delegate thread to a main thread
	TStrings m_disconnectedEndpoints;
	// Is used to pass sysinformation from a delegate thread to a main thread
	TStringVectorMap m_sysinfoContents;

public slots:
	// QT components
	void onListenClicked();
	void onRequestSysInfoClicked();
	void onEndpointsSelectionChanged();
	void ShowContextMenu(const QPoint& pos);

	// main thread callbacks for IServiceDelegate delegates
	void updateSysinfo();
	void updateConnectedEndpoints();
	void updateDisconnectedEndpoints();
};
