#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#define MAXLINE 1024 

int main()
{
	int			sockfd ;
	struct sockaddr_in	serv_addr;

	int i;
	char buf[MAXLINE];
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

	serv_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(6000);
	if ((connect(sockfd, (struct sockaddr *) &serv_addr,
						sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}
	
	send(sockfd, "getwords", strlen("getwords")+1, 0);
	int n = recv(sockfd, buf, 99, 0);
	if(n<0)
	{
		perror("Receive failed");
		exit(0);
	}
	int count = 0;
	while(n>0)
	{
		for(i = 0; i<n; i++)
		{
			// temp = 0;
			if(buf[i]=='\0')
			{
				// printf("%s\n", buf);
				count++;
				// printf("%c %d\n", buf[i], i);
			}
		}
		// buf[n] = '\0';
		for(int i = 0; i<n; i++)	//printing character wise so that print does not terminate at '\0'
		{
			if(buf[i]=='\0')
				printf("\n");
			else printf("%c", buf[i]);
		}
		memset(&buf, '\0', sizeof(buf));
		n = recv(sockfd, buf, 99, 0);
		int temp=0, i;
		
		// count += (n - temp);
		if(n<0)
		{
			perror("Receive failed");
			exit(0);
		}
	}
	printf("Number of words = %d\n", count);

	close(sockfd);
	return 0;
}
