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

#define MAXLINE 1024

int max(int a, int b)
{
	if(a>b) return a;
	return b;
}

int main()
{
	int	tcpsockfd, newsockfd, udpsockfd, nfd; 
	int	clilen;
	struct sockaddr_in	cli_addr, serv_addr;
	fd_set readfs, writefs;
	char buff[MAXLINE];
	int i;
	char buf[100];

	if ((tcpsockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("tcp socket creation failed\n");
		exit(0);
	}

	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port			= htons(6000);

	if (bind(tcpsockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		perror("tcp bind failed\n");
		exit(0);
	}
	listen(tcpsockfd, 5);

	udpsockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( udpsockfd < 0 ) { 
        perror("udp socket creation failed\n"); 
        exit(EXIT_FAILURE); 
    } 
    if ( bind(udpsockfd, (const struct sockaddr *)&serv_addr,  
            sizeof(serv_addr)) < 0 ) 
    { 
        perror("udp bind failed"); 
        exit(EXIT_FAILURE); 
    } 

    nfd = max(udpsockfd, tcpsockfd) + 1;
    	

    while(1)
    {
    	FD_ZERO(&readfs);
    	FD_SET(udpsockfd, &readfs);
    	FD_SET(tcpsockfd, &readfs);
    	int r = select(nfd, &readfs, &writefs, 0, 0);

    	if(r==-1)
    	{
    		perror("select failed\n");
    		continue;
    	}
    	if(FD_ISSET(udpsockfd, &readfs))
    	{
    		if(fork()==0)
    		{
    		 	int len = sizeof(cli_addr);
			    for(i=0; i < MAXLINE; i++) 
	   				buff[i] = '\0';
			    int udp_ret = recvfrom(udpsockfd, (char *)buff, MAXLINE, 0,
			    	(struct sockaddr *) &cli_addr, &len);
				// printf("BUFF: %s\n", buff);    
			    struct hostent *ip = gethostbyname(buff);

				if(ip!=NULL)
				{
					int flag = sendto(udpsockfd, inet_ntoa( *( struct in_addr*)(ip->h_addr)), strlen(inet_ntoa( *( struct in_addr*)(ip->h_addr))), 0,
						(const struct sockaddr *) &cli_addr, sizeof(cli_addr));
					if(flag < 0)
						perror("UDP send failed");
					continue;
			    }
	    		else
	    		{
	        		perror("gethostbyname");
					int flag = sendto(udpsockfd, "Couldn't get host", strlen("Couldn't get host"), 0,
						(const struct sockaddr *) &cli_addr, sizeof(cli_addr));
					if(flag < 0)
						perror("UDP send failed");
	    		}
	    		exit(0);
	    	}
    	}
    	if(FD_ISSET(tcpsockfd, &readfs))
    	{
    		int clilen = sizeof(cli_addr);
   			int newsockfd = accept(tcpsockfd, (struct sockaddr *) &cli_addr,
   				 &clilen);

    		if (newsockfd < 0) 
    		{
				perror("Accept error");
				exit(0);
			}
    		if(fork()==0)
    		{
    			close(tcpsockfd);
    			for(i = 0; i<MAXLINE; i++)
    				buff[i] = '\0';
    			int flag = recv(newsockfd, buff, MAXLINE, 0);
    			int fd = open("word.txt", O_RDONLY);
				if(fd < 0)
			    {
					perror("No file found\n");
					close(newsockfd);
					exit(EXIT_FAILURE);
			    }
		  	    printf("File 'word.txt' has been opened on server\n");
			  	char dataFromFile[MAXLINE];
		    	int n = 1;
		    	int bytes = 0;
		    	int inword = 1;
		    	char sendData[MAXLINE];
		    	int j=0;
		    	while(1)
		    	{
		    		n = read(fd, dataFromFile, 99);
		    		if(n<=0) break;
		    		// bytes++;
		    		// dataFromFile[n] = '\0';
		    		// printf("%s\n", dataFromFile);
		    		int i;
		    		inword=1;
		    		for(i=0; i<n; i++)
					{
						if(inword)
						{
							if(dataFromFile[i]=='\n'||dataFromFile[i]=='\0')
							{
								sendData[j]='\0';
								printf("%s\n", sendData);
								send(newsockfd, sendData, j+1, 0);
								// printf("%d\n", j+1);
								memset(&sendData, '\0', sizeof(sendData));
								j=0;
								inword=0;
								bytes++;
							}
							else sendData[j++]=dataFromFile[i];
						}
						else if(dataFromFile[i]!='\n') 
						{
							inword=1;
							sendData[j++]=dataFromFile[i];
						}
					}
			    	// send(newsockfd, dataFromFile, n, 0);
			    	memset(&dataFromFile, '\0', sizeof(dataFromFile));
		    	}
		    	printf("Words: %d\n", bytes);
		    	// close(fd);
		    	close(newsockfd);
		    	exit(0);
    		}
    		close(newsockfd);
    		
    	}


    	



    		
    }

    return 0;
}
			
