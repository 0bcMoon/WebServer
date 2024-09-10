#include "Tokenizer.hpp"

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

bool Tokenizer::readConfig(std::string path)
{
	std::string line;

	this->configFile.open(path);
	if (!this->configFile.is_open())
	{
		std::cout << "Error: could not open file :" << path << std::endl;
		return (false);
	}
	while (std::getline(this->configFile, line))
		*this->config += line;
	if (!this->configFile.eof())
	{
		std::cout << "Warning: file reading stopped before reaching the end" << std::endl;
		this->configFile.close();
		return false;
	}
	this->configFile.close();
	return (true);
}

bool Tokenizer::IsId(char c) const
{
	return (c == '{' || c == '}' || c == ';');
}

bool Tokenizer::IsSpace(char c) const
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r');
}

std::string Tokenizer::getNextToken()
{
	static size_t offset = 0;
	std::string token;

	while (offset < this->config->size() && IsSpace(this->config->at(offset)))
		offset++;
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
		{
			std::cout << this->tokens->at(i) << std::endl;
			level++;
		}
		if (this->tokens->at(i) == "}")
		{
			std::cout << this->tokens->at(i) << std::endl;
			level--;
		}
		else
			std::cout << std::string(level * 4, ' ');
	}
}
