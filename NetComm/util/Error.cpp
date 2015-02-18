#include "Error.hpp"

namespace util {

Error::Error(const std::string& errorDescription)
: m_errorDescription(errorDescription)
{
}

const char*
Error::what() const
{
	return m_errorDescription.c_str();
}

} // namespace util
