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
		enum responseType 
		{
			LOAD_FILE,	
			NO_TYPE
		};
		enum reqMethode
		{
			GET  = 0b1,
			POST = 0b10,
			DELETE = 0b100,
			NONE = 0
		};
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
		enum responseType					resType;
		int									fd;
		enum reqMethode						methode;
		std::vector<unsigned char>							body;
		std::string							strMethod;
		httpError							status;	
		bool								isCgiBool;
		errorResponse						errorRes;

		std::string							fullPath;
		static const int					fileReadingBuffer = 10240;
	public:
		std::vector<std::vector<unsigned char> >			responseBody;
		int													keepAlive; // bool? // bool?
		Location											*location;
		enum responseState									state;
		std::string											path;
		std::map<std::string, std::string>					headers;
		std::map<std::string, std::string>					resHeaders;

		HttpResponse(int fd);
		HttpResponse	operator=(const HttpRequest& req);

		void							responseCooking();
		bool							isCgi();
		void							cgiCooking();

		int								getStatusCode() const;
		std::string						getStatusDescr() const;

		bool							isPathFounded();
		bool							isMethodAllowed();
		int								pathChecking();
		void							setHttpResError(int code, std::string str);

		std::string						getErrorRes();
		std::string						getContentLenght(); // TYPO
		int								directoryHandler();
		int								loadFile(const std::string& pathName);
};

std::string decimalToHex(int decimal);
#endif
