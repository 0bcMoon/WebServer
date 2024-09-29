
#include "Debug.hpp"

#include <sstream>

Debug::Debug(std::string msg, const char *file, int line)
{
	std::stringstream ss;
	ss << msg << " at " << file << ":" << line;
	message = ss.str();
}

Debug::Debug() : message("") {}

const char *Debug::what() const throw()
{
	return (message.c_str());
}


Debug::~Debug() throw() {}
