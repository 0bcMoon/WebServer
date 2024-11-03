#include "Client.hpp"
#include <cstddef>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <cassert>
#include <iostream>
#include <iterator>
#include <string>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Log.hpp"
#include "ServerContext.hpp"

int Client::getFd() const
{
	return (this->fd);
}


Client::Client(int fd, int serverFd, ServerContext *ctx) : fd(fd), serverFd(serverFd), ctx(ctx), request(fd), response(fd, ctx, &request)
{
	state = None;
	this->timerType = NEW_CONNECTION;
}

void Client::respond(size_t data)
{
	response.eventByte = data;
	if (request.state != REQUEST_FINISH && request.state != REQ_ERROR)
		return ;

	response = request;
	std::map<std::string, std::string>::iterator kv = response.headers.find("Connection");

	if ((kv != response.headers.end()
		&& (kv->second.find("close") != std::string::npos
			|| kv->second.find("Close") != std::string::npos))
		|| request.state == REQ_ERROR)
		response.keepAlive = 0;
	if (request.state == REQUEST_FINISH)
		response.responseCooking();
	if (response.state == CGI_EXECUTING)
	{
		response.bodyType = HttpResponse::CGI;
		response.writeCgiResponse();
	}
	if (response.isCgi() && response.state != END_BODY
		&& response.state != ERROR) 
		response.state = CGI_EXECUTING;
	if (response.state == ERROR)
	{
		response.write2client(fd, response.getErrorRes().c_str(), response.getErrorRes().size());
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


Client::TimerType Client::getTimerType() const 
{
	return (this->timerType);
}
