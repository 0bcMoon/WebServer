#ifndef ServerContext_HPP
#define ServerContext_HPP

#include "DataType.hpp"
#include "VirtualServer.hpp"

// TODO: validate server count should  > 0
class ServerContext
{
  private:
	static const int CGITimeOut = 5;// 5 second
	static const int ClientReadTime = 30;// 30 second
	typedef std::map<std::string, std::string> Type;
	int keepAliveTimeout;
	GlobalConfig globalParam;
	long maxBodySize; // in bytes
	long maxHeaderSize; // in bytes

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

	void setMaxBodySize(Tokens &token, Tokens &end);
	long getMaxBodySize() const;

	void setMaxHeaderSize(Tokens &token, Tokens &end);
	long getMaxHeaderSize() const;
	const std::string &getType(const std::string &ext);
	void setErrorLog(Tokens &token, Tokens &end);
	void setAccessLog(Tokens &token, Tokens &end);
	void setKeepAlive(Tokens &token, Tokens &end);
	int	getKeepAliveTime() const;
	int getCGITimeOut() const;
	int getClientReadTime() const ;

};

#endif
