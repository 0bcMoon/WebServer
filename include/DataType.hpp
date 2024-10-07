#ifndef DataType_H
#define DataType_H

#include <map>
#include <string>
#include <vector>

typedef std::vector<std::string>::iterator Tokens;

struct ErrorPage
{
  private:
	struct Page
	{
		std::string		content;
		bool			IsRedirection;
	};

  public:
	std::map<int, Page> errorPages;
	ErrorPage() {}
};

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
	std::string							accessLog;
	std::string							errorLog;

	std::string							root;
	bool								autoIndex;
	long								maxBodySize; // in bytes
	long								maxHeaderSize; // in bytes
	ErrorPage							errorPages;
	std::map<std::string, std::string>	cgiMap;
	std::vector<std::string>			indexes;

  public:
	void loadFile(Tokens &token, Tokens &end, std::string &buffer);
	bool isValidStatusCode(std::string &str);
	void setMethods(Tokens &token, Tokens &end);
	bool isMethodAllowed(const std::string &method) const;
	void validateOrFaild(Tokens &token, Tokens &end);
	void CheckIfEnd(Tokens &token, Tokens &end);
	std::string &consume(Tokens &token, Tokens &end);
	GlobalConfig &operator=(const GlobalConfig &globalParam);
	GlobalConfig();
	~GlobalConfig();

	bool parseTokens(Tokens &token, Tokens &end);

	static bool IsId(std::string &token);

	void setRoot(Tokens &token, Tokens &end);

	std::string getRoot() const;

	void setAutoIndex(Tokens &token, Tokens &end);
	bool getAutoIndex() const;

	void setAccessLog(Tokens &token, Tokens &end);
	std::string getAccessLog() const;

	void setErrorLog(Tokens &token, Tokens &end);
	std::string getErrorLog() const;

	void setMaxBodySize(Tokens &token, Tokens &end);
	long getMaxBodySize() const;

	void setMaxHeaderSize(Tokens &token, Tokens &end);
	long getMaxHeaderSize() const;

	void setIndexes(Tokens &token, Tokens &end);
	// TODO Indexes getter may be with caching ??

	void setCGI(Tokens &token, Tokens &end);
	// TODO CGI getter may be with caching ??
	void setErrorPages(Tokens &token, Tokens &end);
};

#endif
