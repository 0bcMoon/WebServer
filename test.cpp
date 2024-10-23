#include <iostream>
#include <libc.h>
#include <sys/fcntl.h>

int main()
{
	int mainFd = open("/tmp/84Y64r8Wi5NnHHMkIKkSEZAE", O_RDONLY);
	int copyFd = open("main", O_RDONLY);

	char buff1;
	char buff2;
	while (1)
	{
	}
}
