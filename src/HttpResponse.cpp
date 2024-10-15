#include "HttpResponse.hpp"
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>

HttpResponse::HttpResponse(int fd) : fd(fd)
{
	keepAlive = 1;
	location = NULL;
	isCgiBool = false;
	resType = NO_TYPE;
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

bool	HttpResponse::isMethodAllowed()
{
	// return (setHttpResError(405, "Method Not Allowed"),
	// 	this->location->globalConfig.isMethodAllowed(this->methode));
	return (true);
}

void			HttpResponse::cgiCooking()
{
	//TODO:implenting the fucking cgi;
}

int				HttpResponse::directoryHandler()
{
	const std::vector<std::string> &indexes = this->location->globalConfig.getIndexes();
	if (this->fullPath[fullPath.size() -1] != '/')
		fullPath += "/";
	std::cout << "full path: " << fullPath << std::endl;
	for (size_t i = 0; i < indexes.size(); i++)
	{
		if (access((this->fullPath + indexes[i]).c_str(), F_OK) != -1)
			return (loadFile(fullPath + indexes[i]));
	}
	return (setHttpResError(404, "Not Found"), 0);
}

int HttpResponse::loadFile(const std::string& pathName)
{
	int fd;
	char buffer[fileReadingBuffer];
	int j = 0;

	fd = open(pathName.c_str(), O_RDONLY);
	if (fd < 0)
		return (setHttpResError(500, "Internal Server Error"), 0);
	while (1)
	{
		int r = read(fd, buffer, fileReadingBuffer);
		if (r < 0)
			return (close(fd), setHttpResError(500, "Internal Server Error"), 0);
		if (r == 0)
			break;
		responseBody.push_back(std::vector<unsigned char >());
		for (int i = 0; i < r;i++)
		{
			responseBody[j].push_back(buffer[i]);
		}
		j++;
	}
	return (close(fd), 1);
}

// std::ifstream file(pathName.c_str());
// unsigned char  tmp;
// if (!file.is_open())
// 	return (setHttpResError(500, "Internal Server Error"), 0);
// while (!(file >> tmp).eof())
// 	responseBody.push_back(tmp);

int		HttpResponse::pathChecking()
{
	this->fullPath = location->globalConfig.getRoot() + this->path;

	struct stat sStat;
	stat(fullPath.c_str(), &sStat);
	if (S_ISDIR(sStat.st_mode))
		return (directoryHandler());
	if (access(fullPath.c_str(), F_OK) != -1)
		return (loadFile(fullPath));
	else
		return (state = ERROR,  setHttpResError(404, "Not Found"), 0);
	return (1);
}

void			HttpResponse::responseCooking()
{
	if (!isPathFounded())
		return ;
	if (isCgi())
		cgiCooking();
	else
	{
		if (!isMethodAllowed() || !pathChecking())
			return ;
	}
}

std::string decimalToHex(int decimal) 
{
    if (decimal == 0) {
        return "0";
    }

    const char hexDigits[] = "0123456789ABCDEF";
    std::string hexResult;
    bool isNegative = false;

    if (decimal < 0) {
        isNegative = true;
        decimal = -decimal;
    }

    while (decimal > 0) {
        hexResult += hexDigits[decimal % 16];
        decimal /= 16;
    }

    if (isNegative) {
        hexResult += '-';
    }

    std::reverse(hexResult.begin(), hexResult.end());
    return hexResult;
}
