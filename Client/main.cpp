#include <windows.h>
#include <string.h>
#include <wchar.h>
#include <atlconv.h>

#include <util/Error.hpp>
#include <util/GetOpt.hpp>

#include "Logger.hpp"
#include "Service.hpp"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "NetComm.lib")

#define K_REG_VALUE_NAME _T("KevinNetComm")
#pragma execution_character_set("utf-8")

namespace {

void
install(const TCHAR* tszAddress)
{
	typedef std::basic_string<TCHAR> tstring;

	TCHAR buf[MAX_PATH * 2];
	memset(buf, 0, sizeof(buf));
	DWORD dw = ::GetModuleFileName(GetModuleHandle(NULL), buf, sizeof(buf) / sizeof(buf[0]));
	if (0 >= dw)
		throw std::logic_error("Failed to get executable path");

	_tcscpy(buf + _tcslen(buf), _T(" \""));
	_tcscpy(buf + _tcslen(buf), tszAddress);
	_tcscpy(buf + _tcslen(buf), _T("\""));

	HKEY hKey = 0;
	if (ERROR_SUCCESS !=
		::RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), &hKey))
	{
		throw std::logic_error("Failed to open HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run registry key");
	}

	LONG error = ::RegSetKeyValue(hKey, 0, K_REG_VALUE_NAME, REG_SZ, buf, _tcslen(buf));

	::RegCloseKey(hKey);

	if (ERROR_ACCESS_DENIED == error)
	{
		throw util::Error("Access denied setting registry key, try restart the program as admin");
	}
	else if (ERROR_SUCCESS != error)
	{
		std::stringstream msg;
		msg << "Failed to set registry key, error code: " << error;
		throw util::Error(msg.str());
	}
}

void
uninstall()
{
	HKEY hKey = 0;
	if (ERROR_SUCCESS !=
		::RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), &hKey))
	{
		throw std::logic_error("Failed to open HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run registry key");
	}

	LONG error = ::RegDeleteValue(hKey, K_REG_VALUE_NAME);

	::RegCloseKey(hKey);

	if (ERROR_ACCESS_DENIED == error)
	{
		throw util::Error("Access denied setting registry key, try restart the program as admin");
	}
	else if (ERROR_SUCCESS != error)
	{
		std::stringstream msg;
		msg << "Failed to set registry key, error code: " << error;
		throw util::Error(msg.str());
	}
}

} // namespace


int APIENTRY
_tWinMain(HINSTANCE hInstance,
					 HINSTANCE hPrevInstance,
					 LPTSTR    lpCmdLine,
					 int       nCmdShow)
{
	USES_CONVERSION;

	WSADATA wsaData;
	memset(&wsaData, 0, sizeof(wsaData));

	try
	{
		int res = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (res != NO_ERROR)
			throw util::Error("WSAStartup function failed");

		util::GetOpt opt(lpCmdLine);

		enum {
			CMD_NONE = 0,
			CMD_INSTALL,
			CMD_UNINSTALL
		} command = CMD_NONE;

		typedef util::GetOpt::tstring tstring;
		tstring address;

		for (util::GetOpt::TArgs::const_iterator ii  = opt.argv.begin();
			ii != opt.argv.end();
			++ii)
		{
			const tstring& param = ii->c_str();

			if (param == _T("/i") ||
				param == _T("/I"))
			{
				command = CMD_INSTALL;
			}
			else if (param == _T("/u") ||
				param == _T("/U"))
			{
				command = CMD_UNINSTALL;
			}
			else
			{
				address = param;
			}
		}

		switch (command)
		{
		case CMD_INSTALL:
			install(address.c_str());
			break;
		case CMD_UNINSTALL:
			uninstall();
			break;
		}

		Service service(T2CA(address.c_str()));
		service.neverStop();
	}
	catch (const std::exception& x)
	{
		ignore_unused(x);
		logger::out("ERROR");
		logger::out(x.what());

		::MessageBoxA(::GetActiveWindow(), x.what(), "NetComm Client", MB_ICONERROR);

		return 1;
	}
	catch (...)
	{
		logger::out("UNKNOWN ERROR");

		::MessageBoxA(::GetActiveWindow(), "Unknown error", "NetComm Client", MB_ICONERROR);

		return 2;
	}

	::WSACleanup();

	return 0;
}
