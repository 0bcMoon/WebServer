#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <sys/event.h>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

#define URI_MAX		 2048
#define REQSIZE_MAX  1024*10240
#define BUFFER_SIZE  1024*10240
#define BODY_MAX	 1024*1024000

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
	std::vector<std::string>				strsHeaders;
};

typedef struct data_s {
	std::string                         path;
	std::string							strMethode;
	std::map<std::string, std::string>	headers;
	std::vector<char>					body;
	httpError							error;
	enum reqState								state;
	std::vector<multiPart>						multiPartBodys;
	
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

		void						handleNewBody();
		void						handleMultiPartHeaders();
		void						handleStoring();
		void						parseMultiPartHeaderVal();
		int							isBodycrlf();
		int							isBorder();
		int							checkMultiPartEnd();
		void						parseBodyCrlf();

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

		std::string                         path;

		// std::map<std::string, std::string>	headers;
		std::string							currHeaderName;
		std::string							currHeaderVal;

		std::vector<char>							body;
		int									bodySize;
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
		
		char						buffer[1000000];

	public:
		std::vector<data_t *>         data;
		void										clearData();
		reqBodyType									reqBody;
		std::string									bodyBoundary;
		enum reqState								state;
		// std::vector<multiPart>						multiPartBodys;

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
		const std::string					&getPath() const;
		
		int 								parseMultiPart();
};

#endif
