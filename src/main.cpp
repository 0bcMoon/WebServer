#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "Debug.hpp"
#include "VirtualServer.hpp"
#include "Tokenizer.hpp"
#ifdef __cplusplus
extern "C"
#endif
	const char *
	__asan_default_options()
{
	return "detect_leaks=0";
}
void atexist()
{
		system("leaks  webserv");
}
int main()
{
	atexit(atexist);
	
	HttpContext *http = NULL;
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
	delete http;
}
