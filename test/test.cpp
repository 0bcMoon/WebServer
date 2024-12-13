#include <netdb.h>
#include <sys/netport.h>
#include <libc.h>
int main () 
{
	struct addrinfo hints, *result;
	int status;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP socket
	hints.ai_flags = AI_PASSIVE;     // Use my IP address for binding

	status = getaddrinfo("127.0.0.1", "80", &hints, &result);
	if (status != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}

	freeaddrinfo(result);
	return 0;
}
