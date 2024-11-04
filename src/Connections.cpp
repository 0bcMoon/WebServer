#include "Connections.hpp"
#include <sys/event.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include "Client.hpp"


Connections::Connections(ServerContext *ctx, int kqueueFd) : ctx(ctx), kqueueFd(kqueueFd)
{
}

Connections::~Connections()
{
	for (ClientsIter it = clients.begin(); it != clients.end(); ++it) {
		delete it->second;
	}
}

void	Connections::closeConnection(int	fd)
{
	// std::cout << "client disconnect\n";
	close(fd); // after close file fd all event will be clear
	delete clients[fd];
	clients.erase(fd);
}


void	Connections::addConnection(int	fd, int server)
{
	// std::cout << "new Connections\n";
	this->clients[fd] = new Client(fd, server, ctx);
}


// TODO: return client object : search once
Client		*Connections::requestHandler(int	fd, int data)
{
	ClientsIter clientIter = this->clients.find(fd);
	if ( clientIter == this->clients.end()) // TODO : fix
		return (NULL);

	clientIter->second->request.readRequest(data);
	clientIter->second->request.feed();
	return (clientIter->second);
}

void		Connections::init(ServerContext *ctx, int kqueueFd)
{
	this->ctx = ctx;
	this->kqueueFd = kqueueFd;
}

Client		*Connections::getClient(int fd)
{
	std::map<int, Client *>::iterator kv = this->clients.find(fd);
	if (kv == this->clients.end())
		return (NULL);
	return (kv->second);
}
