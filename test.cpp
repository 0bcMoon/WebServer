#include <iostream>
#include <libc.h>
#include <sys/fcntl.h>

int main()
{
	std::string url = "/dir";
	std::string root = "/Users/hibenouk/Desktop/";
	std::string req = "/dir/fileana";
	size_t n = url.size();
	std::cout << req.substr(n) << "\n";
}
