#include "Event.hpp"
#include "../include/Client.hpp"
#include "Connections.hpp"
#include "HttpRequest.hpp"
#include "VirtualServer.hpp"
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

Event::Event() : MAX_CONNECTION_QUEUE(32), MAX_EVENTS(1024) {
	this->evList = NULL;
	this->eventChangeList = NULL;
	this->kqueueFd = -1;
	this->numOfSocket = 0;
}

Event::Event(int max_connection, int max_events, ServerContext *ctx)
	: MAX_CONNECTION_QUEUE(max_connection), MAX_EVENTS(max_events) {
		this->ctx = ctx;
		this->evList = NULL;
		this->eventChangeList = NULL;
		this->kqueueFd = -1;
		this->numOfSocket = 0;
	}
Event::~Event() {
	delete this->evList;
	VirtualServerMap_t::iterator it = this->virtuaServers.begin();
	for (; it != this->virtuaServers.end(); it++) {
		close(it->first);
	}
	std::map<int, VirtualServer *>::iterator it2 = this->defaultServer.begin();
	for (; it2 != this->defaultServer.end(); it2++) {
		if (this->virtuaServers.find(it->first) == this->virtuaServers.end())
			close(it2->first);
	}
	if (this->kqueueFd >= 0)
		close(this->kqueueFd);
}

void Event::InsertDefaultServer(VirtualServer *server, int socketFd) {
	if (this->defaultServer.find(socketFd) != this->defaultServer.end())
		throw std::runtime_error("default server already exist on this port");
	this->defaultServer[socketFd] = server;
}

void Event::insertServerNameMap(ServerNameMap_t &serverNameMap,
		VirtualServer *server, int socketFd) {
	std::set<std::string> &serverNames = server->getServerNames();
	if (serverNames.size() == 0) // if there is no server name  uname will be the
								 // default server in that port
		this->InsertDefaultServer(server, socketFd);
	else {
		std::set<std::string>::iterator it = serverNames.begin();
		for (; it != serverNames.end(); it++) {
			if (serverNameMap.find(*it) != serverNameMap.end())
				throw std::runtime_error(
						"confilict: server name already exist on some port host");
			serverNameMap[*it] = server;
		}
	}
}

void Event::init() {
	std::vector<VirtualServer> &virtualServers = ctx->getServers();

	for (size_t i = 0; i < virtualServers.size(); i++) {
		SocketAddrSet_t &socketAddr = virtualServers[i].getAddress();
		SocketAddrSet_t::iterator it = socketAddr.cbegin();
		for (; it != socketAddr.end(); it++) {
			int socketFd;
			if (this->socketMap.find(*it) ==
					this->socketMap
					.end()) // no need to create socket if it already exists
			{
				socketFd = this->CreateSocket(it);
				this->socketMap[*it] = socketFd;
			} else
				socketFd = this->socketMap.at(*it);

			VirtualServerMap_t::iterator it = this->virtuaServers.find(socketFd);
			if (it ==
					this->virtuaServers.end()) // create empty for socket map if not exist
			{
				this->virtuaServers[socketFd] = ServerNameMap_t();
				it = this->virtuaServers.find(
						socketFd); // take the reference of server map;
			}
			ServerNameMap_t &serverNameMap = it->second;
			this->insertServerNameMap(serverNameMap, &virtualServers[i], socketFd);
		}
	}
}

int set_non_blocking(int sockfd) // TODO make it more oop
{
	int flags = fcntl(sockfd, F_GETFL, 0);
	if (flags == -1) {
		perror("fcntl(F_GETFL)");
		return -1;
	}
	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
		perror("fcntl(F_SETFL)");
		return -1;
	}
	flags = fcntl(sockfd, F_GETFL, 0);
	if (flags & O_NONBLOCK)
		printf("socket is non-blocking\n");
	return 0;
}

int Event::CreateSocket(SocketAddrSet_t::iterator &address) {
	sockaddr_in address2;

	int socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFd < 0)
		throw std::runtime_error("socket creation failed: " +
				std::string(strerror(errno)));
	this->setNonBlockingIO(socketFd);
	int optval = 1;
	setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	memset(&address2, 0, sizeof(struct sockaddr_in));

	address2.sin_family = AF_INET;
	address2.sin_addr.s_addr = htonl(address->host);
	address2.sin_port = htons(address->port);
	if (bind(socketFd, (struct sockaddr *)&address2, sizeof(address2)) < 0) {
		close(socketFd);
		throw std::runtime_error("bind failed: " + std::string(strerror(errno)));
	}
	this->numOfSocket++;
	this->sockAddrInMap[socketFd] = address2;
	return (socketFd);
}

std::string Event::get_readable_ip(const VirtualServer::SocketAddr address) {
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

void Event::setNonBlockingIO(int sockfd) {
	int flags = fcntl(sockfd, F_GETFL, 0);
	if (flags == -1)
		perror("fcntl(F_GETFL)"), exit(1);
	if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
		perror("fcntl(F_SETFL)"), exit(1); // TODO: remove  this
}

bool Event::Listen(int socketFd) {
	// TODO: this->MAX_CONNECTION_QUEUE

	if (listen(socketFd, 5) < 0) {
		perror("listen failed");
		return false;
	}
	return true;
}

bool Event::Listen() {
	SocketMap_t::iterator it = this->socketMap.begin();

	for (; it != this->socketMap.end(); it++) {
		if (!this->Listen(it->second))
			throw std::runtime_error(
					"could not listen: " + get_readable_ip(it->first) + " " +
					strerror(errno));
		std::cout << "server listen on " << this->get_readable_ip(it->first)
			<< std::endl;
	}
	return true;
}

void Event::CreateChangeList() {
	int i = 0;
	SockAddr_in::iterator it = this->sockAddrInMap.begin();
	// TODO : monitor read and write
	for (; it != this->sockAddrInMap.end(); it++) {
		EV_SET(&this->eventChangeList[i], it->first, EVFILT_READ,
				EV_ADD | EV_ENABLE, 0, 0, NULL);
		EV_SET(&this->eventChangeList[i + this->numOfSocket], it->first,
				EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
		i++;
	}
	if (kevent(this->kqueueFd, this->eventChangeList, this->numOfSocket * 2, NULL,
				0, NULL) < 0)
		throw std::runtime_error(
				"kevent failed: could not regester server event: " +
				std::string(strerror(errno)));
	delete this->eventChangeList;
	this->eventChangeList = NULL;
}

void Event::initIOmutltiplexing() {
	this->evList = new struct kevent[this->MAX_EVENTS];
	this->eventChangeList = new struct kevent[this->numOfSocket * 2];
	this->kqueueFd = kqueue();
	if (this->kqueueFd < 0)
		throw std::runtime_error("kqueue faild: " + std::string(strerror(errno)));
	this->CreateChangeList();
}
// TODO:  fix me i may faild ??
int Event::newConnection(int socketFd, Connections &connections) {
	struct sockaddr_in address = this->sockAddrInMap[socketFd];
	socklen_t size = sizeof(struct sockaddr);
	std::cout << "waiting for new connnection to accept\n";
	int newSocketFd = accept(socketFd, (struct sockaddr *)&address, &size);
	if (newSocketFd < 0)
		return -1;

	std::cout << "connnection accept\n";
	this->setNonBlockingIO(newSocketFd);
	this->RegsterClient(
			newSocketFd); // TODO: user udata feild to store server id;
	connections.addConnection(newSocketFd, socketFd);
	return newSocketFd;
}

void read_from_client(int fd) {
	char buffer[2048] = {0};
	printf("try to read from %d", fd);
	int r = read(fd, buffer, 254);
	if (r <= 0) {
		if (r < 0)
			perror("read faild");
		close(fd);
		printf("client %d disconnected\n", fd);
		return;
	} else {
		buffer[r] = 0;
		printf("new message from %d: %s\n", fd, buffer);
	}
}

int Event::RemoveClient(int clientFd) {
	// TODO : hendel if there is an error server should never go down
	struct kevent ev_set;
	EV_SET(&ev_set, clientFd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	kevent(this->kqueueFd, &ev_set, 1, NULL, 0, NULL);
	EV_SET(&ev_set, clientFd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
	kevent(this->kqueueFd, &ev_set, 1, NULL, 0, NULL);
	close(clientFd);
	return 0;
}

int Event::RegsterClient(int clientFd) {
	// TODO : hendel if there is an error server should never go down
	struct kevent ev_set;

	EV_SET(&ev_set, clientFd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	kevent(this->kqueueFd, &ev_set, 1, NULL, 0, NULL);

	EV_SET(&ev_set, clientFd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
	kevent(this->kqueueFd, &ev_set, 1, NULL, 0, NULL);
	return 0;
}

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_EVENTS 10

void response(int fd) {
	const char *http_200_response = "HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html; charset=UTF-8\r\n"
		"Connection: keep-alive\r\n"
		"Server: YOUR DADDY\r\n"
		"Host: YOUR DADDY\r\n"
		"Content-Length: 130\r\n"
		"\r\n"
		"<!DOCTYPE html>\n"
		"<html>\n"
		"<head><title>200 OK</title></head>\n"
		"<body>\n"
		"<h1>200 OK</h1>\n"
		"<p>The request has succeeded.</p>\n"
		"</body>\n"
		"</html>";

	const char *http_404_response =
		"HTTP/1.1 404 Not Found\r\n"
		"Content-Type: text/html; charset=UTF-8\r\n"
		"Connection: keep-alive\r\n"
		"Content-Length: 175\r\n"
		"\r\n"
		"<!DOCTYPE html>\n"
		"<html>\n"
		"<head><title>404 Not Found</title></head>\n"
		"<body>\n"
		"<h1>404 Not Found</h1>\n"
		"<p>The requested resource could not be found on this server.</p>\n"
		"</body>\n"
		"</html>";
	// write(fd, http_404_response, strlen(http_404_response));
	write(fd, http_200_response, strlen(http_200_response));
}

void Event::eventLoop() {
	int client_fd;
	int socketFd;
	Connections connections(ctx);
	// HttpRequest *req;

	// this->ctx->getMaxBodySize();
	while (1) {
		// std::cout << "waiting for event\n";
		int nev = kevent(this->kqueueFd, NULL, 0, this->evList, MAX_EVENTS, NULL);
		if (nev == -1)
			throw std::runtime_error("kevent failed: " +
					std::string(strerror(errno)));
		for (int i = 0; i < nev; i++) {
			const struct kevent *ev = &this->evList[i];
			if (this->checkNewClient(ev->ident))
				this->newConnection(ev->ident, connections);
			else if (ev->filter == EVFILT_READ) {
				// std::cout << "READ event\n"; // Handel half close
				connections.requestHandler(ev->ident);
			} else if (ev->filter == EVFILT_WRITE) {
				// std::cout << "write event\n";
				if (ev->flags & EV_EOF) {
					// std::cout << "client disconnected\n";
					connections.closeConnection(ev->ident);
					this->RemoveClient(ev->ident);
				} else {
					clients_it kv = connections.clients.find(ev->ident);
					if (kv == connections.clients.end())
						continue;
					Client *client = kv->second;
					if (client->request.state != REQUEST_FINISH)
						continue;
					std::string path = "/intra";
					std::string host = "intra.com";
					client->response.location = this->getLocation(LocationConf(host, path, 3));
					client->respond();
					response(ev->ident);

					//       std::cout << location->globalConfig.getRoot() << "\n";
					//       std::cout << location->globalConfig.getAutoIndex() << "\n";
					// exit(1);

					// 		if (connections.clients.count(ev->ident))
					// 		{
					// 			if
					// (connections.clients[ev->ident]->request.state == REQUEST_FINISH)
					// 			{
					// 				// clients[ev->ident]
					// 				std::cout << "response sent\n";
					// 			}
					// 			else
					// 				std::cout << "request not finished
					// yet\n";
					// 		}
					// 		else
					// 			std::cerr << "client un exist\n";
				}
				// std::cout << "write ended\n";
			}
		}
		// std::cout << "all  event has been process: " << nev << '\n';
	}
}
bool Event::checkNewClient(int socketFd) {
	return (this->sockAddrInMap.find(socketFd) != this->sockAddrInMap.end());
}

Location *Event::getLocation(const Event::LocationConf &locationConf) {
	int serverfd = locationConf.serverFd;
	std::string &path = locationConf.path;
	std::string &host = locationConf.host;

	// serverFd should always exist
	ServerNameMap_t serverNameMap = this->virtuaServers.find(serverfd)->second;
	ServerNameMap_t::iterator _Vserver = serverNameMap.find("intra.com");
	if (_Vserver == serverNameMap.end())
		throw std::runtime_error("server does not exist"); // remove later

	VirtualServer *Vserver = _Vserver->second;
	Location *location = Vserver->getRoute("/intra");
	return (location);
}

Event::LocationConf::LocationConf(std::string &path, std::string &host,
		int serverFd)
	: path(path), host(host), serverFd(serverFd) {}
