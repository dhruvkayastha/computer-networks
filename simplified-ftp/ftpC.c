#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAXLINE 180 



int main()
{
	int			sockfd ;
	struct sockaddr_in	serv_addr;
	int i;
	char buf[MAXLINE];
	memset(buf, '\0', sizeof(buf));
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

	serv_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(50000);
	if ((connect(sockfd, (struct sockaddr *) &serv_addr,
						sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}
	
	while(strlen(buf)==0)
	{
		printf(">>> ");
		size_t size = sizeof(buf);
		memset(buf, '\0', sizeof(buf));
		fgets(&buf, sizeof(buf), stdin);
		buf[strlen(buf)-1]='\0';
	}


	send(sockfd, buf, strlen(buf)+1, 0);
	short code;
	recv(sockfd, &code, sizeof(code), 0);
	// printf("Error code: %d\n", code);
	code = ntohs(code);
	// printf("Error code: %d\n", code);
	if(code!=200)
	{
		printf("Error code: %d\n", code);
		if(code==503)
		{
			printf("Port must be specified first\n");
		}
		close(sockfd);
		exit(0);
	}
	else {
		printf("OK\n");
	}
	int portY = atoi(buf+5);
	printf("Port is %d\n", portY);
	while(1)
	{
		memset(buf, '\0', sizeof(buf));

		while(strlen(buf)==0)
		{
			memset(buf, '\0', sizeof(buf));
			printf(">>> ");
			fgets(&buf, sizeof(buf), stdin);
			buf[strlen(buf)-1]='\0';
		}
		// printf("Buff: %s\n", buf);

		int fd;
		if(strncmp(buf, "get ", 4)==0)
		{
			//create new file to store data
			int ret = fork();
			if(ret==0)			//in child
			{
				int sockfdD;
				struct sockaddr_in cli_addrD, serv_addrD;

				if((sockfdD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
				{
					perror("TCP Server socket creation failed for D");
					exit(EXIT_FAILURE);
				}
				if (setsockopt(sockfdD, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
			    	perror("setsockopt(SO_REUSEADDR) failed for D");


				serv_addrD.sin_family		= AF_INET;
				serv_addrD.sin_addr.s_addr	= INADDR_ANY;
				serv_addrD.sin_port			= htons(portY);

				if(bind(sockfdD, (struct sockaddr *) &serv_addrD, sizeof(serv_addrD)) < 0)
				{
					perror("TCP Server bind failed for D");
					exit(EXIT_FAILURE);
				}

				listen(sockfdD, 5);
				int clilenD = sizeof(cli_addrD);
				int newsockfdD = accept(sockfdD, (struct sockaddr *) &cli_addrD, &clilenD);
				if(newsockfdD<0)
				{
					perror("Accept error in D");
					close(sockfdD);
					exit(0);
				}
				// int sockfdD;
				// struct sockaddr_in	serv_addrD;
				// //sleep(1);
				
				// if ((sockfdD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				// 	perror("Unable to create socket for D\n");
				// 	exit(0);
				// }

				// serv_addrD.sin_family	= AF_INET;
				// inet_aton("127.0.0.1", &serv_addrD.sin_addr);
				// serv_addrD.sin_port	= htons(portY);
				// if ((connect(sockfdD, (struct sockaddr *) &serv_addrD,
				// 					sizeof(serv_addrD))) < 0) {
				// 	perror("Unable to connect to server D\n");
				// 	exit(0);
				// }

				char fileData[100];
				char *fileName;
				int ii=0, start = 3;
				for(ii=0; buf[ii]!='\0'; ii++)
				{
					if(buf[ii]=='/')
						start=ii;
				}
				fileName = buf+start+1;
				// printf("File name is %s\n", fileName);
				fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
				if(fd<0)
				{
					perror("Could not create file on client\n");
					close(sockfdD);
					exit(0);
				}
				
				int isLast=0;

				int bytes = recv(newsockfdD, fileData, 100, 0); 
				short packet_size = fileData[1]*256 + fileData[2];
				// printf("file data :%s", fileData+3);
				int total = packet_size;
				while(1)
				{
					if(fileData[0]=='L')
						isLast=1;
					packet_size = fileData[1]*256 + fileData[2];
					// printf("File desc: %d\n", fd);
					if(bytes<0)
					{
						perror("Recv failed");
						break;
					}

					if(write(fd, fileData+3, packet_size) < 0)
					{
						perror("Write failed");
						break;
					}
					if(isLast)
						break;
					memset(fileData, '\0', sizeof(fileData));
					bytes = recv(newsockfdD, fileData, 100, 0);
					
					// printf("%s", fileData+3);
					// printf("bytes = %d\n", bytes);
					total+=packet_size;

				}
				// printf("Got file %s with %d bytes\n", buf+4, total);


				close(newsockfdD);
				close(sockfdD);
				close(fd);
				exit(0);
			}
			else 
			{
				send(sockfd, buf, strlen(buf)+1, 0);
				short code;
				recv(sockfd, &code, sizeof(code), 0);
				code = ntohs(code);
				printf("Error code: %d\n", code);

				if(code==550)
				{
					printf("File(s) could not be opened\n");
					kill(ret, SIGTERM);
					wait(NULL);
				}
				else if(code==501)
				{
					printf("Invalid argument(s)\n");
					kill(ret, SIGTERM);
					wait(NULL);
				}
				else
				{ 
					int temp2;
					pid_t temp = waitpid(ret, &temp, WEXITED);
					// printf("code = %d %d\n", code, htons(code));
					if(code==250)
						printf("File received\n");
				}
			}
		}
		else if(strncmp(buf, "put ", 4)==0)
		{
			int ret = fork();
			if(ret==0)			//in child
			{
				int sockfdD;
				struct sockaddr_in cli_addrD, serv_addrD;

				if((sockfdD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
				{
					perror("TCP Server socket creation failed for D");
					exit(EXIT_FAILURE);
				}
				if (setsockopt(sockfdD, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
			    	perror("setsockopt(SO_REUSEADDR) failed for D");


				serv_addrD.sin_family		= AF_INET;
				serv_addrD.sin_addr.s_addr	= INADDR_ANY;
				serv_addrD.sin_port			= htons(portY);

				if(bind(sockfdD, (struct sockaddr *) &serv_addrD, sizeof(serv_addrD)) < 0)
				{
					perror("TCP Server bind failed for D");
					// close(sockfdD);
					exit(EXIT_FAILURE);
				}

				listen(sockfdD, 5);
				int clilenD = sizeof(cli_addrD);
				int newsockfdD = accept(sockfdD, (struct sockaddr *) &cli_addrD, &clilenD);
				if(newsockfdD<0)
				{
					perror("Accept error in D");
					close(sockfdD);
					exit(EXIT_FAILURE);
				}
				// int sockfdD;
				// struct sockaddr_in	serv_addrD;
				// // printf("Child is going to sleep\n");
				// sleep(1);
				
				// // printf("Child is awake\n");
				// if ((sockfdD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				// 	perror("Unable to create socket for D\n");
				// 	exit(0);
				// }

				// serv_addrD.sin_family	= AF_INET;
				// inet_aton("127.0.0.1", &serv_addrD.sin_addr);
				// serv_addrD.sin_port	= htons(portY);
				// if ((connect(sockfdD, (struct sockaddr *) &serv_addrD,
				// 					sizeof(serv_addrD))) < 0) {
				// 	perror("Unable to connect to server D\n");
				// 	// sleep(1);
				// 	exit(0);
				// }

				char fileData[100];
				// char *token = strtok(buf, " \t");
				fd = open(buf+4, O_RDONLY, 0666);
				if(fd<0)
				{
					// perror("No file found on client");
					close(newsockfdD);
					exit(EXIT_FAILURE);
				}
				
				char dataFromFile[100];
		    	short bytes = 1;
		    	while(bytes)
		    	{
		    		bytes = read(fd, dataFromFile+3, 97);
		    		if(bytes<0) break;
		    		if(bytes==97)
		    			dataFromFile[0]='X';
		    		else dataFromFile[0]='L';		//last block
					// printf("%s", dataFromFile+3);
		    		
	    			dataFromFile[1]=bytes/256;
	    			dataFromFile[2]=bytes%256;
		    		
			    	int t = send(newsockfdD, dataFromFile, bytes+3, 0);
			    	if(t<0)
			    	{
			    		printf("**********Send failed\n");
			    	}
			    	memset(&dataFromFile, '\0', sizeof(dataFromFile));
		    	}

		    	// printf("File %s sent successfully\n", buf+4);

				
				close(sockfdD);
				close(fd);
				exit(EXIT_SUCCESS);
			}
			else 
			{
				// printf("in parent\n");
				send(sockfd, buf, strlen(buf)+1, 0);
				short code;
				recv(sockfd, &code, sizeof(code), 0);
				code = ntohs(code);
				printf("Error code: %d\n", code);
				if(code==550)
				{
					printf("File could not be sent\n");
					// kill(ret, SIGTERM);
				}
				else { 
					int temp2;
					// pid_t temp = wait();
					if(code==250)
						printf("File sent\n");
				}
			}
		}

		else 
		{
			// printf("Command is %s\n", buf);


			send(sockfd, buf, strlen(buf)+1, 0);
			short code;

			recv(sockfd, &code, sizeof(code), 0);
			// printf("Error code: %d\n", code);
			code = ntohs(code);
			if(code==421)
			{
				printf("Goodbye!\n");
				close(sockfd);
				exit(0);
			}
			if(code==501)
				printf("Invalid argument(s)\n");
			if(code==502)
				printf("Invalid command\n");
		}
	
	}

	return 0;
}