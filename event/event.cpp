#include "Event.hpp"
#include <sys/_endian.h>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "CGIProcess.hpp"
#include "Client.hpp"
#include "Connections.hpp"
#include "DataType.hpp"
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
		return (std::cout << "-----accept faild--------\n", -1);
	if (this->setNonBlockingIO(newSocketFd))
		return (close(newSocketFd), std::cout << "-----accept faild--------\n", -1);

	struct kevent ev_set[2];
	EV_SET(&ev_set[0], newSocketFd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	EV_SET(&ev_set[1], newSocketFd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, NULL);

	if (kevent(this->kqueueFd, ev_set, 2, NULL, 0, NULL) < 0)
		return (close(newSocketFd)); // report faild connections
	connections.addConnection(newSocketFd, socketFd);

	return (newSocketFd);
}

// TODO: fix me may i fails
void Event::setWriteEvent(int fd, uint16_t flags)
{
	struct kevent ev;
	EV_SET(&ev, fd, EVFILT_WRITE, flags, 0, 0, NULL);
	if(kevent(this->kqueueFd, &ev, 1, NULL, 0, NULL) < 0)
		throw Event::EventExpection("kevent faild:" + std::string(strerror(errno)));
}
void Event::ReadEvent(const struct kevent *ev)
{
	if (ev->flags & EV_EOF && ev->data <= 0)
	{
		// std::cout << "client disconnected\n";
		connections.closeConnection(ev->ident);
	}
	else
	{
		connections.requestHandler(ev->ident, ev->data);
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

void Event::RegisterNewProc(Client *client)
{
	HttpResponse &response = client->response;
	// std::cout << " a process event has been regster\n";
	CGIProcess cgi;
	GlobalConfig::Proc proc = cgi.RunCGIScript(response);
	if (proc.pid == -1)
		return;
	struct kevent ev[4];
	int evSize = (response.strMethod == "POST") + 3;
	EV_SET(&ev[0], proc.pid, EVFILT_PROC, EV_ADD | EV_ENABLE, NOTE_EXIT, 0, (void *)(size_t)client->getFd());
	EV_SET(
		&ev[1],
		proc.pid,
		EVFILT_TIMER,
		EV_ADD | EV_ENABLE | EV_ONESHOT,
		NOTE_SECONDS,
		this->ctx->getCGITimeOut(),
		(void *)(size_t)client->getFd());
	EV_SET(&ev[2], proc.fout, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void *)(size_t)client->getFd());
	EV_SET(&ev[3], proc.fin, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, (void *)(size_t)client->getFd());
	if (kevent(this->kqueueFd, ev, evSize, 0, 0, NULL) < 0)
	{
		proc.die();
		proc.clean();
		response.setHttpResError(500, "Internal Server Error");
		throw Event::EventExpection("kevent faild:" + std::string(strerror(errno)));
	}
	client->proc = proc;
	this->setWriteEvent(client->getFd(), EV_DISABLE);
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
		// client->request.feed();
		// if (client->request.state != REQUEST_FINISH && client->request.state != REQ_ERROR
		// 	&& client->response.state != WRITE_BODY && client->response.state iCGI_EXECUTING)
		// 	return;
		// std::cout << "path: " << client->request.data[0]->path << std::endl;
		// std::cout << "strMethode: " << client->request.data[0]->strMethode << std::endl;
		// std::cout << "-------------------------------------------------------" << std::endl;
		// write(1, client->request.data[0]->body.data() , client->request.data[0]->body.size());
		// std::cout << "\n-------------------------------------------------------" << std::endl;
		if (client->request.data.size() == 0 || (client->request.data[0]->state != REQUEST_FINISH
			&& client->request.data[0]->state != REQ_ERROR))
			return ;
		if (client->response.state == START)
		{
			client->response.location = this->getLocation(client);
			client->respond(ev->data, 0);
		}
		if (client->response.state == UPLOAD_FILES)
		{
			client->response.eventByte = ev->data;	
			client->response.uploadFile();
		}
		if (client->response.state == CGI_EXECUTING)
			return this->RegisterNewProc(client);
		if (client->response.state == WRITE_BODY)
		{
			client->response.eventByte = ev->data;
			client->response.sendBody(-1, client->response.bodyType);
		}
		if (client->response.state == UPLOAD_FILES)
			client->response.uploadFile();
		if (client->response.state == ERROR)
			client->handleResError();
		if (client->response.state == END_BODY)
		{
			client->response.clear();
			delete client->request.data[0];
			client->request.data.erase(client->request.data.begin());
			if (client->request.data.size() == 0)
				this->setWriteEvent(ev->ident, EV_DISABLE);
		}
	}
}

void Event::rpipe(const struct kevent *ev)
{
	int fd = (size_t)ev->udata;
	Client *client = this->connections.getClient(fd);
	if (!client)
		return (void)(std::cout << "client has been free");
	HttpResponse *response = &client->response;
	if (ev->flags & EV_EOF && !ev->data)
		return ;
	size_t OldSize = response->CGIOutput.size();
	size_t NewSize = ev->data + OldSize;
	response->CGIOutput.resize(NewSize);
	// add extra logique if a internal server Error happend
	int r = read(ev->ident, response->CGIOutput.data() + OldSize, ev->data);
	if (r < 0)
		throw HttpResponse::IOException();
}

void Event::wpipe(const struct kevent *ev)
{
	// add some writes offset that does cl
	// std::cout << "WriteEvent pipe enter\n"; // TODO: test with post
	int fd = (size_t)ev->udata;
	Client *client = this->connections.getClient(fd);
	if (!client)
		return (void)(std::cout << "client has been free3\n");
	HttpResponse *response = &client->response;
	GlobalConfig::Proc &proc = client->proc;

	int avdata = std::abs(proc.woffset - (int)response->getBody().size());
	int wdata = std::min((int)ev->data, avdata);
	proc.woffset += wdata;
	if (write(ev->ident, response->getBody().data() + proc.woffset, wdata) < 0)
		throw HttpResponse::IOException();
}

void Event::TimerEvent(const struct kevent *ev)
{
	std::cout << "timer event\n";
	exit(3);
	if (!ev->udata)
		return this->connections.closeConnection(ev->ident); // if client take too long to send full request
	exit(1);
	int fd = (size_t)ev->udata;
	Client *client = this->connections.getClient(fd);
	if (!client)
		return (void)(std::cout << "client has been free1\n");
	GlobalConfig::Proc &proc = client->proc;
	// else cgi take too long to process it data
	proc.state = GlobalConfig::Proc::TIMEOUT; // set the state for error TIMEOUT to response
	proc.die(); // we kill the cgi with SIGKILL
}

int Event::waitProc(int pid)
{
	int status;
	int signal;
	waitpid(pid, &status, 0); // this event only run if process has finish so witpid would not block
	signal = WIFSIGNALED(status); // check if process exist normally
	status = WEXITSTATUS(status); // check exist state
	struct kevent event;
	EV_SET(&event, pid, EVFILT_TIMER, EV_DELETE, 0, 0, NULL);
	int r = kevent(this->kqueueFd, &event, 1, NULL, 0, NULL);
	assert(r >= 0 && "Major error need to be fixed");
	return (signal | status);
}

void Event::ProcEvent(const struct kevent *ev)
{
	int state = this->waitProc(ev->ident);
	int fd = (size_t)ev->udata;
	Client *client = this->connections.getClient(fd);
	if (!client)
		return (void)(std::cerr << "client got dead before cgi process\n");
	this->setWriteEvent(client->getFd(), EV_ENABLE);
	HttpResponse *response = &client->response;
	GlobalConfig::Proc &proc = client->proc;
	if (proc.state == GlobalConfig::Proc::TIMEOUT)
		response->setHttpResError(504, "Gateway Timeout");
	else if (state)
		response->setHttpResError(503, "Bad Gateway"); // this may be change to 499 server error
	proc.clean();
}
void Event::eventLoop()
{
	connections.init(this->ctx, this->kqueueFd);
	int nev;
	while (1)
	{
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
				if (ev->fflags & EV_ERROR)
					connections.closeConnection(ev->ident);
				else if (ev->filter == EVFILT_READ && ev->udata)
					rpipe(ev);
				else if (ev->filter == EVFILT_WRITE && ev->udata)
					wpipe(ev);
				else if (ev->filter == EVFILT_READ)
					this->ReadEvent(ev);
				else if (ev->filter == EVFILT_WRITE)
					this->WriteEvent(ev);
				else if (ev->filter == EVFILT_PROC)
					this->ProcEvent(ev);
				else if (ev->filter == EVFILT_TIMER)
					this->TimerEvent(ev);
				else
					throw std::runtime_error("Errror unkonw event\n");
			}
			catch (HttpResponse::IOException &e)
			{
				std::cerr << e.what() << " :" << strerror(errno) << "\n";
				this->connections.closeConnection(ev->ident);
			}
			catch (Event::EventExpection &e)
			{
				std::cout << e.what() << "\n";
				this->connections.closeConnection(ev->ident);
			}
			catch (std::bad_alloc &e)
			{
				std::cout <<"memory faild: "<< e.what() << "\n";
				this->connections.closeConnection(ev->ident);
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
		IsDefault = false; // plz dont fail all my hope on you // crying like a little bitch, write a proper code like men WARNING
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

// void Event::StartTimer(Client *client)
// {
// 	int time;
// 	struct kevent ev;

// 	switch (client->getTimerType())
// 	{
// 		case Client::NEW_CONNECTION:
// 		case Client::KEEP_ALIVE:
// 			time = this->ctx->getKeepAliveTime();
// 			break;
// 		case Client::READING:
// 			time = this->ctx->getClientReadTime();
// 			break;
// 		}
// 		EV_SET(&ev, client->getFd(), EVFILT_TIMER, EV_ADD | EV_ENABLE | EV_ONESHOT, NOTE_SECONDS, time, NULL);
// 	// if (kevent(this->kqueueFd, ev, 1, NULL, 1, NULL) < 0)
// 	// 	throw
//
// }

Event::EventExpection::EventExpection(const std::string &msg) throw()
{
	this->msg = msg;
}

const char *Event::EventExpection::what() const throw()
{
	return (this->msg.data());
}

Event::EventExpection::~EventExpection() throw() {}
