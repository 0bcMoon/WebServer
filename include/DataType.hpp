#ifndef DataType_H
#define DataType_H

#include <map>
#include <string>
#include <vector>

typedef std::vector<std::string>::iterator Tokens;



class GlobalConfig
{
	private:
		std::map<std::string, std::string>  errorPages;
		std::string							root;
		int									autoIndex;
		std::string							upload_file_path;
		std::vector<std::string>			indexes;
		bool									IsAlias;

	public:

		bool		isValidStatusCode(std::string &str); // WARNING:t5arbi9
		void		setMethods(Tokens &token, Tokens &end);
		void		setAlias(Tokens &token, Tokens &end);
		void		validateOrFaild(Tokens &token, Tokens &end);
		void		CheckIfEnd(Tokens &token, Tokens &end);
		std::string &consume(Tokens &token, Tokens &end);
		GlobalConfig &operator=(const GlobalConfig &globalParam);

		//INFO:
		bool isMethodAllowed(int method) const;
		std::string getRoot() const;
		bool		getAutoIndex() const;

		GlobalConfig();
		GlobalConfig(int autoIndex, const std::string &upload_file_path);
		~GlobalConfig();
		GlobalConfig(const GlobalConfig &other);

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
		bool getAliasOffset() const ;

};

#endif
