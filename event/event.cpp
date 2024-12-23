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

#define red "\x1B[31m"
#define reset "\033[0m"
#define blue "\x1B[34m"
#define green "\x1B[32m"
#define yellow "\x1B[33m"

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
	const std::set<std::string> &serverNames = server->getServerNames();
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

	assert(result->ai_next == NULL);
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
	int sock_buf_size = BUFFER_SIZE;
	int result = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &sock_buf_size, sizeof(sock_buf_size));
	if (result < 0)
	{
		std::cout << "faild cause -- " << strerror(errno) << "\n";
		return (-1);
	}
	result = setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sock_buf_size, sizeof(sock_buf_size));
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
	EV_SET(&ev_set[1], newSocketFd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, NULL); // disable write event to stop
																					// client from spamming the server

	if (kevent(this->kqueueFd, ev_set, 2, NULL, 0, NULL) < 0)
		return (close(newSocketFd)); // report faild connections
	connections.addConnection(newSocketFd, socketFd);

	return (newSocketFd);
}

void Event::setWriteEvent(Client *client, uint16_t flags)
{
	if (client->writeEventState == flags)
		return;
	struct kevent ev;
	EV_SET(&ev, client->getFd(), EVFILT_WRITE, flags, 0, 0, NULL);
	if (kevent(this->kqueueFd, &ev, 1, NULL, 0, NULL) < 0)
		throw Event::EventExpection("kevent faild:" + std::string(strerror(errno)));
	client->writeEventState = flags;
}
void log(Client *client)
{
	data_t *req = client->request.data.back();
	std::cout << green;
	std::cout <<"HTTP/1.1 " << req->strMethode << " " << req->path  <<  std::endl;
	std::cout << reset;
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
		while (!client->request.eof && client->request.state != REQ_ERROR) // whats is header does not arrive fully ?
		{
			client->request.feed();
			if (!client->request.data.back()->isRequestLineValidated() && client->request.state == BODY)
			{
				client->request.decodingUrl();
				client->request.splitingQuery();
				client->request.location = this->getLocation(client);
				client->request.validateRequestLine();
				client->request.data.back()->isRequestLineValid = 1;
				log(client);
			}
		}
		client->request.eof = 0;
		this->setWriteEvent(client, EV_ENABLE);
	}
}

void Event::RegisterNewProc(Client *client)
{
	HttpResponse &response = client->response;
	CGIProcess cgi;
	Proc proc = cgi.RunCGIScript(response);
	if (proc.pid < 0)
		return;
	struct kevent ev[3];
	int evSize = 3;
	EV_SET(&ev[0], proc.pid, EVFILT_PROC, EV_ADD | EV_ENABLE | EV_ONESHOT, NOTE_EXIT, 0, (void *)(size_t)proc.pid);
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
		proc.clean();
		proc.die();
		throw Event::EventExpection("kevent faild:" + std::string(strerror(errno)));
	}
	client->cgi_pid = proc.pid;
	proc.client = client->getFd();
	proc.input = client->request.data.front()->bodyHandler.bodyFile;
	this->procs[proc.pid] = proc;
	this->setWriteEvent(client, EV_DISABLE);
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
		|| (client->request.data.front()->state != REQUEST_FINISH && client->request.data.front()->state != REQ_ERROR))
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
		client->handleResponseError();
	if (client->response.state == END_BODY)
	{
		client->response.clear();
		delete client->request.data[0];
		client->request.data.erase(client->request.data.begin());
		if (client->request.data.size() == 0)
			this->setWriteEvent(client, EV_DISABLE);
	}
}

void Event::ReadPipe(const struct kevent *ev)
{
	const char seq[4] = {'\r', '\n', '\r', '\n'};
	if (ev->flags & EV_EOF && !ev->data)
		return (void)close(ev->ident);
	ProcMap_t::iterator p = this->procs.find((size_t)ev->udata);
	Proc &proc = p->second;
	Client *client = this->connections.getClient(proc.client);
	if (!client)
		return proc.die();
	HttpResponse *response = &client->response;
	int read_size = std::min(ev->data, CGI_BUFFER_SIZE);
	int r = read(ev->ident, proc.buffer.data() + proc.offset, read_size); // create a event buffer
	if (r < 0)
		return response->setHttpResError(500, "Internal server Error"), proc.die();
	assert(r == read_size && "this should't happend"); // TODO: remove later
	read_size += proc.offset;
	proc.offset = 0;
	if (proc.outToFile)
	{
		if (proc.writeBody(proc.buffer.data(), read_size) < 0)
			return response->setHttpResError(500, "Internal server Error"), proc.die(); // kill cgi
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
		if (proc.writeBody(&(*it), buffer.begin() + read_size - it) < 0)
			return response->setHttpResError(500, "Internal server Error"), proc.die(); // kill cgi
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
	std::cout << "timer event happend\n";
	ProcMap_t::iterator p = this->procs.find((size_t)ev->udata);
	if (p == this->procs.end())
		return ((void)(std::cout << "proc already dead false alarm\n"));
	Proc &proc = p->second;
	proc.state = Proc::TIMEOUT;
	proc.die();
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
	kevent(this->kqueueFd, &event, 1, NULL, 0, NULL); // if timer has not been trigger better to delete it
	return (signal || status);
}

void log_file(std::string &filename)
{
	std::cerr << "log cgi output\n";
	std::ofstream f(filename);
	std::stringstream ss;
	if (!f)
		return ;

	ss << f.rdbuf();
	std::cerr << ss.str() << std::endl;
	std::cerr << "------------log cgi output\n";

}
void Event::ProcEvent(const struct kevent *ev)
{
	int status = this->waitProc(ev->ident);
	ProcMap_t::iterator p = this->procs.find(ev->ident); // need to crash for testing
	Proc &proc = p->second;
	Client *client = this->connections.getClient(proc.client);
	if (!client)
		return this->deleteProc(p);
	this->setWriteEvent(client, EV_ENABLE);
	if (status)
		log_file(proc.output);
	if (proc.state == Proc::TIMEOUT)
		return (client->response.setHttpResError(504, "Gateway Timeout"), this->deleteProc(p));
	else if (status)
		return (client->response.setHttpResError(502, "Bad Gateway"), this->deleteProc(p));
	client->response.state = START_CGI_RESPONSE;
	proc.clean();
	int fd = open(proc.output.data(), O_RDONLY);
	if (fd < 0)
		return client->response.setHttpResError(500, "Internal Server Error"), this->deleteProc(p);
	client->response.responseFd = fd;
	client->response.cgiOutFile = proc.output;
	this->deleteProc(p);
}

void Event::eventLoop()
{
	connections.init(this->ctx, this->kqueueFd);
	int nev;
	std::cout << "TODO: edit method how to find if it a cgi or not\n";
	std::cout << "TODO: parser header before getting location\n";
	std::cout << "TODO: The CGI should be run in the correct directory for relative path file access\n";
	std::cout << "-------------------\n";
	while (1)
	{
		// std::cout << "Wating for event....\n";
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
					this->ReadPipe(ev);
				else if (ev->filter == EVFILT_READ)
					this->ReadEvent(ev);
				else if (ev->filter == EVFILT_WRITE)
					this->WriteEvent(ev);
				else if (ev->filter == EVFILT_PROC)
					this->ProcEvent(ev);
				else if (ev->filter == EVFILT_TIMER)
					this->TimerEvent(ev);
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

void Event::deleteProc(ProcMap_t::iterator &it)
{
	Proc &p = it->second;
	p.clean();
	this->procs.erase(it);
}
Location *Event::getLocation(Client *client)
{
	VirtualServer *Vserver;
	int serverfd;
	bool IsDefault = true;

	serverfd = client->getServerFd();
	const std::string &path = client->getPath();
	std::string host = client->getHost();
	ServerNameMap_t serverNameMap = this->virtuaServers.find(serverfd)->second; // always exist
	ServerNameMap_t::iterator _Vserver = serverNameMap.find(host);
	if (_Vserver == serverNameMap.end())
	{
		Vserver = this->defaultServer.find(client->getServerFd())->second;
		IsDefault = true;
	}
	else
		Vserver = _Vserver->second;
	Location *location = Vserver->getRoute(path);
	if (!location)
		return (NULL);
	else if (IsDefault)
		client->response.server_name = *Vserver->getServerNames().begin();
	struct sockaddr *addr = &this->sockAddrInMap.find(client->getServerFd())->second;
	struct sockaddr_in *addr2 = (struct sockaddr_in *)addr;
	int port = ntohs(addr2->sin_port);
	client->response.server_port = port;
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
	printStackTrace();
	this->msg = msg;
}

const char *Event::EventExpection::what() const throw()
{
	return (this->msg.data());
}

Event::EventExpection::~EventExpection() throw() {}
