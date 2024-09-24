#ifndef Server_H
#define Server_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <set>
#include <vector>
#include "DataType.hpp"
#include "Location.hpp"

class Server
{
  private:
	struct SocketAddr
	{
		int port;
		int host; // wich interface bind to (require ip address )
		bool operator<(const SocketAddr &rhs) const // code from the great chatGpt
		{
			if (host != rhs.host)
				return host < rhs.host;
			return port < rhs.port;
		}
	};
	std::vector<int> fds;
	std::set<std::string> serverNames; // todo as trie
	GlobalParam globalParam;
	std::vector<Location> locations;
	std::set<SocketAddr> listen;

  public:
	void setListen(Tokens &Token, Tokens &end);
	std::set<SocketAddr> &getListen();
	bool isListen(const SocketAddr &addr) const;
	void setServerNames(Tokens &Token, Tokens &end);
	void pushLocation(Tokens &tokens, Tokens &end);
	void parseTokens(Tokens &tokens, Tokens &end);
	Server();
};

#endif
