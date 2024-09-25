
#include "Location.hpp"
#include <cstddef>
#include "DataType.hpp"
#include "ParserException.hpp"
Location::Location()
{
}

void Location::setPath(std::string &path)
{
	// TODO validate path
	// maybe encode and decode path
	this->path = path;
}

bool Location::isValidStatusCode(std::string &str)
{
	int status = 0;
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
	token++;
	this->globalParam.validateOrFaild(token, end);
	if (isValidStatusCode(*token))
		throw ParserException("Invalid status code: " + *token);
	this->redirect.status = *token;
	if (token == end)
		throw ParserException("Unexpected end of file");
	if (*token != ";")
		this->redirect.url = *token;
	token++;
	if (token == end || *token != ";")
		throw ParserException("Unexpected token: " + *token);
}

void Location::parseTokens(Tokens &token, Tokens &end)
{
	if (token == end)
		throw ParserException("Unexpected end of file");

	if (*token == "return")
		this->setRedirect(token, end);

}
