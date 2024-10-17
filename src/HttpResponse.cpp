#include "HttpResponse.hpp"
#include "CgiHandler.hpp"
#include "HttpRequest.hpp"
#include "ServerContext.hpp"
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <system_error>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <stdio.h>
#include <dirent.h>

HttpResponse::HttpResponse(int fd, ServerContext *ctx) : fd(fd) , ctx(ctx)
{
	keepAlive = 1;
	location = NULL;
	isCgiBool = false;
	bodyType = NO_TYPE;
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
	return (location->geCGItPath("." + getExtension(path)).size());
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
	CgiHandler	cgi(*this);
	cgi.execute();
}

int				HttpResponse::autoIndexCooking()
{
	std::vector<std::string> dirContent;
	DIR *dirStream = opendir(fullPath.c_str());
	if (dirStream == NULL)
		return (setHttpResError(500, "Internal Server Error"), 0);
	struct dirent *_dir ;
	while ((_dir = readdir(dirStream)) != NULL) {
		dirContent.push_back(_dir->d_name);
	}
	if (path[path.size() - 1] != '/')
		path += '/';
	autoIndexBody = "<!DOCTYPE html>\n"
					"<html lang=\"en\">\n"
					"	<head>\n"
					"		<title>YOUR DADDY</title>\n"
					"		<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
					"	</head>\n"
					"	<body>\n";
	autoIndexBody += "<h1>liste of files<h1>\n";
	for (size_t i = 0; i < dirContent.size();i++)
	{
		autoIndexBody += "<a href=\""; 
		autoIndexBody += path  + dirContent[i] + "\">" + dirContent[i] + "</a><br>";
	}
	autoIndexBody += "	</body>\n"
					"</html>\n";
	// if (path[path.size() - 1] != '/')
	// 	path -= '/';
	return (closedir(dirStream), 1);
}

int				HttpResponse::directoryHandler()
{
	const std::vector<std::string> &indexes = this->location->globalConfig.getIndexes();
	if (this->fullPath[fullPath.size() -1] != '/')
		fullPath.push_back('/');
	for (size_t i = 0; i < indexes.size(); i++)
	{
		if (access((this->fullPath + indexes[i]).c_str(), F_OK) != -1)
			return (bodyType = LOAD_FILE, (fullPath += indexes[i]), loadFile(fullPath));
	}
	if (location->globalConfig.getAutoIndex())
		return (bodyType = AUTO_INDEX, autoIndexCooking());
	return (setHttpResError(404, "Not Found"), 0);
}

int HttpResponse::loadFile(const std::string& pathName)
{
	int _fd;
	char buffer[fileReadingBuffer];
	int j = 0;

	_fd = open(pathName.c_str(), O_RDONLY);
	if (_fd < 0)
		return (setHttpResError(500, "Internal Server Error"), 0);
	while (1)
	{
		int r = read(_fd, buffer, fileReadingBuffer);
		if (r < 0)
			return (close(_fd), setHttpResError(500, "Internal Server Error"), 0);
		if (r == 0)
			break;
		responseBody.push_back(std::vector<unsigned char >(r));
		for (int i = 0; i < r;i++) {
			responseBody[j][i] = buffer[i];
		}
		j++;
	}
	return (close(_fd), 1);
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
		return (bodyType = LOAD_FILE, loadFile(fullPath));
	else
		return (state = ERROR,  setHttpResError(404, "Not Found"), 0);
	return (1);
}

void			HttpResponse::writeResponse()
{
	write(this->fd, getStatusLine().c_str(), getStatusLine().size());
	write(this->fd, getConnectionState().c_str(), getConnectionState().size());
	write(this->fd, getContentType().c_str(), getContentType().size());
	write(this->fd, getContentLenght(bodyType).c_str(), getContentLenght(bodyType).size());
	write(fd, getDate().c_str(), getDate().size());
	write(fd, "Server: YOUR DADDY\r\n", strlen("Server: YOUR DADDY\r\n"));
	for (map_it it = resHeaders.begin(); it != resHeaders.end(); it++)
	{
		write(this->fd, it->first.c_str(), it->first.size());
		write(this->fd, ": ", 2);
		write(this->fd, it->second.c_str(), it->second.size());
		write(fd, "\r\n", 2);
	}
	write(this->fd, "\r\n", 2);
	sendBody(-1, bodyType);
}

std::string						HttpResponse::getStatusLine()
{
	std::ostringstream oss;
	oss << status.code;

	return ("HTTP/1.1 " + oss.str() + " " + status.description + "\r\n");
}

std::string						HttpResponse::getConnectionState()
{
	if (keepAlive)
		return ("Connection: Keep-Alive\r\n");
	return ("Connection: Close\r\n");
}

std::string						HttpResponse::getExtension(std::string str)
{
	std::string ext;
	int i = str.size() - 1;
	while (i >= 0)
	{
		if (str[i] == '.')
			return (str.substr(i + 1));
		i--;
	}
	return ("");
}

std::string						HttpResponse::getContentType()
{
	if (bodyType == AUTO_INDEX)
		return ("Content-Type: text/html\r\n");
	return ("Content-Type: " + ctx->getType(getExtension(fullPath)) + "\r\n");
}

std::string						HttpResponse::getDate()
{//TODO:date;
	return ("");
}

int					HttpResponse::sendBody(int _fd, enum responseBodyType type)
{
	if (type == LOAD_FILE)
	{
		for (size_t i = 0; i < responseBody.size(); i++)
		{
			for (size_t j = 0; j < responseBody[i].size(); j++)
			{
				write(this->fd, &responseBody[i][j], 1);
			}
		}
	}
	else if (type == AUTO_INDEX) {
		write(this->fd, autoIndexBody.c_str(), autoIndexBody.size());
	}
	return (1);
}

std::string						HttpResponse::getContentLenght(enum responseBodyType type)
{
	if (type == LOAD_FILE)
	{
		std::ostringstream oss;

		size_t size = 0;
		for (size_t i = 0; i < responseBody.size(); i++)
		{
			size += responseBody[i].size();
		}
		oss << size;
		return ("Content-Length: " + oss.str() + "\r\n");
	}
	if (type == AUTO_INDEX)
	{
		std::ostringstream oss;
		oss << autoIndexBody.size();
		return ("Content-Length: " + oss.str() + "\r\n");
	}
	return ("");
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
		writeResponse();
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
