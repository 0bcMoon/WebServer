#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Location.hpp"
#include "ServerContext.hpp"

enum clientState
{
	REQUEST,
	RESPONSE,
	None,
};

class Client
{
	private:
		int				fd;	
		int				serverFd;
		ServerContext	*ctx;
	public:
		enum TimerType
		{
			KEEP_ALIVE,
			READING,
			NEW_CONNECTION
		};
		enum TimerType		timerType;
		enum clientState	state;
		Location			*location;
		HttpRequest			request;
		HttpResponse		response;
		int					cgi_pid;
		uint16_t			writeEventState;


		void							handleResponseError();

		Client(int	fd, int server, ServerContext *ctx);
		~Client();
		int					getFd() const;
		int					getServerFd() const;
		const std::string	&getHost() const;	
		const std::string	&getPath() const;
		void				respond(size_t data, size_t index);
		enum TimerType		getTimerType() const ;
};

#endif
