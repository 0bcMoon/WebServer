#ifndef PARSEREXCEPTION_HPP
# define PARSEREXCEPTION_HPP

#include <exception>
#include <string>
#include "Debug.hpp"

class ParserException: public std::exception
{
	private:
		std::string message;

	public:
		// ~ParserException();
		ParserException(std::string msg) ;
		ParserException();
		~ParserException() throw();
		virtual const char *what() const throw();
};


#define ParserException(msg) Debug(msg, __FILE__, __LINE__)
#endif
