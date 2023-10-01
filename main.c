#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#include <string.h>

void appendStringToGrowingBuffer(char** growingBuffer, const char* str) {
    // Calculate the new length of the combined string
    size_t newLength = strlen(*growingBuffer) + strlen(str);
    
    // Allocate memory for the growing buffer
    char* newBuffer = (char*)malloc(newLength + 1); // +1 for the null-terminator
    if (newBuffer == NULL) {
        perror("Memory allocation error");
        return;
    }

    // Copy the existing content to the new buffer
    strcpy(newBuffer, *growingBuffer);

    // Append the new string to the new buffer
    strcat(newBuffer, str);
    
    // Free the old buffer
    free(*growingBuffer);

    // Update the growing buffer pointer to point to the new buffer
    *growingBuffer = newBuffer;
}
int ReadHttpStatus(int sock){
    char c;
    char buff[1024]="",*ptr=buff+1;
    int bytes_received, status;
    printf("Begin Response ..\n");
    while(bytes_received = recv(sock, ptr, 1, 0)){
        if(bytes_received==-1){
            perror("ReadHttpStatus");
            exit(1);
        }

        if((ptr[-1]=='\r')  && (*ptr=='\n' )) break;
        ptr++;
    }
    *ptr=0;
    ptr=buff+1;

    sscanf(ptr,"%*s %d ", &status);

    printf("%s\n",ptr);
    printf("status=%d\n",status);
    printf("End Response ..\n");
    return (bytes_received>0)?status:0;

}

//the only filed that it parsed is 'Content-Length' 
int ParseHeader(int sock){
    char c;
    char buff[1024]="",*ptr=buff+4;
    int bytes_received, status;
    printf("Begin HEADER ..\n");
    while(bytes_received = recv(sock, ptr, 1, 0)){
        if(bytes_received==-1){
            perror("Parse Header");
            exit(1);
        }

        if(
            (ptr[-3]=='\r')  && (ptr[-2]=='\n' ) &&
            (ptr[-1]=='\r')  && (*ptr=='\n' )
        ) break;
        ptr++;
    }

    *ptr=0;
    ptr=buff+4;
    //printf("%s",ptr);

    if(bytes_received){
        ptr=strstr(ptr,"Content-Length:");
        if(ptr){
            sscanf(ptr,"%*s %d",&bytes_received);

        }else
            bytes_received=-1; //unknown size

       printf("Content-Length: %d\n",bytes_received);
    }
    printf("End HEADER ..\n");
    return  bytes_received ;

}

//***************************************************************************************************************************//
char* runHttp(char *domain_passed,char *path_passed,char *outputfile,int rangestart,int rangeend){
char *domain = domain_passed;
char *path=path_passed; 

    int sock, bytes_received;  
    char send_data[1024],recv_data[1024], *p,*ret = (char*)malloc(1); // Start with an empty string
     
    struct sockaddr_in server_addr;
    struct hostent *he;
    
    ret[0] = '\0';
    he = gethostbyname(domain);
    if (he == NULL){
       herror("gethostbyname");
       exit(1);
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0))== -1){
       perror("Socket");
       exit(1);
    }
    server_addr.sin_family = AF_INET;     
    server_addr.sin_port = htons(80);
    server_addr.sin_addr = *((struct in_addr *)he->h_addr);
    bzero(&(server_addr.sin_zero),8); 

    printf("Connecting ...\n");
    if (connect(sock, (struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1){
       perror("Connect");
       exit(1); 
    }

    printf("Sending data ...\n");
   
    snprintf(send_data, sizeof(send_data), "GET /%s HTTP/1.1\r\nHost: %s\r\nRange: bytes=%d-%d\r\n\r\n", path, domain,rangestart,rangeend);

    if(send(sock, send_data, strlen(send_data), 0)==-1){
        perror("send");
        exit(2); 
    }
    printf("Data sent.\n");  

    //fp=fopen("received_file","wb");
    printf("Recieving data...\n\n");

    int contentlengh;

    if(ReadHttpStatus(sock) && (contentlengh=ParseHeader(sock))){

        int bytes=0;
        FILE* fd=fopen(outputfile,"wb");
        FILE* fd2=fopen("temp3.jpg","wb");
        printf("Saving data...\n\n");
         
        while(bytes_received = recv(sock, recv_data, 1024, 0)){
            if(bytes_received==-1){
                perror("recieve");
                exit(3);
            }
            
            
            fwrite(recv_data,1,bytes_received,fd);
            fwrite(recv_data,1,bytes_received,fd2);
            
            appendStringToGrowingBuffer(&ret,recv_data);
            
           printf("\n\n\n\n============================compstart===========================\n\n\n");
           int count=0;
           printf("len and size of recv_data is %ld and %ld\n",sizeof(recv_data),strlen(recv_data));
           while(count<=100){
                printf("count %d = %c ",count,recv_data[count]);
                count++;
            }
           printf("\n\n\n\n============================compmid===========================\n\n\n");
            printf("%s",recv_data);
             printf("\n\n\n\n============================compend1===========================\n\n\n");
            bytes+=bytes_received;
            // printf("Bytes recieved: %d from %d\n",bytes,contentlengh);
            if(bytes==contentlengh)
            break;
        }
        fclose(fd);
        fclose(fd2);
    }


    printf("\n\nDone.\n\n");
    return ret;
}

int main(void){
    char *domain = "cobweb.cs.uga.edu", *path="/~perdisci/CSCI6760-F21/Project2-TestFiles/story_hairydawg_UgaVII.jpg";
    char *outputfile1="temp.jpg";
    char *outputfile2="temp2.jpg";
    char *outputfile3="temp3.jpg";
    int number_of_chunks=5;
    int rangestart=0;
    int rangeend=2000;
    
    runHttp(domain,path,outputfile1,rangestart,rangeend);
    
    return 0;
}