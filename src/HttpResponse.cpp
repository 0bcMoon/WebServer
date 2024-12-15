#include "HttpResponse.hpp"
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/dirent.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <unistd.h>
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <ios>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "CgiHandler.hpp"
#include "HttpRequest.hpp"
#include "ServerContext.hpp"

HttpResponse::HttpResponse(int fd, ServerContext *ctx, HttpRequest *request) : ctx(ctx), request(request), fd(fd)
{
	keepAlive = 1;
	location = NULL;
	isCgiBool = false;
	bodyType = NO_TYPE;
	state = START;
	responseFd = -1;
	isErrDef = 1;
	// i = 0;
	// j = 0;
	errorRes.headers =
		"Content-Type: text/html; charset=UTF-8\r\n"
		"Server: XXXXXXXX\r\n"; // TODO:name the server;
	errorRes.contentLen = "Content-Length: 0\r\n";
	errorRes.bodyHead =
		"\r\n"
		"<!DOCTYPE html>\n"
		"<html>\n"
		"<head><title>";
	errorRes.body =
		"</title></head>\n"
		"<body>\n"
		"<h1>";
	errorRes.bodyfoot =
		"</h1>\n"
		"</body>\n"
		"</html>";
	errorRes.connection = "Connection: Keep-Alive\r\n";
}

void HttpResponse::clear()
{
	std::cout << "CLEAR\n";
	if (responseFd >= 0)
		close(responseFd);
	responseFd = -1;
	isErrDef = 1;

	errorRes.statusLine.clear();
	errorRes.headers.clear();
	errorRes.connection.clear();
	errorRes.contentLen.clear();
	errorRes.bodyHead.clear();
	errorRes.title.clear();
	errorRes.body.clear();
	errorRes.htmlErrorId.clear();
	errorRes.bodyfoot.clear();
	errorPage.clear();
	cgiRes.state = HEADERS;
	cgiRes.bodyStartIndex = 0;
	cgiRes.cgiStatusLine.clear();
	cgiRes.lines.resize(0);
	CGIOutput.resize(0);
	methode = NONE;
	body.clear();
	status.code = 200;
	status.description = "OK";
	isCgiBool = 0;
	fullPath.clear();
	autoIndexBody.clear();
	writeByte = 0;
	eventByte = 0;
	fileSize = 0;
	sendSize = 0;
	queryStr.clear();
	strMethod.clear();
	// responseBody.clear();
	state = START;
	path.clear();
	headers.clear();
	resHeaders.clear();
	keepAlive = 1;
	location = NULL;
	isCgiBool = false;
	bodyType = NO_TYPE;
	close(responseFd);
	responseFd = -1;
	errorRes.headers =
		"Content-Type: text/html; charset=UTF-8\r\n"
		"Server: XXXXXXXX\r\n"; // TODO:name the server;
	errorRes.contentLen = "Content-Length: 0\r\n";
	errorRes.bodyHead =
		"\r\n"
		"<!DOCTYPE html>\n"
		"<html>\n"
		"<head><title>";
	errorRes.body =
		"</title></head>\n"
		"<body>\n"
		"<h1>";
	errorRes.bodyfoot =
		"</h1>\n"
		"</body>\n"
		"</html>";
	errorRes.connection = "Connection: Keep-Alive\r\n";
}

HttpResponse::~HttpResponse()
{
	// std::cout << "Destructor has been called\n";
	if (responseFd >= 0)
		close(responseFd);
}

std::vector<char> HttpResponse::getBody() const
{
	return (this->body);
}

std::string HttpResponse::getContentLenght()
{
	std::ostringstream oss;
	struct stat s;
	if (errorPage.size() && !stat(errorPage.c_str(), &s))
	{
		isErrDef = 0;
		oss << s.st_size;
		fileSize = s.st_size;
	}
	else
	{
		oss
			<< (errorRes.bodyHead.size() + errorRes.title.size() + errorRes.body.size() + errorRes.htmlErrorId.size()
				+ errorRes.bodyfoot.size() - 2);
	}
	return ("Content-Length: " + oss.str() + "\r\n");
}

void HttpResponse::write2client(int fd, const char *str, size_t size)
{
	if (write(fd, str, size) < 0)
	{
		state = WRITE_ERROR;
		throw IOException();
	}
	writeByte += size;
}

HttpResponse::IOException::~IOException() throw() {}
HttpResponse::IOException::IOException() throw()
{
	this->msg = "IOException: " + std::string(strerror(errno));
}

HttpResponse::IOException::IOException(const std::string &msg) throw()
{
	this->msg = msg;
}
const char *HttpResponse::IOException::what() const throw()
{
	return (this->msg.data());
}

std::string HttpResponse::getErrorRes()
{
	std::ostringstream oss;
	oss << status.code;
	std::string errorCode = oss.str();
	if (location)
		this->errorPage = location->globalConfig.getErrorPage(errorCode);
	errorRes.statusLine = "HTTP/1.1 " + oss.str() + " " + status.description + "\r\n";
	errorRes.title = oss.str() + " " + status.description;
	errorRes.htmlErrorId = oss.str() + " " + status.description;
	if (!keepAlive)
		errorRes.connection = "Connection: close\r\n";
	errorRes.contentLen = getContentLenght();
	if (this->responseFd >= 0)
		close(responseFd);
	this->responseFd = open(errorPage.c_str(), O_RDONLY);
	if (!isErrDef && responseFd >= 0)
		return (
			bodyType = LOAD_FILE,
			state = WRITE_BODY,
			errorRes.statusLine + errorRes.headers + errorRes.connection + errorRes.contentLen + "\r\n");
	return (
		errorRes.statusLine + errorRes.headers + errorRes.connection + errorRes.contentLen + errorRes.bodyHead
		+ errorRes.title + errorRes.body + errorRes.htmlErrorId + errorRes.bodyfoot);
}

HttpResponse &HttpResponse::operator=(const HttpRequest &req)
{
	this->isCgiBool = req.data.front()->bodyHandler.isCgi;
	path = req.data[0]->path;
	headers = req.data[0]->headers;
	// body = req.data[0]->body;
	status.code = req.data[0]->error.code;
	status.description = req.data[0]->error.description;
	strMethod = req.data[0]->strMethode;
	if (req.data[0]->state == REQ_ERROR)
		state = ERROR;
	// path = req.getPath();
	// headers = req.getHeaders();
	// body = req.getBody();
	// status.code = req.getStatus().code;
	// status.description = req.getStatus().description;
	// strMethod = req.getStrMethode();
	// if (req.state == REQ_ERROR)
	// 	state = ERROR;
	// else
	// 	state = ;
	return (*this);
}

int HttpResponse::getStatusCode() const
{
	return (status.code);
}

std::string HttpResponse::getStatusDescr() const
{
	return (status.description);
}

bool HttpResponse::isCgi()
{
	return (this->isCgiBool);
}

void HttpResponse::setHttpResError(int code, const std::string &str)
{
	std::cerr << "http Error has been set: " << code << "\n";
	state = ERROR;
	status.code = code;
	status.description = str;
}

bool HttpResponse::isPathFounded()
{
	if (location == NULL)
		return (setHttpResError(404, "Not Found"), false);
	return (true);
}

bool HttpResponse::isMethodAllowed()
{
	if (strMethod == "GET")
		methode = GET;
	else if (strMethod == "POST")
		methode = POST;
	else if (strMethod == "DELETE")
		methode = DELETE;
	else
		methode = NONE;
	return (this->location->isMethodAllowed(this->methode));
	// return (setHttpResError(405, "Method Not Allowed"));
	// return (true);
}

void HttpResponse::cgiCooking()
{
	CgiHandler cgi(*this);
	cgi.execute(location->getCGIPath("." + getExtension(path)));
	if (state == ERROR)
		return;
	bodyType = CGI;
	writeCgiResponse();
}

int HttpResponse::autoIndexCooking()
{
	std::vector<std::string> dirContent;
	DIR *dirStream = opendir(fullPath.c_str());
	if (dirStream == NULL)
		return (setHttpResError(500, "Internal Server Error"), 0);
	struct dirent *_dir;
	while ((_dir = readdir(dirStream)) != NULL)
	{
		dirContent.push_back(_dir->d_name);
	}
	if (path[path.size() - 1] != '/')
		path += '/';
	autoIndexBody =
		"<!DOCTYPE html>\n"
		"<html lang=\"en\">\n"
		"	<head>\n"
		"		<title>YOUR DADDY</title>\n"
		"		<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
		"	</head>\n"
		"	<body>\n";
	autoIndexBody += "<h1>liste of files<h1>\n";
	for (size_t i = 0; i < dirContent.size(); i++)
	{
		autoIndexBody += "<a href=\"";
		autoIndexBody += path + dirContent[i] + "\">" + dirContent[i] + "</a><br>";
	}
	autoIndexBody +=
		"	</body>\n"
		"</html>\n";
	// if (path[path.size() - 1] != '/')
	// 	path -= '/';
	return (closedir(dirStream), 1);
}

int HttpResponse::directoryHandler()
{
	const std::vector<std::string> &indexes = this->location->globalConfig.getIndexes();
	if (this->fullPath[fullPath.size() - 1] != '/')
		fullPath.push_back('/');
	for (size_t i = 0; i < indexes.size(); i++)
	{
		if (access((this->fullPath + indexes[i]).c_str(), F_OK) != -1)
			return (bodyType = LOAD_FILE, (fullPath += indexes[i]), 1 /* , loadFile(fullPath) */);
	}
	if (location->globalConfig.getAutoIndex())
		return (bodyType = AUTO_INDEX, autoIndexCooking());
	// else
	// 	return (bodyType = NO_TYPE, 1);
	return (setHttpResError(404, "Not Found"), 0);
}

// int HttpResponse::loadFile(const std::string &pathName)
// {
// 	int _fd;
// 	char buffer[fileReadingBuffer];
// 	int j = 0;

// 	_fd = open(pathName.c_str(), O_RDONLY);
// 	if (_fd < 0)
// 		return (setHttpResError(500, "Internal Server Error"), 0);
// 	while (1)
// 	{
// 		int r = read(_fd, buffer, fileReadingBuffer);
// 		if (r < 0)
// 			return (close(_fd), setHttpResError(500, "Internal Server Error"), 0);
// 		if (r == 0)
// 			break;
// 		responseBody.push_back(std::vector<char>(r));
// 		for (int i = 0; i < r; i++)
// 		{
// 			responseBody[j][i] = buffer[i];
// 		}
// 		j++;
// 	}
// 	return (close(_fd), 1);
// }

// int HttpResponse::loadFile(int _fd)
// {
// 	char buffer[fileReadingBuffer];
// 	int j = 0;

// 	std::cout << "CGI response: " << std::endl;
// 	while (1)
// 	{
// 		int r = read(_fd, buffer, fileReadingBuffer);
// 		buffer[r] = 0;
// 		std::cout << buffer << "\n";
// 		if (r < 0)
// 			return (setHttpResError(500, "Internal Server Error"), 0);
// 		if (r == 0)
// 			break;
// 		responseBody.push_back(std::vector<char>(r)); // Gay pepole code
// 		for (int i = 0; i < r; i++)
// 		{
// 			std::cout << buffer[i];
// 			responseBody[j][i] = buffer[i];
// 		}
// 		j++;
// 	}
// 	return (1);
// }
// std::ifstream file(pathName.c_str());
// unsigned char  tmp;
// if (!file.is_open())
// 	return (setHttpResError(500, "Internal Server Error"), 0);
// while (!(file >> tmp).eof())
// 	responseBody.push_back(tmp);

int HttpResponse::pathChecking()
{
	size_t offset = location->globalConfig.getAliasOffset() ? this->location->getPath().size() : 0;
	this->fullPath = location->globalConfig.getRoot() + this->path.substr(offset);

	struct stat sStat;
	stat(fullPath.c_str(), &sStat);
	if (S_ISDIR(sStat.st_mode))
		return (directoryHandler());
	if (access(fullPath.c_str(), F_OK) != -1)
		return (bodyType = LOAD_FILE, 1 /* loadFile(fullPath) */);
	else
	{
		return (state = ERROR, setHttpResError(404, "Not Found"), 0);
	}
	return (1);
}

static int isValidHeaderChar(char c)
{
	return (std::isalpha(c) || std::isdigit(c) || c == '-' || c == ':');
}

int HttpResponse::parseCgiHaders(std::string str)
{
	size_t pos = str.find(':');
	std::string tmpHeaderName;
	std::string tmpHeaderVal;

	if (str.size() < 3)
		return (1);
	if (pos == std::string::npos || pos == 0 || str.back() != '\n')
		return (setHttpResError(502, "Bad Gateway"), 0);
	tmpHeaderName = str.substr(0, pos);
	for (size_t i = 0; i < tmpHeaderName.size(); i++)
	{
		if (!isValidHeaderChar(tmpHeaderName[i]))
		{
			return (setHttpResError(502, "Bad Gateway"), 0);
		}
	}
	tmpHeaderVal = str.substr(pos + 1);
	if (tmpHeaderVal.size() < 3 || tmpHeaderVal[0] != ' ')
		return (setHttpResError(502, "Bad Gateway"), 0);
	resHeaders[tmpHeaderName] = tmpHeaderVal.substr(0, tmpHeaderVal.size() - 1);
	return (1);
}

static int isLineCrlf(std::vector<char> vec)
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

static std::string vec2str(std::vector<char> vec)
{
	std::string str;

	for (size_t i = 0; i < vec.size(); i++)
	{
		str.push_back(vec[i]);
	}
	return (str);
}

int HttpResponse::parseCgistatus()
{
	map_it it = resHeaders.find("Status");
	std::stringstream ss;

	if (it == resHeaders.end())
		return (1);
	if (it->second.size() < 3)
		return (0);
	ss << it->second;
	ss >> this->status.code;
	if (this->status.code > 599 || this->status.code < 100)
		return (0);
	this->status.description = ss.str().substr(3);
	return (1);
}

void HttpResponse::parseCgiOutput()
{
	std::string headers(CGIOutput.data(), CGIOutput.size());
	size_t pos = headers.find("\n");
	size_t strIt = 0;

	cgiRes.state = HEADERS;
	if (CGIOutput.size() == 0 || headers.find("\r\n\r\n") == std::string::npos)
		return setHttpResError(502, "Bad Gateway");
	while (pos != std::string::npos)
	{
		if (!parseCgiHaders(headers.substr(strIt, (pos - strIt + 1))))
			return;
		strIt = pos + 1;
		pos = headers.find("\n", strIt);
	}
	if (strIt < headers.size())
		setHttpResError(502, "Bad Gateway");
	if (!parseCgistatus())
		setHttpResError(502, "Bad Gateway");
}

std::string HttpResponse::getCgiContentLenght()
{
	size_t len = 0;
	for (size_t i = cgiRes.bodyStartIndex; i < cgiRes.lines.size(); i++)
	{
		len += cgiRes.lines[i].size();
	}
	std::ostringstream oss;
	oss << len;
	return (oss.str());
}

void HttpResponse::writeCgiResponse()
{
	if (keepAlive)
		resHeaders["Connection"] = "keep-alive";
	else
		resHeaders["Connection"] = "Close";
	parseCgiOutput();
	if (state == ERROR)
		return;
	write2client(this->fd, getStatusLine().c_str(), getStatusLine().size());
	if (resHeaders.find("Transfer-Encoding") == resHeaders.end() || resHeaders["Transfer-Encoding"] != "Chunked"
		|| resHeaders.count("Content-Length") == 0)
		resHeaders["Content-Length"] = getContentLenght(bodyType);
	std::cout << resHeaders["Content-Length"] << std::endl;
	for (map_it it = resHeaders.begin(); it != resHeaders.end(); it++)
	{
		write2client(this->fd, it->first.c_str(), it->first.size());
		write2client(this->fd, ": ", 2);
		write2client(this->fd, it->second.c_str(), it->second.size());
		write2client(fd, "\r\n", 2);
	}
	write2client(this->fd, "\r\n", 2);
	state = WRITE_BODY;
}

void HttpResponse::writeResponse()
{
	writeByte = 0;
	write2client(this->fd, getStatusLine().c_str(), getStatusLine().size());
	write2client(this->fd, getConnectionState().c_str(), getConnectionState().size());
	write2client(this->fd, getContentType().c_str(), getContentType().size());
	write2client(this->fd, getContentLenght(bodyType).c_str(), getContentLenght(bodyType).size());
	write2client(fd, getDate().c_str(), getDate().size());
	write2client(fd, "Server: YOUR DADDY\r\n", strlen("Server: YOUR DADDY\r\n"));
	for (map_it it = resHeaders.begin(); it != resHeaders.end(); it++)
	{
		write2client(this->fd, it->first.c_str(), it->first.size());
		write2client(this->fd, ": ", 2);
		write2client(this->fd, it->second.c_str(), it->second.size());
		write2client(fd, "\r\n", 2);
	}
	write2client(this->fd, "\r\n", 2);
	state = WRITE_BODY;
}

std::string HttpResponse::getStatusLine()
{
	std::ostringstream oss;
	oss << status.code;

	return ("HTTP/1.1 " + oss.str() + " " + status.description + "\r\n");
}

std::string HttpResponse::getConnectionState()
{
	if (keepAlive)
		return ("Connection: keep-alive\r\n");
	return ("Connection: Close\r\n");
}

std::string HttpResponse::getExtension(const std::string &str)
{
	int i = str.size() - 1;
	while (i >= 0)
	{
		if (str[i] == '.')
			return (str.substr(i + 1));
		i--;
	}
	return ("");
}

std::string HttpResponse::getContentType()
{
	if (bodyType == NO_TYPE)
		return ("");
	if (bodyType == AUTO_INDEX)
		return ("Content-Type: text/html\r\n");
	return (
		"Content-Type: " + ctx->getType(getExtension(fullPath)) + "\r\n"
		+ "Content-Type: " + ctx->getType(getExtension(fullPath)) + "\r\n" + "Content-Type: text/html\r\n");
}

std::string HttpResponse::getDate()
{ // TODO:date;
	return ("");
}

int HttpResponse::sendBody(int _fd, enum responseBodyType type)
{
	state = WRITE_BODY;

	if (type == LOAD_FILE || type == CGI)
	{
		size_t readbuffer;

		// while (eventByte > writeByte)
		// {
		readbuffer = BUFFER_SIZE < (eventByte - writeByte) ? BUFFER_SIZE : (eventByte - writeByte);
		int size = read(responseFd, buff, readbuffer);
		if (size < 0)
			throw IOException("Read : ");
		write2client(fd, buff, size);
		this->sendSize += size;
		if (this->sendSize >= fileSize)
			state = END_BODY;
		writeByte = 0;
	}
	else if (type == AUTO_INDEX)
	{
		write2client(this->fd, autoIndexBody.c_str(), autoIndexBody.size());
		state = END_BODY;
	}
	return (/* state = END_BODY, */ 1);
}

std::string HttpResponse::getContentLenght(enum responseBodyType type)
{
	if (type == LOAD_FILE || type == CGI)
	{
		std::stringstream ss;
		struct stat s;
		if (type == CGI)
			stat(this->cgiOutFile.c_str(), &s);
		else
			stat(this->fullPath.c_str(), &s);
		ss << s.st_size;
		fileSize = s.st_size;
		if (type == CGI)
			return (ss.str());
		return ("Content-Length: " + ss.str() + "\r\n");
	}
	if (type == AUTO_INDEX)
	{
		std::ostringstream oss;
		oss << autoIndexBody.size();
		return ("Content-Length: " + oss.str() + "\r\n");
	}
	return ("Content-Length: 0\r\n");
}

static int isHex(char c)
{
	std::string B = "0123456789ABCDEF";
	std::string b = "0123456789abcdef";

	// noob code
	if (b.find(c) == std::string::npos && B.find(c) == std::string::npos)
		return (0);
	return (1);
}

void HttpResponse::decodingUrl()
{
	std::string decodedUrl;
	std::stringstream ss;

	for (size_t i = 0; i < path.size(); i++)
	{
		if (i < path.size() - 2 && path[i] == '%' && isHex(path[i + 1]) && isHex(path[i + 2]))
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
}

void HttpResponse::splitingQuery()
{
	if (path.find('?') == std::string::npos)
		return;
	size_t pos = path.find('?');
	queryStr = path.substr(pos + 1);
	path = path.substr(0, pos);
}

// void		HttpResponse::multiPartParse()
// {
// 	std::stringstream     ss(body.data());
// 	std::string			  line;
// 	std::string			  boundary("--" + request->bodyBoundary + "\r\n");
// 	std::string			  fileName;
// 	size_t				  pos;
// 	int					  uploadFd;

// 	while (1)
// 	{
// 		line.clear();
// 		std::getline(ss, line);
// 		if (line != boundary)
// 			return setHttpResError(400, "Bad Request");
// 		std::getline(ss, line);

// 		pos = line.find("; filename=\"");
// 		if (pos == std::string::npos || std::string::npos == line.find("\"", pos +1))
// 			continue;
// 	    fileName = line.substr(pos, line.find("\"", pos +1) - pos);
// 		while (std::getline(ss, line) && line != "\r\n")
// 			;
// 		uploadFd = open((location->getFileUploadPath() + fileName).c_str(), O_CREAT | O_RDONLY, 0644);
// 		if (uploadFd < 0)
// 			return setHttpResError(500, "Internal Server Error");
// 		// while (std::getline(ss, ))
// 	}
// }

int HttpResponse::uploadFile()
{
	// std::vector<multiPart> &vec = request->data[0]->multiPartBodys;

	// if (request->data[0]->multiPartBodys.size() > 0)
	// {
	// 	for (size_t _i = uploadData.it; _i < request->data[0]->multiPartBodys.size(); _i++)
	// 	{
	// 		if (uploadData.__fd < 0)
	// 			uploadData.__fd =
	// 				open((location->getFileUploadPath() + uploadData.fileName).c_str(), O_CREAT | O_WRONLY, 0644);
	// 		if (uploadData.__fd < 0)
	// 			return (setHttpResError(500, "Internal Server Error"), 0);
	// 		size_t nbytes = vec[_i].body.size() - uploadData.fileIt;
	// 		if (nbytes > eventByte)
	// 			nbytes = eventByte;
	// 		write(uploadData.__fd, &vec[_i].body.data()[uploadData.fileIt], nbytes);
	// 		uploadData.fileIt += nbytes;
	// 		eventByte -= nbytes;
	// 		if (eventByte <= 0)
	// 		{
	// 			uploadData.it = _i;
	// 			return (1);
	// 		}
	// 		std::cout << "Debug: " << vec[_i].body.size() << ", " << uploadData.fileIt << std::endl;
	// 		if (uploadData.fileIt >= vec[_i].body.size())
	// 		{
	// 			uploadData.fileIt = 0;
	// 			uploadData.it++;
	// 			uploadData.fileName = getRandomName();
	// 			close(uploadData.__fd);
	// 			uploadData.__fd = -1;
	// 		}
	// 		// else
	// 		// 	return (close(uploadData.__fd), 1);
	// 		// close(uploadData.__fd);
	// 		// std::cout << "Debug: " << uploadData.it << std::endl;
	// 	}
	// }
	// else
	// {
	// 	int __fd = open((location->getFileUploadPath() + getRandomName()).c_str(), O_CREAT | O_WRONLY, 0644);
	// 	if (__fd < 0)
	// 		return (setHttpResError(500, "Internal Server Error"), 0);
	// 	// for (size_t __i = 0; __i < request->multiPartBodys[__i].body.size(); __i++)
	// 	write(__fd, body.data(), body.size());
	// 	close(__fd);
	// }
	status.code = 201;
	status.description = "Created";
	writeResponse();
	return (1);
}

void HttpResponse::responseCooking()
{
	decodingUrl();
	splitingQuery();
	if (!isPathFounded() || isCgi())
		return;
	else
	{
		if (!isMethodAllowed())
			return setHttpResError(405, "Method Not Allowed");
		if (!pathChecking())
			return;
		// if (methode == POST && !uploadFile())
		// return;
		if (methode == POST)
			state = UPLOAD_FILES;
		if (methode == GET)
			writeResponse();
		if (bodyType == LOAD_FILE)
		{
			this->responseFd = open(fullPath.c_str(), O_RDONLY);
			if (responseFd < 0)
				setHttpResError(500, "Internal Server Error");
		}
	}
}

std::string decimalToHex(int decimal)
{
	if (decimal == 0)
		return "0";

	const char hexDigits[] = "0123456789ABCDEF";
	std::string hexResult;
	bool isNegative = false;

	if (decimal < 0)
	{
		isNegative = true;
		decimal = -decimal;
	}

	while (decimal > 0)
	{
		hexResult += hexDigits[decimal % 16];
		decimal /= 16;
	}
	if (isNegative)
		hexResult += '-';
	std::reverse(hexResult.begin(), hexResult.end());
	return hexResult;
}

std::string HttpResponse::getRandomName()
{
	std::stringstream ss;
	time_t now = std::time(0);
	struct tm *tstruct = std::localtime(&now);

	ss << "_" << tstruct->tm_year + 1900 << "_";
	ss << tstruct->tm_mon << "_";
	ss << tstruct->tm_mday << "_";
	ss << tstruct->tm_hour << "_";
	ss << tstruct->tm_min << "_";
	ss << tstruct->tm_sec;
	std::string rstr(24, ' ');
	const char charset[] = {

		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
		'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',

		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
		'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
	int n = sizeof(charset) / sizeof(charset[0]);
	for (int i = 0; i < 24; i++)
	{
		int idx = (std::rand() % n);
		rstr[i] = charset[idx];
	}
	return (rstr + ss.str());
}
