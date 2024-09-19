#ifndef Server_H
# define Server_H

#include <vector>
#include "DataType.hpp"
#include "Location.hpp"

class Server
{
	private:
		std::vector<int>			fds;
		std::vector<std::string>	serversNames; // todo as redix tree 
		GlobalParam					globalParam;
		std::vector<Location>		locations;
		std::vector<std::string>	*listen;
	public:
		void pushLocation(Tokens &tokens, Tokens &end);
		Server();
};

#endif
