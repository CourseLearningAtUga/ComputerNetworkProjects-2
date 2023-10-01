#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define SERVER_IP "cobweb.cs.uga.edu"
#define SERVER_PORT 443

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    SSL_CTX *ctx;
    SSL *ssl;
    char request[] = "GET /~perdisci/CSCI6760-F21/Project2-TestFiles/story_hairydawg_UgaVII.jpg HTTP/1.1\r\nHost: cobweb.cs.uga.edu\r\nConnection: close\r\n\r\n";
    char response[4096];

    // Initialize OpenSSL
    SSL_library_init();
    ctx = SSL_CTX_new(SSLv23_client_method());

    if (ctx == NULL) {
        perror("SSL_CTX_new");
        exit(EXIT_FAILURE);
    }

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // Create an SSL object and associate it with the socket
    ssl = SSL_new(ctx);
    if (ssl == NULL) {
        perror("SSL_new");
        exit(EXIT_FAILURE);
    }

    SSL_set_fd(ssl, sockfd);

    // Establish the SSL/TLS connection
    if (SSL_connect(ssl) != 1) {
        perror("SSL_connect");
        exit(EXIT_FAILURE);
    }

    // Send the HTTP request
    if (SSL_write(ssl, request, strlen(request)) <= 0) {
        perror("SSL_write");
        exit(EXIT_FAILURE);
    }

    // Receive and print the HTTP response
    int bytes_received = 0;
    while ((bytes_received = SSL_read(ssl, response, sizeof(response))) > 0) {
        fwrite(response, 1, bytes_received, stdout);
        printf("response= %s",response);
    }

    // Clean up
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sockfd);
    SSL_CTX_free(ctx);

    return 0;
}
