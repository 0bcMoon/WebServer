#ifndef HttpContext_H
# define HttpContext_H

#include "DataType.hpp"
#include "Server.hpp"

class HttpContext
{
	private:
		int				keepAliveTimeout;
		GlobalParam		globalParam;
		std::vector<Server *> servers;
	public:
		HttpContext();
		~HttpContext();
		void pushServer(Tokens &token, Tokens &end);
		void parseTokens(Tokens &token, Tokens &end);
};
#endif
