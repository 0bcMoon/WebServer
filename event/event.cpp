#include "Event.hpp"
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include "VirtualServer.hpp"

Event::Event() : MAX_CONNECTION_QUEUE(32), MAX_EVENTS(1024)
{
	this->eventList = NULL;
	this->eventChangeList = NULL;
	this->kqueueFd = -1;
	this->numOfSocket = 0;
}

Event::Event(int max_connection, int max_events) : MAX_CONNECTION_QUEUE(max_connection), MAX_EVENTS(max_events)
{
	this->eventList = NULL;
	this->eventChangeList = NULL;
	this->kqueueFd = -1;
	this->numOfSocket = 0;
}
Event::~Event()
{
	delete this->eventList;
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
	if (this->kqueueFd >= 0)
		close(this->kqueueFd);
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
	this->numOfSocket++;
	return (socketFd);
}

std::string Event::get_readable_ip(const VirtualServer::SocketAddr address)
{
	uint32_t ip = address.host;
	int port = address.port;
	std::stringstream ss;

	int a = ip & 0xFF;
	int b = (ip >> 8) & 0xFF;
	int c = (ip >> 16) & 0xFF;
	int d = (ip >> 24) & 0xFF;
	ss << d << "." << c << "." << b << "." << a << ":" << port;
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
		std::cout << "server listen on " << this->get_readable_ip(it->first) << std::endl;
	}
	return true;
}

void Event::CreateChangeList()
{
	int i = 0;
	VirtualServerMap_t::iterator it = this->VirtuaServers.begin();
	for (; it != this->VirtuaServers.end(); it++)
	{
		EV_SET(&this->eventChangeList[i], it->first, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
		EV_SET(&this->eventChangeList[i + this->numOfSocket], it->first, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
		i++;
	}
	std::map<int, VirtualServer *>::iterator dit = this->defaultServer.begin();
	for (; dit != this->defaultServer.end(); dit++)
	{
		if (this->VirtuaServers.find(dit->first) == this->VirtuaServers.end())
		{
			EV_SET(&this->eventChangeList[i], dit->first, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
			EV_SET(
				&this->eventChangeList[i + this->numOfSocket],
				dit->first,
				EVFILT_WRITE,
				EV_ADD | EV_ENABLE,
				0,
				0,
				NULL);
			i++;
		}
	}
	if (kevent(this->kqueueFd, this->eventChangeList, this->numOfSocket * 2, NULL, 0, NULL) < 0)
		throw std::runtime_error("kevent failed: could not regester event: " + std::string(strerror(errno)));
}

void Event::initIOmutltiplexing()
{
	this->eventList = new struct kevent[this->MAX_EVENTS];
	this->eventChangeList = new struct kevent[this->numOfSocket * 2];
	this->kqueueFd = kqueue();
	if (this->kqueueFd < 0)
		throw std::runtime_error("kqueue faild: " + std::string(strerror(errno)));
	this->CreateChangeList();
}

void Event::eventLoop()
{
	int nevents;

	std::cout << "Server is leistening on for new connnection\n";
	while (1)
	{
		nevents = kevent(this->kqueueFd, NULL, 0, this->eventList, this->MAX_EVENTS, NULL);
		if (nevents < 0)
			throw std::runtime_error("kevent failed:could not pull event " + std::string(strerror(errno)));
		for (int i = 0; i < nevents; i++)
		{
			if (eventList[i].filter == EVFILT_READ)
				std::cout << "read event\n";
			else if (eventList[i].filter == EVFILT_WRITE)
				std::cout << "write event\n";
			else if (eventList[i].filter == EV_ERROR) // free client  file descriptor
				throw std::runtime_error("client discount");
		}
	}
}
