// A Simple UDP Server that sends a HELLO message
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
  
#define MAXLINE 1024 
  
int main() { 
    int sockfd; 
    struct sockaddr_in servaddr, cliaddr; 
      
    // Create socket file descriptor 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockfd < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
      
    servaddr.sin_family    = AF_INET; 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(8181); 
      
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    printf("\nServer Running....\n");
  
    int n; 
    socklen_t len;
    char buffer[MAXLINE]; 
 
    len = sizeof(cliaddr);
    n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0, 
			( struct sockaddr *) &cliaddr, &len); 
    buffer[n] = '\0'; 
    printf("%s\n", buffer); 
    FILE *fp = fopen(buffer,"r");
    if(fp==NULL){
    	sendto(sockfd, "NOTFOUND", strlen("NOTFOUND"), 0, 
			(const struct sockaddr *) &cliaddr, sizeof(cliaddr)); 
    	exit(EXIT_FAILURE);
    }
    printf("File opened\n");
    char dataFromFile[100];
    while(fscanf(fp, "%s", dataFromFile)==1)
    {
    	//printf("%s", dataFromFile);
    	sendto(sockfd, dataFromFile, strlen(dataFromFile), 0, 
			(const struct sockaddr *) &cliaddr, sizeof(cliaddr)); 
    }

    fclose(fp);
    printf("File closed\n");

    // printf("%s\n", buffer); 
    
    return 0; 
} 
