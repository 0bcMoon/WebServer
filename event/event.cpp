#include "Event.hpp"
#include <sys/_endian.h>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "CgiHandler.hpp"
#include "Client.hpp"
#include "Connections.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "VirtualServer.hpp"
#define CGI_VAR_SIZE 11

void log_file(const std::string &name)
{
	std::stringstream ss;
	ss << "lsof -p " << getpid() << " > " << name;
	system(ss.str().data());
}
Event::Event() : connections(NULL, -1), MAX_CONNECTION_QUEUE(32), MAX_EVENTS(1024)
{
	this->evList = NULL;
	this->eventChangeList = NULL;
	this->kqueueFd = -1;
	this->numOfSocket = 0;
}

Event::Event(int max_connection, int max_events, ServerContext *ctx)
	: connections(ctx, -1), MAX_CONNECTION_QUEUE(max_connection), MAX_EVENTS(max_events)
{
	this->ctx = ctx;
	this->evList = NULL;
	this->eventChangeList = NULL;
	this->kqueueFd = -1;
	this->numOfSocket = 0;
}
Event::~Event()
{
	delete this->evList;
	VirtualServerMap_t::iterator it = this->virtuaServers.begin();
	for (; it != this->virtuaServers.end(); it++)
	{
		close(it->first);
	}
	std::map<int, VirtualServer *>::iterator it2 = this->defaultServer.begin();
	for (; it2 != this->defaultServer.end(); it2++)
	{
		if (this->virtuaServers.find(it->first) == this->virtuaServers.end())
			close(it2->first);
	}
	if (this->kqueueFd >= 0)
		close(this->kqueueFd);
}

void Event::InsertDefaultServer(VirtualServer *server, int socketFd)
{
	std::map<int, VirtualServer *>::iterator dvserver = this->defaultServer.find(socketFd);
	if (dvserver != this->defaultServer.end())
	{
		if (dvserver->second->getServerNames().size() == 0 && server->getServerNames().size() == 0)
			throw std::runtime_error("conflict default server (uname server) already exist on same port");
	}
	this->defaultServer[socketFd] = server;
}

void Event::insertServerNameMap(ServerNameMap_t &serverNameMap, VirtualServer *server, int socketFd)
{
	std::set<std::string> &serverNames = server->getServerNames();
	if (serverNames.size() == 0) // if there is no server name  uname will be the
		this->InsertDefaultServer(server, socketFd); // default server in that port
	else
	{
		std::set<std::string>::iterator it = serverNames.begin();
		for (; it != serverNames.end(); it++)
		{
			if (serverNameMap.find(*it) != serverNameMap.end())
				throw std::runtime_error("confilict: server name already exist on some port host");
			serverNameMap[*it] = server;
		}
	}
	if (this->defaultServer.find(socketFd) == this->defaultServer.end())
		this->InsertDefaultServer(server, socketFd);
}

void Event::init()
{
	std::vector<VirtualServer> &virtualServers = ctx->getServers();

	for (size_t i = 0; i < virtualServers.size(); i++)
	{
		SocketAddrSet_t &socketAddr = virtualServers[i].getAddress();
		SocketAddrSet_t::iterator it = socketAddr.cbegin();
		for (; it != socketAddr.end(); it++)
		{
			int socketFd;
			if (this->socketMap.find(*it) == this->socketMap.end()) // no need to create socket if it already exists
			{
				socketFd = this->CreateSocket(it);
				this->socketMap[*it] = socketFd;
			}
			else
				socketFd = this->socketMap.at(*it);

			VirtualServerMap_t::iterator it = this->virtuaServers.find(socketFd);
			if (it == this->virtuaServers.end()) // create empty for socket map if not exist
			{
				this->virtuaServers[socketFd] = ServerNameMap_t();
				it = this->virtuaServers.find(socketFd); // take the reference of server map;
			}
			ServerNameMap_t &serverNameMap = it->second;
			this->insertServerNameMap(serverNameMap, &virtualServers[i], socketFd);
		}
	}
}

int Event::CreateSocket(SocketAddrSet_t::iterator &address)
{
	sockaddr_in address2;

	int socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFd < 0)
		throw std::runtime_error("socket creation failed: " + std::string(strerror(errno)));
	this->setNonBlockingIO(socketFd);
	int optval = 1;
	setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	memset(&address2, 0, sizeof(struct sockaddr_in));

	this->setNonBlockingIO(socketFd);

	address2.sin_family = AF_INET;
	address2.sin_addr.s_addr = htonl(address->host);
	address2.sin_port = htons(address->port);
	if (bind(socketFd, (struct sockaddr *)&address2, sizeof(address2)) < 0)
	{
		close(socketFd);
		throw std::runtime_error("bind failed: " + std::string(strerror(errno)));
	}
	this->numOfSocket++;
	this->sockAddrInMap[socketFd] = address2;
	return (socketFd);
}

std::string Event::get_readable_ip(const VirtualServer::SocketAddr address)
{
	uint32_t ip = address.host;
	int port = address.port;
	std::stringstream ss;

	int a = ip & 0xFF;
	int b = (ip >> 8) & 0xFF;
	int c = (ip >> 16) & 0xFF;
	int d = (ip >> 24) & 0xFF;
	ss << d << "." << c << "." << b << "." << a << ":" << port;
	return ss.str();
}

int Event::setNonBlockingIO(int sockfd)
{
	int flags = fcntl(sockfd, F_GETFL, 0);
	if (flags == -1)
		return (-1);
	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK | O_CLOEXEC) < 0)
		return (-1);
	return (0);
}

bool Event::Listen(int socketFd)
{
	if (listen(socketFd, this->MAX_CONNECTION_QUEUE) < 0)
		return (false);
	return (true);
}

bool Event::Listen()
{
	SocketMap_t::iterator it = this->socketMap.begin();

	for (; it != this->socketMap.end(); it++)
	{
		if (!this->Listen(it->second))
			throw std::runtime_error("could not listen: " + get_readable_ip(it->first) + " " + strerror(errno));
		std::cout << "server listen on " << this->get_readable_ip(it->first) << std::endl;
	}
	return true;
}

void Event::CreateChangeList()
{
	SockAddr_in::iterator it = this->sockAddrInMap.begin();
	// SERVER socket does not need to monitor writes
	for (int i = 0; it != this->sockAddrInMap.end(); it++, i++)
		EV_SET(&this->eventChangeList[i], it->first, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	if (kevent(this->kqueueFd, this->eventChangeList, this->numOfSocket, NULL, 0, NULL) < 0)
		throw std::runtime_error("kevent failed: could not regester server event: " + std::string(strerror(errno)));
	delete this->eventChangeList;
	this->eventChangeList = NULL;
}

void Event::initIOmutltiplexing()
{
	this->evList = new struct kevent[this->MAX_EVENTS];
	this->eventChangeList = new struct kevent[this->numOfSocket];
	this->kqueueFd = kqueue();
	if (this->kqueueFd < 0)
		throw std::runtime_error("kqueue faild: " + std::string(strerror(errno)));
	this->CreateChangeList();
}
// TODO:  i need someone to speek here (logger )
int Event::newConnection(int socketFd, Connections &connections)
{
	struct sockaddr_in address = this->sockAddrInMap[socketFd];
	socklen_t size = sizeof(struct sockaddr);
	int newSocketFd = accept(socketFd, (struct sockaddr *)&address, &size);
	if (newSocketFd < 0) // TODO: add logger here
		return (-1);
	if (this->setNonBlockingIO(newSocketFd))
		return (close(newSocketFd));

	struct kevent ev_set[2];
	EV_SET(&ev_set[0], newSocketFd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	EV_SET(&ev_set[1], newSocketFd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, NULL);

	if (kevent(this->kqueueFd, ev_set, 2, NULL, 0, NULL) < 0)
		return (close(newSocketFd)); // report faild connections
	connections.addConnection(newSocketFd, socketFd);

	return (newSocketFd);
}

// TODO: fix me may i fails
int Event::setWriteEvent(int fd, uint16_t flags)
{
	struct kevent ev;

	EV_SET(&ev, fd, EVFILT_WRITE, flags, 0, 0, NULL);
	return kevent(this->kqueueFd, &ev, 1, NULL, 0, NULL);
}

void Event::ReadEvent(const struct kevent *ev)
{
	if (ev->flags & EV_EOF && ev->data <= 0)
	{
		std::cout << "client disconnected\n";
		connections.closeConnection(ev->ident);
	}
	else
	{
		connections.requestHandler(ev->ident);
		ClientsIter kv = connections.clients.find(ev->ident);

		if (kv == connections.clients.end())
			return;
		Client *client = kv->second;
		// if (client->request.state != REQUEST_FINISH && client->request.state != REQ_ERROR
		// 	&& client->response.state != WRITE_BODY)
		// 	return;
		this->setWriteEvent(client->getFd(), EV_ENABLE);
	}
}

char *joinCstr(const std::string &s1, const std::string &s2)
{
	char *str = NULL;
	str = new char[s1.size() + s1.size() + 2];
	size_t idx = 0;
	for (size_t i = 0; i < s1.size(); i++)
		str[idx++] = s1[i];
	str[idx++] = '=';
	for (size_t i = 0; i < s2.size(); i++)
		str[idx++] = s2[i];
	str[idx] = '\0';
	return (str);
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


void loadEnv(HttpResponse &response, std::vector<std::string> &envArr)
{
	std::map<std::string, std::string> env;
	std::stringstream ss;
	std::map<std::string, std::string>::iterator it = response.headers.begin();
	for (int i = 0; it != response.headers.end(); it++, i++)
		envArr.push_back(ToEnv(it));
	ss << response.location->getPort();
	env["SERVER_SOFTWARE"] = "macOS";
	env["GATEWAY_INTERFACE"] = "CGI/1.1";
	env["SERVER_PROTOCOL"] = "HTTP/1.1";
	env["SERVER_PORT"] = ss.str();
	env["SERVER_NAME"] = response.location->getHost();
	env["REQUEST_METHOD"] = response.strMethod;
	env["SCRIPT_NAME"] = response.path;
	env["QUERY_STRING"] = response.queryStr;
	env["REMOTE_ADDR"] = ""; // TODO:
	env["PATH_INFO"] = "/cgi";
	env["HTTP_HOST"] = response.headers["Host"];
	env["CONTENT_TYPE"] = response.headers["Content-type"];
	if (response.strMethod == "POST")
	{
		ss.clear();
		ss << response.getBody().size();
		env["CONTENT_LENGTH"] = ss.str();
	}
	else
		env["CONTENT_LENGTH"] = "0"; // -1 ??

	it = env.begin();
	for (int i = 0; it != env.end(); it++, i++)
		envArr.push_back(it->first + "=" + it->second);
}
bool Event::IsFileExist(HttpResponse &response)
{
	size_t offset =
		response.location->globalConfig.getAliasOffset() ? response.location->getPath().size() : 0; // offset for alais
	std::string scriptPath = response.location->globalConfig.getRoot() + response.path.substr(offset);

	if (access(scriptPath.c_str(), F_OK) == -1)
		return (response.setHttpResError(404, "Not Found"), 1);
	if (access(scriptPath.c_str(), R_OK) == -1)
		return (response.setHttpResError(403, "Forbidden"), 1);
	return (0);
}

void closePipe(int fd[2])
{
	close(fd[1]);
	close(fd[0]);
}

int redirectPipe(int pipeIn[2], int pipeOut[2])
{
	close(pipeIn[1]);
	close(pipeOut[0]);

	if (dup2(pipeOut[1], STDOUT_FILENO) < 0)
		return (-1);
	if (dup2(pipeIn[0], STDIN_FILENO) < 0)
		return (-1);
	close(pipeOut[1]);
	close(pipeIn[0]);
	return (0);
}

void child_process(HttpResponse &response, int pipeIn[2], int pipeOut[2])
{
	size_t offset =
		response.location->globalConfig.getAliasOffset() ? response.location->getPath().size() : 0; // offset for alais
	std::string scriptPath = response.location->globalConfig.getRoot() + response.path.substr(offset);

	std::vector<std::string> env;
	loadEnv(response, env);

	std::string path = response.location->getCGIPath(".php");
	const char *args[3] = {path.data(), scriptPath.data(), NULL};
	char **argv = new char *[env.size() + 1];
	size_t i = 0;
	for (; i < env.size(); i++)
		argv[i] = (char *)env[i].data();
	argv[i] = NULL;
	if (redirectPipe(pipeIn, pipeOut))
	{
		closePipe(pipeOut);
		closePipe(pipeIn);
		throw std::runtime_error("Child RunTime\n");
	}
	execve(*args, (char *const *)args, argv);
	std::cerr << "your should't be here mister\n" << strerror(errno) << "\n";
	throw std::runtime_error("child process faild");
}
Event::Proc Event::RunCGIScript(HttpResponse &response)
{
	Event::Proc proc;
	int pipeIn[2], pipeOut[2];

	if (this->IsFileExist(response))
		return proc;
	if (pipe(pipeIn) < 0)
		return (response.setHttpResError(500, "Internal Server Error"), Proc());
	else if (pipe(pipeOut) < 0)
		return (closePipe(pipeIn), response.setHttpResError(500, "Internal Server Error"), Proc());

	proc.pid = fork();
	if (proc.pid < 0)
	{
		closePipe(pipeIn);
		closePipe(pipeOut);
		return (response.setHttpResError(500, "Internal Server Error"), Proc());
	}
	else if (proc.pid == 0)
		child_process(response, pipeIn, pipeOut);
	close(pipeIn[0]);
	close(pipeOut[1]);
	proc.fout = pipeOut[0];
	proc.fin = pipeIn[1];
	return (proc);
}

void Event::RegesterNewProc(HttpResponse &response)
{
	Event::Proc proc = RunCGIScript(response);
	if (proc.pid == -1)
		return ;
	struct kevent ev[3];
	EV_SET(&ev[0], proc.pid, EVFILT_PROC, NOTE_EXIT, 0, 0, NULL);
	EV_SET(&ev[1], proc.fin, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, (void *)PIPE);
	EV_SET(&ev[0], proc.fout, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void *)PIPE);
	if (kevent(this->kqueueFd, ev, 3, 0, 0, NULL) < 0)
		response.setHttpResError(500, "Internal Server Error");
}

void Event::WriteEvent(const struct kevent *ev)
{
	if (ev->flags & EV_EOF)
		connections.closeConnection(ev->ident);
	else
	{
		ClientsIter kv = connections.clients.find(ev->ident);
		if (kv == connections.clients.end())
			return;
		Client *client = kv->second;
		client->request.feed();
		if (client->request.state != REQUEST_FINISH && client->request.state != REQ_ERROR
			&& client->response.state != WRITE_BODY)
			return;
		if (client->response.state != WRITE_BODY)
		{
			client->response.location = this->getLocation(client);
			client->respond(ev->data);
		}
		if (client->response.state == CGI_EXECUTING)
		{
			this->RegesterNewProc(client->response);
			// client->getCGIEnv();
			// CgiHandler cgi(client->response);
			// cgi.initEnv();
			// cgi.envMapToArr(cgi.)
		}
		if (client->response.state == WRITE_BODY)
		{
			client->response.eventByte = ev->data;
			client->response.sendBody(-1, client->response.bodyType);
		}
		if (client->response.state != WRITE_BODY)
		{
			client->response.clear();
			client->request.clear();
			if (client->request.eof)
				this->setWriteEvent(ev->ident, EV_DISABLE);
		}
	}
}
void Event::eventLoop()
{
	connections.init(this->ctx, this->kqueueFd);
	int nev;

	while (1)
	{
		std::cout << "Waiting for event\n";

		nev = kevent(this->kqueueFd, NULL, 0, this->evList, MAX_EVENTS, NULL);
		if (nev < 0)
			throw std::runtime_error("kevent failed: " + std::string(strerror(errno)));
		for (int i = 0; i < nev; i++)
		{
			const struct kevent *ev = &this->evList[i];
			if (this->checkNewClient(ev->ident))
			{
				this->newConnection(ev->ident, connections);
				continue;
			}
			try
			{
				if (ev->filter == EVFILT_READ)
					this->ReadEvent(ev);
				else if (ev->filter == EVFILT_WRITE)
					this->WriteEvent(ev);
				else if (ev->filter == EVFILT_PROC)
					;
				else if (ev->filter == EVFILT_TIMER)
					;
				else
					throw std::runtime_error("Errror unkonw event\n");
			}
			catch (HttpResponse::IOException &e)
			{
				this->connections.closeConnection(ev->ident);
				std::cout << e.what() << "\n";
			}
			catch (std::bad_alloc &e)
			{
				this->connections.closeConnection(ev->ident);
				std::cout << e.what() << "\n";
			}
		}
	}
}

bool Event::checkNewClient(int socketFd)
{
	return (this->sockAddrInMap.find(socketFd) != this->sockAddrInMap.end());
}

Location *Event::getLocation(const Client *client)
{
	VirtualServer *Vserver;
	int serverfd;
	bool IsDefault = true;

	struct sockaddr_in addr = this->sockAddrInMap.find(client->getServerFd())->second;
	int port = ntohs(addr.sin_port);
	serverfd = client->getServerFd();
	const std::string &path = client->getPath();
	std::string host = client->getHost();
	ServerNameMap_t serverNameMap = this->virtuaServers.find(serverfd)->second; // always exist
	ServerNameMap_t::iterator _Vserver = serverNameMap.find(host);
	if (_Vserver == serverNameMap.end())
		Vserver = this->defaultServer.find(client->getServerFd())->second,
		IsDefault = false; // plz dont fail all my hope on you
	else
		Vserver = _Vserver->second;
	Location *location = Vserver->getRoute(path);
	if (!location)
		return (NULL);
	else if (!IsDefault)
		host = *Vserver->getServerNames().begin();
	location->setHostPort(host, port);
	return (location);
}
Event::Proc::Proc()
{
	this->fin = -1l;
	this->fout = -1l;
	this->pid = -1l;
}
