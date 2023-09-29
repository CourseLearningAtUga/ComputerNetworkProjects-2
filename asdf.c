#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFER_SIZE 10000

void binaryStringToJpeg(const char* binaryString, const char* outputFileName) {
    // Open the output file for writing
    FILE* outputFile = fopen(outputFileName, "wb");
    if (outputFile == NULL) {
        perror("Error opening output file");
        return;
    }

    // Determine the size of the binary string
    size_t dataSize = strlen(binaryString);

    // Write the binary string to the output file
    size_t bytesWritten = fwrite(binaryString, 1, dataSize, outputFile);

    if (bytesWritten != dataSize) {
        perror("Error writing binary data to the output file");
        fclose(outputFile);
        return;
    }

    // Close the output file
    fclose(outputFile);
}


int extract_http_body(const char *response, char *body, int max_body_size) {
    // Check if the response contains "\r\n\r\n" to separate headers from the body
    char *body_start = strstr(response, "\r\n\r\n");

    if (body_start == NULL) {
        // Response doesn't contain a body
        return 0;
    }

    // Move past the "\r\n\r\n" to the start of the body
    body_start += 4;

    // Calculate the length of the body
    int body_length = strlen(body_start);

    // Ensure the provided buffer is large enough
    if (body_length >= max_body_size) {
        return -1; // Buffer too small to hold the entire body
    }

    // Copy the body to the provided buffer
    strcpy(body, body_start);

    return body_length;
}
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


int main() {
    const char *host = "cobweb.cs.uga.edu"; // Change to the desired host
    const char *port = "80";          // HTTP port

    const char *path = "/~perdisci/CSCI6760-F21/Project2-TestFiles/story_hairydawg_UgaVII.jpg"; // Change to the desired path
    const char *output_file = "downloaded.jpg"; // Change to the desired output file name
    const char *output_file1 = "http response";
    int sockfd = create_socket(host, port);
    if (sockfd == -1) {
        return 1;
    }

    // HTTP 1.1 GET request
    char request[512];
    snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\n"
                                      "Host: %s\r\n"
                                      "Connection: close\r\n\r\n",
             path, host);

    if (send(sockfd, request, strlen(request), 0) == -1) {
        perror("send");
        close(sockfd);
        return 1;
    }

    FILE *file = fopen(output_file, "wb");
    if (file == NULL) {
        perror("fopen");
        close(sockfd);
        return 1;
    }
    FILE *file1 = fopen(output_file1, "wb");
    if (file1 == NULL) {
        perror("fopen");
        close(sockfd);
        return 1;
    }

    char response_buffer[BUFFER_SIZE];
    int bytes_received;
const char* outputFileName = "output.jpg";

    while ((bytes_received = recv(sockfd, response_buffer, BUFFER_SIZE, 0)) > 0) {
        char body[BUFFER_SIZE];
        int body_length = extract_http_body(response_buffer, body, sizeof(body));
        
        printf("body: %s \n",body);
    
        fwrite(body, 1, bytes_received, file);
        binaryStringToJpeg(body, outputFileName);
        fwrite(response_buffer,1,bytes_received,file1);
    }
    // Adjust the buffer size as needed

    // Extract the body from the HTTP response
    

    
    fclose(file);
    close(sockfd);

    return 0;
}
