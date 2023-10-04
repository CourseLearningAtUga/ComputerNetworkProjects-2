#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <netdb.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <URL>\n", argv[0]);
        return 1;
    }

    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
    if (!ctx) {
        fprintf(stderr, "SSL_CTX_new failed.\n");
        return 1;
    }

    int sock;
    struct hostent *host;
    struct sockaddr_in server_addr;

    char *url = argv[1];
    char *hostname;
    char *path;

    // Parse the URL to extract hostname and path
    if (strncmp(url, "https://", 8) == 0) {
        url += 8; // Skip "https://"
    }

    char *slash = strchr(url, '/');
    if (slash) {
        *slash = '\0'; // Split hostname and path
        hostname = url;
        path = slash + 1;
    } else {
        hostname = url;
        path = "";
    }

    host = gethostbyname(hostname);
    if (!host) {
        fprintf(stderr, "Unable to resolve hostname: %s\n", hostname);
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(443); // HTTPS default port
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(server_addr.sin_zero), 8);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("Connect");
        return 1;
    }

    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        fprintf(stderr, "SSL_new failed.\n");
        return 1;
    }

    SSL_set_fd(ssl, sock);

    if (SSL_connect(ssl) != 1) {
        fprintf(stderr, "SSL_connect failed.\n");
        return 1;
    }

    // Send an HTTP GET request
    char request[1024];
    snprintf(request, sizeof(request), "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, hostname);
    if (SSL_write(ssl, request, strlen(request)) <= 0) {
        fprintf(stderr, "SSL_write failed.\n");
        return 1;
    }
    FILE *fd=fopen("temp.txt", "wb");
    // Read and print the response
    char response[4096];
    int bytes_received;
    while ((bytes_received = SSL_read(ssl, response, sizeof(response))) > 0) {
        fwrite(response, 1, bytes_received, fd);
    }
    fclose(fd);
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(sock);

    return 0;
}
