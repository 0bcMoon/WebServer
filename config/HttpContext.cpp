
#include "HttpContext.hpp"
#include "DataType.hpp"
#include "ParserException.hpp"
#include "Server.hpp"
#include <iostream>
#include <vector>

HttpContext::HttpContext()
{
}

// TODO : check if value is was set  for duplicates
//
std::vector<Server> &HttpContext::getServers()
{
	return this->servers;
}
void HttpContext::parseTokens(Tokens &token, Tokens &end)
{
	this->globalParam.parseTokens(token, end);
}
void HttpContext::pushServer(Tokens &token, Tokens &end)
{

	token++;
	if (token == end)
		throw ParserException("Unexpected end of file");
	else if (*token != "{")
		throw ParserException("Unexpact token: " + *token);
	token++;
	std::cout << "\n---------------->Server<---------------\n\n";
	Server server;
	while (token != end && *token != "}")
	{
		if (*token == "location")
			server.pushLocation(token, end);
		else
			server.parseTokens(token, end);
	}
	if (token == end || *token != "}")
		throw ParserException("Unexpected end of file");
	this->servers.push_back(server);
	token++;
}
