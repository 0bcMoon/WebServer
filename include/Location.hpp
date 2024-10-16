#ifndef Location_H
# define Location_H

#include <map>
#include <string>
#include <vector>
#include "DataType.hpp"
#include <set>

class Location
{
	private:

	typedef int http_method_t;
	enum methods_e
	{
		GET = 0b1,
		POST = 0b10,
		DELETE = 0b100,
	};
		struct Redirection
		{
			std::string status;
			std::string url;
			std::string body;// INFO: body file name;
		};

		Redirection							redirect;
		http_method_t						methods;
		bool								isRedirection;
		std::string							path;
		std::map<std::string, std::string>	cgiMap;
		std::string							upload_file_path;

	public:

		bool isMethodAllowed(int method) const;
		Location();
		Location							&operator=(const Location &location);
		void								setPath(std::string &path);
		const std::string					&geCGItPath(std::string &ext);
		const std::string					&getPath();
		const std::string					&geCGIext();
		void								setRedirect(Tokens &token, Tokens &end);
		void								parseTokens(Tokens &token, Tokens &end);
		static bool							isValidStatusCode(std::string &str);
		GlobalConfig						globalConfig;
		bool								HasRedirection() const ;
		const Redirection					&getRedirection() const;
		void								setCGI(Tokens &token, Tokens &end);
		void setMethods(Tokens &token, Tokens &end);
};
#endif
