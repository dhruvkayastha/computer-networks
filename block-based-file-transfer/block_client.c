#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define B 20

int main()
{
	int sockfd ;
	struct sockaddr_in serv_addr;
	char fileName[B], errorMsg;
	uint32_t FSIZE;


	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket");
		exit(EXIT_FAILURE);
	}

	serv_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(20000);


	if ((connect(sockfd, (struct sockaddr *) &serv_addr,
		sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server");
		exit(EXIT_FAILURE);
	}
	printf("Connected to Server\n");

	printf("Enter filename to fetch\n");
	scanf("%s", fileName);

	send(sockfd, fileName, strlen(fileName) + 1, 0);

	recv(sockfd, &errorMsg, sizeof(errorMsg), 0);

	if(errorMsg=='L')
		printf("File opened in server\n");
	else if(errorMsg=='E')
	{
		printf("File %s not found at server. Exiting..\n", fileName);
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	recv(sockfd, &FSIZE, sizeof(FSIZE), 0);
	FSIZE = ntohl(FSIZE);

	printf("File size: %d bytes\n", FSIZE);

	int fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);

	if(fd<0)
	{
	  	perror("Open failed");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	int packets=FSIZE/B+1;
	int bytes;
	int count=1;
	char file_data[B];
	
	bytes = recv(sockfd, file_data, B, MSG_WAITALL);

  	while(1)
  	{
	  	if(count == packets)
	  	{
	  		if(write(fd, file_data, FSIZE%B) < 0)
	  		{
	  			perror("Write failed");
	  			close(sockfd);
	  			exit(EXIT_FAILURE);
	  		}
	  		break;
	  	}
	  	count++;

	  	if(bytes<0)
	  	{
	  		perror("Receive failed");
	  		exit(EXIT_FAILURE);
	  	}

	  	if(write(fd, file_data, B) < 0)
	  	{
	  		perror("Write failed");
	  		close(sockfd);
	  		exit(EXIT_FAILURE);
	  	}

	  	memset(file_data, '\0', sizeof(file_data));

	  	if(count == packets)
	  		bytes = recv(sockfd, file_data, FSIZE%B, MSG_WAITALL);
	  	else
	  		bytes = recv(sockfd, file_data, B, MSG_WAITALL);
  	}

    printf("Received %d packets\n", count);
    printf("Size of the last packet: %d bytes\n", bytes);

    close(fd);
    close(sockfd);

    return 0;

}
