#ifndef Location_H
# define Location_H

#include <string>
#include <vector>
#include "DataType.hpp"

class Location
{
	private:
		std::string					path; // todo as redix tree
		std::string					redirection[2];
		GlobalParam					globalParam;
	public:
		Location(std::string &path);
};
#endif
