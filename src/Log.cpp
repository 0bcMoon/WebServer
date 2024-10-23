
#include "Log.hpp"
#include <ctime>
#include <iomanip>

// void Log::Error() {}

// void Log::Access() {}

// void Log::displayTimestamp(enum LogType logtype)
// {
// 	time_t now = std::time(0);

// 	struct tm *tstruct = std::localtime(&now);
// 	if (!tstruct)
// 		return;
// 	if (logtype == ERROR)
// 		Log::error_log << std::put_time(tstruct, "[%Y-%m-%d %H:%M:%S] ");
// 	else if (logtype == ACCESS)
// 		Log::access_log << std::put_time(tstruct, "[%Y-%m-%d %H:%M:%S] ");
// }
// void Log::setLogFile(std::string &access, std::string &error)
// {
// 	Log::access_log.open(access);
// }
