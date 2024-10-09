#include "Client.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

int Client::getFd() const
{
	return (fd);
}

Client::Client() : response(-1)
{
	fd = -1;
	state = REQUEST;
}

Client::Client(int	fd) : fd(fd), request(fd), response(fd)
{
	state = REQUEST;
}

void		Client::respond()
{
	state = RESPONSE;
	response = request;
	request.clear();

}
