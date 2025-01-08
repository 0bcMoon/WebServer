
#include "Location.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <unistd.h>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <sstream>
#include <string>
#include "DataType.hpp"
#include "Tokenizer.hpp"
Location::Location()
{
	this->cgiMap["."] = ""; // mean no cgi
	this->isRedirection = false;
	this->methods = GET; // default method is GET only can be overwritten
	this->upload_file_path = "/tmp/"; // default upload loaction if post method was allowed
}

Location &Location::operator=(const Location &location)
{
	this->path = location.path;
	this->redirect = location.redirect;
	this->globalConfig = location.globalConfig;
	this->isRedirection = location.isRedirection;
	this->cgiMap = location.cgiMap;
	this->methods = location.methods;
	this->upload_file_path = location.upload_file_path;
	return *this;
}
void Location::setPath(std::string &path)
{
	// TODO validate path
	this->path = path;
}

bool GlobalConfig::isValidStatusCode(std::string &str)
{
	std::stringstream ss;
	ss << str;
	int status;
	ss >> status;
	if (ss.fail() || !ss.eof())
		return (false);
	return (status >= 100 && status <= 599);
}

void Location::setRedirect(Tokens &token, Tokens &end)
{
	int code;
	std::stringstream ss;
	this->globalConfig.validateOrFaild(token, end);
	std::string status = this->globalConfig.consume(token, end);
	std::string url = this->globalConfig.consume(token, end);
	this->globalConfig.CheckIfEnd(token, end);
	ss << status;
	ss >> code;
	if ( ss.fail() || !ss.eof() || code < 300 || code > 399)
		throw Tokenizer::ParserException("Invalid  status: " + status + " code in Redirection: should be 3xx code");
	this->redirect.status = status;
	this->redirect.url = url;
	this->isRedirection = true;
}

void Location::parseTokens(Tokens &token, Tokens &end)
{
	if (*token == "return")
		this->setRedirect(token, end);
	else if (*token == "cgi_path")
		this->setCGI(token, end);
	else if (*token == "allow")
		this->setMethods(token, end);
	else if (*token == "client_upload_path")
		this->setUploadPath(token, end);
	else if (*token == "max_body_size")
		this->setMaxBodySize(token, end);
	else
		this->globalConfig.parseTokens(token, end);
}

void Location::setCGI(Tokens &token, Tokens &end)
{
	std::string cgi_path;
	std::string cgi_ext;

	this->globalConfig.validateOrFaild(token, end);
	cgi_ext = this->globalConfig.consume(token, end);
	cgi_path = this->globalConfig.consume(token, end);
	this->globalConfig.CheckIfEnd(token, end);
	if (cgi_ext[0] != '.' || cgi_ext.size() <= 1)
		throw Tokenizer::ParserException("Invalid CGI extension" + cgi_ext);
	if (access(cgi_path.c_str(), F_OK | X_OK | R_OK) == -1)
		throw Tokenizer::ParserException("Invalid CGI path" + cgi_path + ": " + std::string(strerror(errno)));
	this->cgiMap[cgi_ext] = cgi_path;
}

const std::string &Location::getCGIPath(const std::string &ext)
{
	std::map<std::string, std::string>::iterator kv;
	kv = this->cgiMap.find(ext); //
	if (kv == this->cgiMap.end())
		return (this->cgiMap.find(".")->second);
	return (kv->second);
}

bool Location::HasRedirection() const
{
	return (this->isRedirection);
}
const Location::Redirection &Location::getRedirection() const
{
	return (this->redirect);
}

static long long toBytes(std::string &size)
{
	const long maxValue = 8796093022208;
	long long sizeValue = 0;

	for (size_t i = 0; i < size.size() - 1; i++)
	{
		if (size[i] < '0' || size[i] > '9')
			throw Tokenizer::ParserException("Invalid size");
		sizeValue = sizeValue * 10 + size[i] - '0';
		if (sizeValue > maxValue)
			throw Tokenizer::ParserException("Size too large");
	}
	switch (size[size.size() - 1])
	{
		case 'k':
		case 'K': sizeValue *= 1024; break;
		case 'M':
		case 'm': sizeValue *= 1024 * 1024; break;
		case 'B':
		case 'b': break;
		default: return (-1);
	}
	return (sizeValue);
}
void Location::setMaxBodySize(Tokens &token, Tokens &end)
{
	this->globalConfig.validateOrFaild(token, end);
	this->maxBodySize = toBytes(*token);
	if (this->maxBodySize == -1)
		throw Tokenizer::ParserException("Invalid max body size or too large max is 100");
	token++;
	this->globalConfig.CheckIfEnd(token, end);
}
bool Location::isMethodAllowed(int method) const
{
	return ((this->methods & method) != 0); // TODO: TEST
}

void Location::setMethods(Tokens &token, Tokens &end)
{
	this->globalConfig.validateOrFaild(token, end);
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
			throw Tokenizer::ParserException("Invalid or un support method: " + *token);
		token++;
	}
	this->globalConfig.CheckIfEnd(token, end);
	if (this->methods == 0)
		throw Tokenizer::ParserException("empty  method list");
}
const std::string &Location::getPath()
{
	return (this->path);
}


const std::string &Location::getFileUploadPath()
{
	return (this->upload_file_path);
}

void Location::setUploadPath(Tokens &token, Tokens &end)
{
	struct stat buf;

	this->globalConfig.validateOrFaild(token, end);
	this->upload_file_path = this->globalConfig.consume(token, end);
	this->globalConfig.CheckIfEnd(token, end);
	if (this->upload_file_path.back() != '/')
		this->upload_file_path.push_back('/');
	if (stat(this->upload_file_path.data(), &buf) != 0)
		throw Tokenizer::ParserException("Upload path does directory does not exist");
	if (S_ISDIR(buf.st_mode) == 0)
		throw Tokenizer::ParserException("Upload Path is not a directory");
	if (access(this->upload_file_path.data(), W_OK) != 0)
		throw Tokenizer::ParserException("invalid Upload path directory");
}
