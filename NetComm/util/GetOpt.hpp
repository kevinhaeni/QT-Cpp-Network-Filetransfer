#pragma once

#include <tchar.h>
#include <string>
#include <vector>

namespace util {

/// Command line parser
class GetOpt
{
public:
	GetOpt(const TCHAR* tszCmdLine);

	typedef std::basic_string<TCHAR> tstring;
	typedef std::vector<tstring> TArgs;

	TArgs argv;
};

} // namespace util
