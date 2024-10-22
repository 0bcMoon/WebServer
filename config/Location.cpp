
#include "Location.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <unistd.h>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <string>
#include "DataType.hpp"
#include "ParserException.hpp"
Location::Location() 
{
	this->cgiMap["."] = "";	
	this->isRedirection = false;
	this->methods = GET; // default method is GET only can be overwritten
}

Location &Location::operator=(const Location &location)
{
	this->path = location.path;
	this->redirect = location.redirect;
	this->globalConfig = location.globalConfig;
	this->isRedirection = location.isRedirection;
	this->cgiMap = location.cgiMap;
	this->methods = location.methods;
	return *this;
}
void Location::setPath(std::string &path)
{
	// TODO validate path
	// may be support encode
	this->path = path;
}

bool GlobalConfig::isValidStatusCode(std::string &str)
{
	int status = 0;
	if (str.size() != 3)
		return (false);
	for (size_t i = 0; i < str.size(); i++)
	{
		if (str[i] < '0' || str[i] > '9')
			return (false);
		status = status * 10 + str[i] - 48;
		if (status > 999)
			return (false);
	}
	return (status >= 100 && status <= 599);
}

void Location::setRedirect(Tokens &token, Tokens &end)
{
	std::vector<std::string> vec;
	this->globalConfig.validateOrFaild(token, end);
	while (token != end && *token != ";")
		vec.push_back(this->globalConfig.consume(token, end));
	this->globalConfig.CheckIfEnd(token, end);
	if (vec.size() != 3)
		throw ParserException("invalid redirects argument: usage return status url|None Body|None");
	redirect.body = vec[2];
	redirect.url = vec[1];
	redirect.status = vec[0];
	if (redirect.body == "None" && redirect.url == "None")
		throw ParserException("Body or redirection url most be define in redirection");
	else if (redirect.body != "None" && access(redirect.body.data(), F_OK | R_OK) != 0)
		throw ParserException("invalid file in Redirection: " + redirect.body + " " + std::string(strerror(errno)));
	else if (!this->globalConfig.isValidStatusCode(redirect.status) || redirect.status[0] != '3')
		throw ParserException("Invalid Redirection Status Code: " + *token);
	if (redirect.body == "None")
		redirect.body.clear();
	if (redirect.url == "None")
		redirect.url.clear();

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
	else
		this->globalConfig.parseTokens(token, end);
}

void Location::setCGI(Tokens &token, Tokens &end)
{

	std::string							cgi_path;
	std::string							cgi_ext;
	this->globalConfig.validateOrFaild(token, end);
	cgi_ext = this->globalConfig.consume(token, end);
	cgi_path = this->globalConfig.consume(token, end);
	this->globalConfig.CheckIfEnd(token, end);
	if (cgi_ext[0] != '.' || cgi_ext.size() <= 1)
		throw ParserException("Invalid CGI extension" + cgi_ext);
	if (access(cgi_path.c_str(), F_OK | X_OK | R_OK) == -1)
		throw ParserException("Invalid CGI path" + cgi_path + ": " + std::string(strerror(errno)));
	this->cgiMap[cgi_ext] = cgi_path;
}

const std::string &Location::geCGItPath(const std::string &ext)
{
	
	std::map<std::string, std::string>::iterator	kv;
	kv = this->cgiMap.find(ext);
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
			throw ParserException("Invalid or un support method: " + *token);
		token++;
	}
	this->globalConfig.CheckIfEnd(token, end);
	if (this->methods == 0)
		throw ParserException("empty  method list");
}
const std::string &Location::getPath()
{
	return (this->path);
}
void Location::setINFO(const std::string &host, int port)
{
	this->host = host;
	this->port = port;
}

int Location::getPort() const
{
	return (this->port);
}

const std::string& Location::getHost() const
{
	return (this->host);
}

const std::string &Location::getFileUploadPath()
{
	return (this->upload_file_path);
}
