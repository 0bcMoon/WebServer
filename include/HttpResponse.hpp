#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include "DataType.hpp"
#include "HttpRequest.hpp"
#include "Location.hpp"
#include "ServerContext.hpp"
#include <cstddef>
#include <string>

enum responseState 
{
	START,
	WRITE_BODY,
	ERROR,
	CGI_EXECUTING,
	WRITE_ERROR,
	UPLOAD_FILES,
	END_BODY,
	START_CGI_RESPONSE
};

enum pathType
{
	_DIR,
	INDEX,
	DEF_INDEX
};

class HttpResponse
{
	private:
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

		enum cgiResponeState
		{
			HEADERS,
			BODY,
			PARSING_DONE
		};
		struct cgiRespone 
		{
			cgiResponeState	state;
			size_t			bodyStartIndex;
			std::string		cgiStatusLine;
			std::vector<std::vector<char > > lines;// mok li kadiro
		};
		
		cgiRespone							cgiRes;
		enum reqMethode						methode;
		std::vector<char>					body; // seperate 
		httpError							status;	
		bool								isCgiBool;
		errorResponse						errorRes;

		std::string							fullPath;
		static const int					fileReadingBuffer = 10240;
		std::string							autoIndexBody;
		ServerContext						*ctx;
		HttpRequest							*request;
		char								buff[BUFFER_SIZE]; // TODO: make me 
		std::string							errorPage;	
		bool								isErrDef;	
		int									parseCgistatus();
	public:
		struct upload_data {
			size_t			 it;
			size_t		     fileIt;
			std::string		 fileName;
			int				 __fd;
		};
		enum responseBodyType 
		{
			LOAD_FILE,	
			NO_TYPE,
			AUTO_INDEX,
			CGI
		};
		std::vector<char>					CGIOutput;

		enum responseBodyType				bodyType;
		size_t								writeByte;
		size_t								eventByte;
		int									responseFd;
		void								write2client(int fd, const char *str, size_t size);

		size_t								fileSize;
		size_t								sendSize;
		int									fd;
		std::string							cgiOutFile;

		class IOException : public std::exception
		{
			private:
				std::string msg;
			public :
				IOException(const std::string &msg) throw();
				IOException() throw();
				~IOException() throw();
				virtual const char* what() const throw();
		};

		static std::string getRandomName();
		std::string											queryStr;
		std::string											path_info; // TODO: clear all used varible
		std::string											server_name;
		int													server_port;
		std::string											bodyFileName; // TODO: is this should be here
		std::string											getCgiContentLenght();
		int													parseCgiHaders(std::string str);
		std::string											strMethod;
		bool												keepAlive;
		Location											*location;
		enum responseState									state;
		std::string											path;
		std::map<std::string, std::string>					headers;
		std::map<std::string, std::string>					resHeaders;

		HttpResponse(int fd, ServerContext *ctx, HttpRequest *request);
		HttpResponse&	operator=(const HttpRequest& req);
		void			clear();
		~HttpResponse();

		void							responseCooking();
		bool							isCgi();

		int								getStatusCode() const;
		std::string						getStatusDescr() const;

		bool							isPathFounded();
		bool							isMethodAllowed();
		int								pathChecking();
		void							setHttpResError(int code, const std::string &str);

		std::string						getErrorRes();
		std::string						getContentLenght(); // TYPO
		int								directoryHandler();
		int								loadFile(const std::string& pathName);
		int								loadFile(int _fd);//for cgi

		void							writeResponse();

		std::string						getStatusLine();
		std::string						getConnectionState();
		std::string						getContentType();
		std::string						getDate();
		int								sendBody(int _fd, enum responseBodyType type);
		std::string						getContentLenght(enum responseBodyType type); // TYPO

		int								autoIndexCooking();
		static std::string						getExtension(const std::string &str);

		std::vector<char>				getBody() const;


		void							parseCgiOutput();
		void							writeCgiResponse();

		void							decodingUrl();
		void							splitingQuery();
		int								uploadFile();
		void							multiPartParse();
		void							deleteMethodeHandler();
};

int					isHex(char c);
std::string			decimalToHex(int	decimal);
std::string			getRandomName();

#endif
