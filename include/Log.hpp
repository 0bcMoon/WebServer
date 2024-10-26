#ifndef Log_H
# define Log_H

#include <fstream>
#include <string>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
class Log
{
	private:
		enum LogType
		{
			ERROR,
			ACCESS
		};
		static std::ofstream access_log;
		static std::ofstream error_log;
		static void displayTimestamp(enum LogType logtype);

	public:
		static void setErrorLogFile(const std::string &error);
		static void setAccessLogFile(const std::string &access);
		static void Error();

		void connect(int fd);
		void disconnected(int fd);

		static void Error(const HttpRequest &req);
		static void Error(const HttpResponse &res) ;

		void Access(HttpRequest &req);
		void Access(HttpResponse &res) ;
		static void Access();
		static void close();
		static void init();
};

#endif
