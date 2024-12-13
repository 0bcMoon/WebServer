#ifndef Event_HPP
#define Event_HPP
#include <sys/event.h>
#include <exception>
#include <vector>
#include "Connections.hpp"
#include "DataType.hpp"
#include "ServerContext.hpp"
#include "VirtualServer.hpp"


class Event
{

	public:
	private:
		enum EventType
		{
			SOCKET = 0,
			PIPE = 1,
		};

		ServerContext *ctx;
		Connections connections;
		const int MAX_CONNECTION_QUEUE;
		const int MAX_EVENTS;
		typedef std::map<std::string, VirtualServer *> ServerNameMap_t;
		typedef std::map<int, ServerNameMap_t> VirtualServerMap_t;
		typedef std::map<VirtualServer::SocketAddr, int> SocketMap_t;
		typedef std::map<int, struct sockaddr> SockAddr_in;
		typedef std::set<VirtualServer::SocketAddr> SocketAddrSet_t;
		typedef std::map<int, Proc> ProcMap_t;

		// typedef std::map<int, > SockAddr_in;
		VirtualServerMap_t virtuaServers;
		SocketMap_t socketMap;
		ProcMap_t procs;
		std::map<int, VirtualServer *> defaultServer;
		SockAddr_in sockAddrInMap;
		struct kevent *eventChangeList;
		struct kevent *evList;
		int kqueueFd;
		int numOfSocket;

		int CreateSocket(SocketAddrSet_t::iterator &address);
		int setNonBlockingIO(int serverFd);
		bool Listen(int serverFd);
		void insertServerNameMap(ServerNameMap_t &serverNameMap, VirtualServer *server, int socketFd);
		void InsertDefaultServer(VirtualServer *server, int socketFd);
		void CreateChangeList();
		int newConnection(int socketFd, Connections &connections);
		bool checkNewClient(int socketFd);
		void setWriteEvent(int fd, uint16_t flags);
		Location *getLocation(const Client *client);
		bool IsFileExist(HttpResponse &response);
		Proc RunCGIScript(HttpResponse &response);

		void ReadEvent(const struct kevent *ev);
		void WriteEvent(const struct kevent *ev);
		void RegisterNewProc(Client *client);
		void TimerEvent(const struct kevent *ev);
		void ProcEvent(const struct kevent *ev);
		void StartTimer(Client *client);


		int waitProc(int pid);
		void wpipe(const struct kevent *ev);
		void rpipe(const struct kevent *ev);
	class EventExpection: public std::exception
	{
		private:
			std::string msg;
		public:
			EventExpection(const std::string &msg) throw();
			~EventExpection() throw();
			virtual const char* what() const throw();
	};

	public:
		void initIOmutltiplexing();
		~Event();
		Event(int max_connection, int max_events, ServerContext *ctx);
		void init();
		bool Listen();
		void eventLoop();
};

#endif
