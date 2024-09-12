#include "ParserException.hpp"

ParserException::ParserException(std::string msg)
{
	message = msg;
}

ParserException::ParserException() : message(nullptr) {}

const char *ParserException::what() const throw()
{
	return (message.c_str());
}



ParserException::~ParserException() throw() {}
