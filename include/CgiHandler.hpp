#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Location.hpp"
#include "ServerContext.hpp"
#include <map>
#include <string>
class CgiHandler {
	private:
		std::map<std::string, std::string>	env;
		std::string							cgiPath;
		std::string							scriptName;
		std::string							scriptPath;

		HttpResponse						*response;
		// std::string							reqPath;
		// httpError							&status;							
		// Location							*location;
		// ServerContext						*ctx;
	public:
		// CgiHandler(std::string reqPath, httpError& status, Location *location, ServerContext *ctx);

		CgiHandler(HttpResponse &response);

		int					checkCgiFile();
		void					execute();
		int						initEnv();
};
#endif
