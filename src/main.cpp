#include <cassert>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <new>
#include <vector>
#include "Debug.hpp"
#include "ParserException.hpp"
#include "Server.hpp"
#include "Tokenizer.hpp"
#ifdef __cplusplus
extern "C"
#endif
	const char *
	__asan_default_options()
{
	return "detect_leaks=0";
}
int main()
{
	HttpContext *http;
	try // ugly but fix the problem
	{
		Tokenizer tokenizer;
		tokenizer.readConfig("config/nginx.conf");
		tokenizer.CreateTokens();
		http = tokenizer.parseConfig();
	}
	catch (const Debug &e)
	{
		std::cout << e.what() << std::endl;
		return (1);
	}
	catch (const std::bad_alloc &e)
	{
		std::cout << e.what() << std::endl;
		return (1);
	}
	std::vector<Server> servers = http->getServers();
	for (auto it: servers)
	{
		std::cout << "Server: " << it.isListen(Server::SocketAddr(12, 10)) << std::endl;
	}
	std::cout << "Number of servers: " << servers.size() << std::endl;
}
