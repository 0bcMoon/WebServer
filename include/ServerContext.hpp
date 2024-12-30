#ifndef ServerContext_HPP
#define ServerContext_HPP

#include "DataType.hpp"
#include "VirtualServer.hpp"

// TODO: validate server count should  > 0
class ServerContext
{
  private:
	static const int CGITimeOut = 3;// 5 second //TODO: make it dynamique and set min value of 3 seconds
	static const int ClientReadTime = 30;// 30 second
	typedef std::map<std::string, std::string> Type;
	int keepAliveTimeout;
	GlobalConfig globalConfig;

	std::vector<VirtualServer> servers;
	Type types;
	void addTypes(Tokens &token, Tokens &end);

 public:
	ServerContext();
	~ServerContext();

	void init();
	void pushServer(Tokens &token, Tokens &end);
	void pushTypes(Tokens &token, Tokens &end);
	std::vector<VirtualServer> &getServers();
	void parseTokens(Tokens &token, Tokens &end);
	std::vector<VirtualServer> &getVirtualServers();

	const std::string &getType(const std::string &ext);
	void setKeepAlive(Tokens &token, Tokens &end);
	int	getKeepAliveTime() const;
	int getCGITimeOut() const;
	int getClientReadTime() const ;

};

#endif
