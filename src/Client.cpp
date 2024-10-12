#include "Client.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <sys/fcntl.h>
#include <unistd.h>

int Client::getFd() const
{
	return (fd);
}

Client::Client() : response(-1)
{
	fd = -1;
	state = REQUEST;
}

Client::Client(int	fd) : fd(fd), request(fd), response(fd)
{
	state = REQUEST;
}


Client::Client(int	fd, int serverFd) : fd(fd), serverFd(serverFd), request(fd), response(fd)
{
	state = REQUEST;
}
int tmpResponse(int fd)
{
	std::string buffer;
	char tmp[10000];

	int resFd = open("request.req", O_RDWR, 0777);
	read(resFd, tmp, 10000);
	buffer = tmp;
	write (fd, buffer.c_str(), buffer.size());
	return (1);
}

void		Client::respond()
{
	// state = RESPONSE;
	response = request;
	request.clear();
	tmpResponse(fd);
}
