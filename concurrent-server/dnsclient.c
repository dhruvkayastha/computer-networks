// A Simple Client Implementation
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
  
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockfd < 0 ) { 
        perror("udp socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(6000); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
      
    int n;
    socklen_t len; 
    char buff[100];

    printf("Enter domain name: ");
    scanf("%s", buff);

    sendto(sockfd, (const char* )buff, strlen(buff), 0, 
        (const struct sockaddr*) &servaddr, sizeof(servaddr));

    char ip[100];
    n = recvfrom(sockfd, (const char*) ip, 100, 0,
        (struct sockaddr*) &servaddr, sizeof(servaddr));

    printf("IP Address: %s\n", ip);
    close(sockfd);
    return 0; 
} 
