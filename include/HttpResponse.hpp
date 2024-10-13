#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include "HttpRequest.hpp"
#include "Location.hpp"


enum responseState 
{
	START,
	ERROR
};

class HttpResponse
{
	private:
		int									fd;
		enum reqMethode						methode;
		std::vector<unsigned char>							body;
		std::string							strMethod;
		httpError							status;	
		bool								isCgiBool;
	public:
		Location							*location;
		enum responseState					state;
		std::string                         path;
		std::map<std::string, std::string>	headers;

		HttpResponse(int fd);
		HttpResponse	operator=(const HttpRequest& req);

		void							responseCooking();
		bool							isCgi();
		void							cgiCooking();

		int								getStatusCode() const;
		std::string						getStatusDescr() const;

		bool							isPathFounded() const;
		bool							isMethodAllowed() const;
		// Location *location;
};

#endif
