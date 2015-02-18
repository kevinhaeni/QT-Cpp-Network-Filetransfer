#include "GetOpt.hpp"
#include <cassert>

namespace util {

GetOpt::GetOpt(const TCHAR* tszCmdLine)
{
	enum {
		OUTSIDE_QUOTES = 0,
		INSIDE_QUOTES,
		QUOTE_INSIDE_QUOTES
	} state = OUTSIDE_QUOTES;

	if (tszCmdLine && tszCmdLine)
	{
		tstring param;

		for (const TCHAR* pch = tszCmdLine; *pch; ++pch)
		{
			switch (state)
			{
			case OUTSIDE_QUOTES:
				if (_T('"') == *pch)
				{
					state = INSIDE_QUOTES;
				}
				else if (_T(' ') >= *pch)
				{
					argv.push_back(param);
					param.clear();
				}
				else
				{
					param += *pch;
				}
				break;
			case INSIDE_QUOTES:
				if (_T('"') == *pch)
				{
					state = QUOTE_INSIDE_QUOTES;
				}
				else
				{
					param += *pch;
				}
				break;
			case QUOTE_INSIDE_QUOTES:
				if (_T('"') == *pch)
				{
					state = INSIDE_QUOTES;
					param += *pch;
				}
				else if (_T(' ') >= *pch)
				{
					argv.push_back(param);
					param.clear();
					state = OUTSIDE_QUOTES;
				}
				else
				{
					param += *pch;
					state = OUTSIDE_QUOTES;
				}
				break;
			default:
				assert(!"Unknown state");
			}
		}

		if (!param.empty())
			argv.push_back(param);
	}
}

} // namespace util
