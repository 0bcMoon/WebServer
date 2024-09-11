#include "Tokenizer.hpp"
#include <iostream>
#include <string>
#include "ParserException.hpp"

Tokenizer::Tokenizer()
{
	this->tokens = new std::vector<std::string>;
	this->config = new std::string;
}

Tokenizer::~Tokenizer()
{
	delete this->tokens;
	delete this->config;
}

void Tokenizer::readConfig(const std::string path)
{
	std::string line;
	std::ifstream configFile;

	configFile.open(path);
	if (!configFile.is_open())
		throw ParserException("WebServ: could not open file: " + path);
	while (std::getline(configFile, line))
	{
		line.push_back('\n'); // getline trim last \n in line 
		*this->config += line;
	}
	if (!configFile.eof())
	{
		configFile.close();
		throw ParserException("WebServ: could not open file: " + path);
	}
	configFile.close();
}

bool Tokenizer::IsId(char c) const
{
	return (c == '{' || c == '}' || c == ';');
}

bool Tokenizer::IsSpace(char c) const
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r');
}

std::string Tokenizer::getQuotedString(size_t &offset)
{
	std::string token = "";
	char quote;
	if (offset >= this->config->size())
		return "";
	quote = this->config->at(offset);
	if (quote != '"' && quote != '\'')
		return "";
	token.push_back(quote);
	offset++;
	while (offset < this->config->size() && this->config->at(offset) != quote)
		token.push_back(this->config->at(offset++));
	if (offset >= this->config->size())
		throw ParserException("Error: missing closing quote");
	offset++;
	token.push_back(quote);
	if (!this->IsSpace(this->config->at(offset)))
		throw ParserException("Error: unexpected  token: " + std::string(1, this->config->at(offset)));// cpp  whats i can do else 
	return token;
}

std::string Tokenizer::getNextToken()
{
	static size_t offset;
	std::string token = "";

	while (offset < this->config->size() && IsSpace(this->config->at(offset)))
		offset++;
	token = getQuotedString(offset);
	if (token != "")
		return token;
	if (offset < this->config->size() && IsId(this->config->at(offset)))
		return std::string(1, this->config->at(offset++));

	while (offset < this->config->size() && !IsSpace(this->config->at(offset)) && !this->IsId(this->config->at(offset)))
		token.push_back((*this->config)[offset++]);
	return token;
}

void Tokenizer::CreateTokens()
{
	std::string token;
	while ((token = this->getNextToken()) != "")
		this->tokens->push_back(token);

	int level = 0;
	for (size_t i = 0; i < this->tokens->size(); i++)
	{
		if (this->tokens->at(i) == "{")
			level++;
		else if (this->tokens->at(i) == "}")
			level--;
		if (this->tokens->at(i) == "{")
		{
			if (level > 0)
				std::cout << std::string((level - 1) * 4, ' ');
		}
		else
			std::cout << std::string(level * 4, ' ');
		std::cout << this->tokens->at(i) << std::endl;
	}
}
