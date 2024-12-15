#include "../include/HttpRequest.hpp"
#include <sys/fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <cctype>
#include <climits>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include "DataType.hpp"
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
	reqSize = 0;
	location = NULL;
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
	location = NULL;
	chunkState = SIZE;
	totalChunkSize = 0;
	chunkSize = 0;
	state = NEW;
	bodyState = _NEW;
	chunkIndex = 0;
	sizeStr.clear();
	crlfState = READING;
	currHeaderName.clear();
	body.clear();
	bodySize = -1;
	reqSize = 0;
	reqBody = NON;
	bodyBoundary.clear();
	error.code = 200;
	error.description = "OK";
	methodeStr.eqMethodeStr.clear();
	methodeStr.tmpMethodeStr.clear();
	httpVersion.clear();
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

void HttpRequest::clearData()
{
	for (size_t i = 0; i < data.size(); i++)
	{
		delete data[i];
	}
	data.resize(0);
}

HttpRequest::~HttpRequest()
{
	this->clearData();
}

void HttpRequest::readRequest(int data)
{
	size_t read_size = std::min(data, BUFFER_SIZE);
	int size = read(fd, this->reqBuffer.data(), read_size);
	std::cout << "read size -- " << size << "\n";
	if (size < 0)
		throw HttpResponse::IOException();
	if (size == 0)
		return;
	this->reqBufferSize = size;
	reqBufferIndex = 0;
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

void HttpRequest::andNew()
{ // TODO:fix size to 5
	data.push_back(new data_t);
	data[data.size() - 1]->error.code = 200;
	data[data.size() - 1]->error.description = "OK";
	state = METHODE;
	data[data.size() - 1]->state = NEW;
	data.back()->isRequestLineValid = 0;
}

int HttpRequest::checkMultiPartEnd()
{
	std::string border = "\r\n--" + bodyBoundary + "--\r\n";
	if (reqBody != MULTI_PART)
		return (1);
	if (data.back()->bodyHandler.currFd >= 0
		&& write(data.back()->bodyHandler.currFd, border.c_str(), data.back()->bodyHandler.borderIt) < 0)
	{
		setHttpReqError(500, "Internal Server Error");
		bodyState = _ERROR;
		return (0);
	}
	if (data.back()->bodyHandler.header != "\r\n--" + bodyBoundary + "--\r\n")
	{
		setHttpReqError(400, "Bad Request");
		bodyState = _ERROR;
		return (0);
	}
	return (1);
}

void HttpRequest::feed()
{
	while (reqBufferIndex < reqBufferSize && state != REQ_ERROR && state != DEBUG)
	{
		if (state == NEW)
			andNew();
		if (state == METHODE)
			parseMethod();
		if (state == PATH)
			parsePath();
		if (state == HTTP_VERSION)
			parseHttpVersion();
		if (state == REQUEST_LINE_FINISH)
		{
			crlfGetting();
			if (state == HEADER_NAME)
				break;
		}
		if (state == HEADER_NAME)
			parseHeaderName();
		if (state == HEADER_VALUE)
			parseHeaderVal();
		if (state == HEADER_FINISH)
		{
			crlfGetting();
		}
		if (state == BODY)
		{
			parseBody();
		}
		if (state == BODY_FINISH && checkMultiPartEnd())
		{
			state = REQUEST_FINISH;
		}
		if (state == REQUEST_FINISH)
		{
			std::cout << "DEBUG_8" << std::endl;
			data.back()->state = state;
			this->clear();
		}
	}
	// for (size_t i = 0; i < multiPartBodys.size();i++)
	// {
	// 	std::cout << "WAAAAAAAAA\n";
	// 	for (map_it it = multiPartBodys[i].headers.begin(); it !=
	// multiPartBodys[i].headers.end(); ++it) { 		std::cout << "Key: " << it->first
	// << ", Value: " << it->second << "|" <<  std::endl;
	// 	}
	// }
	// // if (state == DEBUG && response(fd))
	// // 	std::cout << "FUCKING DONE" << std::endl;

	// INFO: print request information;

	// std::cout << error.code << ": " << error.description << std::endl;
	// std::cout << " --> " << methodeStr.tmpMethodeStr << " --> " << path << "
	// --> " << httpVersion << std::endl; for (map_it it = headers.begin(); it !=
	// headers.end(); ++it) {
	//        std::cout << "Key: " << it->first << ", Value: " << it->second <<
	//        "|" <<  std::endl;
	//    }
	// // int __fd = open("log", O_RDWR, 0777);
	// for (auto& it : body)
	// {
	// 	// write(__fd, &it, 1);
	// 	std::cout << (char)it;
	// }
}

#include <execinfo.h>
#include <csignal>
#include <iostream>
#include <stdexcept>
void print_stack_trace()
{
	void *callstack[128];
	int frames = backtrace(callstack, 128);
	char **symbols = backtrace_symbols(callstack, frames);

	std::cerr << "Stack Trace:" << std::endl;
	for (int i = 0; i < frames; ++i)
	{
		std::cerr << symbols[i] << std::endl;
	}

	free(symbols);
}
void HttpRequest::setHttpReqError(int code, std::string str)
{
	// print_stack_trace();
	// exit(1);
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
		// if (reqBuffer[reqBufferIndex] == ' ' && methodeStr.tmpMethodeStr.size()
		// == 0)
		if (reqBuffer[reqBufferIndex] == ' ' && data[data.size() - 1]->strMethode.size() == 0)
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
		data[data.size() - 1]->strMethode.push_back(reqBuffer[reqBufferIndex]);
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
		if (data[data.size() - 1]->path.size() == 0 && reqBuffer[reqBufferIndex] == ' ')
		{
			reqBufferIndex++;
			continue;
		}
		if (data[data.size() - 1]->path.size() != 0 && reqBuffer[reqBufferIndex] == ' ')
		{
			state = HTTP_VERSION;
			return;
		}
		if (!verifyUriChar(reqBuffer[reqBufferIndex])
			|| (data[data.size() - 1]->path.size() == 0 && reqBuffer[reqBufferIndex] != '/'))
		{
			setHttpReqError(400, "Bad Request");
			return;
		}
		data[data.size() - 1]->path.push_back(reqBuffer[reqBufferIndex]);
		reqBufferIndex++;
		if (data[data.size() - 1]->path.size() > URI_MAX)
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

	while (i < data[data.size() - 1]->headers["Content-Type"].size()
		   && data[data.size() - 1]->headers["Content-Type"][i] == ' ')
		i++;
	if (i == data[data.size() - 1]->headers["Content-Type"].size())
		return (0);
	while (data[data.size() - 1]->headers["Content-Type"].size() > i
		   && data[data.size() - 1]->headers["Content-Type"][i] != ';')
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
	// std::cout << "i ==> "  << i << " | size ==> " <<  data[data.size() -
	// 1]->headers["Content-Type"].size()<< std::endl;
	if (j != tmp.size() || i == data[data.size() - 1]->headers["Content-Type"].size())
		return (setHttpReqError(400, "Bad Request"), 1);
	bodyBoundary = data[data.size() - 1]->headers["Content-Type"].substr(i);
	// bodyBoundary = "jj";
	return (0);
}

// bool HttpRequest::isCGI()
// {
// 	size_t pos = this->path.rfind(".");
// 	if (pos == std::string::npos)
// 		return (false);
// 	for (size_t j = pos;pos < this->path.size();j++)
// 	{

// 	}

bool HttpRequest::isMethodAllowed()
{
	int methode = 0;

	if (data.back()->strMethode == "GET")
		methode = GET;
	else if (data.back()->strMethode == "POST")
		methode = POST;
	else if (data.back()->strMethode == "DELETE")
		methode = DELETE;
	else
		methode = NONE;
	return (this->location->isMethodAllowed(methode));
}

bool HttpRequest::isCGI()
{
	std::string &path = this->data.back()->path;
	size_t pos = path.rfind('.');
	if (pos == std::string::npos)
		return (false);
	size_t j = pos;
	for (; j < path.size(); j++)
	{
		if (path[j] == '?')
			break;
		if (path[j] == '/')
			break;
	}
	std::string ext = path.substr(pos, j - pos);
	if (this->location->getCGIPath(ext).empty())
		return (false);
	//TODO: check here if cgi is not excutable or not exist or if request file does not exist
	if (path[j] == '/')
		this->data.back()->path_info = path.substr(j);
	return (true);
}

bool HttpRequest::validateRequestLine()
{
	if (location == NULL)
		return (setHttpReqError(404, "Not Found"), 0);
	if (!this->isMethodAllowed())
		return (setHttpReqError(405, "Method Not Allowed"), 0);

	data.back()->bodyHandler.isCgi = location->getCGIPath("." + HttpResponse::getExtension(data.back()->path)).size();
	return (1);
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
	// this->isCGI();
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
		if (data[data.size() - 1]->bodyHandler.bodySize + bodySize > (size_t)BODY_MAX)
		{
			setHttpReqError(413, "Payload Too Large");
			return;
		}
	}
	std::vector<char> &body = data.back()->bodyHandler.body;
	bodyHandler &bodyHandler = data.back()->bodyHandler;

	body.clear();
	if (bodySize - bodyHandler.bodySize > reqBufferSize - reqBufferIndex)
		body.insert(
			body.begin(),
			reqBuffer.begin() + reqBufferIndex,
			reqBuffer.begin() + reqBufferSize); // WARNING : double check
	else
		body.insert(
			body.begin(),
			reqBuffer.begin() + reqBufferIndex,
			reqBuffer.begin() + reqBufferIndex + bodySize - bodyHandler.bodySize);
	bodyHandler.bodySize += body.size();
	reqBufferIndex += body.size();
	bodyHandler.bodyIt = 0;

	if (bodyHandler.isCgi)
	{
		if (!bodyHandler.writeBody())
			setHttpReqError(500, "Internal Server Error");
	}
	else if (reqBody == MULTI_PART)
		parseMultiPart();
	if ((size_t)bodySize <= data.back()->bodyHandler.bodySize)
	{
		std::cout << "DEBUG_7" << std::endl;
		state = BODY_FINISH;
	}
	// else if (reqBody == MULTI_PART && bodyHandler.bodySize + body.size() >= (size_t)bodySize - bodyBoundary.size() -
	// 8) if (reqBody == MULTI_PART && 		!data.back()->bodyHandler.upload2file(bodyBoundary)) 	setHttpReqError(500,
	// "Internal Server Error"); data.back()->bodyHandler.bodyIt = 0; if (!data.back()->bodyHandler.writeBody() )
	// 	setHttpReqError(500, "Internal Server Error");
}

int bodyHandler::push2fileBody(char c, const std::string &boundary)
{
	// std::string		border = "\r\n--" + boundary + "\r\n";

	// std::cout << "-- " << fileBodyIt<< std::endl;;
	fileBody[fileBodyIt++] = c;
	if (boundary[borderIt] == c)
		borderIt++;
	else
		borderIt = 0;
	if (borderIt == boundary.size())
		return (fileBodyIt -= boundary.size(), borderIt = 0, 0);
	return (1);
}

void bodyHandler::push2body(char c)
{
	body[bodyIt++] = c;
	bodySize++;
}

int HttpRequest::convertChunkSize()
{
	char *end;
	long tmp = std::strtol(sizeStr.c_str(), &end, 16);
	if (*end != 0 || tmp > INT_MAX || sizeStr.size() == 0)
		return (setHttpReqError(400, "Bad Request"), 1);
	if (tmp + data.back()->bodyHandler.bodySize > BODY_MAX)
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
			// data[data.size() - 1]->body.push_back(reqBuffer[reqBufferIndex]);
			data.back()->bodyHandler.push2body(reqBuffer[reqBufferIndex]);
			if (reqBody == MULTI_PART && !parseMultiPart())
				return;
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

void HttpRequest::handleNewBody()
{
	std::string border = "--" + bodyBoundary + "\r\n";
	bodyHandler &bodyHandler = data.back()->bodyHandler;

	for (size_t &i = bodyHandler.bodyIt; i < bodyHandler.body.size(); i++)
	{
		if (border[bodyHandler.borderIt] != bodyHandler.body[i])
			return (bodyState = _ERROR, setHttpReqError(400, "Bad Request"));
		bodyHandler.borderIt++;
		if (bodyHandler.borderIt == border.size())
		{
			bodyHandler.borderIt = 0;
			bodyState = MULTI_PART_HEADERS;
			bodyHandler.bodyIt++;
			return;
		}
	}
}

int bodyHandler::openNewFile()
{
	std::string fileName;

	size_t pos = header.find("filename=\"");
	if (pos == std::string::npos || pos + 10 == header.size())
		return (1);
	pos += 10;
	if (header.find('\"', pos) == std::string::npos)
		return (0);
	fileName = "/tmp/"; // TODO: make it dynamique
	fileName += header.substr(pos, header.find('\"', pos) - pos);
	currFd = open(fileName.c_str(), O_CREAT | O_RDWR, 0644);
	perror("why");
	std::cout << fileName << "--" << currFd << "\n";
	if (currFd < 0)
		return (0);
	return (1);
}

void HttpRequest::handleMultiPartHeaders()
{
	bodyHandler &bodyHandler = data.back()->bodyHandler;
	const std::vector<char> &body = data.back()->bodyHandler.body;

	std::cout << "|";
	write(1, &body.data()[bodyHandler.bodyIt], 5);
	std::cout << "|\n";
	for (size_t &i = bodyHandler.bodyIt; i < body.size(); i++)
	{
		if (!std::isprint((int)body[i]) && body[i] != '\r' && body[i] != '\n')
		{
			std::cout << "-- | " << (int)body[i] << " |-- \n";
			exit(88);
			return (bodyState = _ERROR, setHttpReqError(400, "Bad Request"));
		}
		bodyHandler.header.push_back(body[i]);
		if (bodyHandler.header.find("\r\n\r\n") != std::string::npos)
		{
			i++;
			if (!bodyHandler.openNewFile())
			{
				setHttpReqError(500, "Internal Server Error");
				bodyState = _ERROR;
				return;
			}
			data.back()->bodyHandler.header.clear();
			bodyState = STORING;
			return;
		}
	}
	// char c = reqBuffer[reqBufferIndex];

	// if (currHeaderName.size() == 0 && c == '\r')
	// 	return;
	// if (currHeaderName.size() == 0 && isBodycrlf()) {
	// 	bodyState = STORING;
	// 	if (!data.back()->bodyHandler.openNewFile()) {
	// 		setHttpReqError(500, "Internal Server Error");
	// 		bodyState = _ERROR;
	// 	}
	// 	data.back()->bodyHandler.headers.clear();
	// 	return;
	// }
	// if ((currHeaderName.size() == 0 && !isAlpha(c)) ||
	// 		(currHeaderName.size() > 0 && !isValidHeaderChar(c))) {
	// 	setHttpReqError(400, "Bad Request");
	// 	bodyState = _ERROR;
	// 	return;
	// }
	// if (c == ':') {
	// 	bodyState = MULTI_PART_HEADERS_VAL;
	// 	return;
	// }
	// currHeaderName.push_back(c);
}

int HttpRequest::isBodycrlf()
{
	return (reqBuffer[reqBufferIndex] == '\n');
}

void HttpRequest::handleStoring()
{
	const std::string border = "\r\n--" + bodyBoundary + "\r\n";
	std::vector<char> &body = data.back()->bodyHandler.body;
	bodyHandler &bodyHandler = data.back()->bodyHandler;

	if (bodyHandler.borderIt > 0)
	{
		for (size_t &i = bodyHandler.bodyIt; i < body.size(); i++)
		{
			if (border[bodyHandler.borderIt] != body[i])
			{
				if (bodyHandler.currFd >= 0 && write(bodyHandler.currFd, border.c_str(), bodyHandler.borderIt) < 0)
					return (bodyState = _ERROR, setHttpReqError(500, "Internal Server Error"));
				break;
			}
			bodyHandler.borderIt++;
			if (bodyHandler.borderIt == border.size())
			{
				// if (bodyHandler.currFd >= 0 &&
				//   write(1, &body.data()[i - bodyHandler.borderIt], bodyHandler.borderIt) < 0)
				// 	return (bodyState = _ERROR, setHttpReqError(500, "Internal Server Error"));
				bodyHandler.bodyIt++;
				bodyState = MULTI_PART_HEADERS;
				if (bodyHandler.currFd >= 0)
					close(bodyHandler.currFd);
				bodyHandler.currFd = -1;
				bodyHandler.borderIt = 0;
				return;
			}
		}
	}

	size_t pos = bodyHandler.bodyIt;
	std::vector<char>::iterator it = body.begin() + pos - 1;

	while (1)
	{
		it = std::find(it + 1, body.end(), border[bodyHandler.borderIt]);
		if (it == body.end())
			break;
		std::vector<char>::iterator tmp;
		for (tmp = it; tmp != body.end(); tmp++)
		{
			if (*tmp != border[bodyHandler.borderIt])
			{
				bodyHandler.borderIt = 0;
				break;
			}
			bodyHandler.borderIt++;
			if (border.size() == bodyHandler.borderIt)
			{
				size_t nbuff = it - (body.begin() - pos);
				if (bodyHandler.currFd >= 0 && write(bodyHandler.currFd, &body.data()[bodyHandler.bodyIt], nbuff) < 0)
					return (bodyState = _ERROR, setHttpReqError(500, "Internal Server Error"));
				bodyHandler.bodyIt += nbuff + border.size();
				bodyState = MULTI_PART_HEADERS;
				bodyHandler.borderIt = 0;
				if (bodyHandler.currFd >= 0)
					close(bodyHandler.currFd);
				bodyHandler.currFd = -1;
				return;
			}
		}
		if (tmp == body.end())
			break;
	}

	if (bodyHandler.currFd >= 0)
	{
		if (write(bodyHandler.currFd, &body.data()[bodyHandler.bodyIt], body.size() - bodyHandler.bodyIt) < 0)
			return (bodyState = _ERROR, setHttpReqError(500, "Internal Server Error"));
		bodyHandler.bodyIt += body.size() - bodyHandler.bodyIt;
	}
	// if (!data.back()->bodyHandler.push2fileBody(
	// 			reqBuffer[reqBufferIndex], "\r\n--" + bodyBoundary + "\r\n")) {
	// 	bodyState = MULTI_PART_HEADERS;
	// 	if (!data.back()->bodyHandler.upload2file(bodyBoundary)) {
	// 		setHttpReqError(500, "Internal Server Error");
	// 		bodyState = _ERROR;
	// 	}
	// 	if (data.back()->bodyHandler.currFd >= 0) {
	// 		close(data.back()->bodyHandler.currFd);
	// 		data.back()->bodyHandler.currFd = -1;
	// 	}
	// }
}

void HttpRequest::parseMultiPartHeaderVal()
{
	char c = reqBuffer[reqBufferIndex];

	if (c == '\r')
	{
		data.back()->bodyHandler.headers[currHeaderName] += currHeaderVal;
		bodyState = BODY_CRLF;
		currHeaderName.clear();
		currHeaderVal.clear();
		return;
	}
	if (!std::isprint((int)c) && c != '\r')
	{
		setHttpReqError(400, "Bad Request");
		bodyState = _ERROR;
		return;
	}
	currHeaderVal.push_back(c);
}

void HttpRequest::parseBodyCrlf()
{
	if (reqBuffer[reqBufferIndex] != '\n')
	{
		setHttpReqError(400, "Bad Request");
		bodyState = _ERROR;
	}
	else
		bodyState = MULTI_PART_HEADERS;
}

int HttpRequest::parseMultiPart()
{
	std::vector<char> &vec = data.back()->bodyHandler.body;
	size_t size = data.back()->bodyHandler.bodySize - ((size_t)bodySize - bodyBoundary.size() - 8);
	bodyHandler &tmp = data.back()->bodyHandler;
	if (size > vec.size())
		size = 0;
	if (bodyState == STORING && data.back()->bodyHandler.bodySize >= (size_t)bodySize - bodyBoundary.size() - 8)
	{
		if (size > vec.size())
		{
			tmp.header += std::string(vec.data(), vec.size());
			return (1);
		}
		else
		{
			tmp.header += std::string(&vec.data()[vec.size() - size], size);
			vec.resize(vec.size() - size);
		}
	}
	if (bodyState == _NEW)
		handleNewBody();
	while (vec.size() > tmp.bodyIt)
	{
		if (bodyState == MULTI_PART_HEADERS)
			handleMultiPartHeaders();
		if (bodyState == STORING)
			handleStoring();
		if (bodyState == _ERROR)
			return (0);
	}
	return (1);
}

void HttpRequest::parseBody()
{
	if (!(data.back()->bodyHandler.bodySize) && firstHeadersCheck())
		return;
	if (data.back()->headers.find("Content-Length") != data.back()->headers.end())
		contentLengthBodyParsing();
	if (data.back()->headers.find("Transfer-Encoding") != data.back()->headers.end())
		chunkedBodyParsing();
}

int bodyHandler::writeBody()
{
	std::cout << "ANA KANKTB" << std::endl;
	if (bodyFd < 0)
		bodyFd = open(bodyFile.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0644);
	if (bodyFd < 0)
		return (0);
	if (write(bodyFd, body.data(), bodyIt) < 0)
		return (0);
	bodyIt = 0;
	return (1);
}

int bodyHandler::upload2file(std::string &boundary)
{
	const std::string &border = "\r\n--" + boundary + "\r\n";

	if (currFd < 0)
		return (1);
	if (write(currFd, fileBody.data(), fileBodyIt - borderIt) < 0)
	{
		std::cout << "Faild -- " << strerror(errno) << std::endl;
		exit(1);
		return (0);
	}
	fileBodyIt = borderIt;
	for (size_t i = 0; i < borderIt; i++)
		fileBody[i] = border[i];
	return (1);
}

bodyHandler::bodyHandler() : bodyFd(-1), body(BUFFER_SIZE), currFd(-1), fileBody(BUFFER_SIZE + 10)
{
	bodyIt = 0;
	isCgi = false;
	fileBodyIt = 0;
	bodySize = 0;
	borderIt = 0;
	bodyFile = Proc::mktmpfileName();
}

bodyHandler::~bodyHandler()
{
	if (bodyFd >= 0)
		close(bodyFd);
	if (currFd >= 0)
		close(currFd);
}

void bodyHandler::clear()
{
	bodyIt = 0;
	fileBodyIt = 0;
	bodySize = 0;
	borderIt = 0;
	if (bodyFd >= 0)
		close(bodyFd);
	if (currFd >= 0)
		close(currFd);
}

void HttpRequest::decodingUrl()
{
	std::string decodedUrl;
	std::stringstream ss;

	for (size_t i = 0; i < data.back()->path.size(); i++)
	{
		if (i < data.back()->path.size() - 2 && data.back()->path[i] == '%' 
			&& isHex(data.back()->path[i + 1]) && isHex(data.back()->path[i + 2]))
		{
			int tmp;
			ss << data.back()->path[i + 1] << data.back()->path[i + 2];
			ss >> std::hex >> tmp;
			ss.clear();
			decodedUrl.push_back(tmp);
			i += 2;
		}
		else
			decodedUrl.push_back(data.back()->path[i]);
	}
	data.back()->path = decodedUrl;
}

void HttpRequest::splitingQuery()
{
	if (data.back()->path.find('?') == std::string::npos)
		return;
	size_t pos = data.back()->path.find('?');
	data.back()->queryStr = data.back()->path.substr(pos + 1);
	data.back()->path = data.back()->path.substr(0, pos);
}

const std::string &HttpRequest::getHost() const
{
	return (/* headers.find("Host")->second */ ""); // ERROR
}
