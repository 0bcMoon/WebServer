#include "../include/HttpRequest.hpp"
#include <cstring>
#include <ostream>
#include <sys/fcntl.h>
#include <unistd.h>
#include <cctype>
#include <climits>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include "HttpResponse.hpp"

HttpRequest::HttpRequest() : fd(-1)
{
	// methode = NONE;
	reqSize = 0;
	bodySize = 0;
	bodyState = _NEW;
	state = NEW;
	reqBufferSize = 0;
	error.code = 200;
	error.description = "OK";
	reqBody = NON;
	if (reqBufferIndex < reqBufferSize)
		eof = 0;
	else
		eof = 1;
}

HttpRequest::HttpRequest(int fd) : fd(fd), reqBuffer(BUFFER_SIZE)
{
	// methode = NONE;
	reqSize = 0;
	bodySize = -1;
	state = NEW;
	bodyState = _NEW;
	reqBufferSize = 0;
	error.code = 200;
	reqBufferIndex = 0;
	error.description = "OK";
	chunkState = SIZE;
	totalChunkSize = 0;
	reqBody = NON;
	if (reqBufferIndex < reqBufferSize)
		eof = 0;
	else
		eof = 1;
}

void HttpRequest::clear()
{
	// std::cout << "--------------------------->"  << methodeStr.tmpMethodeStr << std::endl;
	chunkState = SIZE;
	totalChunkSize = 0;
	chunkSize = 0;
	state = NEW;
	bodyState = _NEW;
	chunkIndex = 0;
	sizeStr.clear();
	crlfState = READING;
	path.clear();
	// headers.clear();
	currHeaderName.clear();
	body.clear();
	bodySize = -1;
	reqSize = 0;
	reqBody = NON;
	bodyBoundary.clear();
	// reqBufferSize = 0;
	// reqBufferIndex = 0;
	// reqBuffer.clear();
	error.code = 200;
	error.description = "OK";
	methodeStr.eqMethodeStr.clear();
	methodeStr.tmpMethodeStr.clear();
	httpVersion.clear();
	// std::cout << "index: " << reqBufferIndex << ", size: " << reqBufferSize << std::endl;
	if (reqBufferIndex < reqBufferSize)
		eof = 0;
	else
		eof = 1;
}

static int isAlpha(char c)
{
	return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

static int isValidHeaderChar(char c)
{
	return (isAlpha(c) || (c >= '1' && c <= '9') || c == '-' || c == ':');
}

const std::string &HttpRequest::getPath() const
{
	return (this->path);
}

// const std::map<std::string, std::string> &HttpRequest::getHeaders() const
// {
// 	return (headers);
// }

std::vector<char> HttpRequest::getBody() const
{
	return (body);
}

httpError HttpRequest::getStatus() const
{
	return (error);
}

std::string HttpRequest::getStrMethode() const
{
	return (methodeStr.tmpMethodeStr);
}


void		HttpRequest::clearData()
{
	for (size_t i = 0; i < data.size();i++)
	{
		delete data[i];
	}
	data.resize(0);
}

HttpRequest::~HttpRequest() {
	this->clearData();
}

// void HttpRequest::readRequest(int data)
// {
// 	int size = read(fd, this->buffer, data);
// 	if (size == -1)
// 		throw HttpResponse::IOException();
// 	if (size == 0)
// 		return ;
// 	else if (size > 0)
// 	{
// 		reqBuffer.resize(reqBufferSize + size);
// 		size_t j = 0;
// 		for (size_t i = reqBufferSize - size ; i < reqBufferSize; i++)
// 		{
// 			reqBuffer[i] = buffer[j];
// 			j++;
// 		}
// 	}
// }

// zero copy 
// TODO: 
void HttpRequest::readRequest(int data)
{
	//add check for full payload size (payload can have multiple request)
	// size_t offset = this->reqBufferSize;
	// this->reqBuffer.resize(offset + data); // alloc space for new comming data and take a pointer to new alloc 
										   // chunk and make read copy to it
	int size = read(fd, this->reqBuffer.data(), BUFFER_SIZE);
	if (size < 0)
		throw HttpResponse::IOException();
	if (size == 0)
		return;
	this->reqBufferSize = size;
	// reqBuffer.resize(offset + size); // may read 
									 //
	// write(1, reqBuffer.data(), reqBufferSize);
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

static int isValidHeader(std::vector<char> vec, std::map<std::string, std::string> &map)
{
	if (vec[vec.size() - 1] != '\n' || vec[vec.size() - 2] != '\r')
		return (0);
	for (size_t i = 0; i < vec.size(); i++)
	{
		if (vec[i] != '\r' && vec[i] != '\n' && !std::isprint(vec[i]))
			return (0);
	}
	std::string tmp = vec2str(vec);
	tmp = tmp.substr(0, tmp.size() - 2);
	size_t pos = tmp.find(": ");
	std::string headerName;
	if (pos == 0 || pos == std::string::npos || pos == tmp.size() - 1)
		return (0);
	for (size_t _i = 0; _i < pos; _i++)
	{
		if (!isValidHeaderChar(tmp[_i]))
			return (0);
	}
	headerName = tmp.substr(0, pos);
	map[headerName] = tmp.substr(pos + 2);
	return (1);
}

void		HttpRequest::andNew()
{//TODO:fix size to 5
	data.push_back(new data_t);
	data[data.size() - 1]->error.code = 200;
	data[data.size() - 1]->error.description = "OK";
	state = METHODE;
	data[data.size() -1]->state = NEW;
}

int		HttpRequest::checkMultiPartEnd()
{
	if (reqBody != MULTI_PART && data.back()->multiPartBodys.size() == 0)
		return (1);
	std::vector<char> &vec = data.back()->multiPartBodys.back().body;
	if (vec.size() < bodyBoundary.size() + 8 || std::string(&vec.data()[vec.size() - bodyBoundary.size() - 8]
		, bodyBoundary.size() + 8) != "\r\n--" + bodyBoundary + "--" + "\r\n")
	{
		setHttpReqError(400, "Bad Request");
		bodyState = _ERROR;
		return (0);
	}
	vec.resize(vec.size() - bodyBoundary.size() - 8);
	return (1);
}

void	HttpRequest::feed()
{
	reqBufferIndex = 0;
	while (reqBufferIndex < reqBufferSize && state != REQ_ERROR && state != DEBUG)
	{
		if (state == NEW)
			andNew();
		if (state == METHODE)
			parseMethod();
		if (state == PATH )
			parsePath();
		if (state == HTTP_VERSION)
			parseHttpVersion();
		if (state == REQUEST_LINE_FINISH)
			crlfGetting();
		if (state == HEADER_NAME)
			parseHeaderName();
		if (state == HEADER_VALUE)
			parseHeaderVal();
		if (state == HEADER_FINISH)
			crlfGetting();
		if (state == BODY)
		{
			parseBody();
		}
		if (state == BODY_FINISH &&  checkMultiPartEnd())
		{
			state = REQUEST_FINISH;
		}
		if (state == REQUEST_FINISH)
		{ 
			std::cout << "DEBUG_8" << std::endl;
			data[data.size() - 1]->state = state;
			this->clear();
		}
	}
	// for (size_t i = 0; i < multiPartBodys.size();i++)
	// {
	// 	std::cout << "WAAAAAAAAA\n";
	// 	for (map_it it = multiPartBodys[i].headers.begin(); it != multiPartBodys[i].headers.end(); ++it) {
	// 		std::cout << "Key: " << it->first << ", Value: " << it->second << "|" <<  std::endl;
	// 	}
	// }
	// // if (state == DEBUG && response(fd))
	// // 	std::cout << "FUCKING DONE" << std::endl;

	// INFO: print request information;

	// std::cout << error.code << ": " << error.description << std::endl;
	// std::cout << " --> " << methodeStr.tmpMethodeStr << " --> " << path << " --> " << httpVersion << std::endl;
	// for (map_it it = headers.begin(); it != headers.end(); ++it) {
	//        std::cout << "Key: " << it->first << ", Value: " << it->second << "|" <<  std::endl;
	//    }
	// // int __fd = open("log", O_RDWR, 0777);
	// for (auto& it : body)
	// {
	// 	// write(__fd, &it, 1);
	// 	std::cout << (char)it;
	// }
}

void HttpRequest::setHttpReqError(int code, std::string str)
{
	state = REQ_ERROR;
	error.code = code;
	error.description = str;
	data[data.size() - 1]->state = REQ_ERROR;
	data[data.size() - 1]->error.code = code;
	data[data.size() - 1]->error.description = str;
}

void HttpRequest::parseMethod()
{
	// while (reqBufferSize > reqBufferIndex && reqBuffer[reqBufferIndex] == '\n'
	// 	   && data[data.size() -1]->strMethode.size() == 0)
	// 	reqBufferIndex++;
	while (reqBufferSize > reqBufferIndex)
	{
		// if (reqBuffer[reqBufferIndex] == ' ' && methodeStr.tmpMethodeStr.size() == 0)
		if (reqBuffer[reqBufferIndex] == ' ' && data[data.size() -1]->strMethode.size() == 0)
		{
			setHttpReqError(400, "Bad Request");
			return;
		}
		if (reqBuffer[reqBufferIndex] == ' ')
		{
			state = PATH;
			return;
		}
		if (reqBuffer[reqBufferIndex] < 'A' || reqBuffer[reqBufferIndex] > 'Z')
		{
			std::cout << ":->" << (int)reqBuffer[reqBufferIndex] << std::endl;
			setHttpReqError(400, "Bad Request");
			return;
		}
		data[data.size() -1]->strMethode.push_back(reqBuffer[reqBufferIndex]);
		// methodeStr.tmpMethodeStr.push_back(reqBuffer[reqBufferIndex]);
		reqBufferIndex++;
	}
}

int HttpRequest::verifyUriChar(char c)
{
	return (
		(c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_'
		|| c == '~' || c == '!' || c == '$' || c == '&' || c == '\'' || c == '(' || c == ')' || c == '*' || c == '+'
		|| c == ',' || c == ';' || c == '=' || c == '@' || c == '/' || c == ':' || c == '%' || c == '?');
}

void HttpRequest::parsePath()
{
	while (reqBufferIndex < reqBufferSize)
	{
		// if (path.size() == 0 && reqBuffer[reqBufferIndex] == ' ')
		if (data[data.size() -1]->path.size() == 0 && reqBuffer[reqBufferIndex] == ' ')
		{
			reqBufferIndex++;
			continue;
		}
		// if (path.size() != 0 && reqBuffer[reqBufferIndex] == ' ')
		if (data[data.size() -1]->path.size() != 0 && reqBuffer[reqBufferIndex] == ' ')
		{
			state = HTTP_VERSION;
			return;
		}
		// if (!verifyUriChar(reqBuffer[reqBufferIndex]) || (path.size() == 0 && reqBuffer[reqBufferIndex] != '/'))
		if (!verifyUriChar(reqBuffer[reqBufferIndex])
				|| (data[data.size() -1]->path.size() == 0 && reqBuffer[reqBufferIndex] != '/'))
		{
			setHttpReqError(400, "Bad Request");
			return;
		}
		data[data.size() -1]->path.push_back(reqBuffer[reqBufferIndex]);
		// path.push_back(reqBuffer[reqBufferIndex]);
		reqBufferIndex++;
		// if (path.size() > URI_MAX)
		if (data[data.size() -1]->path.size() > URI_MAX)
		{
			setHttpReqError(414, "URI Too Long");
			return;
		}
	}
}

// void HttpRequest::checkHttpVersion()
// {
// 	std::string str(&httpVersion.c_str()[5]);
// 	int state = 0;
// 	for (size_t i = 0;i < str.size(); i++)
// 	{
// 		if (state == 0 && str[i] == '.' && i != 0)
// 			state++;
// 		else if (str[i] != '0' && state == 1)
// 		{
// 			if (i != str.size() - 1 || str[i] < '1' || str[i] > '9')
// 			{
// 				setHttpError(400, "Bad Request");
// 				return ;
// 			}
// 			else if (str[i] > '1')
// 			{
// 				setHttpError(505, "HTTP Version Not Supported");
// 				return ;
// 			}
// 			else
// 				return ;
// 		}
// 		else if ((state == 0 && (str[i] < '1' || str[i] > '9'))
// 			|| (state == 1 && str[i] != '0'))
// 		{
// 			setHttpError(400, "Bad Request*");
// 			return ;
// 		}
// 		else if (state == 0 && str[i] != '1')
// 		{
// 			setHttpError(505, "HTTP Version Not Supported");
// 			return;
// 		}
// 	}
// 	std::cout << "Hi" << std::endl;
// }

void HttpRequest::checkHttpVersion(int *state)
{
	if (*state == 0)
	{
		if (httpVersion.size() == 6
			&& (httpVersion[httpVersion.size() - 1] < '1' || httpVersion[httpVersion.size() - 1] > '9'))
		{
			setHttpReqError(400, "Bad Request");
			return;
		}
		else if (httpVersion.size() == 6 && (httpVersion[httpVersion.size() - 1] != '1'))
		{
			setHttpReqError(505, "HTTP Version Not Supported");
			return;
		}
		else if (httpVersion.size() > 6 && httpVersion[httpVersion.size() - 1] == '.')
		{
			(*state)++;
			return;
		}
		else if (
			httpVersion.size() > 6
			&& (httpVersion[httpVersion.size() - 1] < '0' || httpVersion[httpVersion.size() - 1] > '9'))
		{
			setHttpReqError(400, "Bad Request");
			return;
		}
		else if (
			httpVersion.size() > 6
			&& (httpVersion[httpVersion.size() - 1] >= '0' && httpVersion[httpVersion.size() - 1] <= '9'))
		{
			setHttpReqError(505, "HTTP Version Not Supported");
			return;
		}
	}
	else if (*state == 1)
	{
		if (httpVersion[httpVersion.size() - 1] == '1')
			(*state)++;
		else
		{
			if (httpVersion[httpVersion.size() - 1] < '0' || httpVersion[httpVersion.size() - 1] > '9')
			{
				setHttpReqError(400, "Bad Request");
				return;
			}
			else
			{
				setHttpReqError(505, "HTTP Version Not Supported");
				return;
			}
		}
	}
	else if (*state == 2)
	{
		setHttpReqError(400, "Bad Request");
		return;
	}
}

void HttpRequest::parseHttpVersion()
{
	const std::string tmp("HTTP/");
	int _state = 0;

	while (reqBufferIndex < reqBufferSize - 1 && state != REQ_ERROR)
	{
		if (httpVersion.size() == 0 && reqBuffer[reqBufferIndex] == ' ')
		{
			reqBufferIndex++;
			continue;
		}
		if (httpVersion.size() != 0
			&& (reqBuffer[reqBufferIndex] == ' ' || reqBuffer[reqBufferIndex] == '\n'
				|| reqBuffer[reqBufferIndex] == '\r'))
		{
			crlfState = READING;
			state = REQUEST_LINE_FINISH;
			break;
		}
		httpVersion.push_back(reqBuffer[reqBufferIndex]);
		reqBufferIndex++;
		if (tmp.size() >= httpVersion.size() && httpVersion[httpVersion.size() - 1] != tmp[httpVersion.size() - 1])
		{
			setHttpReqError(400, "Bad Request");
			return;
		}
		checkHttpVersion(&_state);
	}
}

void HttpRequest::returnHandle()
{
	if (crlfState == READING)
		crlfState = RETURN;
	else if (crlfState == NLINE)
		crlfState = LRETURN;
	else if (crlfState == RETURN)
		;
	else if (crlfState == LRETURN)
		;
	else if (crlfState == LNLINE)
		;
}

void HttpRequest::nLineHandle()
{
	if (crlfState == READING || crlfState == RETURN)
		crlfState = NLINE;
	else if (crlfState == NLINE)
		crlfState = LNLINE;
	else if (crlfState == LRETURN)
		crlfState = LNLINE;
	else if (crlfState == LNLINE)
		;
}

void HttpRequest::crlfGetting()
{
	while (reqBufferSize > reqBufferIndex && crlfState != LNLINE)
	{
		if (crlfState == READING && reqBuffer[reqBufferIndex] == ' ')
			;
		else if (reqBuffer[reqBufferIndex] == '\n')
			nLineHandle();
		else if (reqBuffer[reqBufferIndex] == '\r')
			returnHandle();
		else if (crlfState == NLINE)
		{
			state = HEADER_NAME;
			crlfState = READING;
			return;
		}
		else if (crlfState != NLINE)
		{
			setHttpReqError(400, "Bad Request");
			return;
		}
		reqBufferIndex++;
		// std::cout << reqBuffer[reqBufferIndex] << std::endl;
		// std::cout << "HI\n";
	}
	if (crlfState == LNLINE)
	{
		state = BODY;
		crlfState = READING;
	}
}
// if (reqBuffer[reqBufferIndex])
// if (reqBuffer[reqBufferIndex] == '\n' && crlfState != '\n')
// 	crlfState = NLINE;
// else if (reqBuffer[reqBufferIndex] == '\n')
// while (reqBufferSize > reqBufferIndex && state != E*RROR)
// {
// 	if (crlfState == READING)
// 	{
// 		if (reqBuffer[reqBufferIndex] == ' ')
// 		{
// 			reqBufferIndex++;
// 			continue;
// 		}
// 		else if (reqBuffer[reqBufferIndex] == '\n')
// 			crlfState = NLINE;
// 		else
// 		{
// 			setHttpError(400, "Bad Request");
// 			return ;
// 		}
// 		reqBufferIndex++;
// 	}
// 	if (crlfState == NLINE)
// 	{
// 		if (reqBuffer[reqBufferIndex] != '\r')
// 		{
// 			setHttpError(400, "Bad Request");
// 			return ;
// 		}
// 		crlfState = RETURN;
// 		reqBufferIndex++;
// 	}
// 	if (crlfState == RETURN)
// 	{
// 		if (reqBuffer[reqBufferIndex] == '\n')
// 		{
// 			crlfState = LNLINE;
// 			reqBufferIndex++;
// 		}
// 		else
// 		{
// 			crlfState = READING;
// 			state = HEADER_NAME;
// 			return;
// 		}
// 	}
// }

void HttpRequest::parseHeaderName()
{
	while (reqBufferSize > reqBufferIndex && state == HEADER_NAME)
	{
		if ((currHeaderName.size() == 0 && !isAlpha(reqBuffer[reqBufferIndex]))
			|| !isValidHeaderChar(reqBuffer[reqBufferIndex]))
		{
			setHttpReqError(400, "Bad Request");
			return;
		}
		if (reqBuffer[reqBufferIndex] == ':' && currHeaderName.size() == 0)
		{
			setHttpReqError(400, "Bad Request");
			return;
		}
		if (reqBuffer[reqBufferIndex] == ':')
		{
			state = HEADER_VALUE;
			reqBufferIndex++;
			if (reqBufferIndex < reqBufferSize && reqBuffer[reqBufferIndex] == ' ')
				reqBufferIndex++;
			return;
			// headers[currHeaderName];
		}
		currHeaderName.push_back(reqBuffer[reqBufferIndex]);
		reqBufferIndex++;
	}
}

void HttpRequest::parseHeaderVal()
{
	while (reqBufferSize > reqBufferIndex && state == HEADER_VALUE)
	{
		if (reqBuffer[reqBufferIndex] == '\n' || reqBuffer[reqBufferIndex] == '\r')
		{
			if (data[data.size() - 1]->headers[currHeaderName].size() != 0)
				data[data.size() - 1]->headers[currHeaderName] += ",";
			data[data.size() - 1]->headers[currHeaderName] += currHeaderVal;
			// if (headers[currHeaderName].size() != 0)
			// 	headers[currHeaderName] += ",";
			// headers[currHeaderName] += currHeaderVal;
			state = HEADER_FINISH;
			currHeaderName.clear();
			currHeaderVal.clear();
			return;
		}
		if (!std::isprint((int)reqBuffer[reqBufferIndex]))
		{
			setHttpReqError(400, "Bad Request");
			return;
		}
		// headers[currHeaderName].push_back(reqBuffer[reqBufferIndex]);
		currHeaderVal.push_back(reqBuffer[reqBufferIndex]);
		reqBufferIndex++;
	}
}

int HttpRequest::isNum(const std::string &str)
{
	for (size_t i = 0; i < str.size(); i++)
	{
		if (!std::isdigit(str[i]))
			return (0);
	}
	return (1);
}

int HttpRequest::checkContentType()
{
	if (data[data.size() - 1]->headers.find("Content-Type") == data[data.size() - 1]->headers.end())
		return (0);
	size_t i = 0;
	std::string tmp;

	while (i < data[data.size() - 1]->headers["Content-Type"].size() && data[data.size() - 1]->headers["Content-Type"][i] == ' ')
		i++;
	if (i == data[data.size() - 1]->headers["Content-Type"].size())
		return (0);
	while (data[data.size() - 1]->headers["Content-Type"].size() > i && data[data.size() - 1]->headers["Content-Type"][i] != ';')
	{
		tmp.push_back(data[data.size() - 1]->headers["Content-Type"][i]);
		i++;
	}
	if ((tmp == "text/plain" || tmp == "application/x-www-form-urlencoded")
		&& data[data.size() - 1]->headers["Content-Type"].size() != i)
		return (setHttpReqError(400, "Bad Request"), 1);
	if (tmp == "text/plain")
		return (reqBody = TEXT_PLAIN, 0);
	if (tmp == "application/x-www-form-urlencoded")
		return (reqBody = URL_ENCODED, 0);
	if (tmp != "multipart/form-data")
		return (0);
	reqBody = MULTI_PART;
	tmp = "; boundary=";
	size_t j = 0;
	while (j < tmp.size() && i < data[data.size() - 1]->headers["Content-Type"].size()
		&& tmp[j] == data[data.size() - 1]->headers["Content-Type"][i])
	{
		i++;
		j++;
	}
	// std::cout << "j ==> "  << j << " | size ==> " << tmp.size() << std::endl;
	// std::cout << "i ==> "  << i << " | size ==> " <<  data[data.size() - 1]->headers["Content-Type"].size()<< std::endl;
	if (j != tmp.size() || i == data[data.size() - 1]->headers["Content-Type"].size())
		return (setHttpReqError(400, "Bad Request"), 1);
	bodyBoundary = data[data.size() - 1]->headers["Content-Type"].substr(i);
	// bodyBoundary = "jj";
	return (0);
}

int HttpRequest::firstHeadersCheck()
{
	if (data[data.size() - 1]->headers.find("Host") == data[data.size() - 1]->headers.end())
		return (setHttpReqError(400, "Bad Request"), 1);
	if (data[data.size() - 1]->headers.find("Content-Length") != data[data.size() - 1]->headers.end() 
		&& !isNum(data[data.size() - 1]->headers["Content-Length"]))
		return (setHttpReqError(400, "Bad Request"), 1);
	if (data[data.size() - 1]->headers.find("Transfer-Encoding") != data[data.size() - 1]->headers.end()
		&& data[data.size() - 1]->headers["Transfer-Encoding"] != "chunked")
		return (setHttpReqError(501, "Not Implemented"), 1);
	if (data[data.size() - 1]->headers.find("Content-Length") != data[data.size() - 1]->headers.end()
		&& data[data.size() - 1]->headers.find("Transfer-Encoding") != data[data.size() - 1]->headers.end())
		return (setHttpReqError(400, "Bad Request"), 1);
	if (data[data.size() - 1]->headers.find("Content-Length") == data[data.size() - 1]->headers.end()
		&& data[data.size() - 1]->headers.find("Transfer-Encoding") == data[data.size() - 1]->headers.end())
		return (state = BODY_FINISH, 1);
	if (data[data.size() - 1]->headers.find("Content-Type") != data[data.size() - 1]->headers.end()
		&& data[data.size() - 1]->headers["Content-Type"].find(",") != std::string::npos)
		return (setHttpReqError(400, "Bad Request"), 1);
	return (checkContentType());
}

void HttpRequest::contentLengthBodyParsing()
{
	if (bodySize == -1)
	{
		std::stringstream ss(data[data.size() - 1]->headers["Content-Length"]);
		if (!(ss >> bodySize) || !(ss.eof()))
		{
			setHttpReqError(400, "Bad Request");
			return;
		}
		if (data[data.size() - 1]->body.size() + bodySize > BODY_MAX)
		{
			setHttpReqError(413, "Payload Too Large");
			return;
		}
	}
	// data[data.size() - 1]->body.insert()
	while (reqBufferIndex < reqBufferSize && (size_t)bodySize > data[data.size() - 1]->body.size())
	{
		data[data.size() - 1]->body.push_back(reqBuffer[reqBufferIndex]);
		if (reqBody == MULTI_PART && !parseMultiPart())
			return ;
		reqBufferIndex++;
	}
	if (data.back()->body.size() > BODY_MAX)
		return setHttpReqError(413, "Paylood too large");
	if ((size_t)bodySize == data[data.size() - 1]->body.size())
		state = BODY_FINISH;
}

int HttpRequest::convertChunkSize()
{
	char *end;
	long tmp = std::strtol(sizeStr.c_str(), &end, 16);
	if (*end != 0 || tmp > INT_MAX || sizeStr.size() == 0)
		return (setHttpReqError(400, "Bad Request"), 1);
	if (tmp + data.back()->body.size() > BODY_MAX)
		return (setHttpReqError(413, "Payload Too Large"), 1);
	chunkSize = tmp;
	sizeStr = "";
	chunkIndex = 0;
	return (0);
}

void HttpRequest::chunkEnd()
{
	if (reqBufferSize > reqBufferIndex)
	{
		if (reqBuffer[reqBufferIndex] == '\n')
		{
			reqBufferIndex++;
			chunkState = SIZE;
			state = BODY_FINISH;
			return;
		}
		if ((reqBuffer[reqBufferIndex] != '\r' && chunkIndex == 0) || chunkIndex != 0)
		{
			setHttpReqError(400, "Bad Request");
			return;
		}
		chunkIndex++;
		reqBufferIndex++;
	}
}

void HttpRequest::chunkedBodyParsing()
{
	if (chunkState == SIZE)
	{
		while (reqBufferIndex < reqBufferSize)
		{
			if (sizeStr[sizeStr.size() - 1] == '\r' && reqBuffer[reqBufferIndex] != '\n')
			{
				setHttpReqError(400, "Bad Request");
				return;
			}
			if (reqBuffer[reqBufferIndex] == '\n')
			{
				if (sizeStr[sizeStr.size() - 1] == '\r')
					sizeStr = sizeStr.substr(0, sizeStr.size() - 1);
				chunkState = LINE;
				reqBufferIndex++;
				break;
			}
			else if (
				!std::isdigit(reqBuffer[reqBufferIndex]) && reqBuffer[reqBufferIndex] != '\r'
				&& (reqBuffer[reqBufferIndex] < 'A' || reqBuffer[reqBufferIndex] > 'F')
				&& (reqBuffer[reqBufferIndex] < 'a' || reqBuffer[reqBufferIndex] > 'f'))
			{
				setHttpReqError(400, "Bad Request");
				return;
			}
			sizeStr.push_back(reqBuffer[reqBufferIndex]);
			reqBufferIndex++;
		}
		if (chunkState == LINE)
		{
			if (convertChunkSize())
				return;
		}
	}
	if (chunkState == LINE)
	{
		if (chunkSize == 0)
		{
			chunkEnd();
			return;
		}
		while (reqBufferIndex < reqBufferSize && chunkSize > chunkIndex)
		{
			data[data.size() - 1]->body.push_back(reqBuffer[reqBufferIndex]);
			if (reqBody == MULTI_PART && !parseMultiPart())
				return ;
			reqBufferIndex++;
			chunkIndex++;
		}
		if (chunkSize == chunkIndex)
		{
			chunkIndex = 0;
			chunkState = END_LINE;
		}
	}
	while (chunkState == END_LINE && reqBufferIndex < reqBufferSize)
	{
		if (reqBuffer[reqBufferIndex] == '\n')
			chunkState = SIZE;
		else if ((chunkIndex == 0 && reqBuffer[reqBufferIndex] != '\r') || chunkIndex == 1)
		{
			setHttpReqError(400, "Bad Request");
			return;
		}
		chunkIndex++;
		reqBufferIndex++;
	}
}
 
void				HttpRequest::handleNewBody()
{
	if (data[data.size() - 1]->body.size() < (bodyBoundary.size() + 4))
		return ;
	std::string expectedBoundary = "--" + bodyBoundary + "\r\n";
	std::string tmp(data.back()->body.data(), data.back()->body.size());
	if (tmp == expectedBoundary)
	{
		data[data.size() - 1]->multiPartBodys.push_back(multiPart());
		bodyState = MULTI_PART_HEADERS;
	}
	else
	{
		setHttpReqError(400, "Bad Request");
		bodyState = _ERROR;
	}
}

void				HttpRequest::handleMultiPartHeaders()
{
	std::vector<char> &vec = data.back()->body;
	char	c = vec.back();
	if (currHeaderName.size() == 0 && c == '\r')
		return ;
	// if (currHeaderName.size() == 0 && c == '\n' && vec.size() >= 2 
	// 	&& vec[vec.size() - 2] == '\r')
	if (currHeaderName.size() == 0 && isBodycrlf())
	{
		bodyState = STORING;
		return ;
	}
	if (currHeaderName.size() == 0 && ((c != '\n' && vec.size() >= 2 && 
		vec[vec.size() - 2] == '\r') || (!isAlpha(c) || !isValidHeaderChar(c))))
	{
		setHttpReqError(400, "Bad Request");
		bodyState = _ERROR;
		return;
	}
	if (c == ':')
	{
		bodyState = MULTI_PART_HEADERS_VAL;
		return;
	}
	currHeaderName.push_back(c);
}

int					HttpRequest::isBodycrlf()
{
	return (data.back()->body.size() > 2 
		&& data.back()->body[data.back()->body.size() - 1] == '\n'
		&& data.back()->body[data.back()->body.size() - 2] == '\r');
}

int					HttpRequest::isBorder()
{
	if (data.back()->multiPartBodys.size() == 0)
		return (0);
	std::vector<char> &vec = data.back()->multiPartBodys.back().body;
	return (vec.size() >= (bodyBoundary.size() + 6)
		&& std::string(&vec.data()[vec.size() - (bodyBoundary.size() + 6)], (bodyBoundary.size() + 6))
		== "\r\n--" + bodyBoundary + "\r\n");
}

void				HttpRequest::handleStoring()
{
	std::vector<char> &vec = data.back()->multiPartBodys.back().body;
	
	vec.push_back(reqBuffer[reqBufferIndex]);
	if (isBodycrlf() && isBorder())
	{
		bodyState = MULTI_PART_HEADERS;
		vec.resize(vec.size() - (bodyBoundary.size() + 6));
		data.back()->multiPartBodys.push_back(multiPart());
		// write(1, &vec.data()[vec.size() - (bodyBoundary.size() + 4)], (bodyBoundary.size() + 4));
		// std::cout << vec.size() << std::endl;
		// data[data.size() - 1]->multiPartBodys.resize(vec.size() - (bodyBoundary.size() + 4));
		return ;
	}
}

void				HttpRequest::parseMultiPartHeaderVal()
{
	std::vector<char> &vec = data[data.size() - 1]->body;
	char	c = vec[vec.size() - 1];

	if (c == '\r')
	{
		// if (data[data.size() - 1]->headers[currHeaderName].size() != 0)
		// 	data[data.size() - 1]->headers[currHeaderName] += ",";
		data.back()->multiPartBodys.back().headers[currHeaderName] += currHeaderVal;
		bodyState = BODY_CRLF;
		currHeaderName.clear();
		currHeaderVal.clear();
		return;
	}
	if (!std::isprint((int)c))
	{
		setHttpReqError(400, "Bad Request");
		bodyState = _ERROR;
		return;
	}
	currHeaderVal.push_back(c);
}

void				HttpRequest::parseBodyCrlf()
{
	if (data.back()->body.back() != '\n')
	{
		setHttpReqError(400, "Bad Request");
		bodyState = _ERROR;
	}
	else
		bodyState = MULTI_PART_HEADERS;
}

int 				HttpRequest::parseMultiPart()
{
	if (bodyState == _NEW)
		handleNewBody();
	else if (bodyState == MULTI_PART_HEADERS)
		handleMultiPartHeaders();
	else if (bodyState == MULTI_PART_HEADERS_VAL)
		parseMultiPartHeaderVal();
	else if (bodyState == BODY_CRLF)
		parseBodyCrlf();
	else if (bodyState == STORING)
		handleStoring();
	if (bodyState == _ERROR)
		return (0);
	return (1);
}

void				HttpRequest::parseBody()
{
	if (!(data[data.size() - 1]->body.size()) && firstHeadersCheck())
	{
		return;
	}
	if (data.back()->headers.find("Content-Length") != data.back()->headers.end())
		contentLengthBodyParsing();
	if (data.back()->headers.find("Transfer-Encoding") != data.back()->headers.end())
		chunkedBodyParsing();
}

const std::string &HttpRequest::getHost() const
{
	return (/* headers.find("Host")->second */ "");//ERROR
}
