#include <exception>
#include <iostream>
#include <new>
#include "ParserException.hpp"
#include "Tokenizer.hpp"

int main()
{
		try // ugly but fix the problem
		{
			Tokenizer tokenizer;
			tokenizer.readConfig("config/nginx.conf");
			tokenizer.CreateTokens();
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
