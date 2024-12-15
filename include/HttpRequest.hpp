#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include "Location.hpp"
#include <sys/event.h>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

#define URI_MAX		 2048
#define REQSIZE_MAX  1024*10240
#define BUFFER_SIZE  5 * 1024*1024
#define BODY_MAX	 1024*10240000

typedef std::map<std::string, std::string>::iterator map_it; // WARNING 

enum reqMethode
{
	GET  = 0b1,
	POST = 0b10,
	DELETE = 0b100,
	NONE = 0
};

enum reqState
{
	NEW,
	METHODE,
	PATH,
	HTTP_VERSION,
	REQUEST_LINE_FINISH,
	HEADER_NAME,
	HEADER_VALUE,
	HEADER_FINISH,
	BODY,
	BODY_FINISH,
	REQUEST_FINISH,
	REQ_ERROR,
	DEBUG
};

enum chunkState {
	SIZE,
	LINE,
	END_LINE
};

enum crlfState {
	READING,
	NLINE,
	RETURN,
	LNLINE,
	LRETURN
};


typedef struct httpError 
{
	int         code;
	std::string description;
} httpError;

typedef struct methodeStr
{
	std::string							tmpMethodeStr;
	std::string							eqMethodeStr;
	
} methodeStr;

enum reqBodyType {
	MULTI_PART,
	TEXT_PLAIN,
	URL_ENCODED,
	NON
};

struct multiPart 
{
	std::vector<char>						body;
	int										fd;
	std::map<std::string, std::string>		headers;
	// std::vector<std::string>				strsHeaders;
};

struct bodyHandler {
	bodyHandler();
	~bodyHandler();	
	void								clear();
	void								push2body(char c);
	int									push2fileBody(char c, const std::string &boundary);
	int									upload2file(std::string &boundary);
	int									openNewFile();
	int									writeBody();
	bool								isCgi;

	std::map<std::string, std::string>	headers;

	int									bodyFd;
	size_t								bodyIt;
	std::vector<char>					body;//raw body;
	size_t								bodySize;

	std::string							header;
	std::string							tmpBorder;

	int									currFd;
	size_t								fileBodyIt;
	size_t								borderIt;
	std::vector<char>				    fileBody;
};

typedef struct data_s {
	std::string                         path;
	std::string							queryStr;
	
	bool									isRequestLineValid;

	std::string							strMethode;
	std::map<std::string, std::string>	headers;

	httpError							error;
	enum reqState						state;

	bodyHandler							bodyHandler;
	
} data_t;

class HttpRequest 
{
	private:
		enum multiPartState {
			_ERROR,
			_NEW,
			BORDER,
			MULTI_PART_HEADERS,
			BODY_CRLF,
			MULTI_PART_HEADERS_VAL,
			STORING,
			FINISHED
		};

		bool isCGI();
		void						handleNewBody();
		void						handleMultiPartHeaders();
		void						handleStoring();
		void						parseMultiPartHeaderVal();
		int							isBodycrlf();
		int							isBorder();
		int							checkMultiPartEnd();
		void						parseBodyCrlf();
		bool						isMethodAllowed();

		enum multiPartState							bodyState;
		struct kevent *ev;
		typedef std::map<std::string, std::string> Headers;
		 chunkState							chunkState;
		size_t								totalChunkSize;
		size_t								chunkSize;
		size_t								chunkIndex;
		std::string							sizeStr;

		const int							fd;
		enum crlfState						crlfState;

		// std::string                         path;

		// std::map<std::string, std::string>	headers;
		std::string							currHeaderName;
		std::string							currHeaderVal;

		std::vector<char>							body;
		long long									bodySize;
		int                                 reqSize;
		size_t								reqBufferSize;
		size_t								reqBufferIndex;
		std::vector<char>					reqBuffer;

		httpError							error;

		methodeStr							methodeStr;
		std::string							httpVersion;


		int 		convertChunkSize();
		void			chunkEnd();


		void		parseMethod();
		void		parsePath();
		void		parseHttpVersion();
		void		parseHeaderName();
		void		parseHeaderVal();
		void		parseBody();
		void		crlfGetting();

		int			firstHeadersCheck();

		int			verifyUriChar(char c);
		void		checkHttpVersion(int *state);

		void		contentLengthBodyParsing();
		void		chunkedBodyParsing();

		void		returnHandle();
		void		nLineHandle();
		int			checkContentType();

		int			parseMuliPartBody();
		void		andNew();
		

	public:
		std::vector<data_t *>         data;
		void										clearData();
		reqBodyType									reqBody;
		std::string									bodyBoundary;
		enum reqState								state;
		// std::vector<multiPart>						multiPartBodys;
		Location									*location;
		bool										eof;

		HttpRequest();
		HttpRequest(int fd);
		~HttpRequest();

		void		setHttpReqError(int code, std::string str);
		void		feed();
		void		readRequest(int data);

		void		setFd(int fd);

		static int	isNum(const std::string& str);
		void		clear();

		const std::map<std::string, std::string>	&getHeaders() const;
		std::vector<char>					getBody() const;
		httpError							getStatus() const;
		std::string							getStrMethode() const;
		const std::string					&getHost() const;
		bool								validateRequestLine();

		void								splitingQuery();
		void								decodingUrl();
		
		int 								parseMultiPart();
};

#endif
