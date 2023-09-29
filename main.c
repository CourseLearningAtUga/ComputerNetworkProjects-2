
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#define BUFFER_SIZE 4096

int create_socket(const char *host, const char *port) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        perror("getaddrinfo");
        return -1;
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, 0);
    if (sockfd == -1) {
        perror("socket");
        return -1;
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

void downloadfile(char *httplink,char *filename){
printf("%s\n",httplink);
printf("%s\n",filename);


    const char *host =httplink; // Change to the desired host
    const char *port = "80";          // HTTP port

    int sockfd = create_socket(host, port);
    if (sockfd == -1) {
        return 1;
    }

    // HTTP 1.1 GET request
    const char *request = "GET / HTTP/1.1\r\nwww.google.com\r\n"
                          "Connection: close"
                          "Host: \r\n\r\n";

    if (send(sockfd, request, strlen(request), 0) == -1) {
        perror("send");
        close(sockfd);
        return 1;
    }

    char response_buffer[BUFFER_SIZE];
    int bytes_received;

    while ((bytes_received = recv(sockfd, response_buffer, BUFFER_SIZE - 1, 0)) > 0) {
        response_buffer[bytes_received] = '\0';
        printf("%s", response_buffer);
    }

    close(sockfd);

}


int main(){
    char *httplink="https://cobweb.cs.uga.edu/~perdisci/CSCI6760-F21/Project2-TestFiles/topnav-sport2_r1_c1.gif";
    char *filename="image.gif";
    int numberofparts=5;
    downloadfile(httplink,filename);
    
    return 0;
}