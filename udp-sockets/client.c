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
    struct sockaddr_in servaddr; 
  
    // Creating socket file descriptor 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockfd < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(8181); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
      
    int n;
    socklen_t len; 
    char *hello = "CLIENT:HELLO"; 
  
  	printf("Enter file name to read from\n");
    char fileName[MAXLINE];
    scanf("%s",&fileName);
  

    sendto(sockfd, (const char *)fileName, strlen(fileName), 0, 
			(const struct sockaddr *) &servaddr, sizeof(servaddr)); 
    // printf("Hello message sent from client\n");
    char buffer[MAXLINE];
    char requests[MAXLINE];
    len = sizeof(servaddr);
    n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0, 
			( struct sockaddr *) &servaddr, &len);
    buffer[n] = '\0';
    // printf("%s\n",buffer);
    if(strcmp(buffer, "NOTFOUND")==0)
    {
    	printf("File not found. Exiting...\n");
    	exit(EXIT_FAILURE);
    }
	if(strcmp(buffer, "HELLO")!=0)
	{
		printf("First word in file is not \"HELLO\". Exiting...\n");
		exit(EXIT_FAILURE);
	}

	FILE *fp = fopen("output.txt", "w");

	int i = 1;
    while(strcmp(buffer, "END"))
    {
    	
        sendto(sockfd, (const char *)requests, strlen(requests), 0,
      			(const struct sockaddr *) &servaddr, sizeof(servaddr));

    	n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0, 
			( struct sockaddr *) &servaddr, &len);
	    buffer[n] = '\0';
	    
	    if(strcmp(buffer, "END")==0)
	    	break;

	    fprintf(fp, "%s\n", buffer);

		sprintf(requests, "WORD%d", i);
    	printf("Client: %s\n", requests);
    	printf("SERVER: %s\n\n",buffer);
    	i++;
    }
    printf("Recieved \"END\" from server....closing file\n");
    fclose(fp);
    close(sockfd); 
    return 0; 
} 
