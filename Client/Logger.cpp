#include "Logger.hpp"

#include <windows.h>

namespace logger
{

void out(const std::string& s)
{
	::OutputDebugStringA("\n*** ");
	::OutputDebugStringA(s.c_str());
	::OutputDebugStringA("\n");
}

} // namespace logger
