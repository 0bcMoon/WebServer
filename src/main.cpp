#include <iostream>
#include "Tokenizer.hpp"
int main()
{
	{
		Tokenizer tokenizer;
		if (!tokenizer.readConfig("config/server.conf"))
			return (1);
		tokenizer.CreateTokens();
	}
}
