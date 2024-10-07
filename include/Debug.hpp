#ifndef Debug_H
#define Debug_H

#include <exception>
#include <iostream>
#include <string>
#include <set>
#include <utility>

class Debug : public std::exception
{
  private:
	std::string message;

  public:

	Debug(std::string msg, const char *file, int line);
	Debug();
	~Debug() throw();
	virtual const char *what() const throw();
};




#endif
