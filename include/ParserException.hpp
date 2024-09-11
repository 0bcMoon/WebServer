#ifndef PARSEREXCEPTION_HPP
# define PARSEREXCEPTION_HPP

#include <exception>
#include <string>

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


#endif
