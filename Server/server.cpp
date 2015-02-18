#include "Server.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "NetComm.lib")

Server::Server(QWidget *parent)	
	: QMainWindow(parent)
{
	ui.setupUi(this);
	ui.leListen->setText("127.0.0.1:7777");

	connect(ui.btnListen, SIGNAL(clicked()), this, SLOT(onListenClicked()));
	connect(ui.btnRequestSysInfo, SIGNAL(clicked()), this, SLOT(onRequestSysInfoClicked()));
	connect(ui.lstEndpoints, SIGNAL(itemSelectionChanged()), this, SLOT(onEndpointsSelectionChanged()));
	connect(ui.lstEndpoints, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ShowContextMenu(const QPoint&)));
}

/// *********************************************************
///  network messenger events (delegate thread) 
/// *********************************************************

void Server::onEndpointConnected(const std::string& endpointId)
{
	util::ScopedLock lock(&m_sync);

	m_connectedEndpoints.push_back(endpointId);

	// main thread UI Update call
	QMetaObject::invokeMethod(this, "updateConnectedEndpoints", Qt::QueuedConnection);
}

void Server::onEndpointDisconnected(const std::string& endpointId)
{
	util::ScopedLock lock(&m_sync);
	
	m_disconnectedEndpoints.push_back(endpointId);
	
	// main thread UI Update call
	QMetaObject::invokeMethod(this, "updateDisconnectedEndpoints", Qt::QueuedConnection);
}

void Server::onResponseSysInfo(const std::string& endpointId, const std::vector<std::string>& sysinfo)
{
	util::ScopedLock lock(&m_sync);

	// update vector stringmap
	m_sysinfoContents[endpointId] = sysinfo;

	// main thread UI Update call
	QMetaObject::invokeMethod(this, "updateSysinfo", Qt::QueuedConnection);
}


/// *********************************************************
///  QT Component Signals / Slots
/// *********************************************************

void Server::onListenClicked()
{
	if (!m_service)
	{
		QString address = ui.leListen->text();

		try
		{
			if (address.isEmpty())
				throw std::logic_error("Empty address");

			// Important !!! First delete previous, then create another one.
			m_service.reset();
			m_service.reset(new Service(net::BindingFactory::BINDING_TCP_SERVER, address.toStdString(), this));

			ui.btnListen->setText(_T("Stop"));
			ui.lstEndpoints->setEnabled(true);
			ui.leListen->setEnabled(false);
		}
		catch (const std::exception& x)
		{
			::MessageBoxA(NULL, x.what(), "Error", MB_ICONERROR);
		}
	}
	else
	{
		m_service.reset();

		ui.btnListen->setText(_T("Listen"));
		ui.lstEndpoints->setEnabled(false);
		ui.leListen->setEnabled(true);
	}
}

void Server::onRequestSysInfoClicked()
{
	if (m_service) {
		if(ui.lstEndpoints->selectedItems().size() >= 1) {
			QString sId = ui.lstEndpoints->currentItem()->text();
			m_service->requestSysInfo(sId.toStdString());
		}
	}
}

void Server::onEndpointsSelectionChanged()
{
	updateSysinfo();
}

void Server::ShowContextMenu(const QPoint& pos)
{
	QMenu menu;
	menu.addAction("Start Filetransfer");

	QAction* selectedItem = menu.exec(ui.lstEndpoints->mapToGlobal(pos));
	if (selectedItem && m_service) {
		if(ui.lstEndpoints->selectedItems().size() >= 1) {
			QString sId = ui.lstEndpoints->currentItem()->text();
			FileTransferWindow *fwin;
			fwin = new FileTransferWindow(m_service, sId.toStdString());
			m_service->addDelegate(fwin);
			fwin->setWindowTitle(QString::fromStdString("File Transfer [") + sId + QString::fromStdString("]"));
			fwin->show();
		}
	}
}

/// *********************************************************
///  Signals in main thread for UI Update
/// *********************************************************

void Server::updateSysinfo()
{
	ui.txtSysInfo->clear();

	if(ui.lstEndpoints->selectedItems().size() > 0)
	{
		QString selectedEndpoint = ui.lstEndpoints->currentItem()->text();

		TStringVectorMap::const_iterator ii = m_sysinfoContents.find(selectedEndpoint.toStdString());
		if (ii != m_sysinfoContents.end())
		{
			for(std::string s : ii->second)
			{
				ui.txtSysInfo->append(QString::fromStdString(s));
			}
		}
		else
		{
			ui.txtSysInfo->append(_T("<no content retrieved yet>"));
		}
	}
}

void Server::updateConnectedEndpoints()
{
	util::ScopedLock lock(&m_sync);

	for (TStrings::const_iterator ii = m_connectedEndpoints.begin(); ii != m_connectedEndpoints.end(); ++ii)
	{
		std::string endpointId = *ii;
		ui.lstEndpoints->addItem(QString::fromStdString(endpointId));
	}
	m_connectedEndpoints.clear();
}

void Server::updateDisconnectedEndpoints()
{
	for (TStrings::const_iterator ii = m_disconnectedEndpoints.begin(); ii != m_disconnectedEndpoints.end(); ++ii)
	{
		std::string endpointId = *ii;

		// remove sysinfo from stringmap
		TStringVectorMap::const_iterator position = m_sysinfoContents.find(endpointId);
		if (position != this->m_sysinfoContents.end())
		{
			this->m_sysinfoContents.erase(position);
		}

		QListIterator<QListWidgetItem*> i (ui.lstEndpoints->findItems(QString::fromStdString(endpointId), Qt::MatchExactly));
		while(i.hasNext())
		{
			delete i.next();
		}
	}
	m_disconnectedEndpoints.clear();
}
