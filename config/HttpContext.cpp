
#include "HttpContext.hpp"
#include "DataType.hpp"
#include "ParserException.hpp"
#include "Server.hpp"
#include <iostream>

HttpContext::HttpContext()
{
}

void HttpContext::parseTokens(Tokens &token, Tokens &end)
{

}
void HttpContext::pushServer(Tokens &token, Tokens &end)
{
	Server *server;

	token++;
	if (token == end)
		throw ParserException("Unexpected end of file");
	else if (*token != "{")
		throw ParserException("Unexpact token: " + *token);
	token++;
	std::cout << "\n---------------->Server<---------------\n\n";
	server = new Server;
	while (token != end && *token != "}")
	{
		if (*token == "location")
			server->pushLocation(token, end);
		else if (*token == "{")
			throw ParserException("Unexpact token: " + *token);

		// std::cout << *token << "\n";
		token++;
	}
	if (token == end)
		throw ParserException("Unexpected end of file");
	this->servers.push_back(server);
}
