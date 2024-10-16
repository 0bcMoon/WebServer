#ifndef Location_H
# define Location_H

#include <string>
#include <vector>
#include "DataType.hpp"
#include <set>

class Location
{
	private:
		struct Redirection
		{
			std::string status;
			std::string url;
			std::string body;// INFO: body file name;
		};

		Redirection							redirect;
		bool								isRedirection;
		std::string							path;
		std::string							cgi_path;
		std::string							cgi_ext;
		std::string							upload_file_path;

	public:
		Location();
		Location							&operator=(const Location &location);
		void								setPath(std::string &path);
		const std::string					&getPath();
		const std::string					&geCGItPath();
		const std::string					&geCGIext();
		void								setRedirect(Tokens &token, Tokens &end);
		void								parseTokens(Tokens &token, Tokens &end);
		static bool							isValidStatusCode(std::string &str);
		GlobalConfig						globalConfig;
		bool								HasRedirection() const ;
		const Redirection					&getRedirection() const;
		void								setCGI(Tokens &token, Tokens &end);
};
#endif
