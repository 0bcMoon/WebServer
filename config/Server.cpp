
#include "Server.hpp"
#include <cstddef>
#include <string>
#include <vector>
#include "Debug.hpp"
#include "Location.hpp"
#include "ParserException.hpp"

Server::Server() {}

void Server::pushLocation(Tokens &token, Tokens &end)
{
	Location location;
	token++;
	if (token == end)
		throw ParserException("Unexpected end of file");
	if (GlobalParam::IsId(*token))
		throw ParserException("Unexpected token: " + *token);
	location.setPath(*token);
	token++;
	if (token == end || *token != "{")
		throw ParserException("Unexpected end of file");
	token++;
	while (token != end && *token != "}")
	{
		location.parseTokens(token, end);
	}
}

int parseNumber(std::string &str)
{
	int number = 0;

	if (str.empty())
		return (-1);
	for (size_t i = 0; i < str.size(); i++)
	{
		if (str[i] < '0' || str[i] > '9')
			return (-1);
		number = number * 10 + str[i] - 48;
		if (number > (1 << 16))
			return (-1);
	}
	return (number);
}

int parseHost(std::string s_host)
{
	std::vector<std::string> octets;
	size_t pos;
	int addr = 0;
	size_t offset = 0;
	int octet;

	if (s_host.empty())
		return (0);
	while ((pos = s_host.find('.')) != std::string::npos)
	{
		const std::string &token = s_host.substr(offset, pos);
		octets.push_back(token);
		s_host.erase(0, pos + 1); // not nice but is working  (plz don't throw a expcetion)
	}
	octets.push_back(s_host);
	if (octets.size() != 4)
		return (-1);
	for (int i = 0; i < 4; i++)
	{
		octet = parseNumber(octets[i]);
		if (octet < 0 || octet > 255)
			throw ParserException("Invalid host" + s_host);
		addr |= (octet << ((3 - i) * 8)); // TODO : make  right bytes order
	}
	return (addr);
}
void Server::setListen(Tokens &token, Tokens &end)
{

	std::string host;
	std::string port;
	SocketAddr socketAddr;

	this->globalParam.validateOrFaild(token, end);
	size_t pos = token->find(':');
	if (pos == 0 || pos == token->size() - 1)
		throw ParserException("Unvalid [host:]port " + *token);

	if (pos != std::string::npos)
		host = token->substr(0, pos);
	else
		pos = 0;
	port = token->substr(pos + (pos != 0), token->size() - pos);
	socketAddr.port = parseNumber(port);
	if (socketAddr.port <= 0)
		throw ParserException("Invlaid Port number " + port);
	socketAddr.host = parseHost(host);
	if (listen.find(socketAddr) != listen.end())
		throw ParserException("dublicate listen: " + host + ":" + port);
	listen.insert(socketAddr);
	token++;
	if (token == end || *token != ";")
		throw ParserException("Unexpected end of file");
	token++;
}

std::set<Server::SocketAddr> &Server::getListen()
{
	return (this->listen);
}

bool Server::isListen(const SocketAddr &addr) const
{
	return (listen.find(addr) != listen.end());
}

void Server::setServerNames(Tokens &token, Tokens &end)
{
	this->globalParam.validateOrFaild(token, end);

	while (token != end && *token != ";")
	{
		serverNames.insert(*token);
		token++;
	}
	if (token == end || *token != ";")
		throw ParserException("Unexpected end of file"); 
	token++;
}

void Server::parseTokens(Tokens &token, Tokens &end)
{
	if (token == end)
		throw ParserException("Unexpected end of file");
	if (*token == "listen")
		this->setListen(token, end);
	else if (*token == "server_name")
		this->setServerNames(token, end);
	else if (*token == "location")
		this->pushLocation(token, end);
	else if (!globalParam.parseTokens(token, end))
		throw ParserException("Unexpected token: " + *token);
}
