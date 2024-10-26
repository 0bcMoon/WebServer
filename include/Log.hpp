#ifndef Log_H
# define Log_H

#include <fstream>
#include <string>
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
		void displayTimestamp(enum LogType logtype);

	public:
		void setLogFile(std::string &access, std::string &error);
		static void Error();
		static void Access();
};

#endif
