#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFER_SIZE 1024

int main() {
    const char *hostname = "127.0.0.1";
    const char *port = "8080";
    const char *path = "/";

    // Resolve hostname to IP address
    struct addrinfo hints, *res, *p;
    int sockfd;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP

    if (getaddrinfo(hostname, port, &hints, &res) != 0) {
        perror("getaddrinfo");
        return 1;
    }

    // Connect to the first available result
    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0) break;
        close(sockfd);
    }

    if (p == NULL) {
        fprintf(stderr, "Failed to connect to the server\n");
        return 1;
    }

    freeaddrinfo(res);

    // Prepare the HTTP GET request
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             path, hostname);

    // Send the request
    if (send(sockfd, request, strlen(request), 0) == -1) {
        perror("send");
        close(sockfd);
        return 1;
    }

    // Receive the response
    printf("Response:\n");
    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0'; // Null-terminate the buffer
        printf("%s", buffer);
    }

    if (bytes_received == -1) {
        perror("recv");
    }

    close(sockfd);
    return 0;
}
