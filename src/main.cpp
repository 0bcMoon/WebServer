#include <unistd.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include "CGIProcess.hpp"
#include "Event.hpp"
#include "HttpResponse.hpp"
#include "Tokenizer.hpp"

#include <cstring>
#include <cerrno>

#define MAX_EVENTS 256
#define MAX_CONNECTIONS_QUEUE 256
#define CONF_FILE "webserv.conf"

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
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		delete ctx;
		return (NULL);
	}
	return (ctx);
}


int main(int ac, char **argv)
{
	ServerContext *ctx = NULL;
	if (ac > 2)
	{
		std::cerr << "invalid number of argument \n";
		return (1);
	}
	else if (ac == 2)
		ctx = LoadConfig(argv[1]);
	else
		ctx = LoadConfig(CONF_FILE);
	if (!ctx)
		return 1;
	try
	{
		Event event(MAX_EVENTS, MAX_CONNECTIONS_QUEUE, ctx);
		event.init();
		event.Listen();
		event.initIOmutltiplexing();
		event.eventLoop();
	}
	catch (const CGIProcess::ChildException &e)
	{
		delete ctx;
		return (1);
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << "\n";
	}

	delete ctx;
}
