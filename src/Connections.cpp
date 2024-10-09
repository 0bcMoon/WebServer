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
	clients[fd] = NULL;
}

void	Connections::addConnection(int	fd)
{
	clients[fd] = new Client(fd);
}
