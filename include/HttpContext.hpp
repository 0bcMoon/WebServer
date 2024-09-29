#ifndef HttpContext_H
# define HttpContext_H

#include "DataType.hpp"
#include "VirtualServer.hpp"

class ServerContext
{
	private:
		// int				keepAliveTimeout;
		GlobalConfig		globalParam;
		std::vector<VirtualServer> servers;
	public:
		ServerContext();
		~ServerContext();
		void pushServer(Tokens &token, Tokens &end);
		std::vector<VirtualServer> &getServers();
		void parseTokens(Tokens &token, Tokens &end);
};

#endif
