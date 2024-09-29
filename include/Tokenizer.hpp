#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include "HttpContext.hpp"
#include <string>
#include <vector>

class Tokenizer
{
  private:
	std::string *config;
	std::vector<std::string> *tokens;
	std::string getNextToken();
	bool IsSpace(char c) const;
	std::string getQuotedString(size_t &offset);

  public:
	static bool IsId(char c);
	void parseConfig(ServerContext *context);
	void CreateTokens();
	void readConfig(const std::string path);
	Tokenizer();
	~Tokenizer();
};
#endif // !#ifndef TOKENIZER_HPP
