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
	struct kevent ev_set[2];

	EV_SET(&ev_set[0], fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	EV_SET(&ev_set[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
	kevent(this->kqueueFd, ev_set, 2, NULL, 0, NULL); 
	close(fd);
	delete clients[fd];
	clients.erase(fd);
}

void	Connections::addConnection(int	fd)
{
	clients[fd] = new Client(fd);
}

void	Connections::addConnection(int	fd, int server)
{
	this->clients[fd] = new Client(fd, server, ctx);
}

void		Connections::connecting(int fd)
{
	if (clients.find(fd) == clients.end())
		addConnection(fd);
}

// TODO: return client object : search once
Client		*Connections::requestHandler(int	fd)
{
	ClientsIter clientIter = this->clients.find(fd);
	if ( clientIter == this->clients.end()) // TODO : fix
		return (NULL);
	clientIter->second->request.feed();
	return (clientIter->second);
}

void		Connections::init(ServerContext *ctx, int kqueueFd)
{
	this->ctx = ctx;
	this->kqueueFd = kqueueFd;
}
