#include "Client.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <iostream>
#include <string>
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

int tmpResponse(int fd, const std::string& fileName)
{
	std::string buffer;
	char tmp[10000];

	int resFd = open(fileName.c_str(), O_RDWR, 0777);
	if (resFd < 0)
		std::cout << "FUCK" << std::endl;
	read(resFd, tmp, 10000);
	buffer = tmp;
	write (fd, buffer.c_str(), buffer.size());
	return (1);
}

void		Client::respond()
{
	// state = RESPONSE;
	if (request.state == REQUEST_FINISH)
	{
		response = request;
		request.clear();
		tmpResponse(fd, "request.req");
	}
}
