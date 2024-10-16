#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <vector>

std::string loadFile(const char *filename)
{
	struct stat buff;
	std::ifstream f(filename);

	if (stat(filename, &buff) != 0)
		throw std::runtime_error("Root directory does not exist");
	if (S_ISDIR(buff.st_mode) != 0)
		throw std::runtime_error("Root is not a directory");
	std::cout << buff.st_size << '\n';
	std::vector<char> data(buff.st_size);
	f.read(&data[0], buff.st_size);
	return "";
}
int main () {
	
	loadFile("test/test.py");
	return 0;
}
