#ifndef FILETRANSFERWINDOW_H
#define FILETRANSFERWINDOW_H

#include <stdexcept>
#include <string>
#include <iostream>
#include <sstream>
#include <limits>
#include <cassert>
#include <ctime>

#include <windows.h>

#include <QWidget>

#include <util/SharedPtr.hpp>
#include <util/Error.hpp>
#include <util/FileReader.hpp>
#include <util/FileWriter.hpp>
#include <util/GetOpt.hpp>
#include <util/MemoryStream.hpp>
#include <util/ScopedArray.hpp>

#include <net/BindingFactory.hpp>
#include <net/StreamListener.hpp>

#include <msg/Messenger.hpp>

#include <Protocol/DataTypes.hpp>

#include "ui_filetransferwindow.h"
#include "Service.hpp"

class QFileSystemModel;

class FileTransferWindow : public QWidget, public IServiceDelegate
{
	Q_OBJECT

public:
	FileTransferWindow(const ServicePtr& service, const std::string& endpoint, QWidget *parent = 0);
	~FileTransferWindow();

protected:
	//
	// IServiceDelegate
	//
	virtual void onEndpointDisconnected(const std::string& endpointId);
	virtual void onResponseDir(const std::string& endpointId, const TDirItems& content);
	virtual void onResponseFile(const std::string& endpointId, const FileChunk& chunk);
	virtual void onUploadFileReply(const std::string& endpointId, bool ok);

public slots:
	
private slots:
	void onRequestDirClicked();
	void onDownloadFileClicked();
	void onUploadFileClicked();
	void onLocalFileSystemActivated(const QModelIndex& index);
	void onDirRemoteItemActivated(QListWidgetItem*);

	void updateDir();
	void updateFile(qlonglong position, qlonglong total);
	void endpointDisconnect();
	void stopFileTransmission();

private:
	void startFileDownload(const std::string& remoteFileName, const std::string& localPath);
	void startFileUpload(const std::string& localFileName);

private:
	Ui::FileTransferWindow ui;
	util::ThreadMutex m_sync;
	ServicePtr m_service;
	std::string m_endpoint;

	TDirItems m_dirContent;
	
	std::string m_remoteFileName;
	std::string m_localFileName;
	__int64 m_transferringFileSize;
	__int64 m_transferringFilePosition;
	util::FileReader m_fileReader;
	util::FileWriter m_fileWriter;
	QFileSystemModel* m_fileSystemModel; 
	QString m_currentRemoteDir;
};

#endif // FILETRANSFERWINDOW_H
