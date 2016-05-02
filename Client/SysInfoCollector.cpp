#include "SysInfoCollector.hpp"
#include <windows.h>
#include <stdio.h>
#include <sstream>

std::string getWindowsVersion()
{
	DWORD dwVersion = 0;
	DWORD dwMajorVersion = 0;
	DWORD dwMinorVersion = 0;
	DWORD dwBuild = 0;

	dwVersion = GetVersion();

	// Get the Windows version.

	dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
	dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

	// Get the build number.

	if (dwVersion < 0x80000000)
		dwBuild = (DWORD)(HIWORD(dwVersion));

	std::stringstream ss;

	//put arbitrary formatted data into the stream
	ss << dwMajorVersion << "." << dwMinorVersion << "(" << dwBuild << ")";

	//convert the stream buffer into a string
	std::string str = ss.str();
	return str;

}



std::vector<std::string> SysInfoCollector::collect()
{
	std::vector<std::string> data;
	data.push_back("Windows Version: фсдф" + getWindowsVersion());

	return data;
	
}


