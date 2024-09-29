
#include "Location.hpp"
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

std::string &Location::getPath()
{
	return this->path;
}

void Location::parseTokens(Tokens &token, Tokens &end)
{
	if (*token == "return")
		this->setRedirect(token, end);
	else
		this->globalConfig.parseTokens(token, end);
}
