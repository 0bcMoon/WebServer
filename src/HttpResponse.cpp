#include "HttpResponse.hpp"
#include <iostream>
#include <ostream>
#include <string>

HttpResponse::HttpResponse(int fd) : fd(fd)
{
	location = NULL;
	isCgiBool = false;
}

HttpResponse HttpResponse::operator=(const HttpRequest& req)
{
	path = req.getPath();
	headers = req.getHeaders();
	body = req.getBody();
	status.code = req.getStatus().code;
	status.description = req.getStatus().description;
	strMethod = req.getStrMethode();
	if (req.state == REQ_ERROR)
		state = ERROR;
	else
		state = START;
	return (*this);
}

int		HttpResponse::getStatusCode() const
{
	return (status.code);
}

std::string	HttpResponse::getStatusDescr() const
{
	return (status.description);
}

bool			HttpResponse::isCgi()
{
	//TODO:-------------;
	return (isCgiBool);
}

bool	HttpResponse::isPathFounded() const
{
	//TODO:----------;
	if (location == NULL)
		return (HttpRequest::setHttpError(404, "NOT FOUND"), false);

	return (true);
}

bool	HttpResponse::isMethodAllowed() const
{
	//TODO:----------;
	return (true);
}

void			HttpResponse::cgiCooking()
{
	//TODO:implenting the fucking cgi;
}

void			HttpResponse::responseCooking()
{
	if (!isPathFounded())
		return ;
	if (isCgi())
		cgiCooking();
	else
	{
		if (!isMethodAllowed())
			return ;
		std::cout << "IIIIIIIIIIIIIIIIIIIIIIIIII" << std::endl;
	}
}
