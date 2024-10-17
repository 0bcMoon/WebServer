#include "CgiHandler.hpp"
#include "HttpResponse.hpp"
#include <sys/unistd.h>
#include <unistd.h>


// CgiHandler::CgiHandler(std::string reqPath, httpError& status, Location *location, ServerContext *ctx) : reqPath(reqPath), status(status), location(location), ctx(ctx)
// {
// }

CgiHandler::CgiHandler(HttpResponse& response) : response(&response)
{

}

void		CgiHandler::execute()
{
	if (!initEnv())
		return ;
}

int		CgiHandler::checkCgiFile()
{
	scriptPath = response->location->globalConfig.getRoot() + response->path;
	if (access(scriptPath.c_str(), F_OK) == -1)
		return (response->setHttpResError(404, "Not Found"), 0);
	if (access(scriptPath.c_str(), X_OK) == -1)
		return (response->setHttpResError(403, "Forbidden"), 0);
	return (1);
}

int			CgiHandler::initEnv()
{
	if (!checkCgiFile())
		return (0);
	env["SERVER_SOFTWARE"] = "macOS";
	env["SERVER_NAME"] = response->headers["Host"];//TODO:server
	env["GATEWAY_INTERFACE"] = "CGI/1.1";
	env["SERVER_PROTOCOL"] = "HTTP/1.1";
	env["SERVER_PORT"] = "";//TODO
	env["REQUEST_METHOD"] = response->strMethod;
	env["PATH_INFO"];
	return (1);
}
