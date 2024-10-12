

#include <unistd.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "Debug.hpp"
#include "Event.hpp"
#include "Tokenizer.hpp"
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
	// char buffer[100] = {0};
	// sprintf(buffer, "lsof -p  %d", getpid());
	// // system("leaks  webserv"); // there is no leaks
	// system(buffer); // there is no leaks
	system("openport"); // there is no leaks
	sleep(1);
}

ServerContext *LoadConfig(const char *path)
{
	ServerContext *ctx = NULL;

	try // ugly but fix the problem
	{
		Tokenizer tokenizer;
		tokenizer.readConfig(path);
		tokenizer.CreateTokens();
		ctx = new ServerContext();
		tokenizer.parseConfig(ctx);
	}
	catch (const Debug &e)
	{
		std::cout << e.what() << std::endl;
		delete ctx;
		return (NULL);
	}
	catch (const std::bad_alloc &e)
	{
		std::cout << e.what() << std::endl;
		delete ctx;
		return (NULL);
	}
	return (ctx);
}

void sigpipe_handler(int signum)
{
	printf("Caught SIGPIPE. Ignoring.\n");
}
int main()
{
	Event *event = NULL;
	ServerContext *ctx = NULL;
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigpipe_handler;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGPIPE, &sa, NULL) == -1)
	{
		printf("Failed to set SIGPIPE handler: %s\n", strerror(errno));
		return 1;
	}
	ctx = LoadConfig("config/nginx.conf");
	// move this to a function
	if (!ctx)
		return 1;
	try
	{
		event = new Event(MAX_EVENTS, MAX_CONNECTIONS_QUEUE, ctx);
		event->init();
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
