#include "ParserException.hpp"

ParserException::ParserException(std::string msg)
{
	message = msg;
}

ParserException::ParserException() : message(NULL) {}

const char *ParserException::what() const throw()
{
	return (message.c_str());
}



ParserException::~ParserException() throw() {}
