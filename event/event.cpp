#include "Event.hpp"
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <strstream>
#include "VirtualServer.hpp"

Event::Event() : MAX_CONNECTION_QUEUE(32), MAX_EVENTS(1024)
{
	this->eventList  = NULL;
	std::cout << "Event constrator" << std::endl;
}

Event::Event(int max_connection, int max_events) : MAX_CONNECTION_QUEUE(max_connection), MAX_EVENTS(max_events)
{
	this->eventList  = NULL;
	std::cout << "Event constrator" << std::endl;
}
Event::~Event()
{
	VirtualServerMap_t::iterator it = this->VirtuaServers.begin();
	for (; it != this->VirtuaServers.end(); it++)
	{
		close(it->first);
	}
	std::map<int, VirtualServer *>::iterator it2 = this->defaultServer.begin();
	for (; it2 != this->defaultServer.end(); it2++)
	{
		if (this->VirtuaServers.find(it->first) == this->VirtuaServers.end())
			close(it2->first);
	}
	std::cout << "Event destructor" << std::endl;
}

void Event::InsertDefaultServer(VirtualServer *server, int socketFd)
{
	if (this->defaultServer.find(socketFd) != this->defaultServer.end())
		throw std::runtime_error("default server already exist on this port");
	this->defaultServer[socketFd] = server;
}

void Event::insertServerNameMap(ServerNameMap_t &serverNameMap, VirtualServer *server, int socketFd)
{
	std::set<std::string> &serverNames = server->getServerNames();
	if (serverNames.size() == 0) // if there is no server name  uname will be the default server in that port
		this->InsertDefaultServer(server, socketFd);
	else
	{
		std::set<std::string>::iterator it = serverNames.begin();
		for (; it != serverNames.end(); it++)
		{
			if (serverNameMap.find(*it) != serverNameMap.end())
				throw std::runtime_error("confilict: server name already exist on some port host");
			serverNameMap[*it] = server;
		}
	}
}

void Event::init(std::vector<VirtualServer> &VirtualServers)
{
	for (size_t i = 0; i < VirtualServers.size(); i++)
	{
		SocketAddrSet_t &socketAddr = VirtualServers[i].getAddress();
		SocketAddrSet_t::iterator it = socketAddr.cbegin();
		for (; it != socketAddr.end(); it++)
		{
			int socketFd;
			if (this->socketMap.find(*it) == this->socketMap.end()) // no need to create socket if it already exists
			{
				socketFd = this->CreateSocket(it);
				this->socketMap[*it] = socketFd;
			}
			else
				socketFd = this->socketMap.at(*it);

			VirtualServerMap_t::iterator it = this->VirtuaServers.find(socketFd);
			if (it == this->VirtuaServers.end()) // create empty for socket map if not exist
			{
				this->VirtuaServers[socketFd] = ServerNameMap_t();
				it = this->VirtuaServers.find(socketFd); // take the reference of server map;
			}
			ServerNameMap_t &serverNameMap = it->second;
			this->insertServerNameMap(serverNameMap, &VirtualServers[i], socketFd);
		}
	}
}

int Event::CreateSocket(SocketAddrSet_t::iterator &address)
{
	sockaddr_in address2;

	int socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFd < 0)
	{
		std::cout << "socket creation faild:" << strerror(errno) << std::endl;
		return -1;
	}
	this->setNonBlockingIO(socketFd);
	memset(&address2, 0, sizeof(sockaddr_in));
	address2.sin_family = AF_INET;
	address2.sin_addr.s_addr = htonl(address->host);
	address2.sin_port = htons(address->port);
	if (bind(socketFd, (struct sockaddr *)&address2, sizeof(sockaddr)) < 0)
	{
		close(socketFd);
		throw std::runtime_error("bind failed: " + std::string(strerror(errno)));
	}
	return (socketFd);
}

std::string Event::get_readable_ip(const VirtualServer::SocketAddr address)
{
	uint32_t ip = address.host;
	int port = address.port;
	unsigned char bytes[4];
	std::stringstream ss;

	bytes[0] = ip & 0xFF;
	bytes[1] = (ip >> 8) & 0xFF;
	bytes[2] = (ip >> 16) & 0xFF;
	bytes[3] = (ip >> 24) & 0xFF;
	ss << bytes[3] << "." << bytes[2] << "." << bytes[1] << "." << bytes[0] << ":" << port;
	return ss.str();
}

void Event::setNonBlockingIO(int socketFd)
{
	int optval = 1;

	int flags = fcntl(socketFd, F_GETFL, 0);
	if (flags == -1)
		throw std::runtime_error("unable to set the socket as nonBlocking : " + std::string(strerror(errno)));
	if (fcntl(socketFd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw std::runtime_error("unable to set the socket as nonBlocking : " + std::string(strerror(errno)));

	if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == -1)
		std::cout << "could not set socket port to reusable: " << strerror(errno) << '\n';
}

bool Event::Listen(int socketFd)
{
	if (listen(socketFd, this->MAX_CONNECTION_QUEUE) < 0)
		return false;
	return true;
}

bool Event::Listen()
{
	SocketMap_t::iterator it = this->socketMap.begin();

	for (; it != this->socketMap.end(); it++)
	{
		if (!this->Listen(it->second))
			throw std::runtime_error("could not listen: " + get_readable_ip(it->first) + " " + strerror(errno));
	}
	return true;
}
void Event::initIOmutltiplexing() 
{
	this->eventList = new struct kevent[this->MAX_EVENTS];

}
