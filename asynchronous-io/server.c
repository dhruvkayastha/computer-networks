// Signal handlers are not a good solution for this because there can be a system call or another incoming signal during the execution of the signal handler
// In the case of a server with multiple sockets, a select call would be required to know which socket is receiving (complicated)

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

int sockfd;

void sighandler(int signo)
{
	if(signo == SIGIO)
	{
		struct sockaddr_in cli_addr;
		socklen_t cli_len;

		char buf[101];
		memset(buf, 0, sizeof(buf));
		memset(&cli_addr, 0, sizeof(cli_addr));

		int n = recvfrom(sockfd, buf, sizeof(buf), 0, 
			(struct sockaddr *)&cli_addr, &cli_len);

		if(n < 0)
		{
			perror("recv");
			exit(1);
		}

		printf("Message recieved from client:\n%s\n", buf);

		n = sendto(sockfd, buf, sizeof(buf), 0, 
			(struct sockaddr *)&cli_addr, cli_len);

		if(n < 0)
		{
			perror("sendto");
			exit(1);
		}

		signal(SIGIO, sighandler);
	}
}

int main(int argc, char const *argv[])
{
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

	if(bind(sockfd, (struct sockaddr *)&serv_addr, serv_len) < 0)
	{
		perror("Bind failed");
		exit(1);
	}


	if(fcntl(sockfd, F_SETOWN, getpid()) < 0)
	{
		perror("fcntl setown");
		exit(1);
	} 
	int flags = fcntl(sockfd, F_GETFL, NULL);

	if(flags < 0)
	{
		perror("fcntl getfl");
		exit(1);
	}

	if(fcntl(sockfd, F_SETFL, flags | O_ASYNC) < 0)
	{
		perror("fcntl setfl");
		exit(1);
	}

	signal(SIGIO, sighandler);

	while(1){sleep(1);}

	close(sockfd);

	return 0;
}