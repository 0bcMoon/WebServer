#include <fcntl.h>
#include <sys/stat.h>
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
	this->upload_file_path = "/tmp";
	this->autoIndex = false;
	this->methods = GET; // default method is GET only can be overwritten
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
		throw ParserException("Unexpected token: " + (token == end ? "end of file" : *token));
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
	else if (*token == "allow")
		this->setMethods(token, end);
	else if (*token == "error_pages")
		this->setErrorPages(token, end);
	else if (*token == "client_upload_path")
		this->setUploadPath(token, end);
	else
		throw ParserException("Invalid token: " + *token);
	return (true);
}

void GlobalConfig::setMethods(Tokens &token, Tokens &end)
{
	this->validateOrFaild(token, end);
	this->methods = 0;
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
	if (this->methods == 0)
		throw ParserException("empty  method list");
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

void GlobalConfig::setUploadPath(Tokens &token, Tokens &end)
{
	struct stat buf;

	this->validateOrFaild(token, end);
	this->upload_file_path = this->consume(token, end);
	this->CheckIfEnd(token, end);
	if (stat(this->upload_file_path.data(), &buf) != 0)
		throw ParserException("Upload path does directory does not exist");
	if (S_ISDIR(buf.st_mode) == 0)
		throw ParserException("Upload Path is not a directory");
	if (access(this->upload_file_path.data(), W_OK) != 0)
		throw ParserException("invalid Upload path directory");
}

bool GlobalConfig::isMethodAllowed(int method) const
{
	if (this->methods & method) return (true);
	return (false);
}
