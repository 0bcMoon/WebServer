#include "../include/HttpRequest.hpp"
#include <cstddef>
#include <map>
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
typedef std::map<std::string, std::string>::iterator map_it;
void HttpRequest::feed()
{
	readRequest();
	reqBuffer = "GET / HTTP/1.1        \nHost: llklklkl\nlklklk:kjkjkjk\njkjkj: kkh\n";
	reqBufferIndex = 0;
	while (reqBufferIndex < reqBuffer.size() - 1 && state != ERROR && state != DEBUG)
	{
		if (state == METHODE)
			parseMethod();
		if (state == PATH)
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
			parseBody();
		if (state == BODY_FINISH)
			state = REQUEST_FINISH;
		if (state == REQUEST_FINISH)
			state = METHODE;
		if (state == ERROR)
			break;
	}
	if (state == DEBUG)
		std::cout << "HI\n";
	std::cout << error.code << ": " << error.description << std::endl; 
	std::cout << " --> " << methodeStr.tmpMethodeStr << " --> " << path << " --> " << httpVersion << std::endl;
	// std::cout << headers["Host"] << std::endl;
	for (map_it it = headers.begin(); it != headers.end(); ++it) {
        std::cout << "Key: " << it->first << ", Value: " << it->second << std::endl;
    }
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
	while (reqBufferIndex < reqBuffer.size())
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
			&& (httpVersion[httpVersion.size() - 1] < '1' 
			|| httpVersion[httpVersion.size() - 1] > '9'))
		{
			setHttpError(400, "Bad Request");
			return ;
		}
		else if (httpVersion.size() == 6 
			&& (httpVersion[httpVersion.size() -1] != '1'))
		{
			setHttpError(505, "HTTP Version Not Supported");
			return;
		}
		else if (httpVersion.size() > 6 && httpVersion[httpVersion.size() -1] == '.')
		{
			(*state)++;
			return;
		}
		else if (httpVersion.size() > 6
			&& (httpVersion[httpVersion.size() - 1] < '0'
			|| httpVersion[httpVersion.size() - 1] > '9'))
		{
			setHttpError(400, "Bad Request");
			return;
		}
		else if (httpVersion.size() > 6
			&& (httpVersion[httpVersion.size() - 1] >= '0'
			&& httpVersion[httpVersion.size() - 1] <= '9'))
		{
			setHttpError(505, "HTTP Version Not Supported");
			return;
		}
	}
	else if (*state == 1)
	{
		if (httpVersion[httpVersion.size() - 1] == '1')
			(*state)++;
		else
		{
			if (httpVersion[httpVersion.size() - 1] < '0'
				|| httpVersion[httpVersion.size() - 1] > '9')
			{
				setHttpError(400, "Bad Request");
				return;
			}
			else
			{
				setHttpError(505, "HTTP Version Not Supported");
				return;
			}
		}
	}
	else if (*state == 2)
	{
		setHttpError(400, "Bad Request");
		return;
	}
}

void HttpRequest::parseHttpVersion()
{
	const std::string	tmp("HTTP/");
	int					_state = 0;

	while (reqBufferIndex < reqBuffer.size() - 1 && state != ERROR)
	{
		if (httpVersion.size() == 0 && reqBuffer[reqBufferIndex] == ' ')
		{
			reqBufferIndex++;
			continue;
		}
		if (httpVersion.size() != 0 && (reqBuffer[reqBufferIndex] == ' '
			|| reqBuffer[reqBufferIndex] == '\n' || reqBuffer[reqBufferIndex] == '\r'))
		{
			crlfState = READING;
			state = REQUEST_LINE_FINISH;
			break;
		}
		httpVersion.push_back(reqBuffer[reqBufferIndex]);
		reqBufferIndex++;
		if (tmp.size() >= httpVersion.size()
			&& httpVersion[httpVersion.size() - 1] != tmp[httpVersion.size() - 1])
		{
			setHttpError(400, "Bad Request");
			return ;
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

void		HttpRequest::crlfGetting()
{
	while (reqBuffer.size() > reqBufferIndex && crlfState != LNLINE)
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
			setHttpError(400, "Bad Request");
			return ;
		}
		reqBufferIndex++;
		// std::cout << reqBuffer[reqBufferIndex] << std::endl;
		// std::cout << "HI\n";
	}
	if (crlfState == LNLINE)
	{
		state = DEBUG;
		crlfState = READING;
	}
}
// if (reqBuffer[reqBufferIndex])
// if (reqBuffer[reqBufferIndex] == '\n' && crlfState != '\n')
// 	crlfState = NLINE;
// else if (reqBuffer[reqBufferIndex] == '\n')
// while (reqBuffer.size() > reqBufferIndex && state != ERROR)
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

static int isAlpha(char c)
{
	return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

static int isValidHeaderChar(char c)
{
	return (isAlpha(c) || (c >='1' && c <= '9') || c == '-' || c == ':');
}

void	HttpRequest::parseHeaderName()
{
	while (reqBuffer.size() > reqBufferIndex && state == HEADER_NAME)
	{
		if ((currHeaderName.size() == 0 && !isAlpha(reqBuffer[reqBufferIndex]))
			|| !isValidHeaderChar(reqBuffer[reqBufferIndex]))
		{
			setHttpError(400, "Bad Request");
			return ;
		}
		if (reqBuffer[reqBufferIndex] == ':' && currHeaderName.size() == 0)
		{
			setHttpError(400, "Bad Request");
			return ;
		}
		if (reqBuffer[reqBufferIndex] == ':')
		{
			state = HEADER_VALUE;
			reqBufferIndex++;
			// headers[currHeaderName];
		}
		currHeaderName.push_back(reqBuffer[reqBufferIndex]);
		reqBufferIndex++;
	}
}

void HttpRequest::parseHeaderVal()
{
	while (reqBuffer.size() > reqBufferIndex && state == HEADER_VALUE)
	{
		if (reqBuffer[reqBufferIndex] == '\n' || reqBuffer[reqBufferIndex] == '\r')
		{
			state = HEADER_FINISH;
			return;
		}
		headers[currHeaderName].push_back(reqBuffer[reqBufferIndex]);
		reqBufferIndex++;
	}
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
