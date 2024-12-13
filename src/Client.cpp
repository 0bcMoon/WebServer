#include "Client.hpp"
#include <cstddef>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <unistd.h>
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

void Client::respond(size_t data, size_t index)
{
	response.eventByte = data;
	if (request.data[index]->state != REQUEST_FINISH &&  request.data[index]->state != REQ_ERROR)
		return ;

			
	response = request;
	std::cout << "ERROR: "<< response.getStatusCode() << std::endl;			
	std::cout << response.path << std::endl;
	std::cout << response.strMethod << std::endl;
	// std::map<std::string, std::string>::iterator kv = response.headers.find("Connection");

	// if ((kv != response.headers.end()
	// 	&& (kv->second.find("close") != std::string::npos
	// 		|| kv->second.find("Close") != std::string::npos))
	// 	|| request.state == REQ_ERROR)
	// 	response.keepAlive = 0;
	if (request.data[index]->state == REQUEST_FINISH)
		response.responseCooking();
	if (response.state == START_CGI_RESPONSE)
	{
		response.bodyType = HttpResponse::CGI;
		response.writeCgiResponse();
	}
	if (response.state != ERROR && response.isCgi() && response.state != UPLOAD_FILES && response.state != END_BODY) 
		response.state = CGI_EXECUTING;
	// if (response.state == ERROR)
	// {
	// 	response.write2client(fd, response.getErrorRes().c_str(), response.getErrorRes().size());
	// }
}

void	Client::handleResponseError()
{
	std::string ErrorRes = response.getErrorRes();
	response.write2client(fd, ErrorRes.c_str(), ErrorRes.size());
}

const std::string &Client::getHost() const
{
	return (this->request.getHost()); 
}

const std::string &Client::getPath() const
{
	return (this->request.data[0]->path);
}

int Client::getServerFd() const
{
	return (this->serverFd);
}


Client::TimerType Client::getTimerType() const 
{
	return (this->timerType);
}

Client::~Client()
{
	// this->proc.die(); // make process clean it own shit \n
	// this->proc.clean();
}
