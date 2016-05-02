#include "Server.h"
#include <QtWidgets/QApplication>
#pragma execution_character_set("utf-8")

int main(int argc, char *argv[])
{
	// Winsock Initialization
	WSADATA wsaData;
	memset(&wsaData, 0, sizeof(wsaData));
	int res = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res != NO_ERROR)
		throw util::Error("WSAStartup function failed");
	
	QApplication a(argc, argv);
	Server w;

	w.show();
	return a.exec();
}
