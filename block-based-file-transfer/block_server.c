#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <signal.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>

#define B 20
#define MAXSIZE 2000000

int main()
{
	int	sockfd, newsockfd ;
	int	clilen;
	struct sockaddr_in	cli_addr, serv_addr;
	char fileName[B], errorMsg;
	char file[MAXSIZE];

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
    	perror("setsockopt(SO_REUSEADDR) failed");

	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port			= htons(20000);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Unable to bind local address");
		exit(EXIT_FAILURE);
	}

  	listen(sockfd, 5);

	while(1)
	{
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
			&clilen) ;

		if (newsockfd < 0) {
			perror("Accept error");
			exit(EXIT_FAILURE);
		}

		memset(&fileName, '\0', sizeof(fileName));
		recv(newsockfd, fileName, sizeof(fileName), 0);

		int fd = open(fileName,O_RDONLY);
		if(fd < 0)
		{
			perror("File not found");
            errorMsg = 'E';
		    send(newsockfd, &errorMsg, sizeof(errorMsg), 0);
			close(newsockfd);
			continue;
		}

		errorMsg='L';
		send(newsockfd, &errorMsg, sizeof(errorMsg), 0);
		int FSIZE = read(fd, file, MAXSIZE);

		printf("Sending file of size %d bytes ",FSIZE);
		uint32_t size = htonl(FSIZE);
		send(newsockfd, &size, sizeof(size), 0);


		char file_data[B];
		int packets=FSIZE/B+1;
		printf("as %d packets\n", packets);
		int count=1;

		int bytes = 1;
		close(fd);
		fd = open(fileName,O_RDONLY);
		while(count<=packets)
		{
		    bytes = read(fd, file_data, B);
		    
		    if(bytes==0)
		    {
		    	perror("File read");
		    	break;
		    }

		    if(send(newsockfd, file_data, B, 0) < 0)
		    {
		   	    perror("Send failed");
		   	    break;
		    }
		    memset(&file_data, '\0', sizeof(file_data));
		    count++;
		}

        printf("File %s sent to client\n", fileName);
        close(fd);
        close(newsockfd);
	}

	return 0;
}
