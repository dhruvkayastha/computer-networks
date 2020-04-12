#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#define MAXLINE 80

int main()
{
	int sockfdS, len, i, sockfdC, newsockfd;
	socklen_t clilen;
	struct sockaddr_in cli_addr, serv_addr;
	char buff[MAXLINE];

	if((sockfdS = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("TCP Server socket creation failed");
		exit(0);
	}
	if (setsockopt(sockfdS, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
    	perror("setsockopt(SO_REUSEADDR) failed");


	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port			= htons(50000);

	if(bind(sockfdS, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("TCP Server bind failed");
		exit(0);
	}

	listen(sockfdS, 5);

	// int clilen = sizeof(cli_addr);
	// int newsockfd = accept(sockfdS, (struct sockaddr *) &cli_addr, &clilen);

	// if(newsockfd < 0)
	// {
	// 	perror("Accept error");
	// 	exit(0);
	// }
	while(1)
	{
		int portY=0, n=0, flag = 1;
		i=0;
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfdS, (struct sockaddr *) &cli_addr, &clilen);
		if(newsockfd < 0)
		{
			perror("Accept error");
			exit(0);
		}
		while(1)
		{
			if(newsockfd < 0)
			{
				perror("Accept error");
				exit(0);
			}
			memset(buff, '\0', sizeof(buff));
			n=0;
			


			while(1)
			{
				int j = 0;

				int temp = recv(newsockfd, buff+n, 80, 0);
				n+=temp;

				if(temp==0)
					break;

				int tflag = 0;
				for(j = n - temp; j<n; j++)
				{
					if(buff[j]=='\0')
						tflag=1;
				}
				if(tflag==1)
					break;
			}

			

			if(n<=0)
			{
				// printf("n = %d\n", n);
				// printf("N<0\n");
				short un = htons(501);
				send(newsockfd, &un, sizeof(short), 0);
				break;
			}
			// printf("n = %d\n", n);

			// printf("BUFF %s\n", buff);
			char *token = strtok(buff, " \t");
			// printf("Token %s\n", token);
			if(strcmp(token, "port")==0)
			{
				// printf("PORT\n");
				portY = 0;
				token = strtok(NULL, " \t");
				if(token==NULL || strtok(NULL, " \t")!=NULL)
				{
					printf("Invalid no of arguments\n");
					short un = htons(501);
					send(newsockfd, &un, sizeof(short), 0);
				}
				else
				{
					portY = atoi(token);

					if(portY>=1024 && portY<=65535)
					{
						printf("Port is %d\n", portY);
						short un = htons(200);
						send(newsockfd, &un, sizeof(short), 0);
						break;
					}
					else 
					{
						short un = htons(550);
						send(newsockfd, &un, sizeof(short), 0);
						printf("Invalid port entered\n");
					}
				}
			}
			else 
			{
				short un = htons(503);
				send(newsockfd, &un, sizeof(short), 0);
			}
		}
		//printf("Test\n");
		while(1)
		{
			if(portY==0)
				break;
			n=0;
			short status = 200;

			while(1)
			{
				int j = 0;

				int temp = recv(newsockfd, buff+n, 80, 0);
				n+=temp;

				if(temp==0)
					break;

				int tflag = 0;
				for(j = n - temp; j<n; j++)
				{
					if(buff[j]=='\0')
						tflag=1;
				}
				if(tflag==1)
					break;
			}
			if(n<=0)
			{
				close(newsockfd);
				// printf("n<0\n");
				break;
			}

			int argcount=0;
			int ii;
			int state=0;
			for (i = 0;i<strlen(buff);i++)
			{
				if (buff[i] == ' ' || buff[i] == '\t')
					state=0;
				else 
				{    
					if(state==0)
						argcount++;
					state = 1;
				}
			}
			// printf("argc = %d\n", argcount);
			// printf("recvd: %s\n", buff);
			char *token = strtok(buff, " \t");
			// printf("token: %s\n", token);

			char path[1000];
			if(strcmp(token, "cd")==0)
			{
				// printf("Compared!!!!!");
				int flag=-1;
				if(argcount!=2)
					status=501;
				else {
					if(token = strtok(NULL, " \t"))
						flag=chdir(token);
					getcwd(path, 1000);
					
					if(flag!=0)
					{
						//handle error
						status = 501;
					}
					else
					{
						status = 200;
						printf("PWD: %s\n", path);
						fflush(stdout);
					}
				}
				short un = htons(status);
				send(newsockfd, &un, sizeof(short), 0);		

			}
			else if(strcmp(token, "quit")==0)
			{

				short un = htons(421);
				if(argcount!=1)
					un = htons(501);
				send(newsockfd, &un, sizeof(short), 0);	
				close(newsockfd);
				break;
			}
			else if(strcmp(token, "get")==0)
			{


				token = strtok(NULL, " \t");

				int fd = open(token, O_RDONLY | 0666);

				if(fd<0)
				{
					short un = htons(550);
					send(newsockfd, &un, sizeof(short), 0);
					continue;
				}	
				printf("File %s opened\n", token);


				
				pid_t retD = fork();
				if(retD==0) 	// in Sd
				{
					int sockfdD;
					struct sockaddr_in	serv_addrD;
					sleep(1);
					
					if ((sockfdD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
						perror("Unable to create socket for D");
						exit(0);
					}

					serv_addrD.sin_family	= AF_INET;
					inet_aton("127.0.0.1", &serv_addrD.sin_addr);
					serv_addrD.sin_port	= htons(portY);
					if ((connect(sockfdD, (struct sockaddr *) &serv_addrD,
										sizeof(serv_addrD))) < 0) {
						perror("Unable to connect to server D");
						exit(0);
					}



					char dataFromFile[100];
			    	short bytes = 1;
			    	// printf("\n\n");
			    	while(bytes)
			    	{
			    		bytes = read(fd, dataFromFile+3, 97);
			    		if(bytes<0) break;
			    		if(bytes==97)
			    			dataFromFile[0]='X';
			    		else dataFromFile[0]='L';		//last block

		    			dataFromFile[1]=bytes/256;
		    			dataFromFile[2]=bytes%256;
			    		// printf("%s", dataFromFile+3);
			    		
				    	send(sockfdD, dataFromFile, bytes+3, 0);
				    	memset(&dataFromFile, '\0', sizeof(dataFromFile));
			    	}

			    	printf("File %s sent successfully\n", token);

					close(fd);
					close(sockfdD);
					exit(0);
				}
				else 
				{
					if(argcount!=2)
					{
						status = 501;
						short un = htons(status);
						send(newsockfd, &un, sizeof(short), 0);
						close(fd);
					}
					else
					{
						short un = htons(250);
						send(newsockfd, &un, sizeof(un), 0);
						close(fd);	
					}
					
				}

			}
			else if(strcmp(token, "put")==0)
			{
				

				token = strtok(NULL, " \t");
				// printf("putssssss %s\n", token);

				pid_t retD = fork();
				if(retD==0)
				{
					// int sockfdD;
					// struct sockaddr_in cli_addrD, serv_addrD;

					// if((sockfdD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
					// {
					// 	perror("TCP Server socket creation failed for D");
					// 	exit(0);
					// }
					// if (setsockopt(sockfdD, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
				 //    	perror("setsockopt(SO_REUSEADDR) failed for D");


					// serv_addrD.sin_family		= AF_INET;
					// serv_addrD.sin_addr.s_addr	= INADDR_ANY;
					// serv_addrD.sin_port			= htons(portY);

					// if(bind(sockfdD, (struct sockaddr *) &serv_addrD, sizeof(serv_addrD)) < 0)
					// {
					// 	perror("TCP Server bind failed for D");
					// 	exit(0);
					// }

					// listen(sockfdD, 5);
					// int clilenD = sizeof(cli_addrD);
					// int newsockfdD = accept(sockfdD, (struct sockaddr *) &cli_addrD, &clilenD);
					// if(newsockfdD<0)
					// {
					// 	perror("Accept error in D");
					// 	exit(EXIT_FAILURE);
					// }

					int sockfdD;
					struct sockaddr_in	serv_addrD;
					// printf("Child is going to sleep\n");
					sleep(1);
					
					// printf("Child is awake\n");
					if ((sockfdD = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
						perror("Unable to create socket for D\n");
						exit(EXIT_FAILURE);
					}

					serv_addrD.sin_family	= AF_INET;
					inet_aton("127.0.0.1", &serv_addrD.sin_addr);
					serv_addrD.sin_port	= htons(portY);
					if ((connect(sockfdD, (struct sockaddr *) &serv_addrD,
										sizeof(serv_addrD))) < 0) {
						perror("Unable to connect to client D\n");
						// sleep(1);
						exit(EXIT_FAILURE);
					}


					int fd = open(token, O_WRONLY | O_CREAT | O_TRUNC , 0666);

					if(fd<0)
					{
						short un = htons(550);
						send(sockfdS, &un, sizeof(short), 0);
						continue;
					}	
					printf("File %s opened on server\n", token);

					char fileData[100];

					int isLast=0;
					sleep(1);
					int bytes = recv(sockfdD, fileData, 100, 0); 
					if(bytes<=0)
					{
						printf("No file found on client\n");
						close(fd);
						close(sockfdD);
						exit(EXIT_FAILURE);
					}
					short packet_size = fileData[1]*256 + fileData[2];
					// printf("file data :%s", fileData+3);
					int total = bytes;
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
						bytes = recv(sockfdD, fileData, 100, 0);
						
						total+=packet_size;

					}
					printf("Got file %s with %d bytes\n", token, total);


					close(sockfdD);
					close(fd);
					exit(0);
				}
				else 
				{
					if(argcount!=2)
					{
						status = 501;
						short un = htons(status);
						send(newsockfd, &un, sizeof(short), 0);
					}
					else
					{
						int temp;
						short un;
						wait(&temp);
						if(WEXITSTATUS(temp) == EXIT_FAILURE)
							un = htons(550);
						else un = htons(250);
						send(newsockfd, &un, sizeof(un), 0);
					}
				}

			}
			else {

				short un = htons(502);
				send(newsockfd, &un, sizeof(un), 0);
			}

			// close(newsockfd);
			// break;
		}
		// close(newsockfd);
	}

	return 0;
}