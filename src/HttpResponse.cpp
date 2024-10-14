#include "HttpResponse.hpp"
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <unistd.h>

HttpResponse::HttpResponse(int fd) : fd(fd)
{
	keepAlive = 1;
	location = NULL;
	isCgiBool = false;
	errorRes.headers =  "Content-Type: text/html; charset=UTF-8\r\n"
						"Server: XXXXXXXX\r\n";//TODO:name the server;
	errorRes.contentLen = "Content-Length: 0\r\n";
	errorRes.bodyHead = "\r\n"
						"<!DOCTYPE html>\n"
						"<html>\n"
						"<head><title>";
	errorRes.body = "</title></head>\n"
					"<body>\n"
					"<h1>";
	errorRes.bodyfoot = "</h1>\n"
						"</body>\n"
						"</html>";
	errorRes.connection = "Connection: Keep-Alive\r\n";
}

std::string		HttpResponse::getContentLenght()
{
	std::ostringstream oss;

	oss << (errorRes.bodyHead.size() + errorRes.title.size()+ errorRes.body.size()
			+ errorRes.htmlErrorId.size() + errorRes.bodyfoot.size() - 2);
	return ("Content-Length: " + oss.str() + "\r\n");
}

std::string		HttpResponse::getErrorRes()
{
	std::ostringstream oss;
	oss << status.code;
	errorRes.statusLine = "HTTP/1.1 " + oss.str() + " " + status.description + "\r\n";
	errorRes.title = oss.str() + " " + status.description;
	errorRes.htmlErrorId = oss.str() + " " + status.description;
	if (!keepAlive)
		errorRes.connection = "Connection: close\r\n";
	errorRes.contentLen = getContentLenght();
	return (errorRes.statusLine + errorRes.headers + errorRes.connection
			+ errorRes.contentLen + errorRes.bodyHead + errorRes.title + errorRes.body
			+ errorRes.htmlErrorId + errorRes.bodyfoot);
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

void	HttpResponse::setHttpResError(int code, std::string str)
{
	state = ERROR;
	status.code = code;
	status.description = str;
}

bool	HttpResponse::isPathFounded()
{
	//TODO:----------;
	if (location == NULL)
		return (setHttpResError(404, "Not Found"), false);
	return (true);
}

bool	HttpResponse::isMethodAllowed() const
{
	return (this->location->globalConfig.isMethodAllowed(this->methode)); // true of false TODO:
}

void			HttpResponse::cgiCooking()
{
	//TODO:implenting the fucking cgi;
}

int				HttpResponse::directoryHandler()
{
	// const std::vector<std::string> &indexes = this->location->globalConfig.getIndexes();
	// 
	// for (size_t i = 0; i < indexes.size(); i++)
	// {
	// 	if (access((fullPath + ), F_OK) != -1)
	// 	{
	// 		
	// 	}
	// }
}

int		HttpResponse::pathChecking()
{
	// struct stat sStat;
	// if (location->globalConfig.getRoot()[location->globalConfig.getRoot().size() -1] == '/')
	// 	fullPath = location->globalConfig.getRoot() + location->getPath();
	// else
	// 	fullPath = location->globalConfig.getRoot() + "/" + location->getPath();
	// stat(fullPath.c_str(), &sStat);
	// if (S_ISDIR(sStat.st_mode))
	// 	return (directoryHandler());
	// if (access(fullPath.c_str(), F_OK) != -1)
	// 	fullPatiiiiiiiih;
	// else
	// 	return (state = ERROR,  setHttpResError(404, "Not Found"), 0);
	// return (1);
}

void			HttpResponse::responseCooking()
{
	// std::set<std::string> set = this->location->globalConfig.ge
	// root/path
	if (!isPathFounded())
		return ;
	if (isCgi())
		cgiCooking();
	else
	{
		if (!isMethodAllowed())
			return ;
		// pathChecking();
	}
}
