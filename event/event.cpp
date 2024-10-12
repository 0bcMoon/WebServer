#include "Event.hpp"
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include "../include/Client.hpp"
#include "Connections.hpp"
#include "HttpRequest.hpp"
#include "VirtualServer.hpp"

Event::Event() : MAX_CONNECTION_QUEUE(32), MAX_EVENTS(1024)
{
	this->evList = NULL;
	this->eventChangeList = NULL;
	this->kqueueFd = -1;
	this->numOfSocket = 0;
}

Event::Event(int max_connection, int max_events, ServerContext *ctx)
	: MAX_CONNECTION_QUEUE(max_connection), MAX_EVENTS(max_events)
{
	this->ctx = ctx;
	this->evList = NULL;
	this->eventChangeList = NULL;
	this->kqueueFd = -1;
	this->numOfSocket = 0;
}
Event::~Event()
{
	delete this->evList;
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

void Event::init()
{
	std::vector<VirtualServer> &virtualServers = ctx->getServers();

	for (size_t i = 0; i < virtualServers.size(); i++)
	{
		SocketAddrSet_t &socketAddr = virtualServers[i].getAddress();
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
			this->insertServerNameMap(serverNameMap, &virtualServers[i], socketFd);
		}
	}
}

int set_non_blocking(int sockfd) // TODO make it more oop
{
	int flags = fcntl(sockfd, F_GETFL, 0);
	if (flags == -1)
	{
		perror("fcntl(F_GETFL)");
		return -1;
	}
	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		perror("fcntl(F_SETFL)");
		return -1;
	}
	flags = fcntl(sockfd, F_GETFL, 0);
	if (flags & O_NONBLOCK)
		printf("socket is non-blocking\n");
	return 0;
}

int Event::CreateSocket(SocketAddrSet_t::iterator &address)
{
	sockaddr_in address2;

	int socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFd < 0)
		throw std::runtime_error("socket creation failed: " + std::string(strerror(errno)));

	memset(&address2, 0, sizeof(struct sockaddr_in));

	address2.sin_family = AF_INET;
	address2.sin_addr.s_addr = htonl(address->host);
	address2.sin_port = htons(address->port);
	if (bind(socketFd, (struct sockaddr *)&address2, sizeof(address2)) < 0)
	{
		close(socketFd);
		throw std::runtime_error("bind failed: " + std::string(strerror(errno)));
	}
	this->numOfSocket++;
	this->sockAddrInMap[socketFd] = address2;
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
	int flags = fcntl(socketFd, F_GETFL, 0);
	if (flags == -1)
		perror("fcntl(F_GETFL)");
	if (fcntl(socketFd, F_SETFL, flags | O_NONBLOCK) == -1)
		perror("fcntl(F_SETFL)");
	flags = fcntl(socketFd, F_GETFL, 0);
	if (flags & O_NONBLOCK)
		printf("socket is non-blocking\n");
}

bool Event::Listen(int socketFd)
{
	// TODO: this->MAX_CONNECTION_QUEUE

	if (listen(socketFd, 5) < 0)
	{
		perror("listen failed");
		return false;
	}
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
	SockAddr_in::iterator it = this->sockAddrInMap.begin();
	// TODO : monitor read and write
	for (; it != this->sockAddrInMap.end(); it++)
	{
		EV_SET(&this->eventChangeList[i], it->first, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
		EV_SET(&this->eventChangeList[i + this->numOfSocket], it->first, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
		i++;
	}
	if (kevent(this->kqueueFd, this->eventChangeList, this->numOfSocket * 2, NULL, 0, NULL) < 0)
		throw std::runtime_error("kevent failed: could not regester server event: " + std::string(strerror(errno)));
	delete this->eventChangeList;
	this->eventChangeList = NULL;
}

void Event::initIOmutltiplexing()
{
	this->evList = new struct kevent[this->MAX_EVENTS];
	this->eventChangeList = new struct kevent[this->numOfSocket * 2];
	this->kqueueFd = kqueue();
	if (this->kqueueFd < 0)
		throw std::runtime_error("kqueue faild: " + std::string(strerror(errno)));
	this->CreateChangeList();
}

int Event::newConnection(int socketFd)
{
	struct sockaddr_in address = this->sockAddrInMap[socketFd];
	socklen_t size = sizeof(struct sockaddr);
	std::cout << "waiting for new connnection to accept\n";
	int newSocketFd = accept(socketFd, (struct sockaddr *)&address, &size);
	if (newSocketFd < 0)
		return -1;
	std::cout << "connnection accept\n";
	set_non_blocking(socketFd);
	return newSocketFd;
}

void read_from_client(int fd)
{
	char buffer[2048] = {0};
	printf("try to read from %d", fd);
	int r = read(fd, buffer, 254);
	if (r <= 0)
	{
		if (r < 0)
			perror("read faild");
		close(fd);
		printf("client %d disconnected\n", fd);
		return;
	}
	else
	{
		buffer[r] = 0;
		printf("new message from %d: %s\n", fd, buffer);
	}
}

int Event::RemoveClient(int clientFd)
{
	// TODO : hendel if there is an error server should never go down
	struct kevent ev_set;
	EV_SET(&ev_set, clientFd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	kevent(this->kqueueFd, &ev_set, 1, NULL, 0, NULL);
	EV_SET(&ev_set, clientFd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
	kevent(this->kqueueFd, &ev_set, 1, NULL, 0, NULL);
	close(clientFd);
	return 0;
}

int Event::RegsterClient(int clientFd)
{
	// TODO : hendel if there is an error server should never go down
	struct kevent ev_set;

	EV_SET(&ev_set, clientFd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
	kevent(this->kqueueFd, &ev_set, 1, NULL, 0, NULL);

	EV_SET(&ev_set, clientFd, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
	kevent(this->kqueueFd, &ev_set, 1, NULL, 0, NULL);
	return 0;
}

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_EVENTS 10

void Event::eventLoop()
{
	int client_fd;
	int socketFd;
	Connections connections(ctx);
	// HttpRequest *req;

	// this->ctx->getMaxBodySize();
	while (1)
	{
		int nev = kevent(this->kqueueFd, NULL, 0, this->evList, MAX_EVENTS, NULL);
		if (nev == -1)
			throw std::runtime_error("kevent failed: " + std::string(strerror(errno)));

		for (int i = 0; i < nev; i++)
		{
			socketFd = this->evList[i].ident;
			if (this->evList->flags & EV_ERROR)
			{
				fprintf(stderr, "EV_ERROR: %s\n", strerror(this->evList->data));
				continue;
			}
			if (this->checkNewClient(socketFd))
			{
				client_fd = this->newConnection(socketFd);
				this->RegsterClient(client_fd);
			}
			else if (this->evList[i].filter == EVFILT_READ)
			{
				std::cout << "READ Event \n";
				connections.requestHandler(socketFd);
				connections.clients[socketFd]->respond();
			}
			else if (this->evList[i].filter == EVFILT_WRITE)
			{
				// if (connections.clients.count(socketFd))
				// {
				// }
				// else
				// 	std::cerr << "client un exist\n";
			}
		}
	}
}
bool Event::checkNewClient(int socketFd)
{
	return (this->sockAddrInMap.find(socketFd) != this->sockAddrInMap.end());
}
