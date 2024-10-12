#include "Connections.hpp"
#include <stdexcept>
#include "Client.hpp"


Connections::Connections()
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
	clients[fd] = new Client(fd, server);
}

void		Connections::connecting(int fd)
{
	if (clients.find(fd) == clients.end())
		addConnection(fd);
}

void		Connections::requestHandler(int	fd)
{
	if (this->clients.count(fd) == 0)
		throw std::runtime_error("this should not happend");
	clients[fd]->request.feed();
}
