#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "Debug.hpp"
#include "Event.hpp"
#include "Tokenizer.hpp"
#include "VirtualServer.hpp"
#ifdef __cplusplus
extern "C"
#endif
	const char *
	__asan_default_options()
{
	return "detect_leaks=0";
}

#define MAX_EVENTS 64
#define MAX_CONNECTIONS_QUEUE 128

void atexist()
{
	char buffer[100] = {0};
	sprintf(buffer, "lsof -p  %d", getpid());
	// system("leaks  webserv"); // there is no leaks
	system(buffer); // there is no leaks
	sleep(1);
	system("openport"); // there is no leaks
}

ServerContext *LoadConfig(const char *path)
{
	ServerContext *http = NULL;

	try // ugly but fix the problem
	{
		Tokenizer tokenizer;
		tokenizer.readConfig(path);
		tokenizer.CreateTokens();
		http = new ServerContext();
		tokenizer.parseConfig(http);
	}
	catch (const Debug &e)
	{
		std::cout << e.what() << std::endl;
		delete http;
		return (NULL);
	}
	catch (const std::bad_alloc &e)
	{
		std::cout << e.what() << std::endl;
		delete http;
		return (NULL);
	}
	return (http);
}

int main()
{ 
	// atexit(atexist);

	Event *event = NULL;
	ServerContext *ctx = NULL;

	ctx = LoadConfig("config/nginx.conf");
	// move this to a function
	if (!ctx) return 1;
	try
	{
		event = new Event(MAX_EVENTS, MAX_CONNECTIONS_QUEUE);
		event->init(ctx->getVirtualServers());
		event->Listen();
		event->initIOmutltiplexing();
		event->eventLoop();
	}
	catch (const std::runtime_error &e)
	{
		std::cerr << e.what() << "\n";
	}
	catch (const std::bad_alloc &e)
	{
		std::cerr << e.what() << "\n";
	}

	delete event;
	delete ctx;
}

