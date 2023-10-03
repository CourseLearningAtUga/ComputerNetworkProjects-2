#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <openssl/md5.h>

#define PORT_NUMBER 80
//*****************************************************helper structures **************************************************//
struct ThreadArgs {    
    char *domain; // Pointer to a string
    size_t domain_length; // Length of the string
    char *path;
    size_t path_length;
    char *smalleroutputfile;
    size_t smalleroutputfile_length;
    int rangest;
    int rangeend;
};
//****************************************************helper structures end************************************************//

//*****************************************************helpler functions *************************************************//
char* integerToString(int num) {
    // Determine the maximum number of digits an integer can have (including sign and null terminator)
    int max_digits = snprintf(NULL, 0, "%d", num) + 1;

    // Allocate memory for the string
    char *str = (char *)malloc(max_digits);

    if (str == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }

    // Convert the integer to a string
    sprintf(str, "%d", num);

    return str;
}

void appendToEndOfOutputFile(const char *file1_path, const char *output_path) {
    char mergebuf[1024];
    snprintf(mergebuf, sizeof(mergebuf),"cat %s >> %s",file1_path,output_path);
    system(mergebuf);
    printf("Files merged successfully.\n");
}
char* concatenateStrings(const char *str1, const char *str2) {
    // Calculate the total length of the resulting string
    size_t total_length = strlen(str1) + strlen(str2) + 1; // +1 for the null terminator

    // Allocate memory for the resulting string
    char *result = (char *)malloc(total_length);

    // Check if memory allocation was successful
    if (result == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }

    // Copy the first string to the result
    strcpy(result, str1);

    // Concatenate the second string to the result
    strcat(result, str2);

    return result;
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
    }
    
    return  bytes_received ;

}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++helper socket functions end ***********************************************//
//******************************************************main socket code*********************************************************************//



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
void runHttp(int sock,char *domain_passed,char *path_passed,char *outputfile,int rangestart,int rangeend){
    char *domain = domain_passed;
    char *path=path_passed; 
    char send_data[1024],recv_data[1024];
    int bytes_received;  
    printf("Sending http get request to server range %d and %d ...\n",rangestart,rangeend);

    snprintf(send_data, sizeof(send_data), "GET /%s HTTP/1.1\r\nHost: %s\r\nRange: bytes=%d-%d\r\n\r\n", path, domain,rangestart,rangeend);

    if(send(sock, send_data, strlen(send_data), 0)==-1){
        perror("send");
        exit(2); 
    }
    printf("http request sent for range.%d and %d ...\n",rangestart,rangeend);  

    //fp=fopen("received_file","wb");
    printf("Recieving http get request to server range %d and %d ...\n",rangestart,rangeend);

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
            // printf("Bytes recieved: %d from %d\n",bytes,contentlengh);
            if(bytes==contentlengh)
            break;
        }
        fclose(fd);
    }

    
    
    printf("done saving the data of  http get request to server range %d and %d ...\n",rangestart,rangeend);
   
}
void *wrapperThreadFunction(void *args){
    struct ThreadArgs *threadArgs = (struct ThreadArgs *)args;
    char *domain=threadArgs->domain;
    char *path=threadArgs->path;
    char *filename=threadArgs->smalleroutputfile;
    int rangest=threadArgs->rangest;
    int rangeend=threadArgs->rangeend;
    // printf("values =========================================================domaain %s,filename %s ,rangeest %d,rangedn %d\n",domain,filename,rangest,rangeend);
    int mainsock1=createSocket(domain,path);
    runHttp(mainsock1,domain,path,filename,rangest,rangeend);//download the first few pieces into files
    close(mainsock1);
    pthread_exit(NULL);
}
//******************************************************main socket code end*********************************************************************//
//*****************************************************handling input***********************************************************************//

void splitUrl(const char *url, char **hostName, char **path) {
    // Find the position of the first "/" after "://"
    char *slashPtr = strstr(url, "://");
    
    if (slashPtr != NULL) {
        slashPtr += 3; // Move past "://"
        char *pathPtr = strchr(slashPtr, '/');
        
        if (pathPtr != NULL) {
            // Calculate the length of the host name and path
            size_t hostNameLength = pathPtr - slashPtr;
            size_t pathLength = strlen(pathPtr);
            
            // Allocate memory for the host name and path
            *hostName = (char *)malloc(hostNameLength + 1);
            *path = (char *)malloc(pathLength + 1);
            
            // Copy the host name and path
            strncpy(*hostName, slashPtr, hostNameLength);
            (*hostName)[hostNameLength] = '\0';
            
            strcpy(*path, pathPtr);
        }
    }
}

void setInputsFromArguementsPassed(int argc,char **argv,char **domain,char **path,char *outputfile,int *number_of_parts){
for(int i=0;i<argc;i++){
if(strcmp(argv[i],"-u")==0)
{
splitUrl(argv[i+1], &(*domain), &(*path));
i++;
continue;
}
if(strcmp(argv[i],"-n")==0)
{
*number_of_parts=atoi(argv[i+1]);
i++;
continue;
}
if(strcmp(argv[i],"-o")==0)
{
    strcpy(outputfile,argv[i+1]);
i++;
continue;
}

}
}

//*****************************************************handling input end***********************************************************************//
//*****************************************************calculating md5 *************************************************************************//
int calculateFileMD5(const char *filename, unsigned char *digest) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    MD5_CTX context;
    MD5_Init(&context);

    size_t buffer_size = 1024;
    unsigned char buffer[buffer_size];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, buffer_size, file))) {
        MD5_Update(&context, buffer, bytes_read);
    }

    MD5_Final(digest, &context);
    fclose(file);

    return 0;
}

//*****************************************************calculating md5 end *************************************************************************//
int main(int argc, char *argv[] ){
    
    char *domain = "cobweb.cs.uga.edu", *path="/~perdisci/CSCI6760-F21/Project2-TestFiles/story_hairydawg_UgaVII.jpg";
    char *filename="part_";
    char outputfile[50];
    strcpy(outputfile, "fullstichedimage.jpg");
    int number_of_parts=5;
    setInputsFromArguementsPassed(argc,argv,&domain,&path,outputfile,&number_of_parts);
    
    char filenames[number_of_parts][256];
    int mainsock0=createSocket(domain,path);
    int contentlength=DownloadOnlyHeadersForContentLength(mainsock0,domain,path);
    close(mainsock0);
    printf("Content-Length= %d\n",contentlength);
    printf("each partsize is %d\n",contentlength/number_of_parts);
    int sizeofeachchunk=contentlength/number_of_parts;
    int rangest=0;
    int rangeend=sizeofeachchunk;
    int count=1;
    pthread_t thread[number_of_parts];
    struct ThreadArgs args[number_of_parts];
    while(count<number_of_parts){
        strcpy(filenames[count-1], concatenateStrings(filename, integerToString(count)));//create the file name
        // int mainsock1=createSocket(domain,path);
        // runHttp(mainsock1,domain,path,filenames[count-1],rangest,rangeend);//download the first few pieces into files
        // close(mainsock1);
        args[count-1].domain=domain;
        args[count-1].path=path;
        args[count-1].smalleroutputfile=filenames[count-1];
        args[count-1].rangest=rangest;
        args[count-1].rangeend=rangeend;
        // printf("++++++++++++++++++++++++++++++++++++domain %s ,filename %s, rangest %d ,rangedn %d \n",args.domain,args.smalleroutputfile,args.rangest,args.rangeend);
        if (pthread_create(&thread[count-1], NULL, wrapperThreadFunction, &args[count-1]) != 0) {
            fprintf(stderr, "Failed to create thread.\n");
            return 1;
        }
        count++;
        rangest=rangeend+1;
        rangeend=rangest+sizeofeachchunk;
        
    }
    
    int mainsock2=createSocket(domain,path);
    strcpy(filenames[count-1], concatenateStrings(filename, integerToString(count)));
    runHttp(mainsock2,domain,path,filenames[count-1],rangest,contentlength);//in the last run download everything
    close(mainsock2);

    for(int i=0;i<number_of_parts-1;i++){
        pthread_join(thread[i], NULL);
    }
    printf("\n\n");
    
    for(int i=0;i<number_of_parts;i++){
        
        appendToEndOfOutputFile(filenames[i], outputfile);
        printf("%s is merge to the output\n",filenames[i]);
    }
    printf("\n\n");
    printf("======================================================================\n");
    unsigned char digest[MD5_DIGEST_LENGTH];
    if (calculateFileMD5(outputfile, digest) == 0) {
        printf("MD5 Hash of %s: ", outputfile);
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
            printf("%02x", digest[i]);
        }
        printf("\n");
    }
    printf("======================================================================\n\n");
    // printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++1\n");
    // int mainsock1=createSocket(domain,path);
    // runHttp(mainsock1,domain,path,concatenateStrings(filename, atoa(1)),rangestart,rangeend);
    // close(mainsock1);
    // printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++2\n");
    // int mainsock2=createSocket(domain,path);
    // runHttp(mainsock2,domain,path,concatenateStrings(filename, atoa(2)),rangeend+1,rangeend+30001);
    // close(mainsock2);
    // printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++3\n");
    
    // runHttp(domain,path,outputfile3,rangestart,rangeend+2001);
    // mergefiles(outputfile1,outputfile2,"merge.jpg");
    // mergeFiles(outputfile1,outputfile2,outputfile3);
    // appendToEndOfOutputFile(concatenateStrings(filename, atoa(1)), outputfile);    
    // appendToEndOfOutputFile(concatenateStrings(filename, atoa(2)), outputfile);
    
    return 0;
}