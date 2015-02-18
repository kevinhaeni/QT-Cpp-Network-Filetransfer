#pragma once

#include <exception>
#include <string>

namespace util {

class Error : public std::exception
{
protected:
	Error()
	{
	}

public:
	/// Constructs an instance of exception with a specified error string
	Error(const std::string& errorDescription);

	/// Returns description of a message
	virtual const char* what() const;

protected:
	std::string m_errorDescription;
};

class ConnectionClosedError : public Error
{
public:
	ConnectionClosedError() : Error("TCP connection was closed") {}
};

} // namespace util
