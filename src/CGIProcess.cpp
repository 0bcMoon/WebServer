
#include "CGIProcess.hpp"
#include <sys/unistd.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <iostream>
#include "DataType.hpp"
#include "Event.hpp"

bool CGIProcess::IsFileExist()
{
	size_t offset =
		response->location->globalConfig.getAliasOffset() ? response->location->getPath().size() : 0; // offset for alais
	std::string scriptPath = response->location->globalConfig.getRoot() + response->path.substr(offset);

	if (access(scriptPath.c_str(), F_OK) == -1)
		return (this->response->setHttpResError(404, "Not Found"), 1);
	if (access(scriptPath.c_str(), R_OK) == -1)
		return (this->response->setHttpResError(403, "Forbidden"), 1);
	return (0);
}

void CGIProcess::closePipe(int fd[2])
{
	close(fd[1]);
	close(fd[0]);
}

int CGIProcess::redirectPipe()
{
	close(this->pipeIn[1]);
	close(this->pipeOut[0]);

	if (dup2(this->pipeOut[1], STDOUT_FILENO) < 0)
		return (-1);
	if (dup2(this->pipeIn[0], STDIN_FILENO) < 0)
		return (-1);
	close(this->pipeOut[1]);
	close(this->pipeIn[0]);
	return (0);
}

std::string ToEnv(std::map<std::string, std::string>::iterator &header)
{
	std::string str = "HTTP_";
	int idx = 0;
	for (size_t i = 0; i < header->first.size(); i++, idx++)
	{
		char c = header->first[i];
		if (c == '-')
			c = '_';
		str.push_back(std::toupper(c));
	}
	str.push_back('=');
	str += header->second;
	return (str);
}

void CGIProcess::loadEnv()
{
	std::map<std::string, std::string> env;
	std::stringstream ss;
	std::map<std::string, std::string>::iterator it = response->headers.begin();
	for (int i = 0; it != response->headers.end(); it++, i++)
		this->env.push_back(ToEnv(it));
	ss << response->location->getPort();
	env["SERVER_SOFTWARE"] = "macOS";
	env["GATEWAY_INTERFACE"] = "CGI/1.1";
	env["SERVER_PROTOCOL"] = "HTTP/1.1";
	env["SERVER_PORT"] = ss.str();
	env["SERVER_NAME"] = response->location->getHost();
	env["REQUEST_METHOD"] = response->strMethod;
	env["SCRIPT_NAME"] = response->path;
	env["QUERY_STRING"] = response->queryStr;
	env["REMOTE_ADDR"] = ""; // TODO:
	env["PATH_INFO"] = "/cgi";
	env["HTTP_HOST"] = response->headers["Host"];
	env["CONTENT_TYPE"] = response->headers["Content-type"];
	if (response->strMethod == "POST")
	{
		ss.clear();
		ss << response->getBody().size();
		env["CONTENT_LENGTH"] = ss.str();
	}
	else
		env["CONTENT_LENGTH"] = "0"; // -1 ??

	it = env.begin();
	for (int i = 0; it != env.end(); it++, i++)
		this->env.push_back(it->first + "=" + it->second);
}

void CGIProcess::child_process()
{
	size_t offset =
		response->location->globalConfig.getAliasOffset() ? response->location->getPath().size() : 0; // offset for alais
	this->cgi_bin = response->location->globalConfig.getRoot() + response->path.substr(offset);

	this->loadEnv();
	
	std::string path = response->location->getCGIPath("." + response->getExtension(response->path)); // INFO: make this dynamique
	const char *args[3] = {path.data(), cgi_bin.data(), NULL};
	char **argv = new char *[env.size() + 1];
	size_t i = 0;
	for (; i < env.size(); i++)
		argv[i] = (char *)this->env[i].data();
	argv[i] = NULL;
	if (redirectPipe())
	{
		closePipe(this->pipeOut);
		closePipe(this->pipeIn);
		throw std::runtime_error("child could not be run: " + std::string(strerror(errno))); 
	}
	execve(*args, (char *const *)args, argv);
	throw std::runtime_error("child process faild: execve: " + std::string(strerror(errno)));
}

GlobalConfig::Proc CGIProcess::RunCGIScript(HttpResponse &response)
{
	GlobalConfig::Proc proc;

	this->response = &response;
	if (this->IsFileExist())
		return (proc);
	if (pipe(this->pipeIn) < 0)
		return (this->response->setHttpResError(500, "Internal Server Error"), proc);
	else if (pipe(this->pipeOut) < 0)
		return (closePipe(this->pipeIn), this->response->setHttpResError(500, "Internal Server Error"), proc);

	proc.pid = fork();
	if (proc.pid < 0)
	{
		std::cout << "fork faild: " << strerror(errno) << "\n";
		closePipe(this->pipeIn);
		closePipe(this->pipeOut);
		return (this->response->setHttpResError(500, "Internal Server Error"), proc);
	}
	else if (proc.pid == 0)
		this->child_process();
	close(this->pipeIn[0]);
	close(this->pipeOut[1]);
	proc.fout = this->pipeOut[0];
	proc.fin = this->pipeIn[1];
	return (proc);
}

