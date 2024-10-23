#include "Client.hpp"
#include <sys/fcntl.h>
#include <unistd.h>
#include <cassert>
#include <iostream>
#include <iterator>
#include <string>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ServerContext.hpp"

int Client::getFd() const
{
	return (fd);
}

Client::Client() : response(-1, NULL, NULL)
{
	fd = -1;
	state = REQUEST;
}

Client::Client(int fd) : fd(fd), request(fd), response(fd, NULL, NULL)
{
	state = REQUEST;
}

Client::Client(int fd, int serverFd, ServerContext *ctx) : fd(fd), serverFd(serverFd), request(fd), response(fd, ctx, &request)
{
	state = REQUEST;
}

void Client::respond()
{
	if (request.state != REQUEST_FINISH && request.state != REQ_ERROR)
		return ;
	response = request;
	if ((response.headers.find("Connection") != response.headers.end() //TODO: create a function that return bool from search from file //TODO: handle connection with Cgi
		&& (response.headers["Connection"].find("close") != std::string::npos
			|| response.headers["Connection"].find("Close") != std::string::npos))
		|| request.state == REQ_ERROR)
		response.keepAlive = 0;
	if (request.state == REQUEST_FINISH)
		response.responseCooking();
	if (response.state == ERROR)
	{
		write(fd, response.getErrorRes().c_str(), response.getErrorRes().size());
		write(1, response.getErrorRes().c_str(), response.getErrorRes().size());
	}

	request.clear();
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

