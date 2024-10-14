#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include "HttpRequest.hpp"
#include "Location.hpp"
#include <string>


enum responseState 
{
	START,
	ERROR
};

enum pathType
{
	DIR,
	INDEX,
	DEF_INDEX
};


class HttpResponse
{
	private:
		struct errorResponse 
		{
			std::string		statusLine;
			std::string		headers;
			std::string		connection;
			std::string		contentLen;

			std::string		bodyHead;
			std::string		title;
			std::string		body;
			std::string		htmlErrorId;
			std::string		bodyfoot;
		};
		int									fd;
		enum reqMethode						methode;
		std::vector<unsigned char>							body;
		std::string							strMethod;
		httpError							status;	
		bool								isCgiBool;
		errorResponse						errorRes;
		std::string							fullPath;
	public:
		int									keepAlive;
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

		bool							isPathFounded();
		bool							isMethodAllowed() const;
		int								pathChecking();
		void							setHttpResError(int code, std::string str);

		std::string						getErrorRes();
		std::string						getContentLenght();
		int							directoryHandler();
};

#endif
