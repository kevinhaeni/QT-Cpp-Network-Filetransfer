#include "filetransferwindow.h"

#include <QDateTime>
#include <QFileInfo>
#include <QFileSystemModel>
#include "Server.h"

namespace {
	// Size of transmitting block of file
	const int kFileChunkSize = 1024*100;
	const char* kDefaultRootPath = "C:\\";
}


FileTransferWindow::FileTransferWindow(const ServicePtr& service, const std::string& endpoint, QWidget *parent)
	: QWidget(parent)
	, m_service(service)
	, m_endpoint(endpoint)
{
	setAttribute(Qt::WA_DeleteOnClose);
	this->setWindowTitle(QString::fromStdString("File Transfer [" + endpoint + "]"));
	ui.setupUi(this);

	ui.lblEndpoint->setText(QString::fromStdString(this->m_endpoint));
	m_fileSystemModel = new QFileSystemModel(this);
	
	m_fileSystemModel->setRootPath(kDefaultRootPath);
	m_fileSystemModel->setFilter(QDir::AllEntries | QDir::NoDot);
	
	ui.tvDirLocal->setModel(m_fileSystemModel);
	ui.tvDirLocal->setRootIndex(m_fileSystemModel->index(kDefaultRootPath));
	connect(ui.tvDirLocal, SIGNAL(activated(const QModelIndex&)), this, SLOT(onLocalFileSystemActivated(const QModelIndex&)));

	connect(ui.lwDirRemote, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(onDirRemoteItemActivated(QListWidgetItem*)));
	connect(ui.txtDir, SIGNAL(returnPressed()), this, SLOT(onRequestDirClicked()));

	connect(ui.btnRequestDir, SIGNAL(clicked()), this, SLOT(onRequestDirClicked()));
	connect(ui.btnDownloadFile, SIGNAL(clicked()), this, SLOT(onDownloadFileClicked()));
	connect(ui.btnUploadFile, SIGNAL(clicked()), this, SLOT(onUploadFileClicked()));
}

FileTransferWindow::~FileTransferWindow()
{
	util::ScopedLock lock(&m_sync);

	m_service->deleteDelegate(this);
}

void FileTransferWindow::onEndpointDisconnected(const std::string& endpointId)
{
	if(this->m_endpoint == endpointId) {
		QMetaObject::invokeMethod(this, "endpointDisconnect", Qt::QueuedConnection);
	}
}

void FileTransferWindow::onResponseDir(const std::string& endpointId, const TDirItems& content)
{
	util::ScopedLock lock(&m_sync);

	if(m_endpoint == endpointId){
		// update vector stringmap
		m_dirContent = content;

		// main thread UI Update call
		QMetaObject::invokeMethod(this, "updateDir", Qt::QueuedConnection);
	}
}

void FileTransferWindow::onResponseFile(const std::string& endpointId, const FileChunk& chunk)
{
	util::ScopedLock lock(&m_sync);

	if(m_endpoint == endpointId) {
		if (!chunk.m_valid 
			|| (chunk.m_fileName != m_remoteFileName)
			|| !m_fileWriter.open(m_localFileName))
		{
			QMetaObject::invokeMethod(this, "stopFileTransmission", Qt::QueuedConnection);
			::MessageBoxA(NULL, "Failed to download file", "Error", MB_ICONERROR);
			return;
		}
		m_transferringFileSize = chunk.m_fileSize;

		// Write data to file
		m_fileWriter.write(chunk.m_fileData);

		if (chunk.m_fileSize > m_fileWriter.size())
		{
			// Get another chunk
			FileRequest request;
			request.m_fileName = m_remoteFileName;
			request.m_startFrom = m_fileWriter.size();

			m_service->requestFile(m_endpoint, request);
			// update ui
			QMetaObject::invokeMethod(this, "updateFile", Qt::QueuedConnection, 
				Q_ARG(qlonglong, m_fileWriter.size()), Q_ARG(qlonglong, chunk.m_fileSize));
		}
		else
		{
			QMetaObject::invokeMethod(this, "stopFileTransmission", Qt::QueuedConnection);
		}
	}
}

void FileTransferWindow::onUploadFileReply(const std::string& endpointId, bool ok)
{
	util::ScopedLock lock(&m_sync);

	if(m_endpoint == endpointId) {
		if (!ok) {
			QMetaObject::invokeMethod(this, "stopFileTransmission", Qt::QueuedConnection);
			::MessageBoxA(NULL, "Failed to upload file", "Error", MB_ICONERROR);
			return;
		}

		FileChunk chunk;
		chunk.m_fileName = m_remoteFileName;
		chunk.m_fileSize = m_fileReader.size();
		chunk.m_positionFrom = m_transferringFilePosition;
		chunk.m_valid = true;

		if (m_transferringFilePosition >= chunk.m_fileSize)
		{
			// File transmited completely, update UI
			QMetaObject::invokeMethod(this, "stopFileTransmission", Qt::QueuedConnection);
			return;
		}
	
		__int64 chunkSize = chunk.m_fileSize - chunk.m_positionFrom;
		if (chunkSize > kFileChunkSize) 
			chunkSize = kFileChunkSize;
		m_transferringFilePosition += chunkSize;

		if (!m_fileReader.read(chunk.m_fileData, chunk.m_positionFrom, static_cast<int>(chunkSize)))
		{
			// Failed to read, signal client to stop receiving file
			chunk.m_valid = false;
			m_service->uploadFile(m_endpoint, chunk);

			// Stop uploading
			QMetaObject::invokeMethod(this, "stopFileTransmission", Qt::QueuedConnection);
			::MessageBoxA(NULL, "Failed to read file", "Error", MB_ICONERROR);
			return;
		}
		m_service->uploadFile(m_endpoint, chunk);
		// Update UI
		QMetaObject::invokeMethod(this, "updateFile", Qt::QueuedConnection,
			Q_ARG(qlonglong, chunk.m_positionFrom), Q_ARG(qlonglong, chunk.m_fileSize));
	}
}

void FileTransferWindow::onRequestDirClicked()
{
	QString strDir = ui.txtDir->text();

	if (strDir.isEmpty())
	{
		::MessageBoxA(NULL, "Directory is empty", "Error", MB_ICONERROR);
		return;
	}

	try
	{
		m_service->requestDir(m_endpoint, strDir.toStdString());
		m_currentRemoteDir = strDir;
	}
	catch (const std::exception& x)
	{
		::MessageBoxA(NULL, x.what(), "Error", MB_ICONERROR);
	}
}

void FileTransferWindow::onDownloadFileClicked()
{
	// Get current selected file
	QList<QListWidgetItem*> items = ui.lwDirRemote->selectedItems();
	if (items.empty() || items.front()->data(Qt::UserRole).toBool())
	{
		// Nothing or directory were selected
		::MessageBoxA(NULL, "Select remote file", "Error", MB_ICONERROR);
		return;
	}

	// File was selected
	QString remoteFileName = m_currentRemoteDir + "/" + items.front()->text();
	startFileDownload(remoteFileName.toStdString(), m_fileSystemModel->rootPath().toStdString());
}

void FileTransferWindow::onUploadFileClicked()
{
	if (m_currentRemoteDir.isEmpty())
	{
		// Need to know remote dir
		::MessageBoxA(NULL, "Load remote directory", "Error", MB_ICONERROR);
		return;
	}

	// Get current selected file
	QModelIndex index = ui.tvDirLocal->currentIndex();
	if (!index.isValid() || !m_fileSystemModel->fileInfo(index).isFile())
	{
		// Nothing or directory were selected
		::MessageBoxA(NULL, "Select local file", "Error", MB_ICONERROR);
		return;
	}

	startFileUpload(m_fileSystemModel->fileInfo(index).absoluteFilePath().toStdString());
}

void FileTransferWindow::endpointDisconnect()
{
	util::ScopedLock lock(&m_sync);

	this->close();
}

void FileTransferWindow::onLocalFileSystemActivated(const QModelIndex& index)
{
	QModelIndex item = index.sibling(index.row(), 0);
	QString str = m_fileSystemModel->fileInfo(index).absoluteFilePath();

	ui.tvDirLocal->setRootIndex(m_fileSystemModel->setRootPath(str));
}

void FileTransferWindow::onDirRemoteItemActivated(QListWidgetItem* item)
{
	if (item && item->data(Qt::UserRole).toBool())
	{
		// This is directory
		QDir dir(m_currentRemoteDir);
		dir.cd(item->text());
		ui.txtDir->setText(dir.absolutePath());
		onRequestDirClicked();
	}
}

void FileTransferWindow::updateDir()
{
	// Update content of remote directory
	util::ScopedLock lock(&m_sync);

	ui.lwDirRemote->clear();

	// Split items to separate arrays by type
	std::vector<std::string> dirs;
	std::vector<std::string> files;

	for (auto item : m_dirContent)
	{
		if (item.m_isDir)
			dirs.push_back(item.m_name);
		else
			files.push_back(item.m_name);
	}

	// Sort dirs and files
	std::sort(dirs.begin(), dirs.end());
	std::sort(files.begin(), files.end());

	// Add items to widget
	QIcon dirIcon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
	QIcon fileIcon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);

	for (auto name : dirs)
	{
		QListWidgetItem* item = new QListWidgetItem(dirIcon, QString::fromStdString(name));
		item->setData(Qt::UserRole, true);
		ui.lwDirRemote->addItem(item);
	}

	for (auto name : files)
	{
		QListWidgetItem* item = new QListWidgetItem(fileIcon, QString::fromStdString(name));
		ui.lwDirRemote->addItem(item);
	}
}

void FileTransferWindow::updateFile(qlonglong position, qlonglong total)
{
	static QDateTime lastUpdate = QDateTime::currentDateTime();

	if (lastUpdate.msecsTo(QDateTime::currentDateTime()) < 200)
		return;
	lastUpdate = QDateTime::currentDateTime();

	double progress = (100.0 * position) / double(total);
	ui.progressBar->setValue(static_cast<int>(progress));
}

void FileTransferWindow::startFileDownload(const std::string& remoteFileName, const std::string& localPath)
{
	ui.btnDownloadFile->setEnabled(false);
	ui.btnUploadFile->setEnabled(false);
	ui.btnRequestDir->setEnabled(false);

	m_remoteFileName = remoteFileName;
	QFileInfo info(QString::fromStdString(remoteFileName));
	std::string fileName = info.fileName().toStdString();
	m_localFileName = localPath + "/" + fileName;

	FileRequest request;
	request.m_fileName = remoteFileName;
	request.m_startFrom = 0;

	m_service->requestFile(m_endpoint, request);
}

void FileTransferWindow::startFileUpload(const std::string& localFileName)
{
	ui.btnDownloadFile->setEnabled(false);
	ui.btnUploadFile->setEnabled(false);
	ui.btnRequestDir->setEnabled(false);

	m_localFileName = localFileName;
	QFileInfo info(QString::fromStdString(localFileName));
	std::string fileName = info.fileName().toStdString();
	m_remoteFileName = m_currentRemoteDir.toStdString() + "/" + fileName;

	if (!m_fileReader.open(m_localFileName))
	{
		::MessageBoxA(NULL, "Failed to open file", "Error", MB_ICONERROR);
		stopFileTransmission();
		return;
	}

	FileChunk chunk;
	chunk.m_fileName = m_remoteFileName;
	chunk.m_fileSize = m_fileReader.size();
	chunk.m_positionFrom = 0;
	chunk.m_valid = true;
	
	int chunkSize = chunk.m_fileSize < kFileChunkSize ? chunk.m_fileSize : kFileChunkSize;
	if (!m_fileReader.read(chunk.m_fileData, 0, chunkSize))
	{
		::MessageBoxA(NULL, "Failed to read file", "Error", MB_ICONERROR);
		stopFileTransmission();
		return;
	}

	m_transferringFilePosition = chunkSize;

	m_service->uploadFile(m_endpoint, chunk);
}

void FileTransferWindow::stopFileTransmission()
{
	m_fileReader.close();
	m_fileWriter.close();

	ui.btnDownloadFile->setEnabled(true);
	ui.btnUploadFile->setEnabled(true);
	ui.btnRequestDir->setEnabled(true);
	ui.progressBar->setValue(0);
}
