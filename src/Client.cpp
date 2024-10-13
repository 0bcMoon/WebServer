#include "Client.hpp"
#include <sys/fcntl.h>
#include <unistd.h>
#include <cassert>
#include <iostream>
#include <iterator>
#include <string>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

int Client::getFd() const
{
	return (fd);
}

Client::Client() : response(-1)
{
	fd = -1;
	state = REQUEST;
}

Client::Client(int fd) : fd(fd), request(fd), response(fd)
{
	state = REQUEST;
}

Client::Client(int fd, int serverFd) : fd(fd), serverFd(serverFd), request(fd), response(fd)
{
	state = REQUEST;
}
// int tmpResponse(int fd, std::string fileName)
// {
// 	std::string buffer;
// 	char tmp[10000];

// 	int resFd = open(fileName.c_str(), O_RDWR, 0777);
// 	if (resFd < 0)
// 		std::cout << "FUCK" << std::endl;
// 	read(resFd, tmp, 10000);
// 	buffer = tmp;
// 	write (fd, buffer.c_str(), buffer.size());
// 	return (1);
// }

void Client::respond()
{
	if (request.state == REQUEST_FINISH)
	{
		response = request;
		request.clear();
		response.responseCooking();
	}
	else if (response.state == ERROR)
	{
		// TODO: handling ERROR
	}
}

const std::string &Client::getHost() const
{
	return (this->request.getHost()); 
}

const std::string &Client::getPath() const
{
	return (this->request.getPath());
}

int Client::getServerFd() const
{
	return (this->serverFd);
}
