#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
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



#define PORT_NUMBER 80


//*****************************************************helpler functions *************************************************//

void appendToEndOfOutputFile(const char *file1_path, const char *output_path) {
    char mergebuf[1024];
    snprintf(mergebuf, sizeof(mergebuf),"cat %s >> %s",file1_path,output_path);
    system(mergebuf);
    printf("Files merged successfully.\n");
}

//*****************************************************helpler functions end*************************************************//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++helper socket functions***********************************************//
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
int DownloadOnlyHeadersForContentLength(int sock,char *domain_passed,char *path_passed){
    char c,send_data[1024],buff[1024]="",*ptr=buff+4;
    char *domain = domain_passed;
    char *path=path_passed; 
    int bytes_received, status;
    snprintf(send_data, sizeof(send_data), "HEAD /%s HTTP/1.1\r\nHost: %s\r\n\r\n", path, domain);

    if(send(sock, send_data, strlen(send_data), 0)==-1){
        perror("send");
        exit(2); 
    }
    printf("HEAD request sent.\n");  
    printf("++++++++++++++++++++output of head request+++++++++++++++++++++++++++++\n");
    while(bytes_received = recv(sock, ptr, 1, 0)){
        if(bytes_received==-1){
            perror("Parse Header");
            exit(1);
        }
        printf("%s",ptr);
        if(
            (ptr[-3]=='\r')  && (ptr[-2]=='\n' ) &&
            (ptr[-1]=='\r')  && (*ptr=='\n' )
        ) break;
        ptr++;
    }
    printf("++++++++++++++++++++end of output of head request+++++++++++++++++++++++++++++\n\n\n");
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
    
    return  bytes_received ;

}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++helper socket functions end ***********************************************//
//******************************************************main socket code*********************************************************************//


void runHttp(int sock,char *domain_passed,char *path_passed,char *outputfile,int rangestart,int rangeend){
    char *domain = domain_passed;
    char *path=path_passed; 
    char send_data[1024],recv_data[1024];
    int bytes_received;  
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
        printf("Saving data...\n\n");

        while(bytes_received = recv(sock, recv_data, 1024, 0)){
            if(bytes_received==-1){
                perror("recieve");
                exit(3);
            }

            
            fwrite(recv_data,1,bytes_received,fd);
            bytes+=bytes_received;
            printf("Bytes recieved: %d from %d\n",bytes,contentlengh);
            if(bytes==contentlengh)
            break;
        }
        fclose(fd);
    }

    
    
    printf("\n\nDone.\n\n");
   
}
int createSocket(char *domain_passed,char *path_passed){
char *domain = domain_passed;
char *path=path_passed; 

    int sock;
    
    struct sockaddr_in server_addr;
    struct hostent *he;
   
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
    server_addr.sin_port = htons(PORT_NUMBER);
    server_addr.sin_addr = *((struct in_addr *)he->h_addr);
    bzero(&(server_addr.sin_zero),8); 

    printf("Connecting ...\n");
    if (connect(sock, (struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1){
       perror("Connect");
       exit(1); 
    }
    return sock;
}


int main(void){
    char *domain = "cobweb.cs.uga.edu", *path="/~perdisci/CSCI6760-F21/Project2-TestFiles/story_hairydawg_UgaVII.jpg";
    char *outputfile1="temp.jpg";
    char *outputfile2="temp2.jpg";
    char *outputfile3="final.jpg";
    int number_of_parts=5;
    int rangestart=0;
    int rangeend=10000;
    int mainsock0=createSocket(domain,path);
    printf("%d\n",DownloadOnlyHeadersForContentLength(mainsock0,domain,path));
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++1\n");
    int mainsock1=createSocket(domain,path);
    runHttp(mainsock1,domain,path,outputfile1,rangestart,rangeend);
    close(mainsock1);
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++2\n");
    int mainsock2=createSocket(domain,path);
    runHttp(mainsock2,domain,path,outputfile2,rangeend+1,rangeend+30001);
    close(mainsock2);
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++3\n");
    
    // runHttp(domain,path,outputfile3,rangestart,rangeend+2001);
    // mergefiles(outputfile1,outputfile2,"merge.jpg");
    // mergeFiles(outputfile1,outputfile2,outputfile3);
    appendToEndOfOutputFile(outputfile1, outputfile3);    
    appendToEndOfOutputFile(outputfile2, outputfile3);
    
    return 0;
}