
#include "HttpContext.hpp"
#include "DataType.hpp"
#include "ParserException.hpp"
#include "VirtualServer.hpp"
#include <iostream>
#include <vector>

HttpContext::HttpContext()
{

}

HttpContext::~HttpContext()
{
	std::cout << "HttpContext destructor" << std::endl;
	std::cout << "handle memory leaks"<<std::endl;
}
// TODO : check if value is was set  for duplicates
//
std::vector<VirtualServer> &HttpContext::getServers()
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
	VirtualServer server;
	while (token != end && *token != "}")
	{
		if (*token == "location")
			server.pushLocation(token, end);
		else
			server.parseTokens(token, end);
	}
	if (token == end || *token != "}")
		throw ParserException("Unexpected end of file");
	token++;
	this->servers.push_back(server);
}
