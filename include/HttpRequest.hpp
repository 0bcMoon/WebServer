#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

enum methode
{
	GET,
	POST,
	DELETE
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
	REQUEST_FINISH
};

class HttpRequest 
{
	private:
		// enum methode;
	public:

};
#endif
