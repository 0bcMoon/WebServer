#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

enum clientState
{
	REQUEST,
	RESPONSE
};

class Client
{
	private:
		int fd;	
	public:
		enum clientState	state;

		Client();
		Client(int	fd);

		HttpRequest		request;
		HttpResponse	response;

		int				getFd() const;

		void			respond();
};

#endif
