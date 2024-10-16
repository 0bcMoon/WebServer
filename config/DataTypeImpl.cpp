#include <fcntl.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <unistd.h>
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
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
}

GlobalConfig::GlobalConfig(const GlobalConfig &other)
{
	*this = other;
}
GlobalConfig &GlobalConfig::operator=(const GlobalConfig &other)
{
	if (this == &other)
		return *this;

	if (root.empty())
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

void GlobalConfig::setIndexes(Tokens &token, Tokens &end)
{
	validateOrFaild(token, end);
	while (token != end && *token != ";")
		this->indexes.push_back(consume(token, end));
	CheckIfEnd(token, end);
}

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
	else if (*token == "error_page")
		this->setErrorPages(token, end);
	else if (*token == "client_upload_path")
		this->setUploadPath(token, end);
	else
		throw ParserException("Invalid token: " + *token);
	return (true);
}


void GlobalConfig::setErrorPages(Tokens &token, Tokens &end)
{
	std::vector<std::string> vec;
	std::string content;

	std::string str;
	this->validateOrFaild(token, end);
	while (token != end && *token != ";")
		vec.push_back(this->consume(token, end));
	if (vec.size() <= 1)
		throw ParserException("Invalid error page define");
	this->CheckIfEnd(token, end);
	if (access(vec.back().data(), F_OK | R_OK) == -1)
		throw ParserException("file does not exist" + vec.back());
	for (size_t i = 0; i < vec.size() - 1; i++)
	{
		if (!this->isValidStatusCode(vec[i]))
			throw ParserException("Invalid status Code " + vec[i]);
		this->errorPages[vec[i]] = vec.back().data();
	}
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


std::string GlobalConfig::loadFile(const char *filename)
{
	std::stringstream buf;
	std::ifstream input(filename, std::ios::binary);
	if (!input)
		throw ParserException("could not open file for reading: " + std::string(filename));
	buf << input.rdbuf();
	return buf.str();
}

const std::string &GlobalConfig::getErrorPage(std::string &StatusCode)
{
	return (this->errorPages.find(StatusCode)->second); // plz don't fail;
}
