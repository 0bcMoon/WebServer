
#include "Log.hpp"
#include <ctime>
#include <iomanip>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Tokenizer.hpp"

std::ofstream Log::access_log;
std::ofstream Log::error_log;

//
void Log::Error(const HttpRequest &req) 
{
	Log::displayTimestamp(ERROR);
	req.getHost();
}

void Log::Error(const HttpResponse &res) 
{
	Log::displayTimestamp(ERROR);
}

void Log::Access(HttpRequest &req) 
{
	Log::displayTimestamp(ACCESS);
}

void Log::Access(HttpResponse &res) 
{
	Log::displayTimestamp(ACCESS);
}

void Log::connect(int fd)
{
	Log::displayTimestamp(ACCESS);
	Log::access_log << "a new client connect : " << fd << std::endl;
}
void Log::disconnected(int fd) 
{
	Log::displayTimestamp(ERROR);
	Log::access_log << "a client disconnected : " << fd << std::endl;
}


void Log::Access() {}

void Log::displayTimestamp(enum LogType logtype)
{
	time_t now = std::time(0);
	struct tm *tstruct = std::localtime(&now);

	if (!tstruct)
		return;
	else if (logtype == ERROR)
		Log::error_log << std::put_time(tstruct, "[%Y-%m-%d %H:%M:%S] [Error] :");
	else if (logtype == ACCESS)
		Log::access_log << std::put_time(tstruct, "[%Y-%m-%d %H:%M:%S] :");
}

void Log::setErrorLogFile(const std::string &error)
{
	Log::error_log.open(error, std::ios::out);

	if (!Log::error_log.is_open())
		throw Tokenizer::ParserException("could not open Error log file :" + error);
}

void Log::setAccessLogFile(const std::string &access)
{
	Log::access_log.open(access, std::ios::out);
	if (!Log::access_log.is_open())
		throw Tokenizer::ParserException("could not open Access log file :" + access);
}

void Log::init()
{
	if (!Log::error_log.is_open())
		Log::setErrorLogFile("./error.log");

	if (!Log::access_log.is_open())
		Log::setAccessLogFile("./access.log");
}

void Log::close()
{
	if (Log::error_log.is_open())
		Log::error_log.close();

	if (Log::access_log.is_open())
		Log::access_log.close();
}
