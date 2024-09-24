#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <vector>
using std::string;

struct SocketAddr
{
	int host; // wich interface bind to (require ip address )
	int port;

	bool operator==(const SocketAddr &rhs) const
	{
		return (port == rhs.port && host == rhs.host);
	}

	bool operator<(const SocketAddr &rhs) const
	{
		if (host != rhs.host)
			return host < rhs.host;
		return port < rhs.port;
	}
};

int main()
{
	std::set<SocketAddr> listen;
	SocketAddr addr = {1, 2};
	listen.insert(addr);
	addr.host = 1;
	addr.port = 7;
	if (listen.find(addr) != listen.end())
		std::cout << "found" << std::endl;
	else
		std::cout << "not found" << std::endl;
	return 0;
}
