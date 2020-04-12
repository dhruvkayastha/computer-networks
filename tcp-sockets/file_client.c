
/*    THE CLIENT PROCESS */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdbool.h>


int main()
{
	int			sockfd ;
	struct sockaddr_in	serv_addr;

	int i;
	char buffer[100];

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

	serv_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(20000);

	if ((connect(sockfd, (struct sockaddr *) &serv_addr,
						sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}

    char fileName[100];
    printf("Enter the file name to be read from the server \n" );
    scanf("%s", fileName);	
	
	send(sockfd, fileName, strlen(fileName) + 1, 0);
	int n = recv(sockfd, buffer, 100, 0);
	if(n==0)
	{
		perror("File not found on server\n");
		exit(-1);
	}
	int fd = open(fileName, O_WRONLY | O_CREAT, 0666);	//0666 for all permissions to all users
	if(fd<0){
		perror("Output file could not be opened\n");
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	int bytes = n;
	int count = 0;
	char delimiters[] = {',', '.', ';', ':', ' ', '\t', '\n'};
	
	int state = 0;

	while(n)
	{
		if(n<0)
		{
			perror("*****Recieve failed\n");
			break;
		}
		// printf("%s", buffer);
		//FSM with 2 states to count no of words
		for(int i=0; i<n; i++)
		{
			int flag = 0;
			for(int j = 0; j<7; j++)
			{
				if(delimiters[j] == buffer[i])
					flag = 1;
			}

			if(state && flag)
				state = !state;
			else		//count++ whenever state transition from delim to char
			{
				if(!flag && !state)
				{
					state = !state;
					count++;
					// printf("%c %d\n", buffer[i], count);
				}
			}
		}
		if(write(fd, buffer, n)<0)
		{
			perror("*****Write failed\n");
			break;
		}
		memset(buffer, '\0', sizeof(buffer));
		n = recv(sockfd, buffer, 100, 0);
		bytes += n;
	}
 	printf("\nThe file transfer is successful.\nSize of the file = %d bytes, No. of words = %d\n", bytes, count);
	// fflush(stdout);
	close(fd);
	// close(sockfd);
	return 0;
}

