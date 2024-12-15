#include <unistd.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "Debug.hpp"
#include "Event.hpp"
#include "HttpResponse.hpp"
#include "Log.hpp"
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

void atexist()
{
	system("openport"); // there is no leaks
	sleep(1);
}

#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <iostream>
#include <string>
std::string demangle(const char *mangled)
{
	if (!mangled)
		return "";

	int status;
	char *demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);

	if (status == 0 && demangled)
	{
		std::string result(demangled);
		free(demangled);
		return result;
	}

	return mangled ? mangled : "";
}

void printStackTrace(int skipFrames = 1)
{
	void *callstack[128];
	int frames = backtrace(callstack, 128);

	std::cerr << "Stack Trace (" << frames - skipFrames << " frames):\n";

	char **symbols = backtrace_symbols(callstack, frames);
	if (!symbols)
	{
		std::cerr << "Failed to get stack trace symbols\n";
		return;
	}

	for (int i = skipFrames; i < frames; ++i)
	{
		Dl_info info;
		if (dladdr(callstack[i], &info))
		{
			std::string funcName = demangle(info.dli_sname);

			std::cerr << "  #" << (i - skipFrames) << ": ";

			if (!funcName.empty())
			{
				std::cerr << funcName << " ";
			}

			if (info.dli_fname)
			{
				std::cerr << "in " << info.dli_fname;
			}

			std::cerr << std::endl;
		}
		else
		{
			std::cerr << "  #" << (i - skipFrames) << ": " << symbols[i] << std::endl;
		}
	}

	free(symbols);
}
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
		Log::init();
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
	printf("Caught SIGPIPE. Ignoring.\n");
}

/*
 * TODO:
 */
int main()
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
	ctx = LoadConfig("config/nginx.conf");
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

	delete event;
	delete ctx;
	Log::close();
}
