#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/ip.h> 
#include <linux/udp.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/cdefs.h>
#include <linux/icmp.h>
#include <time.h>


#define MSG_SIZE 2048
#define LISTEN_PORT 5678
#define LISTEN_IP "127.0.0.1"

int main(int argc, char *argv[])
{
	if(argc!=2)
	{
		printf("Usage: mytraceroute <domain>\n");
		return 0;
	}
	int rawsockfd1, rawsockfd2;
	struct sockaddr_in serv_addr1, serv_addr2, cli_addr, dest_addr;
	socklen_t raw1_len, raw2_len;

	// get IP of domain
	struct hostent *ip = gethostbyname(argv[1]);

	if(ip==NULL)
	{
		herror("gethostbyname");
		exit(1);
	}

	char *dest_ip = inet_ntoa(*(struct in_addr*)(ip->h_addr));
	printf("Sending to IP Address: %s\n", dest_ip);


	char ipaddr[32];
	strcpy(ipaddr, dest_ip);

	// create 2 raw sockets and bind

	if((rawsockfd1 = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0)
	{
		perror("Socket1 creation");
		exit(1);
	}

	serv_addr1.sin_family = AF_INET;
	serv_addr1.sin_port = htons(LISTEN_PORT);
	serv_addr1.sin_addr.s_addr = INADDR_ANY;
	raw1_len = sizeof(serv_addr1);

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(32164);
	dest_addr.sin_addr.s_addr = inet_addr(dest_ip);


	if(bind(rawsockfd1, (struct sockaddr *)&serv_addr1, raw1_len) < 0)
	{
		perror("Socket2 bind");
		exit(1);
	}

	// S1 <- IPHDR_INCL
	if(setsockopt(rawsockfd1, IPPROTO_IP, IP_HDRINCL, &(int){1}, sizeof(int)) < 0)
	{
		perror("setsockopt");
		exit(1);	
	}

	if((rawsockfd2 = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
	{
		perror("Socket2 creation");
		exit(1);
	}

	serv_addr2.sin_family = AF_INET;
	serv_addr2.sin_port = htons(32164);
	serv_addr2.sin_addr.s_addr = INADDR_ANY;
	raw2_len = sizeof(serv_addr2);

	if(bind(rawsockfd2, (struct sockaddr *)&serv_addr2, raw2_len) < 0)
	{
		perror("Socket1 bind");
		exit(1);
	}


	// Loop 3 times till select successful

	int TTL = 1;
	int flag = 0;
	int count = 0;
	while(1)
	{
		// Create UDP datagram
		char datagram[MSG_SIZE], *payload;
		memset(datagram, '\0', MSG_SIZE);

		struct iphdr *hdrip = (struct iphdr *)datagram;
		struct udphdr *hdrudp = (struct udphdr *)(datagram + sizeof(struct iphdr));
			
		payload = datagram + sizeof(struct iphdr) + sizeof(struct udphdr);

		strcpy(payload, "qazwsxedcrfvtgbyhnujmikolppolikujmyhntgbrfvedcwsxqaz");

		// Append IP header and TTL
		hdrip->ihl = 5;
		hdrip->ttl = TTL;
		hdrip->version = 4;
		hdrip->id = 0;
		hdrip->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) 
							+ strlen(payload);
		hdrip->protocol = IPPROTO_UDP;
		hdrip->saddr = 0;
		hdrip->daddr = inet_addr(ipaddr);
		// printf("Dest IP: %s\n", ipaddr);
		
		// Set Dest port
		hdrudp->dest = htons(32164);
		hdrudp->source = htons(LISTEN_PORT);
		hdrudp->len = htons(sizeof(struct udphdr) + strlen(payload));
		hdrudp->check = 0;

		// send packet through S1
		int ret = sendto(rawsockfd1, datagram, hdrip->tot_len, 0, 
			(struct sockaddr *)&dest_addr, sizeof(dest_addr));

		// Start time

		struct timespec send_time = {0, 0}, recv_time = {0, 0};
        timespec_get(&send_time, TIME_UTC);


		if(ret < 0)
		{
			perror("sendto");
			exit(1);
		}

		//select on S2 to wait for ICMP timeout=1s
		
		struct timeval t;
		t.tv_sec = 1;
    	t.tv_usec = 0;	
		fd_set readfds;
    	while(1)
    	{
			FD_ZERO(&readfds);
			FD_SET(rawsockfd2, &readfds);
    		int r = select(rawsockfd2+1, &readfds, 0, 0, &t);

	    	if(r < 0)
	    	{
	    		perror("select");
	    		exit(1);
	    	}

	    	if(r==0)
	    	{
				count++;
				break;
	    	}
	    	else 
	    	{
	    		//TODO: handle select
	    		if(FD_ISSET(rawsockfd2, &readfds))
	    		{
					// printf("Dest IP ISSET: %s\n", ipaddr);
			     	 		
	   				socklen_t len = sizeof(cli_addr);
	    			char buf[MSG_SIZE];
	    			memset(buf, '\0', sizeof(MSG_SIZE));
	    			int ret = recvfrom(rawsockfd2, (char *)buf, MSG_SIZE, 0, 
	    				(struct sockaddr *) &cli_addr, &len);
	    		
			    	timespec_get(&recv_time, TIME_UTC);
					double ping = ((double)recv_time.tv_sec + 1.0e-9*recv_time.tv_nsec) - 
	       					((double)send_time.tv_sec + 1.0e-9*send_time.tv_nsec);
	   				ping*=1000.0;
				

	    			struct iphdr *recv_hdrip = (struct iphdr *)buf;
	    			int proto = recv_hdrip->protocol;
	    			char *recv_ip_dest = inet_ntoa(*(struct in_addr*)&(recv_hdrip->daddr));
	    			char *recv_ip_src = inet_ntoa(*(struct in_addr*)&(recv_hdrip->saddr));
	    			
	    			// printf("IP daddr: %s\n", inet_ntoa(*(struct in_addr*)&(recv_hdrip->daddr)));

	    			struct icmphdr *hdricmp = (struct icmphdr *)(buf + sizeof(struct iphdr));
	    			int recv_type = hdricmp->type;

	    			if(recv_type == 3) 	//Destination Unreachable
	    			{
	    				// printf("%s\t %s\n", recv_ip_src, ipaddr);
	    				if(strcmp(recv_ip_src, ipaddr) == 0)	// Recieved message from correct source
	    				{
	    					printf("Hop_Count(%d)\t%s\t%lf ms\n", TTL, recv_ip_dest, ping);


							close(rawsockfd1);
							close(rawsockfd2);
	    					exit(0);
	    				}
	    				else 
	    				{
	    					printf("not matching\n");
	    				}
	    			}
	    			else if(recv_type == 11)	// Time Exceeded
	    			{
	    				printf("Hop_Count(%d)\t%s\t%lf ms\n", TTL, recv_ip_dest, ping);
						TTL++;
						count = 0;
						break;
	    			}
	    			else 
	    			{
	    				printf("spurious\n");
	    				continue;
	    			}
	    		}
			}
    	}

		if(count == 3)
		{
			printf("Hop_Count(%d)\t*\t*\n", TTL);
			TTL++;
			count = 0;
		}	
	}

	close(rawsockfd1);
	close(rawsockfd2);
		
	return 0;
}