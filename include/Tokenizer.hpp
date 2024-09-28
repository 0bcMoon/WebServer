#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <vector>
#include <string>
#include "HttpContext.hpp"

class Tokenizer
{
  private:
	std::string *config;
	std::vector<std::string> *tokens;
	std::string getNextToken();
	bool IsSpace(char c) const ;
	std::string getQuotedString(size_t &offset);


  public:
	static bool IsId(char c);
	void readConfig(const std::string path);
	void CreateTokens();
	HttpContext *parseConfig();
	Tokenizer();
	~Tokenizer();
};
#endif // !#ifndef TOKENIZER_HPP
