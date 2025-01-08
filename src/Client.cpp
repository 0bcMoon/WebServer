#include "Client.hpp"
#include <cmath>
#include <csignal>
#include <cstddef>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <iostream>
#include <iterator>
#include <string>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ServerContext.hpp"

int Client::getFd() const
{
	return (this->fd);
}


Client::Client(int fd, int serverFd, ServerContext *ctx) : fd(fd), serverFd(serverFd), ctx(ctx), request(fd), response(fd, ctx, &request)
{
	this->writeEventState = 0;
	this->cgi_pid  = -1;
	state = None;
	this->timerType = NEW_CONNECTION;
}

void Client::respond(size_t data, size_t index)
{
	response.eventByte = data;
	if (request.data[index]->state != REQUEST_FINISH &&  request.data[index]->state != REQ_ERROR)
		return ;
	response = request;
	if (request.data[index]->state == REQUEST_FINISH)
		response.responseCooking();
	if (response.state == START_CGI_RESPONSE)
	{
		response.bodyType = HttpResponse::CGI;
		response.writeCgiResponse();
		response.logResponse();
		return ;
	}
	if (response.state != ERROR && response.isCgi() && response.state != UPLOAD_FILES && response.state != END_BODY) 
		response.state = CGI_EXECUTING;

}

void	Client::handleResponseError()
{
	std::string ErrorRes = response.getErrorRes();
	response.write2client(fd, ErrorRes.c_str(), ErrorRes.size());
	if (response.state != WRITE_BODY)
		response.state = END_BODY;
}

const std::string &Client::getHost() const
{
	return (this->request.getHost()); 
}

const std::string &Client::getPath() const
{
	if (this->request.state == BODY)
		return (this->request.data.back()->path);
	return (this->request.data.front()->path);
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
	if (this->cgi_pid > 0)
		::kill(this->cgi_pid, SIGKILL);
}
