/*
			NETWORK PROGRAMMING WITH SOCKETS

In this program we illustrate the use of Berkeley sockets for interprocess
communication across the network. We show the communication between a server
process and a client process.


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

/* The following three files must be included for network programming */
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <fcntl.h>


			/* THE SERVER PROCESS */

int main()
{
	int			sockfd, newsockfd ; /* Socket descriptors */
	int			clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	int i;
	char buf[100];		/* We will use this buffer for communication */

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(20000);

	
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5); /* This specifies that up to 5 concurrent client
			      requests will be queued up while the system is
			      executing the "accept" system call below.
			   */

	while (1) {

		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
					&clilen) ;

		if (newsockfd < 0) {
			perror("Accept error\n");
			exit(0);
		}

		recv(newsockfd, buf, 100, 0);

		int fd = open(buf, O_RDONLY);
		if(fd < 0)
	    {
			perror("No file found\n");
			// send(newsockfd, "NOTFOUND", strlen("NOTFOUND")+1, 0); 
			close(newsockfd);
			exit(EXIT_FAILURE);
	    }
  	    printf("File %s has been opened on server\n",buf);
	  	char dataFromFile[100];
    	int n = 1;
    	int bytes = 0;
    	while(n)
    	{
    		n = read(fd, dataFromFile, 100);
    		if(n<=0) break;
    		bytes += n;
    		dataFromFile[n] = '\0';
	    	send(newsockfd, dataFromFile, n, 0);
	    	// printf("%s", dataFromFile);
	    	// fflush(stdout);
	    	memset(&dataFromFile, '\0', sizeof(dataFromFile));
    	}
    	printf("Number of bytes sent: %d\n", bytes);
    	close(fd);
		close(newsockfd);
		
	}
	return 0;
}
			

