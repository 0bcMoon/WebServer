#include <sys/stat.h>
#include <iostream>
#include "DataType.hpp"
#include "Debug.hpp"
#include "ParserException.hpp"
#include "WebServer.hpp"

GlobalParam::GlobalParam()
{
	this->autoIndex = false;
	this->maxBodySize = 100 * 1024 * 1024;
	this->maxHeaderSize = 100 * 1024 * 1024;
}

GlobalParam::~GlobalParam() {}

void GlobalParam::setRoot(Tokens &token, Tokens &end)
{
	struct stat buf;

	validateOrFaild(token, end);
	this->root = consume(token, end);
	if (stat(this->root.c_str(), &buf) != 0)
		throw ParserException("Root directory does not exist");
	if (S_ISDIR(buf.st_mode) == 0)
		throw ParserException("Root is not a directory");
	CheckIfEnd(token, end);
}

std::string GlobalParam::getRoot() const
{
	return this->root; // Return the root
}

void GlobalParam::setAutoIndex(Tokens &token, Tokens &end)
{
	validateOrFaild(token, end);

	if (*token == "on")
		this->autoIndex = true;
	else if (*token == "off")
		this->autoIndex = false;
	else
		throw ParserException("Invalid value for autoindex");
	token++;

	CheckIfEnd(token, end);
}

bool GlobalParam::getAutoIndex() const
{
	return this->autoIndex; // Return autoIndex
}

// void GlobalParam::setAccessLog(Tokens &token, Tokens &end)
// {
// 	throw ParserException("TODO");
// 	// Empty implementation
// }

// std::string GlobalParam::getAccessLog() const
// {
// 	throw ParserException("TODO");
// 	return this->accessLog; // Return the access log path
// }

// void GlobalParam::setErrorLog(Tokens &token, Tokens &end)
// {
// 	throw ParserException("TODO");
// 	// Empty implementation
// }

// std::string GlobalParam::getErrorLog() const
// {
// 	throw ParserException("TODO");
// 	return this->errorLog; // Return the error log path
// }

static long toBytes(std::string &size)
{
	long long sizeValue = 0;

	for (size_t i = 0; i < size.size() - 1; i++)
	{
		if (size[i] < '0' || size[i] > '9')
			throw ParserException("Invalid size");
		sizeValue = sizeValue * 10 + size[i] - '0';
		if (sizeValue > (1 << 16))
			throw ParserException("Size too large");
	}
	switch (size[size.size() - 1])
	{
		case 'k':
		case 'K': sizeValue *= 1024; break;
		case 'M':
		case 'm': sizeValue *= 1024 * 1024; break;
		default: return (-1);
	}
	if (sizeValue > MAX_REQ_SIZE) // max size 30M;
		return (-1);
	return (sizeValue);
}

void GlobalParam::setMaxBodySize(Tokens &token, Tokens &end)
{
	validateOrFaild(token, end);
	this->maxBodySize = toBytes(*token);
	if (this->maxBodySize == -1)
		throw ParserException("Invalid max body size or too large max is 100");
	token++;
	CheckIfEnd(token, end);
}

long GlobalParam::getMaxBodySize() const
{
	return this->maxBodySize; // Return max body size
}

void GlobalParam::setMaxHeaderSize(Tokens &token, Tokens &end)
{
	validateOrFaild(token, end);
	this->maxHeaderSize = toBytes(*token);

	if (this->maxHeaderSize == -1)
		throw ParserException("Invalid max header size or too large max is 100");
	token++;
	CheckIfEnd(token, end);
}

long GlobalParam::getMaxHeaderSize() const
{
	return this->maxHeaderSize; // Return max header size
}

void GlobalParam::setIndexes(Tokens &token, Tokens &end)
{
	validateOrFaild(token, end);
	while (token != end && *token != ";")
		this->indexes.push_back(consume(token, end));
	CheckIfEnd(token, end);
}

void GlobalParam::setCGI(Tokens &token, Tokens &end)
{
	std::string cgi_path;
	std::string cgi_ext;

	validateOrFaild(token, end);
	cgi_ext = consume(token, end);
	cgi_path = consume(token, end);
	CheckIfEnd(token, end);
	if (cgi_ext[0] != '.')
		throw ParserException("Invalid CGI extension" + cgi_ext);
	if (this->cgiMap.find(cgi_ext) != this->cgiMap.end())
		throw ParserException("Duplicate CGI extension" + cgi_ext);
	this->cgiMap[cgi_ext] = cgi_path;
}

// void GlobalParam::setErrorPages(Tokens &token, Tokens &end)
// {
// 	// Empty implementation
// }

bool GlobalParam::IsId(std::string &token)
{
	return (token == ";" || token == "}" || token == "{");
}

void GlobalParam::validateOrFaild(Tokens &token, Tokens &end)
{
	token++;
	if (token == end || IsId(*token))
		throw ParserException("Unexpected end of file");
}

void GlobalParam::CheckIfEnd(Tokens &token, Tokens &end)
{
	if (token == end )
		throw ParserException("Unexpected end of file");
	else if (*token != ";")
		throw ParserException("Unexpected `;` found: " + *token);
	token++;
}

std::string GlobalParam::consume(Tokens &token, Tokens &end)
{
	if (token == end)
		throw ParserException("Unexpected end of file");
	if (IsId(*token))
		throw ParserException("Unexpected token: " + *token);
	return *token++;
}
bool GlobalParam::parseTokens(Tokens &token, Tokens &end)
{
	if (token == end)
		throw ParserException("Unexpected end of file");
	else if (*token == "root")
		this->setRoot(token, end);
	else if (*token == "autoindex")
		this->setAutoIndex(token, end);
	else if (*token == "index")
		this->setIndexes(token, end);
	else if (*token == "max_body_size")
		this->setMaxBodySize(token, end);
	else if (*token == "max_header_size")
		this->setMaxHeaderSize(token, end);
	else if (*token == "cgi_path")
		this->setCGI(token, end);
	else
		throw ParserException("Invalid token: " + *token);
	return (true);
}
