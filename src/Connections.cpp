#include "Connections.hpp"
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

void		Connections::connecting(int fd)
{
	if (clients.find(fd) == clients.end())
		addConnection(fd);
}

void		Connections::requestHandler(int	fd)
{
	connecting(fd);
	clients[fd]->request.feed();
	if (clients[fd]->request.state == REQUEST_FINISH)
		clients[fd]->respond();
}
