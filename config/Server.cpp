
#include "Server.hpp"
#include "Location.hpp"
#include "ParserException.hpp"
#include "Tokenizer.hpp"

Server::Server()
{

}


void Server::pushLocation(Tokens &token, Tokens &end)
{
	Location *location;

	token++;
	if (token == end)
		throw ParserException("Unexpected end of file");
	else if (Tokenizer::IsId((*token)[0]))
		throw ParserException("Unexpact token: " + *token);
	token++;
	if (token == end)
		throw ParserException("Unexpected end of file");
	location = new Location(*(token++));
	if (token == end && *token != "{")
		throw ParserException("Unexpact token: " + *token);
	token++;
	std::cout << "\n\t\t--->Location<---\n";
	while (token != end && *token != "}")
	{
		std::cout <<"\t\t\t"<< *token << "\n";
		token++;
	}
	if (token == end)
		throw ParserException("Unexpected end of file");
}
