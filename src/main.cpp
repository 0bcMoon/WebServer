#include <exception>
#include <iostream>
#include <new>
#include "../include/HttpRequest.hpp"
#include <fcntl.h>
#include <sys/fcntl.h>

int main()
{

	HttpRequest		req(open("request.req", O_RDWR, 0777));
	req.feed();
		// try // ugly but fix the problem
		// {
		// 	Tokenizer tokenizer;
		// 	tokenizer.readConfig("config/nginx.conf");
		// 	tokenizer.CreateTokens();
		// }
		// catch (const ParserException &e)
		// {
		// 	std::cout << e.what() << std::endl;
		// 	return (1);
		// }
		// catch (const std::bad_alloc &e)
		// {
		// 	std::cout << e.what() << std::endl;
		// 	return (1);
		// }
}
