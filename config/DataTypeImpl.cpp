#include <sys/stat.h>
#include <fcntl.h>
#include <sys/unistd.h>
#include <unistd.h>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include "DataType.hpp"
#include "Debug.hpp"
#include "ParserException.hpp"
#include "WebServer.hpp"

GlobalConfig::GlobalConfig()
{
	this->autoIndex = false;
		this->methods = GET; // default method is GET only
}

GlobalConfig &GlobalConfig::operator=(const GlobalConfig &other)
{
	if (this == &other)
		return *this;
	
	accessLog = other.accessLog;
	errorLog = other.errorLog;
	root = other.root;
	autoIndex = other.autoIndex;
	errorPages = other.errorPages;
	cgiMap = other.cgiMap;
	indexes = other.indexes;
	return *this; // Return *this to allow chained assignments
}
GlobalConfig::~GlobalConfig() {}

void GlobalConfig::setRoot(Tokens &token, Tokens &end)
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

std::string GlobalConfig::getRoot() const
{
	return this->root; // Return the root
}

void GlobalConfig::setAutoIndex(Tokens &token, Tokens &end)
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

bool GlobalConfig::getAutoIndex() const
{
	return this->autoIndex; // Return autoIndex
}

// void GlobalConfig::setAccessLog(Tokens &token, Tokens &end)
// {
// 	throw ParserException("TODO");
// 	// Empty implementation
// }

// std::string GlobalConfig::getAccessLog() const
// {
// 	throw ParserException("TODO");
// 	return this->accessLog; // Return the access log path
// }

// void GlobalConfig::setErrorLog(Tokens &token, Tokens &end)
// {
// 	throw ParserException("TODO");
// 	// Empty implementation
// }

// std::string GlobalConfig::getErrorLog() const
// {
// 	throw ParserException("TODO");
// 	return this->errorLog; // Return the error log path
// }





void GlobalConfig::setIndexes(Tokens &token, Tokens &end)
{
	validateOrFaild(token, end);
	while (token != end && *token != ";")
		this->indexes.push_back(consume(token, end));
	CheckIfEnd(token, end);
}

void GlobalConfig::setCGI(Tokens &token, Tokens &end)
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
	if (access(cgi_path.c_str(), F_OK | X_OK | R_OK) == -1)
		throw ParserException("Invalid CGI path" + cgi_path);
	this->cgiMap[cgi_ext] = cgi_path;
}

// void GlobalConfig::setErrorPages(Tokens &token, Tokens &end)
// {
// 	// Empty implementation
// }

bool GlobalConfig::IsId(std::string &token)
{
	return (token == ";" || token == "}" || token == "{");
}

void GlobalConfig::validateOrFaild(Tokens &token, Tokens &end)
{
	token++;
	if (token == end || IsId(*token))
		throw ParserException("Unexpected end of file");
}

void GlobalConfig::CheckIfEnd(Tokens &token, Tokens &end)
{
	if (token == end)
		throw ParserException("Unexpected end of file");
	else if (*token != ";")
		throw ParserException("Unexpected `;` found: " + *token);
	token++;
}

std::string &GlobalConfig::consume(Tokens &token, Tokens &end)
{
	if (token == end)
		throw ParserException("Unexpected end of file");
	if (IsId(*token))
		throw ParserException("Unexpected token: " + *token);
	return *token++;
}
bool GlobalConfig::parseTokens(Tokens &token, Tokens &end)
{
	if (token == end)
		throw ParserException("Unexpected end of file");
	else if (*token == "root")
		this->setRoot(token, end);
	else if (*token == "autoindex")
		this->setAutoIndex(token, end);
	else if (*token == "index")
		this->setIndexes(token, end);
	else if (*token == "cgi_path")
		this->setCGI(token, end);
	else if (*token == "allow")
		this->setMethods(token, end);
	else if (*token == "error_pages")
		this->setErrorPages(token, end);
	else
		throw ParserException("Invalid token: " + *token);
	return (true);
}

void GlobalConfig::setMethods(Tokens &token, Tokens &end)
{
	this->validateOrFaild(token, end);
	while (token != end && *token != ";")
	{
		if (*token == "GET")
			this->methods |= GET;
		else if (*token == "POST")
			this->methods |= POST;
		else if (*token == "DELETE")
			this->methods |= DELETE;
		else
			throw ParserException("Invalid or un support method: " + *token);
		token++;
	}
	this->CheckIfEnd(token, end);
}

void GlobalConfig::setErrorPages(Tokens &token, Tokens &end)
{
	throw ParserException("TODO: with better implementation");

}


const std::vector<std::string> &GlobalConfig::getIndexes()
{
	return (this->indexes);
}
