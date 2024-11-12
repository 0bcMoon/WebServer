#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
int main()
{
	printf("");
	const char buffer[] =
		"HTTP/1.1 200 OK\n"
		"Connection: keep-alive\n"
		"Content-Type: text/html\n"
		"Content-Length: 19\n"
		"\n\r"
		"<h1>Hello world<h1>\n";
	char buffer2[4096] = {0};
	int r = read(1, buffer2, 4096 - 1);
	buffer2[r] = 0;
	if (write(1, buffer, strlen(buffer)) < 0)
	{
		printf("Error %s\n", strerror(errno));
		return (1);
	}		
	fprintf(stderr, "read %d into :%s\n", r, buffer2);
	return (0);
}
