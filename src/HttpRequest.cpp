#include "../include/HttpRequest.hpp"
#include <cctype>
#include <climits>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/fcntl.h>
#include <unistd.h>
#include <iostream>


HttpRequest::HttpRequest() : fd(-1)
{
	// methode = NONE;
    reqSize = 0;
    bodySize = 0;
	state = METHODE;
	reqBufferSize = 0;
	error.code = 200;
	error.description = "OK";
	reqBody = NON;
	if (reqBufferIndex < reqBuffer.size())
		eof = 0;
	else 
		eof = 1;
}

HttpRequest::HttpRequest(int fd) : fd(fd)
{
	// methode = NONE;
    reqSize = 0;
    bodySize = -1;
	state = METHODE;
	reqBufferSize = 0;
	error.code = 200;
	reqBufferIndex = 0;
	error.description = "OK";
	chunkState = SIZE;
	totalChunkSize = 0;
	reqBody = NON;
	if (reqBufferIndex < reqBuffer.size())
		eof = 0;
	else 
		eof = 1;
}

void	HttpRequest::clear()
{
	// std::cout << "--------------------------->"  << methodeStr.tmpMethodeStr << std::endl;
	chunkState = SIZE;
	totalChunkSize = 0;
	chunkSize = 0;
	state = METHODE;
	chunkIndex = 0;
	sizeStr.clear();
	crlfState = READING;
	path.clear();
	headers.clear();
	currHeaderName.clear();
	body.clear();
	bodySize = -1;
	reqSize = 0;
	reqBody = NON;
	bodyBoundary.clear();
	multiPartBodys.clear();
	// reqBufferSize = 0;
	// reqBufferIndex = 0;
	// reqBuffer.clear();
	error.code = 200;
	error.description = "OK";
	methodeStr.eqMethodeStr.clear();
	methodeStr.tmpMethodeStr.clear();
	httpVersion.clear();
	// std::cout << "index: " << reqBufferIndex << ", size: " << reqBuffer.size() << std::endl;
	if (reqBufferIndex < reqBuffer.size())
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
	return (isAlpha(c) || (c >='1' && c <= '9') || c == '-' || c == ':');
}

const std::string	&HttpRequest::getPath() const
{
	return (this->path);
}

std::map<std::string, std::string>	HttpRequest::getHeaders() const
{
	return (headers);
}

std::vector<char>					HttpRequest::getBody() const
{
	return (body);
}

httpError							HttpRequest::getStatus() const
{
	return (error);
}

std::string							HttpRequest::getStrMethode() const
{
	return (methodeStr.tmpMethodeStr);
}

HttpRequest::~HttpRequest() {

}

void HttpRequest::readRequest()
{
	char buffer[1000000];//TODO:read

	while (1)
	{
		// for (size_t i = 0; i < 1000000;i++)
		// 	buffer[i] = 0;
		int size = read(fd, buffer, 1000000 + 1);
		if (size ==  -1)
			return ;
		if (size == 0)
			return ;
		else if (size > 0)
		{
			reqBuffer.resize(reqBuffer.size() + size);
			size_t j = 0;
			for (size_t i = reqBuffer.size() - size ; i < reqBuffer.size(); i++)
			{
				reqBuffer[i] = buffer[j];
				j++;
			}
		}
	}
}

static std::string vec2str(std::vector<char> vec)
{
	std::string str;

	for (size_t i = 0; i < vec.size();i++)
	{
		str.push_back(vec[i]);
	}
	return (str);
}

static int isValidHeader(std::vector<char> vec, std::map<std::string, std::string>& map)
{
	if (vec[vec.size() - 1] != '\n' || vec[vec.size() - 2] != '\r')
		return (0);
	for (size_t i = 0; i < vec.size();i++)
	{
		if (vec[i] != '\r' && vec[i] != '\n' && !std::isprint(vec[i]))
			return (0);
	}
	std::string tmp = vec2str(vec);
	tmp = tmp.substr(0, tmp.size() - 2);
	size_t pos = tmp.find(": ");
	std::string	headerName;
	if (pos == 0 || pos == std::string::npos || pos == tmp.size() - 1)
		return (0);
	for (size_t _i = 0; _i < pos;_i++)
	{
		if (!isValidHeaderChar(tmp[_i]))
			return (0);
	}
	headerName = tmp.substr(0, pos);
	map[headerName] = tmp.substr(pos + 2);
	return (1);
}

int			HttpRequest::parseMuliPartBody()
{
	if (reqBody != MULTI_PART)
		return (1);
	std::vector<std::vector<char> >	lines;
	size_t										lineIndex = 0;
	std::vector<size_t>                         pos;

	for (size_t i = 0; i < body.size(); i++)
	{
			if (lineIndex == lines.size())
				lines.push_back(std::vector<char >());
			lines[lineIndex].push_back(body[i]);
			if (body[i] == '\n')
				lineIndex++;
	}
	if (lines.size() < 3 || lines[0].size() == 0)
		return (setHttpReqError(400, "Bad Request"), 0);
	lineIndex = 0;
	if (vec2str(lines[lineIndex]) != "--" + bodyBoundary  + "\r\n")
		return (setHttpReqError(400, "Bad Request"), 0);
	while (lineIndex < lines.size() - 1)
	{
		if (vec2str(lines[lineIndex]) == "--" + bodyBoundary  + "\r\n")
		{
			pos.push_back(lineIndex + 1);
		}
		lineIndex++;
	}
	if (vec2str(lines[lines.size() - 1]) != "--" + bodyBoundary + "--\r\n")
		return (setHttpReqError(400, "Bad Request"), 0);
	for (size_t i = 0; i < pos.size();i++)
	{
		multiPartBodys.push_back(multiPart());
		size_t  it = pos[i];
		if (pos[i] == lines.size() - 1)
			return (setHttpReqError(400, "Bad Request"), 0);
		while (it < lines.size() - 1 && (i == pos.size() - 1 || it < pos[i + 1]) 
			&& vec2str(lines[it]) != "\r\n")
		{
			if (!isValidHeader(lines[it], multiPartBodys[i].headers))
				return (setHttpReqError(400, "Bad Request"), 0);
			it++;
		}
		if (vec2str(lines[it]) != "\r\n")
			return (setHttpReqError(400, "Bad Request"), 0);
		it++;
		while (it < lines.size() && (i == pos.size() - 1 || it < pos[i + 1]))
		{
			if (vec2str(lines[it]) == "--" + bodyBoundary + "--\r\n"
				|| vec2str(lines[it]) == "--" + bodyBoundary + "\r\n")
			{
				if (multiPartBodys[i].body.size() >= 2)
					multiPartBodys[i].body.resize(multiPartBodys[i].body.size() - 2);
				it++;
				continue;
			}
			for (size_t __i = 0; __i < lines[it].size();__i++)
			{
				multiPartBodys[i].body.push_back(lines[it][__i]);
			}
			it++;
		}
	}
	return (1);
}

void HttpRequest::feed()
{
	// readRequest();
 	while (reqBufferIndex < reqBuffer.size() && state != REQ_ERROR && state != DEBUG)
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
		if (state == BODY_FINISH /*&& parseMuliPartBody()*/)
		{
			std::cout << "debug" << std::endl;
			state = REQUEST_FINISH;
			break ;
		}
		// if (state == REQ_ERROR)
		// 	break;
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
}


void HttpRequest::parseMethod()
{
	while (reqBuffer.size() > reqBufferIndex && reqBuffer[reqBufferIndex] == '\n'
		&& methodeStr.tmpMethodeStr.size() == 0)
		reqBufferIndex++;
	while (reqBuffer.size() > reqBufferIndex)
	{
		if (reqBuffer[reqBufferIndex] == ' ' && methodeStr.tmpMethodeStr.size() == 0)
		{
			setHttpReqError(400, "Bad Request");
			return ;
		}
		if (reqBuffer[reqBufferIndex] == ' ')
		{
			state = PATH;
			return ;
		}
		if (reqBuffer[reqBufferIndex] < 'A' || reqBuffer[reqBufferIndex] > 'Z')
		{
			setHttpReqError(400, "Bad Request");
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
	|| c == '=' || c == '@' || c == '/' || c == ':' || c == '%' || c == '?');
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
			setHttpReqError(400, "Bad Request");
			return ;
		}
		path.push_back(reqBuffer[reqBufferIndex]);
		reqBufferIndex++;
		if (path.size() > URI_MAX)
		{
			setHttpReqError(414, "URI Too Long");
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
			setHttpReqError(400, "Bad Request");
			return ;
		}
		else if (httpVersion.size() == 6 
			&& (httpVersion[httpVersion.size() -1] != '1'))
		{
			setHttpReqError(505, "HTTP Version Not Supported");
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
			setHttpReqError(400, "Bad Request");
			return;
		}
		else if (httpVersion.size() > 6
			&& (httpVersion[httpVersion.size() - 1] >= '0'
			&& httpVersion[httpVersion.size() - 1] <= '9'))
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
			if (httpVersion[httpVersion.size() - 1] < '0'
				|| httpVersion[httpVersion.size() - 1] > '9')
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
	const std::string	tmp("HTTP/");
	int					_state = 0;

	while (reqBufferIndex < reqBuffer.size() - 1 && state != REQ_ERROR)
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
			setHttpReqError(400, "Bad Request");
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
			setHttpReqError(400, "Bad Request");
			return ;
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
// while (reqBuffer.size() > reqBufferIndex && state != E*RROR)
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



void	HttpRequest::parseHeaderName()
{
	while (reqBuffer.size() > reqBufferIndex && state == HEADER_NAME)
	{
		if ((currHeaderName.size() == 0 && !isAlpha(reqBuffer[reqBufferIndex]))
			|| !isValidHeaderChar(reqBuffer[reqBufferIndex]))
		{
			setHttpReqError(400, "Bad Request");
			return ;
		}
		if (reqBuffer[reqBufferIndex] == ':' && currHeaderName.size() == 0)
		{
			setHttpReqError(400, "Bad Request");
			return ;
		}
		if (reqBuffer[reqBufferIndex] == ':')
		{
			state = HEADER_VALUE;
			reqBufferIndex++;
			if (reqBufferIndex < reqBuffer.size() && reqBuffer[reqBufferIndex] == ' ')
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
	while (reqBuffer.size() > reqBufferIndex && state == HEADER_VALUE)
	{
		if (reqBuffer[reqBufferIndex] == '\n' || reqBuffer[reqBufferIndex] == '\r')
		{
			if (headers[currHeaderName].size() != 0)
				headers[currHeaderName] += ",";
			headers[currHeaderName] += currHeaderVal;
			state = HEADER_FINISH;
			currHeaderName.clear();
			currHeaderVal.clear();
			return;
		}
		if (!std::isprint((int)reqBuffer[reqBufferIndex]))
		{
			setHttpReqError(400, "Bad Request");
			return ;
		}
		// headers[currHeaderName].push_back(reqBuffer[reqBufferIndex]);
		currHeaderVal.push_back(reqBuffer[reqBufferIndex]);
		reqBufferIndex++;
	}
}


int HttpRequest::isNum(const std::string& str)
{
	for (size_t i = 0; i < str.size(); i++)
	{
		if (!std::isdigit(str[i]))
			return (0);
	}
	return (1);
}

int		HttpRequest::checkContentType()
{
	if (headers.find("Content-Type") == headers.end())
		return (0);	
	size_t						i = 0;
	std::string					tmp;

	while (i < headers["Content-Type"].size() && headers["Content-Type"][i] == ' ')
		i++;
	if (i == headers["Content-Type"].size())
		return (0);
	while (headers["Content-Type"].size() > i && headers["Content-Type"][i] != ';')
	{
		tmp.push_back(headers["Content-Type"][i]);
		i++;
	}
	if ((tmp == "text/plain" || tmp == "application/x-www-form-urlencoded") 
		&& headers["Content-Type"].size() != i)
		return (setHttpReqError(400, "Bad Request"), 1);
	if (tmp == "text/plain")
		return (reqBody = TEXT_PLAIN, 1);
	if (tmp == "application/x-www-form-urlencoded")
		return (reqBody = URL_ENCODED, 1);
	if (tmp != "multipart/form-data")
		return (0);
		// return (setHttpReqError(415, "Unsupported Media Type"), 1);
	reqBody = MULTI_PART;
	tmp = "; boundary=";
	size_t j = 0;
	while (j < tmp.size() && i < headers["Content-Type"].size()
		&& tmp[j] == headers["Content-Type"][i])
	{
		i++;
		j++;
	}
	if (j != tmp.size() || i == headers["Content-Type"].size())
		return (setHttpReqError(400, "Bad Request"), 1);
	bodyBoundary = headers["Content-Type"].substr(i);
	return (0);
}

int		HttpRequest::firstHeadersCheck()
{
	if (headers.find("Host") == headers.end())
		return (setHttpReqError(400, "Bad Request"), 1);
	if (headers.find("Content-Length") != headers.end()
		&& !isNum(headers["Content-Length"]))
		return (setHttpReqError(400, "Bad Request"), 1);
	if (headers.find("Transfer-Encoding") != headers.end()
		&& headers["Transfer-Encoding"] != "chunked")
		return (setHttpReqError(501, "Not Implemented"), 1);
	if (headers.find("Content-Length") != headers.end()
		&& headers.find("Transfer-Encoding") != headers.end())
		return (setHttpReqError(400, "Bad Request"), 1);
	if (headers.find("Content-Length") == headers.end()
		&& headers.find("Transfer-Encoding") == headers.end())
		return (state = BODY_FINISH, 1);
	if (headers.find("Content-Type") != headers.end()
		&& headers["Content-Type"].find(",") != std::string::npos)
		return (setHttpReqError(400, "Bad Request"), 1);
	return (/* checkContentType() */0);
}

void		HttpRequest::contentLengthBodyParsing()
{
	if (bodySize == -1)
	{
		std::stringstream ss(headers["Content-Length"]);
		if (!(ss >> bodySize) || !(ss.eof()))
		{
			setHttpReqError(400, "Bad Request");
			return ;
		}
		if (body.size() + bodySize > BODY_MAX)
		{
			setHttpReqError(413, "Payload Too Large");
			return ;
		}
	}
	while (reqBufferIndex < reqBuffer.size() && (size_t)bodySize > body.size())
	{
		body.push_back(reqBuffer[reqBufferIndex]);
		reqBufferIndex++;
	}
	if ((size_t)bodySize == body.size())
		state = BODY_FINISH;
}

int		HttpRequest::convertChunkSize()
{
	char *end;
	long tmp = std::strtol(sizeStr.c_str(), &end, 16);
	if (*end != 0 || tmp > INT_MAX || sizeStr.size() == 0)
		return (setHttpReqError(400, "Bad Request"), 1);
	if (tmp + body.size() > BODY_MAX)
		return (setHttpReqError(413, "Payload Too Large"), 1);
	chunkSize = tmp;
	sizeStr = "";
	chunkIndex = 0;
	return (0);
}

void			HttpRequest::chunkEnd()
{
	if (reqBuffer.size() > reqBufferIndex)
	{
		if (reqBuffer[reqBufferIndex] == '\n')
		{
			reqBufferIndex++;
			chunkState = SIZE;
			state = BODY_FINISH;
			return ;
		}
		if ((reqBuffer[reqBufferIndex] != '\r' && chunkIndex == 0)
			|| chunkIndex != 0)
		{
			setHttpReqError(400, "Bad Request");
			return ;
		}
		chunkIndex++;
		reqBufferIndex++;
	}
}

void		HttpRequest::chunkedBodyParsing()
{
	if (chunkState == SIZE)
	{
		while (reqBufferIndex < reqBuffer.size())
		{
			if (sizeStr[sizeStr.size() - 1] == '\r' && reqBuffer[reqBufferIndex] != '\n')
			{
				setHttpReqError(400, "Bad Request");
				return ;
			}
			if (reqBuffer[reqBufferIndex] == '\n')
			{
				if (sizeStr[sizeStr.size() -1] == '\r')
					sizeStr = sizeStr.substr(0, sizeStr.size() - 1);
				chunkState = LINE;
				reqBufferIndex++;
				break ;
			}
			else if (!std::isdigit(reqBuffer[reqBufferIndex]) && reqBuffer[reqBufferIndex] != '\r'
				&& ( reqBuffer[reqBufferIndex] < 'A' || reqBuffer[reqBufferIndex] > 'F')
				&& ( reqBuffer[reqBufferIndex] < 'a' || reqBuffer[reqBufferIndex] > 'f'))
			{
				setHttpReqError(400, "Bad Request");
				return ;
			}
			sizeStr.push_back(reqBuffer[reqBufferIndex]);
			reqBufferIndex++;
		}
		if (chunkState == LINE)
		{
			if (convertChunkSize())
				return ;
		}
	}
	if (chunkState == LINE)
	{
		if (chunkSize == 0)
		{
			chunkEnd();
			return ;
		}
		while (reqBufferIndex < reqBuffer.size() && chunkSize > chunkIndex)
		{
			body.push_back(reqBuffer[reqBufferIndex]);
			reqBufferIndex++;
			chunkIndex++;
		}
		if (chunkSize == chunkIndex)
		{
			chunkIndex = 0 ;
			chunkState = END_LINE;
		}
	}
	while (chunkState == END_LINE && reqBufferIndex < reqBuffer.size())
	{
		if (reqBuffer[reqBufferIndex] == '\n')
			chunkState = SIZE;
		else if ((chunkIndex == 0 && reqBuffer[reqBufferIndex] != '\r')
				|| chunkIndex == 1)
		{
			setHttpReqError(400, "Bad Request");
			return ;
		}
		chunkIndex++;
		reqBufferIndex++;
	}
}

void HttpRequest::parseBody()
{
	if (firstHeadersCheck())
		return ;
	if (headers.find("Content-Length") != headers.end())
		contentLengthBodyParsing();
	if (headers.find("Transfer-Encoding") != headers.end())
		chunkedBodyParsing();
}

const std::string &HttpRequest::getHost() const
{
	return (headers.find("Host")->second);
}
