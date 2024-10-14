#include "Connections.hpp"
#include <iostream>
#include <stdexcept>
#include "Client.hpp"


Connections::Connections(ServerContext *ctx) : ctx(ctx)
{
}

Connections::~Connections()
{
	for (clients_it it = clients.begin(); it != clients.end(); ++it) {
		delete it->second;
	}
}

void	Connections::closeConnection(int	fd)
{
	delete clients[fd];
	clients.erase(fd);
}

void	Connections::addConnection(int	fd)
{
	clients[fd] = new Client(fd);
}

void	Connections::addConnection(int	fd, int server)
{
	this->clients[fd] = new Client(fd, server);
}

void		Connections::connecting(int fd)
{
	if (clients.find(fd) == clients.end())
		addConnection(fd);
}

void		Connections::requestHandler(int	fd)
{
	if (this->clients.find(fd) == this->clients.end()) // TODO : fix
		return ;
	clients[fd]->request.feed();
}
