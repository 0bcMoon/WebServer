#include <unistd.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "CGIProcess.hpp"
#include "Event.hpp"
#include "HttpResponse.hpp"
#include "Tokenizer.hpp"
#ifdef __cplusplus
extern "C"
#endif
	const char *
	__asan_default_options()
{
	return "detect_leaks=0";
}

#define MAX_EVENTS 128
#define MAX_CONNECTIONS_QUEUE 256


#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <iostream>
#include <string>


ServerContext *LoadConfig(const char *path)
{
	ServerContext *ctx = NULL;

	try
	{
		Tokenizer tokenizer;
		tokenizer.readConfig(path);
		tokenizer.CreateTokens();
		ctx = new ServerContext();
		tokenizer.parseConfig(ctx);



		ctx->init();
	}
	catch (const Tokenizer::ParserException &e)
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
	(void)signum;
}

int main(int ac, char **argv)
{
	Event *event = NULL;
	ServerContext *ctx = NULL;
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigpipe_handler;
	sigemptyset(&sa.sa_mask);
	std::srand(std::time(NULL));
	if (sigaction(SIGPIPE, &sa, NULL) == -1)
	{
		printf("Failed to set SIGPIPE handler: %s\n", strerror(errno));
		return 1;
	}
	if (ac > 2)
	{
		std::cerr << "invalid number of argument \n";
		return (1);
	}
	else if (ac == 2)
		ctx = LoadConfig(argv[1]);
	else
		ctx = LoadConfig("webserv.conf");
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
		std::cerr << "runtime_error -- " << e.what() << "\n";
	}
	catch (const std::bad_alloc &e)
	{
		std::cerr << e.what() << "\n";
	}
	catch (const CGIProcess::ChildException &e)
	{
		delete event;
		delete ctx;
		return (1);
	}
	delete event;
	delete ctx;
}
