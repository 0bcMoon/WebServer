#include "VirtualServer.hpp"
#include <sys/socket.h>
#include <cstddef>
#include <string>
#include <vector>
#include "Location.hpp"
#include "ParserException.hpp"

VirtualServer::VirtualServer() 
{
}

VirtualServer::~VirtualServer() 
{
	
}

VirtualServer::SocketAddr::SocketAddr(int port, int host) : port(port), host(host) {}
VirtualServer::SocketAddr::SocketAddr() {};

void VirtualServer::pushLocation(Tokens &token, Tokens &end)
{
	Location location;

	this->globalConfig.validateOrFaild(token, end);
	location.setPath(this->globalConfig.consume(token, end));
	if (token == end || *token != "{")
		throw ParserException("Unexpected Token: " + *token);
	token++;
	while (token != end && *token != "}")
		location.parseTokens(token, end);

	if (token == end)
		throw ParserException("Unexpected end of file");
	token++;
	 this->routes.insert(location);
}
void VirtualServer::deleteRoutes()
{
	this->routes.deleteNode();
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

//art
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
void VirtualServer::setListen(Tokens &token, Tokens &end)
{
	std::string host;
	std::string port;
	SocketAddr socketAddr;

	this->globalConfig.validateOrFaild(token, end);
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
	this->globalConfig.CheckIfEnd(token, end);
}

std::set<VirtualServer::SocketAddr> &VirtualServer::getAddress()
{
	return (this->listen);
}

bool VirtualServer::isListen(const SocketAddr &addr) const
{
	return (listen.find(addr) != listen.end());
}

void VirtualServer::setServerNames(Tokens &token, Tokens &end)
{
	this->globalConfig.validateOrFaild(token, end);

	while (token != end && *token != ";")
		serverNames.insert(
			this->globalConfig.consume(token, end)); // CONSUME  take curr token and check if its an id then
	this->globalConfig.CheckIfEnd(token, end);
}

void VirtualServer::parseTokens(Tokens &token, Tokens &end)
{
	if (token == end)
		throw ParserException("Unexpected end of file");
	if (*token == "listen")
		this->setListen(token, end);
	else if (*token == "server_name")
		this->setServerNames(token, end);
	else if (*token == "location")
		this->pushLocation(token, end);
	else
		globalConfig.parseTokens(token, end);
}

std::set<std::string> &VirtualServer::getServerNames()
{
	return (this->serverNames);
}


Location *VirtualServer::getRoute(const std::string &path)
{
	return (this->routes.findPath(path));
}
