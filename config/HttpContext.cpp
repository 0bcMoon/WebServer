
#include "HttpContext.hpp"
#include "ParserException.hpp"
#include "Server.hpp"
#include <iostream>

HttpContext::HttpContext()
{
}

void HttpContext::pushServer(tokens_it &token, tokens_it &end)
{
	Server *server;

	token++;
	if (token == end)
		throw ParserException("Unexpected end of file");
	else if (*token != "{")
		throw ParserException("Unexpact token: " + *token);
	token++;
	std::cout << "Server\n";
	server = new Server;
	while (token != end && *token != "}")
	{
		if (*token == "location")
			server->pushLocation(token, end);
		else
			token++;
			
		std::cout << *token << "\n";
	}
	if (token == end)
		throw ParserException("Unexpected end of file");
	token++;
	this->servers.push_back(server);
}
