#include "Event.hpp"
#include <netdb.h>
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
	int optval = 1;

	struct addrinfo *result, hints;
	hints.ai_family = AF_INET; // Allow IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP socket
	hints.ai_flags = AI_PASSIVE; // For binding
	hints.ai_protocol = IPPROTO_TCP; // Any protocol
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	int r = getaddrinfo(address->host.data(), address->port.data(), &hints, &result);
	if (r != 0)
		throw std::runtime_error("Error :getaddrinfo: " + std::string(gai_strerror(r)));

	int socket_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (socket_fd < 0)
	{
		freeaddrinfo(result);
		throw std::runtime_error("Error create socket: " + std::string(strerror(errno)));
	}
	if (this->setNonBlockingIO(socket_fd) < 0)
	{
		freeaddrinfo(result);
		throw std::runtime_error("Error could not set setNonBlockingIO: " + std::string(strerror(errno)));
	}
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)))
	{
		freeaddrinfo(result);
		throw std::runtime_error("Error could not set socket option: " + std::string(strerror(errno)));
	}
	if (bind(socket_fd, result->ai_addr, result->ai_addrlen) < 0)
	{
		freeaddrinfo(result);
		throw std::runtime_error("Error could not bind socket: " + std::string(strerror(errno)));
	}
	this->numOfSocket++;
	this->sockAddrInMap[socket_fd] = *result->ai_addr;
	freeaddrinfo(result);
	return (socket_fd);
}

int Event::setNonBlockingIO(int sockfd)
{
	int flags = fcntl(sockfd, F_GETFL, 0);
	if (flags == -1)
		return (-1);
	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK | O_CLOEXEC) < 0)
		return (-1);
	int rcvbuf_size = BUFFER_SIZE;
	int result = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf_size, sizeof(rcvbuf_size));
	if (result < 0)
	{
		std::cout << "faild cause -- " << strerror(errno) << "\n";
		return (-1);
	}
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
			throw std::runtime_error(
				"could not listen: " + it->first.host + ":" + it->first.port + " " + strerror(errno));
		std::cout << "server listen on " << it->first.host + ":" + it->first.port << std::endl;
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
	struct sockaddr address = this->sockAddrInMap[socketFd];
	socklen_t size = sizeof(struct sockaddr);
	int newSocketFd = accept(socketFd, &address, &size);
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
	if (kevent(this->kqueueFd, &ev, 1, NULL, 0, NULL) < 0)
		throw Event::EventExpection("kevent faild:" + std::string(strerror(errno)));
}
void Event::ReadEvent(const struct kevent *ev)
{
	if (ev->flags & EV_EOF && ev->data <= 0)
	{
		connections.closeConnection(ev->ident);
	}
	else
	{
		Client *client = connections.requestHandler(ev->ident, ev->data);
		if (!client)
			return;
		if (client->request.state >= HEADER_NAME)
		{
			client->request.location = this->getLocation(client);
			client->request.validateRequestLine();
		}
		client->request.feed();
		this->setWriteEvent(client->getFd(), EV_ENABLE);
	}
}

void Event::RegisterNewProc(Client *client)
{
	std::cout << "new cgi has been run\n";
	HttpResponse &response = client->response;
	CGIProcess cgi;
	Proc proc = cgi.RunCGIScript(response);
	if (proc.pid == -1)
		return;
	struct kevent ev[3];
	int evSize = 3;
	EV_SET(&ev[0], proc.pid, EVFILT_PROC, EV_ADD | EV_ENABLE, NOTE_EXIT, 0, (void *)(size_t)proc.pid);
	EV_SET(
		&ev[1],
		proc.pid,
		EVFILT_TIMER,
		EV_ADD | EV_ENABLE | EV_ONESHOT,
		NOTE_SECONDS,
		this->ctx->getCGITimeOut(),
		(void *)(size_t)proc.pid);
	EV_SET(&ev[2], proc.fout, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, (void *)(size_t)proc.pid);
	if (kevent(this->kqueueFd, ev, evSize, 0, 0, NULL) < 0)
	{
		proc.die();
		proc.clean();
		response.setHttpResError(500, "Internal Server Error");
		throw Event::EventExpection("kevent faild:" + std::string(strerror(errno)));
	}
	client->cgi_pid = proc.pid;
	proc.client = client->getFd();
	this->procs[proc.pid] = proc;
	this->setWriteEvent(client->getFd(), EV_DISABLE);
}

void Event::WriteEvent(const struct kevent *ev)
{
	if (ev->flags & EV_EOF && ev->data <= 0)
	{
		connections.closeConnection(ev->ident);
		return;
	}
	ClientsIter kv = connections.clients.find(ev->ident);
	if (kv == connections.clients.end())
		return;
	Client *client = kv->second;
	if (client->request.data.size() == 0
			|| (client->request.data[0]->state != REQUEST_FINISH && client->request.data[0]->state != REQ_ERROR))
		return;
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
	if (client->response.state == START_CGI_RESPONSE)
		client->respond(ev->data, 0);
	if (client->response.state == CGI_EXECUTING)
		return this->RegisterNewProc(client);
	if (client->response.state == WRITE_BODY)
	{
		client->response.eventByte = ev->data;
		client->response.sendBody(-1, client->response.bodyType);
	}
	if (client->response.state == ERROR)
	{
		client->handleResponseError();
	}
	if (client->response.state == END_BODY)
	{
		client->response.clear();
		delete client->request.data[0];
		client->request.data.erase(client->request.data.begin());
		if (client->request.data.size() == 0)
			this->setWriteEvent(ev->ident, EV_DISABLE);
	}
}

void Event::rpipe(const struct kevent *ev)
{
	const char seq[4] = {'\r', '\n', '\r', '\n'};
	if (ev->flags & EV_EOF && !ev->data)
		return (void)close(ev->ident);
	ProcMap_t::iterator p = this->procs.find((size_t)ev->udata);
	if (p == this->procs.end())
		return;
	Proc &proc = p->second;

	Client *client = this->connections.getClient(proc.client);
	if (!client)
		return (void)(std::cout << "client has run away << " << proc.client << "\n");
	HttpResponse *response = &client->response;
	// response->responseFd = -1; // Body to set
	int read_size = std::min(ev->data, CGI_BUFFER_SIZE);
	int r = read(ev->ident, proc.buffer.data() + proc.offset, read_size); // create a event buffer
	if (r < 0)
		return response->setHttpResError(500, "Internal server Error"); // kill cgi
	else if (r == 0)
		return;
	assert(r == read_size && "this should't happend");
	read_size += proc.offset;
	proc.offset = 0;
	if (proc.outToFile)
	{
		if (proc.writeBody(proc.buffer.data(), read_size) < 0)
			return response->setHttpResError(500, "Internal server Error"); // kill cgi
		return;
	}
	else if (read_size < 4)
	{
		proc.offset = read_size;
		return;
	}
	std::vector<char> &buffer = proc.buffer;
	std::vector<char>::iterator it = std::search(buffer.begin(), buffer.begin() + read_size, seq, seq + 4);
	if (it != (buffer.begin() + read_size))
	{
		it = it + 4;
		response->CGIOutput.insert(response->CGIOutput.end(), buffer.begin(), it); // the size of header gonna be small
		if (proc.writeBody(&(*it), buffer.end() - it) < 0)
			return response->setHttpResError(500, "Internal server Error"); // kill cgi
		proc.outToFile = true;
	}
	else
	{
		response->CGIOutput.insert(response->CGIOutput.end(), buffer.begin(), buffer.begin() + read_size - 3);
		int j = 0;
		for (int i = read_size - 3; i < read_size; i++)
			proc.buffer[j++] = proc.buffer[i];
		proc.offset = 3;
	}
}

// void Event::wpipe(const struct kevent *ev)
// {
// 	// add some writes offset that does cl
// 	// std::cout << "WriteEvent pipe enter\n"; // TODO: test with post
// 	int fd = (size_t)ev->udata;
// 	Client *client = this->connections.getClient(fd);
// 	if (!client)
// 		return (void)(std::cout << "client has been free3\n");
// 	HttpResponse *response = &client->response;
// 	GlobalConfig::Proc &proc = client->proc;

// 	int avdata = std::abs(proc.woffset - (int)response->getBody().size());
// 	int wdata = std::min((int)ev->data, avdata);
// 	proc.woffset += wdata;
// 	if (write(ev->ident, response->getBody().data() + proc.woffset, wdata) < 0)
// 		throw HttpResponse::IOException();
// }

void Event::TimerEvent(const struct kevent *ev)
{
	// std::cout << "timer event\n";
	// exit(3);
	// if (!ev->udata)
	// 	return this->connections.closeConnection(ev->ident); // if client take too long to send full request
	// exit(1);
	// int fd = (size_t)ev->udata;
	// Client *client = this->connections.getClient(fd);
	// if (!client)
	// 	return (void)(std::cout << "client has been free1\n");
	// GlobalConfig::Proc &proc = client->proc;
	// // else cgi take too long to process it data
	// proc.state = GlobalConfig::Proc::TIMEOUT; // set the state for error TIMEOUT to response
	// proc.die(); // we kill the cgi with SIGKILL
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
	kevent(this->kqueueFd, &event, 1, NULL, 0, NULL); // an edge case where timer already in user land
	return (signal || status);
}

void Event::ProcEvent(const struct kevent *ev)
{
	int status = this->waitProc(ev->ident);
	ProcMap_t::iterator p = this->procs.find(ev->ident);
	if (p == this->procs.end()) // should always exist
		return;
	Proc &proc = p->second;
	Client *client = this->connections.getClient(proc.client);
	if (!client)
		return (void)(std::cout << "client has run away << " << proc.client << "\n"); // DO a cleanUp
	this->setWriteEvent(client->getFd(), EV_ENABLE);
	if (status)
		return (client->response.setHttpResError(503, "Bad Gateway"));
	client->response.state = START_CGI_RESPONSE;
	proc.clean();
	int fd = open("/tmp/cgi_out", O_RDONLY); // TODO: generate a Random file name
	if (fd < 0)
		return client->response.setHttpResError(500, "Internal Server Error");
	client->response.responseFd = fd;
	client->response.cgiOutFile = "/tmp/cgi_out"; // TODO: generate a Random file name
	proc.clean();
	this->procs.erase(ev->ident); // simple clean
}

void Event::eventLoop()
{
	connections.init(this->ctx, this->kqueueFd);
	int nev;
	std::cout << "TODO: add default index.html\n";
	std::cout << "TODO: edit path algo : handel path info\n";
	std::cout << "TODO: edit method how to find if it a cgi or not\n";
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
					this->rpipe(ev);
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
				std::cout << "memory faild: " << e.what() << "\n";
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

	// TODO add for cgi
	// struct sockaddr_in addr = this->sockAddrInMap.find(client->getServerFd())->second;
	// int port = ntohs(addr.sin_port);
	serverfd = client->getServerFd();
	const std::string &path = client->getPath();
	std::string host = client->getHost();
	ServerNameMap_t serverNameMap = this->virtuaServers.find(serverfd)->second; // always exist
	ServerNameMap_t::iterator _Vserver = serverNameMap.find(host);
	if (_Vserver == serverNameMap.end())
		Vserver = this->defaultServer.find(client->getServerFd())->second,
		IsDefault = false; // plz dont fail all my hope on you // crying like a little bitch, write a proper code like
						   // men WARNING
	else
		Vserver = _Vserver->second;
	Location *location = Vserver->getRoute(path);
	if (!location)
		return (NULL);
	else if (!IsDefault)
		host = *Vserver->getServerNames().begin();
	// location->setHostPort(host, port);
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
