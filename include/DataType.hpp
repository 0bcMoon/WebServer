#ifndef DataType_H
# define DataType_H

#include <map>
#include <string>
#include <vector>

struct CGI 
{
	std::string cgiPath;
	std::string cgiExt;
	CGI() 
		: cgiPath(""), cgiExt("") {}
	CGI(std::string &path, std::string &ext) 
		: cgiPath(path), cgiExt(ext) {}
};

struct ErrorPage
{
	std::map<int, std::string> errorPages;
	ErrorPage(); // todo default pages


};
struct GlobalParam
{
	std::vector<std::string>	errorPages;
	std::string					accessLog;
	std::string					errorLog;
	CGI							cgi;
	bool						autoIndex;
	long						maxBodySize; // in bytes
	std::vector<std::string>	indexes;
	std::string					root;
};

#endif	
