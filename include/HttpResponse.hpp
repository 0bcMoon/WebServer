#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include "HttpRequest.hpp"
class HttpResponse
{
	private:
		int									fd;
		enum reqMethode						methode;
		std::string							strMethod;
		std::string                         path;
		std::map<std::string, std::string>	headers;
		std::vector<int>							body; // TODO : body  may be binary (include '\0') fix this;
		httpError							status;
	public:
		HttpResponse(int fd);
		HttpResponse operator=(const HttpRequest& req);
};

#endif
