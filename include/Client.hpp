#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Location.hpp"
#include "ServerContext.hpp"

enum clientState
{
	REQUEST,
	RESPONSE
};

class Client
{
	private:
		int fd;	
		int serverFd;
	public:
		enum clientState	state;
		Location *location;

		Client();
		Client(int	fd);
		Client(int	fd, int server, ServerContext *ctx);

		HttpRequest		request;
		HttpResponse	response;

		int					getFd() const;
		int					getServerFd() const;
		const std::string	&getHost() const;	
		const std::string	&getPath() const;
		void				respond(size_t data);
};

#endif
