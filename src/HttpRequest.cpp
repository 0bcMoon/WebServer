#include "../include/HttpRequest.hpp"
#include <cstddef>
#include <string>
#include <unistd.h>
#include <iostream>


HttpRequest::HttpRequest() : fd(-1)
{
	methode = NONE;
    reqSize = 0;
    bodySize = 0;
	state = METHODE;
	reqBufferSize = 0;
	error.code = 200;
	error.description = "OK";
}

HttpRequest::HttpRequest(int fd) : fd(fd)
{
	methode = NONE;
    reqSize = 0;
    bodySize = 0;
	state = METHODE;
	reqBufferSize = 0;
	error.code = 200;
	error.description = "OK";
}

HttpRequest::~HttpRequest() {

}

void HttpRequest::readRequest()
{
	char tmp[REQSIZE_MAX + 10];

	int size = read(fd, tmp, REQSIZE_MAX + 10);
	if (size ==  -1)
		setHttpError(500, "Internal Server Error");
	else if ((reqBufferSize + size) > REQSIZE_MAX)
		setHttpError(413, "Content Too Large");
	else
		reqBuffer = tmp;
}

void HttpRequest::feed()
{
	readRequest();
	reqBufferIndex = 0;
	while (reqBufferIndex < reqBuffer.size() && state != ERROR)
	{
		if (state == METHODE)
			parseMethod();
		if (state == PATH)
			parsePath();
		if (state == HTTP_VERSION)
			parseHttpVersion();
		if (state == REQUEST_LINE_FINISH)
			state = HEADER_NAME;
		if (state == HEADER_NAME)
			parseHeaderName();
		if (state == HEADER_VALUE)
			parseHeaderVal();
		if (state == HEADER_FINISH)
			state = BODY;
		if (state == BODY)
			parseBody();
		if (state == BODY_FINISH)
			state = REQUEST_FINISH;
		if (state == REQUEST_FINISH)
			state = METHODE;
		if (state == ERROR)
			break;
	}
	std::cout << error.code << ": " << error.description << std::endl; 
}

void HttpRequest::setHttpError(int code, std::string str)
{
	state = ERROR;
	error.code = code;
	error.description = str;
}


void HttpRequest::parseMethod()
{
	while (reqBuffer.size() > reqBufferIndex)
	{
		if (reqBuffer[reqBufferIndex] == ' ' && methodeStr.tmpMethodeStr.size() == 0)
		{
			setHttpError(400, "Bad Request");
			return ;
		}
		if (reqBuffer[reqBufferIndex] == ' ')
		{
			state = PATH;
			return ;
		}
		if (reqBuffer[reqBufferIndex] < 'A' || reqBuffer[reqBufferIndex] > 'Z')
		{
			setHttpError(400, "Bad Request");
			return ;
		}
		methodeStr.tmpMethodeStr.push_back(reqBuffer[reqBufferIndex]);
		reqBufferIndex++;
	}
}

int HttpRequest::verifyUriChar(char c)
{
	return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')
	|| c == '-' || c == '.' || c == '_' || c == '~' || c == '!' || c == '$' || c == '&'
	|| c == '\'' || c == '(' || c == ')' || c == '*' || c == '+' || c == ',' || c == ';'
	|| c == '=' || c == '@' || c == '/' || c == ':');
}

void HttpRequest::parsePath()
{
	while (reqBufferIndex < reqBuffer.size() - 1)
	{
		if (path.size() == 0 && reqBuffer[reqBufferIndex] == ' ')
		{
			reqBufferIndex++; 
			continue;
		}
		if (path.size() != 0 && reqBuffer[reqBufferIndex] == ' ')
		{
			state = HTTP_VERSION;
			return;
		}
		if (!verifyUriChar(reqBuffer[reqBufferIndex]) || (path.size() == 0 && reqBuffer[reqBufferIndex] != '/'))
		{
			setHttpError(400, "Bad Request");
			return ;
		}
		path.push_back(reqBuffer[reqBufferIndex]);
		reqBufferIndex++;
		if (path.size() > URI_MAX)
		{
			setHttpError(414, "URI Too Long");
			return ;
		}
	}
}

void HttpRequest::checkHttpVersion()
{
	std::string str(&httpVersion.c_str()[5]);
	int state = 0;
	for (size_t i = 0;i < str.size(); i++)
	{
		if (state == 0 && str[i] == '.' && i != 0)
			state++;
		else if (str[i] != '0' && state == 1)
		{
			if (i != str.size() - 1 || str[i] < '1' || str[i] > '9')
			{
				setHttpError(400, "Bad Request");
				return ;
			}
			else if (str[i] > '1')
			{
				setHttpError(505, "HTTP Version Not Supported");
				return ;
			}
			else
				return ;
		}
		else if ((state == 0 && (str[i] < '1' || str[i] > '9'))
			|| (state == 1 && str[i] != '0'))
		{
			setHttpError(400, "Bad Request");
			return ;
		}
		else if (state == 0 && str[i] != 1)
		{
			setHttpError(505, "HTTP Version Not Supported");
			return;
		}
	}
}

void HttpRequest::parseHttpVersion()
{
	const std::string tmp("HTTP/");
	while (reqBufferIndex < reqBuffer.size())
	{
		if (httpVersion.size() == 0 && reqBuffer[reqBufferIndex] == ' ')
		{
			reqBufferIndex++;
			continue;
		}
		if (httpVersion.size() != 0 && reqBuffer[reqBufferIndex] == ' ')
		{
			state = REQUEST_LINE_FINISH;
			checkHttpVersion();
			break;
		}
		httpVersion.push_back(reqBuffer[reqBufferIndex]);
		reqBufferIndex++;
		if (tmp.size() > httpVersion.size())
		{
			if (httpVersion[httpVersion.size() - 1] != tmp[httpVersion.size() - 1])
			{
				setHttpError(400, "Bad Request");
				return ;
			}
		}
	}
}

void HttpRequest::parseHeaderName()
{

}

void HttpRequest::parseHeaderVal()
{

}

void HttpRequest::parseBody()
{

}
// void HttpRequest::parseMethod()
// {	
// 	if (methodeStr.eqMethodeStr.size() == 0)
// 	{
// 		if (reqBuffer[reqBufferIndex] == 'G')
// 			methodeStr.eqMethodeStr = "GET";
// 		else if (reqBuffer[reqBufferIndex] == 'P')
// 			methodeStr.eqMethodeStr = "POST";
// 		else if (reqBuffer[reqBufferIndex] == 'D')
// 			methodeStr.eqMethodeStr = "DELETE";
// 		else
// 		{
// 			setHttpError(400, "Bad Request");
// 			return ;
// 		}
// 		reqBufferIndex++;
// 		methodeStr.tmpMethodeStr.push_back(methodeStr.eqMethodeStr[0]);
// 	}
// 	while (methodeStr.tmpMethodeStr.size() != methodeStr.eqMethodeStr.size() && reqBuffer.size() > reqBufferIndex)
// 	{
// 		if (reqBuffer[reqBufferIndex] != methodeStr.eqMethodeStr[methodeStr.tmpMethodeStr.size()])
// 		{
// 			setHttpError(400, "Bad Request");
// 			return ;
// 		}
// 		methodeStr.tmpMethodeStr.push_back(reqBuffer[reqBufferIndex]);
// 		reqBufferIndex++;
// 	}
// 	if (methodeStr.tmpMethodeStr == methodeStr.eqMethodeStr)
// 	{
// 		if (methodeStr.tmpMethodeStr[0] == 'G')
// 			methode = GET;
// 		else if (methodeStr.tmpMethodeStr[0] == 'P')
// 			methode = POST;
// 		else if (methodeStr.tmpMethodeStr[0] == 'D')
// 			methode = DELETE;
// 		state = PATH;
// 	}
// }
