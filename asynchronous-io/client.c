#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <netdb.h>
#include <sys/select.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char const *argv[])
{
	int sockfd;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t serv_len, cli_len;

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("Socket creation");
		exit(1);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(4545);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_len = sizeof(serv_addr);

	memset(&cli_addr, 0, sizeof(cli_addr));

	while(1)
	{
		char buf[101];

		printf("Enter string to send to server:\n");
		scanf("%s", buf);

		int n = sendto(sockfd, buf, strlen(buf)+1, 0, 
			(struct sockaddr *)&serv_addr, serv_len);
		if(n < 0)
		{
			perror("sendto");
			exit(1);
		}


		char recv_buf[101];
		memset(recv_buf, 0, sizeof(recv_buf));

		n = recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0,
			(struct sockaddr *)&cli_addr, &cli_len);

		if(n < 0)
		{
			perror("recvfrom");
			exit(1);
		}

		printf("Reply from server:\n%s\n", recv_buf);

	}

	
	close(sockfd);
	return 0;
}