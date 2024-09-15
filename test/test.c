
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT 8080
#define BUFFER_SIZE 4096

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];
    const char *host = "localhost";
    const char *request = 
        "GET /  HTTP/1.000000000000009\r\n"
        // "Connection: close\r\n"
		// "Content-Length: 67"
		"Host:ana\r\n"
        "\r\n"
		"JLK: ana\r\n"
		"hjkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkklllllllllllllll"
		;

    // Resolve the hostname
    server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "Error: no such host\n");
        exit(EXIT_FAILURE);
    }

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("Error opening socket");
    }

    // Setup the server address structure
    bzero((char *) &servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&servaddr.sin_addr.s_addr, server->h_length);
    servaddr.sin_port = htons(PORT);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        error("Error connecting");
    }


    if (write(sockfd, request, strlen(request)) < 0) {
        error("Error writing to socket");
    }
    // Send the HTTP GET request
    // if (write(sockfd, request, 30) < 0) {
    //     error("Error writing to socket");
    // }
	// sleep(2);
 //    if (write(sockfd, request + 31, 26) < 0) {
 //        error("Error writing to socket");
	// }

    // Read the response
    bzero(buffer, BUFFER_SIZE);
    ssize_t n;
    while ((n = read(sockfd, buffer, BUFFER_SIZE - 1)) > 0) {
        printf("%s", buffer);
        bzero(buffer, BUFFER_SIZE);
    }

    if (n < 0) {
        error("Error reading from socket");
    }

    // Close the socket
    close(sockfd);

    return 0;
}
