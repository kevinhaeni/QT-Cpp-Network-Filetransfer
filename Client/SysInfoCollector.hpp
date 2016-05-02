#pragma once

#include <string>
#include <vector>

class SysInfoCollector
{	
public:
	SysInfoCollector()
	{
		
	}

	static std::vector<std::string> collect();
};