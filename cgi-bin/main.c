#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main()
{
	const char buffer[] =
		"HTTP/1.1 200 OK\n"
		"Connection: keep-alive\n"
		"Content-Type: text/html\n"
		"\n\r"
		"<h1>Hello world<h1>\n"
		"Content-Type: text/html\n"
		"Content-Type: text/html";
	if (write(1, buffer, strlen(buffer)) < 0)
	{
		printf("Error %s\n", strerror(errno));
		return (1);
	}		
	return (0);
	
}
