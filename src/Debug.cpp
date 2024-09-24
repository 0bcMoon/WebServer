
#include "Debug.hpp"


Debug::Debug(std::string msg, const char *file, int line)
{
	message = msg + " at " + file + ":" + std::to_string(line);
}

Debug::Debug() : message("") {}

const char *Debug::what() const throw()
{
	return (message.c_str());
}


Debug::~Debug() throw() {}
