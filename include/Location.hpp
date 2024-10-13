#ifndef Location_H
# define Location_H

#include <string>
#include <vector>
#include "DataType.hpp"

class Location
{
	private:
		struct Redirection // if  there 4xx or 5xx error it return error page else 3xx it redirect to url
		{
			std::string status;
			std::string url; // will  file path if status != 3xx 
		};
		Redirection					redirect;
		std::string					path; // todo as redix tree
	public:
		Location();
		Location &operator=(const Location &location);
		void setPath(std::string &path);
		std::string &getPath();
		void setRedirect(Tokens &token, Tokens &end);
		void parseTokens(Tokens &token, Tokens &end);
		static bool isValidStatusCode(std::string &str);

		GlobalConfig				globalConfig;
};
#endif
