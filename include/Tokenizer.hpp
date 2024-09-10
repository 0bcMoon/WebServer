#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <fstream>
#include <vector>
#include <string>
#include <iostream>

class Tokenizer
{
  private:
	std::ifstream configFile;
	std::string *config;
	std::vector<std::string> *tokens;
	std::string getNextToken();
	bool IsSpace(char c) const ;
	bool IsId(char c) const ;


  public:
	bool readConfig(std::string path);
	void CreateTokens();
	Tokenizer();
	~Tokenizer();
};
#endif // !#ifndef TOKENIZER_HPP
