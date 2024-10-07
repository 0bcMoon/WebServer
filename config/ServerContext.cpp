
#include "ServerContext.hpp"
#include "DataType.hpp"
#include "ParserException.hpp"
#include "VirtualServer.hpp"
#include <iostream>
#include <vector>

ServerContext::ServerContext()
{
}

ServerContext::~ServerContext()
{
	for (size_t i = 0; i < this->servers.size(); i++)
		this->servers[i].deleteRoutes();
}
// TODO : check if value is was set  for duplicates
//
std::vector<VirtualServer> &ServerContext::getServers()
{
	return this->servers;
}
void ServerContext::parseTokens(Tokens &token, Tokens &end)
{
	this->globalParam.parseTokens(token, end);
}
void ServerContext::pushServer(Tokens &token, Tokens &end)
{
	token++;
	if (token == end)
		throw ParserException("Unexpected end of file");
	else if (*token != "{")
		throw ParserException("Unexpact token: " + *token);
	token++;

	this->servers.push_back(VirtualServer());  // push empty VirtualServer to keep 
											   // the reference in http object in case of exception to cause memory leak

	VirtualServer &server = this->servers.back(); // grants access to the last element

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
}

std::vector<VirtualServer> &ServerContext::getVirtualServers()
{
	return this->servers;
}
