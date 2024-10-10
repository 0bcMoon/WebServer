#ifndef Event_HPP
#define Event_HPP
#include <sys/event.h>
#include "ServerContext.hpp"
#include "VirtualServer.hpp"

class Event
{
  private:
	ServerContext										*ctx;
	const int											MAX_CONNECTION_QUEUE ;
	const int											MAX_EVENTS;
	typedef std::map<std::string, VirtualServer *>		ServerNameMap_t;
	typedef std::map<int,  ServerNameMap_t>				VirtualServerMap_t;
	typedef std::map<VirtualServer::SocketAddr, int>	SocketMap_t;
	typedef std::map<int, struct sockaddr_in>			SockAddr_in;
	typedef std::set<VirtualServer::SocketAddr>			SocketAddrSet_t;

	VirtualServerMap_t									VirtuaServers;
	SocketMap_t											socketMap;
	std::map<int, VirtualServer *>						defaultServer;
	SockAddr_in											sockAddrInMap;

	int													CreateSocket(SocketAddrSet_t::iterator  &address);
	void												setNonBlockingIO(int serverFd);
	bool												Listen(int serverFd);
	std::string											get_readable_ip(VirtualServer::SocketAddr address);
	void												insertServerNameMap(ServerNameMap_t &serverNameMap, VirtualServer *server, int socketFd);
	void												InsertDefaultServer(VirtualServer *server, int socketFd);
	struct kevent										*eventChangeList;
	struct kevent										*evList;
	int													kqueueFd;
	void												CreateChangeList();
	int													numOfSocket;
	int													newConnection(int socketFd);
	bool												checkNewClient(int socketFd);
	int													RemoveClient(int clientFd);
	int													RegsterClient(int clientFd);

  public:
	void												initIOmutltiplexing();
	Event();
	~Event();
	Event(int max_connection, int max_events, ServerContext *ctx);
	void init();
	bool												Listen();
	void												eventLoop();
};

#endif
