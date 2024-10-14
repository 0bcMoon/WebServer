
#include "Location.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <unistd.h>
#include <cerrno>
#include <cstddef>
#include "DataType.hpp"
#include "ParserException.hpp"
Location::Location() 
{
}

Location &Location::operator=(const Location &location)
{
	this->path = location.path;
	this->redirect = location.redirect;
	this->globalConfig = location.globalConfig;
	this->cgi_path = location.cgi_path;
	this->cgi_ext = location.cgi_ext;
	return *this;
}
void Location::setPath(std::string &path)
{
	// TODO validate path
	// may be support encode
	this->path = path;
}

bool GlobalConfig::isValidStatusCode(std::string &str)
{
	int status = 0;
	if (str.size() != 3)
		return (false);
	for (size_t i = 0; i < str.size(); i++)
	{
		if (str[i] < '0' || str[i] > '9')
			return (false);
		status = status * 10 + str[i] - 48;
		if (status > 999)
			return (false);
	}
	return (status >= 100 && status <= 599);
}
void Location::setRedirect(Tokens &token, Tokens &end)
{
	this->globalConfig.validateOrFaild(token, end);
	if (!this->globalConfig.isValidStatusCode(*token))
		throw ParserException("Invalid status code: " + *token);
	this->redirect.status = this->globalConfig.consume(token, end);
	if (token == end)
		throw ParserException("Unexpected end of file");
	if (*token != ";")
		this->redirect.url = *token++;
	this->globalConfig.CheckIfEnd(token, end);
}

const std::string &Location::getPath()
{
	return (this->path);
}

void Location::parseTokens(Tokens &token, Tokens &end)
{
	if (*token == "return")
		this->setRedirect(token, end);
	else if (*token == "cgi_path")
		this->setCGI(token, end);
	else
		this->globalConfig.parseTokens(token, end);
}

void Location::setCGI(Tokens &token, Tokens &end)
{
	this->globalConfig.validateOrFaild(token, end);
	this->cgi_ext = this->globalConfig.consume(token, end);
	this->cgi_path = this->globalConfig.consume(token, end);
	this->globalConfig.CheckIfEnd(token, end);
	if (cgi_ext[0] != '.')
		throw ParserException("Invalid CGI extension" + cgi_ext);
	if (access(cgi_path.c_str(), F_OK | X_OK | R_OK) == -1)
		throw ParserException("Invalid CGI path" + cgi_path + ": " + std::string(strerror(errno)));
}
const std::string &Location::geCGItPath()
{
	return (this->cgi_path);
}

const std::string &Location::geCGIext()
{
	return (this->cgi_ext);
}
// bool Location
