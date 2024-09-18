#include <exception>
#include <iostream>
#include <new>
#include "ParserException.hpp"
#include "Tokenizer.hpp"
#ifdef __cplusplus
extern "C"
#endif
const char* __asan_default_options() { return "detect_leaks=0"; }
int main()
{
		try // ugly but fix the problem
		{
			Tokenizer tokenizer;
			tokenizer.readConfig("config/nginx.conf");
			tokenizer.CreateTokens();
			tokenizer.parseConfig();
		}
		catch (const ParserException &e)
		{
			std::cout << e.what() << std::endl;
			return (1);
		}
		catch (const std::bad_alloc &e)
		{
			std::cout << e.what() << std::endl;
			return (1);
		}
}
