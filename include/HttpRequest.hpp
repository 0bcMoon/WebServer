#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <sys/event.h>
#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

#define URI_MAX		 2048
#define REQSIZE_MAX  1000000000
#define BODY_MAX	 100000000

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
	std::vector<char>				body;
	std::map<std::string, std::string>		headers;
	std::vector<std::string>				strsHeaders;
};

class HttpRequest 
{
	private:
		struct kevent *ev;
		typedef std::map<std::string, std::string> Headers;
		 chunkState							chunkState;
		size_t								totalChunkSize;
		size_t								chunkSize;
		size_t								chunkIndex;
		std::string							sizeStr;

		const int							fd;
		enum crlfState						crlfState;

		std::string                         path;

		std::map<std::string, std::string>	headers;
		std::string							currHeaderName;
		std::string							currHeaderVal;

		std::vector<char>							body; // TODO : body  may be binary (include '\0') fix this;
		int									bodySize;
		int                                 reqSize;
		size_t								reqBufferSize;
		size_t								reqBufferIndex;
		// std::string							reqBuffer; // buffer  may be binary (include '\0') fix this;
		std::vector<char>							reqBuffer; // buffer  may be binary (include '\0') fix this;

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
		char		buffer[1000000];
	public:
		reqBodyType									reqBody;
		std::string									bodyBoundary;
		std::vector<multiPart>						multiPartBodys;
		enum reqState state;
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

		std::map<std::string, std::string>	getHeaders() const;
		std::vector<char>					getBody() const;
		httpError							getStatus() const;
		std::string							getStrMethode() const;
		const std::string &getHost() const;
		const std::string &getPath() const;
};

#endif
