#include "HttpResponse.hpp"
#include "CgiHandler.hpp"
#include "HttpRequest.hpp"
#include "ServerContext.hpp"
#include <cctype>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <ios>
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

std::vector<unsigned char>		HttpResponse::getBody() const
{
	return (body);
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
	cgi.execute(location->geCGItPath("." + getExtension(path)));
	if (state == ERROR)
		return ;
	bodyType = CGI;
	writeCgiResponse();
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

int HttpResponse::loadFile(int _fd)
{
	char buffer[fileReadingBuffer];
	int j = 0;

	while (1)
	{
		int r = read(_fd, buffer, fileReadingBuffer);
		if (r < 0)
			return (setHttpResError(500, "Internal Server Error"), 0);
		if (r == 0)
			break;
		responseBody.push_back(std::vector<unsigned char >(r));
		for (int i = 0; i < r;i++) {
			responseBody[j][i] = buffer[i];
		}
		j++;
	}
	return (1);
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

static int isAlpha(char c)
{
	return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));

}

static int isValidHeaderChar(char c)
{
	return (isAlpha(c) || (c >='1' && c <= '9') || c == '-' || c == ':');
}

static int isStatusLine(std::vector<unsigned char> vec)
{
	return (vec[0] == 'H' && vec[1] == 'T' && vec[2] == 'T' && vec[3] == 'P'
			&& vec[4] == '/' && vec[5] == '1' && vec[6] == '.' && vec[7] == '1');
}

int			HttpResponse::parseCgiHaders(std::string str)
{
	size_t pos = str.find(':');
	std::string tmpHeaderName;
	std::string tmpHeaderVal;

	if (pos == std::string::npos || pos == 0 || str[str.size() - 1] != '\n')
		return (setHttpResError(502, "Bad Gateway"), 0);
	tmpHeaderName = str.substr(0, pos);
	for (size_t i = 0; i < tmpHeaderName.size(); i++)
	{
		if (!isValidHeaderChar(tmpHeaderName[i]))
			return (setHttpResError(502, "Bad Gateway"), 0);
	}
	tmpHeaderVal = str.substr(pos + 1);
	if (tmpHeaderVal.size() < 3 || tmpHeaderVal[0] != ' ')
		return (setHttpResError(502, "Bad Gateway"), 0);
	resHeaders[tmpHeaderName] = tmpHeaderVal.substr(0, tmpHeaderVal.size() - 1);
	return (1);
}

static int	isLineCrlf(std::vector<unsigned char > vec)
{
	size_t _i = 0;
	while (_i < vec.size() - 1)
	{
		if (vec[_i] != '\r')
			return (0);
		_i++;
	}
	return (1);
}

static std::string vec2str(std::vector<unsigned char> vec)
{
	std::string str;

	for (size_t i = 0; i < vec.size();i++)
	{
		str.push_back(vec[i]);
	}
	return (str);
}

static int parseCgiStatusLine(std::string line)
{
	for (size_t i = 8; i < line.size(); i++)
	{
		if (i == 8 && line[i] != ' ')
			break;
		if (i == 9 && (line[i] < '1' || line[i] > '5'))
			break ;
		if ((i == 10 || i == 11) && !isdigit(line[i]))
			break;
		if (i == 12 && line[i] != ' ')
			break;
		// if (i > 12 && line[i] != '\n' && line[i] != '\r')
		// 	break;
		if (line[i] == '\n' && i > 13)
			return (1);
	}
	return (0);
}

void			HttpResponse::parseCgiOutput()
{
	std::string tmpHeaderName;
	std::string tmpHeaderVal;
	size_t			lineIndex = 0;

	cgiRes.state = HEADERS;
	if (responseBody.size() == 0 || responseBody[0].size() == 0)
		return;
	for (size_t i = 0; i < responseBody.size(); i++)
	{
		for (size_t j = 0; j < responseBody[i].size(); j++)
		{
			if (lineIndex == cgiRes.lines.size())
				cgiRes.lines.push_back(std::vector<unsigned char>());
			cgiRes.lines[lineIndex].push_back(responseBody[i][j]);
			if (responseBody[i][j] == '\n')
				lineIndex++;
		}
	}
	/*************************************************************/
	std::cout << "/*************************************************************/\n";
	for (size_t i = 0; i < cgiRes.lines.size(); i++)
	{
		for (size_t j = 0; j < cgiRes.lines[i].size(); j++)
		{
			std::cout << cgiRes.lines[i][j];
		}
		// std::cout << std::endl;
	}
	/*************************************************************/
	for (size_t i = 0; i < cgiRes.lines.size();i++)
	{
		if (isLineCrlf(cgiRes.lines[i]))
		{
			cgiRes.bodyStartIndex = i + 1;
			// for (map_it it = resHeaders.begin(); it != resHeaders.end(); it++)
			// {
			// 	write(1, it->first.c_str(), it->first.size());
			// 	write(1, ": ", 2);
			// 	write(1, it->second.c_str(), it->second.size());
			// 	write(1, "\r\n", 2);
			// }
			return ;
		}
		if (i == 0 && cgiRes.lines[i].size() > 8 && isStatusLine(cgiRes.lines[i]))
		{
			if (!parseCgiStatusLine(vec2str(cgiRes.lines[i])))
			{
				setHttpResError(502, "Bad Gateway");
				return ;
			}
			cgiRes.cgiStatusLine = vec2str(cgiRes.lines[i]);
		}
		else
		{
			if (!parseCgiHaders(vec2str(cgiRes.lines[i])))
				return ;
		}
	}
	setHttpResError(502, "Bad Gateway");
	// while (i < body.size() && (body[i] == '\r' || body[i] == '\n'))
	// {
	// 	if (body[i] == '\n')
	// 	{
	// 		cgiRes.bodyStartIndex = i + 1;
	// 		return ;
	// 	}
	// 	i++;
	// }
	// i = 0;
	// if (body.size() > 8 && isStatusLine(body))
	// {
	// 	while (i < body.size() && body[i] != '\n')
	// 	{
	// 		cgiRes.cgiStatusLine.push_back(body[i]);
	// 		i++;
	// 	}
	// 	cgiRes.cgiStatusLine.push_back(body[i]);
	// }
	// parseCgiHaders(i);
	// for (size_t i = 0; i < body.size(); i++)
	// {
	// 	if ()

	// 	if ((tmpHeaderName.size() == 0 && !isAlpha(body[i]))
	// 		|| !isValidHeaderChar(body[i]))
	// 	{
	// 		setHttpResError(502, "Bad Gateway");
	// 		return ;
	// 	}
	// 	if (body[i] == ':' && tmpHeaderName.size() == 0)
	// 	{
	// 		setHttpResError(502, "Bad Gateway");
	// 		return ;
	// 	}
	// 	if (body[i] == ':')
	// 	{
	// 		isName = 0;
	// 		if (body[i] == ' ')
	// 			i++;
	// 	}
	// 	if (isName)
	// 		tmpHeaderName.push_back(body[i]);
	// }
}

std::string			HttpResponse::getCgiContentLenght()
{
	size_t len = 0;
	for (size_t i = cgiRes.bodyStartIndex; i < cgiRes.lines.size();i++)
	{
		len += cgiRes.lines[i].size();
	}
	std::ostringstream oss;
	oss << len;
	std::cout << "--->" << oss.str() << std::endl;
	return (oss.str());
}

void			HttpResponse::writeCgiResponse()
{
		// return ("Connection: keep-alive\r\n");
	if (keepAlive)
		resHeaders["Connection"] = "keep-alive";
	else
		resHeaders["Connection"] = "Close";
	resHeaders["Content-Type"] = "text/plain";
	parseCgiOutput();
	if (state == ERROR)
		return ;
	if (cgiRes.cgiStatusLine.size() == 0)
		write(this->fd, getStatusLine().c_str(), getStatusLine().size());
	else 
		write(this->fd, cgiRes.cgiStatusLine.c_str(), cgiRes.cgiStatusLine.size());
	if (resHeaders.find("Transfer-Encoding") == resHeaders.end() || resHeaders["Transfer-Encoding"] != "Chunked")
		resHeaders["Content-Length"] = getCgiContentLenght();
	for (map_it it = resHeaders.begin(); it != resHeaders.end(); it++)
	{
		write(this->fd, it->first.c_str(), it->first.size());
		write(this->fd, ": ", 2);
		write(this->fd, it->second.c_str(), it->second.size());
		write(fd, "\r\n", 2);
	}
	write(this->fd, "\r\n", 2);
	sendBody(-1, CGI);
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
		return ("Connection: keep-alive\r\n");
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
	return ("Content-Type: " + ctx->getType(getExtension(fullPath)) + "\r\n"
			+ "Content-Type: " + ctx->getType(getExtension(fullPath)) + "\r\n"
			+ "Content-Type: text/html\r\n");
}

std::string						HttpResponse::getDate()
{//TODO:date;
	return ("");
}

int					HttpResponse::sendBody(int _fd, enum responseBodyType type)
{
	size_t begin = 0; 
	if (type == CGI)
	{
		// size_t len = 0;
		std::istringstream oss(getCgiContentLenght());
		oss >> begin;
		// std::istringstream oss1(getContentLenght(bodyType));
		// oss1 >> len;
		size_t size = 0;
		for (size_t i = 0; i < responseBody.size(); i++)
		{
			size += responseBody[i].size();
		}
		begin =  size - begin;
		std::cout << "-----------> " << begin << std::endl;
	}
	if (type == LOAD_FILE || type == CGI)
	{
		size_t count = 0;
		for (size_t i = 0; i < responseBody.size(); i++)
		{
			for (size_t j = 0; j < responseBody[i].size(); j++)
			{
				if (count >= begin)
					write(this->fd, &responseBody[i][j], 1);
				count++;
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
	if (type == LOAD_FILE || type == CGI)
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

static int isHex(char c)
{
	std::string B = "0123456789ABCDEF";
	std::string b = "0123456789abcdef";

	if (b.find(c) == std::string::npos && B.find(c) == std::string::npos)
		return (0);
	return (1);
}

void			HttpResponse::decodingUrl()
{
	std::string decodedUrl;
	std::stringstream ss;

	for (size_t i = 0; i < path.size(); i++)
	{
		if (i < path.size() - 2 && path[i] == '%' 
			&& isHex(path[i + 1]) && isHex(path[i + 2]))
		{
			int tmp;
			ss << path[i + 1] << path[i + 2];
			ss >> std::hex >> tmp;
			ss.clear();
			decodedUrl.push_back(tmp);
			i += 2;
		}
		else
			decodedUrl.push_back(path[i]);
	}
	path = decodedUrl;
	std::cout << "Decoded-url: " << path << std::endl;
}

void			HttpResponse::splitingQuery()
{
	if (path.find('?') == std::string::npos)
		return;
	size_t pos = path.find('?');
	queryStr = path.substr(pos + 1);
	path = path.substr(0, pos);
}

void			HttpResponse::responseCooking()
{
	decodingUrl();
	splitingQuery();
	std::cout << "queryStr: " << queryStr << std::endl;
	std::cout << "pure path: " << path << std::endl;
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
