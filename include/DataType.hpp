#ifndef DataType_H
#define DataType_H

#include <map>
#include <string>
#include <vector>

typedef std::vector<std::string>::iterator Tokens;



class GlobalConfig
{
  private:
	typedef int http_method_t;
	enum methods_e
	{
		GET = 0b1,
		POST = 0b10,
		DELETE = 0b100,
	};
	http_method_t						methods;
	std::map<std::string, std::string>  errorPages;
	std::string							root;
	bool								autoIndex;
	std::string							upload_file_path;
	std::vector<std::string>			indexes;

  public:

	std::string loadFile(const char *filename);
	// void	loadFile(Tokens &token, Tokens &end, std::string &buffer); // WARNING:t5arbi9
	bool	isValidStatusCode(std::string &str); // WARNING:t5arbi9
	void 	setMethods(Tokens &token, Tokens &end);
	void	setUploadPath(Tokens &token, Tokens &end);
	void	validateOrFaild(Tokens &token, Tokens &end);
	void	CheckIfEnd(Tokens &token, Tokens &end);
	std::string &consume(Tokens &token, Tokens &end);
	GlobalConfig &operator=(const GlobalConfig &globalParam);

	//INFO:
	bool isMethodAllowed(int method) const;
	std::string getRoot() const;
	bool		getAutoIndex() const;
	
	GlobalConfig();
	~GlobalConfig();

	bool parseTokens(Tokens &token, Tokens &end);

	static bool IsId(std::string &token);

	void setRoot(Tokens &token, Tokens &end);


	void setAutoIndex(Tokens &token, Tokens &end);

	void setAccessLog(Tokens &token, Tokens &end);
	std::string getAccessLog() const;

	void setErrorLog(Tokens &token, Tokens &end);
	std::string getErrorLog() const;


	const std::vector<std::string> &getIndexes();
	void setIndexes(Tokens &token, Tokens &end);
	// TODO Indexes getter may be with caching ??

	void setErrorPages(Tokens &token, Tokens &end);
	const std::string&getErrorPage(std::string &StatusCode);
		
};

#endif
