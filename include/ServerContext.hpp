#ifndef ServerContext_HPP
# define ServerContext_HPP

#include "DataType.hpp"
#include "VirtualServer.hpp"

class ServerContext
{
	private:
	int									keepAliveTimeout;
	GlobalConfig						globalParam;
	long								maxBodySize; // in bytes
	long								maxHeaderSize; // in bytes

	std::vector<VirtualServer>			servers;
	public:
	ServerContext();
	~ServerContext();
	void pushServer(Tokens &token, Tokens &end);
	std::vector<VirtualServer> &getServers();
	void parseTokens(Tokens &token, Tokens &end);
	std::vector<VirtualServer> &getVirtualServers();

	void setMaxBodySize(Tokens &token, Tokens &end);
	long getMaxBodySize() const;

	void setMaxHeaderSize(Tokens &token, Tokens &end);
	long getMaxHeaderSize() const;
};

#endif
