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
		std::string							cgiPath;
		std::string							scriptName;
		std::string							scriptPath;

		HttpResponse						*response;

		char								**envArr;
		char								**argv;
		std::map<std::string, std::string>	env;
	public:
		// CgiHandler(std::string reqPath, httpError& status, Location *location, ServerContext *ctx);

		CgiHandler(HttpResponse &response);

		~CgiHandler();
		void					envMapToArr(std::map<std::string, std::string> mapEnv);
		int						checkCgiFile();
		void					execute(std::string cgiPath);
		int						initEnv();
		void					initArgv();
		char					**getEnv() const;
		char					**getArgv() const;
};
#endif
