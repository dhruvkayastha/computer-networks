// Run as - Usage: %s [-t ttl] [-i interval] [-c count] [-W timeout] [-s packetsize] [-w deadline] <domain>
// Use root privileges
// Author: Dhruv Kayastha
// Email: dhruvkayastha@gmail.com

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

#define LISTEN_PORT 5678
#define LISTEN_IP "127.0.0.1"


int loop = 1;
int TIMEOUT = 4;
int TTL = 255;
int count = -1;
double PING_INTERVAL = 1.0;
int PACKET_SIZE = 56;
int w = 0;

void handler(int x)
{
	loop = 0;
}

unsigned short checksum(void *b, int size) 
{   
	unsigned short *buf = b; 
    unsigned int sum=0; 
    unsigned short result; 
  
    for (sum = 0; size > 1; size -= 2) 
        sum += *buf++; 
    if (size == 1) 
        sum += *(unsigned char*)buf; 
    sum = (sum >> 16) + (sum & 0xFFFF); 
    sum += (sum >> 16);
    return ~sum; 
} 

int main(int argc, char* argv[])
{
	if(argc == 1)
    {
        printf("Usage: %s [-t ttl] [-i interval] [-c count] [-W timeout] [-s packetsize] [-w deadline] <domain>\n", argv[0]);
        exit(1);
    }

    int opt;
    while ((opt = getopt(argc, argv, "i:c:W:s:t:w:")) != -1)
    {
		switch (opt)
		{
			case 'c':
				count = atoi(optarg);
				break;
            case 'i':
                PING_INTERVAL = atof(optarg);
                break;
            case 'W':
            	TIMEOUT = atoi(optarg);
            	break;
            case 's':
            	PACKET_SIZE = atoi(optarg);
            	break;
            case 't':
            	TTL = atoi(optarg);
            	break;
            case 'w':
            	w = atoi(optarg);
            	break;
            default:
                printf("Usage: %s [-t ttl] [-i interval] [-c count] [-W timeout] [-s packetsize] [-w deadline] <domain>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

	PACKET_SIZE += 8; // 8 bytes required for IP header

	if (optind >= argc) {
	   printf("Expected domain name after options\n");
	   exit(1);
	}

	int sockfd1, sockfd2;

	struct sockaddr_in serv_addr1, serv_addr2, cli_addr, dest_addr;
	socklen_t len1, len2;

    char *domain = argv[optind];
	

	struct hostent *ip = gethostbyname(argv[optind]);
	if(ip == NULL)
	{
		perror("gethostbyname");
		exit(1);
	}

	char *dest_ip = inet_ntoa(*(struct in_addr*)(ip->h_addr));
	// printf("Pinging IP Address: %s\n", dest_ip);
	char ipaddr[32];
	strcpy(ipaddr, dest_ip);


    struct sockaddr_in temp_addr;	 
	socklen_t len; 
	char buf[NI_MAXHOST], *reverse_hostname; 

	temp_addr.sin_family = AF_INET; 
	temp_addr.sin_addr.s_addr = inet_addr(ipaddr); 
	len = sizeof(struct sockaddr_in); 

	if (getnameinfo((struct sockaddr *) &temp_addr, len, buf, 
					sizeof(buf), NULL, 0, NI_NAMEREQD)) 
	{ 
		herror("reverse lookup"); 
		exit(1);
	} 
	reverse_hostname = (char*)malloc((strlen(buf) +1)*sizeof(char) ); 
	strcpy(reverse_hostname, buf);

	// printf("Reverse lookup: %s\n", reverse_hostname);
	
	if((sockfd1 = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
	{
		perror("Socket1 creation");
		printf("Try running the code with sudo access\n");
		exit(1);
	}

	serv_addr1.sin_family = AF_INET;
	serv_addr1.sin_port = htons(LISTEN_PORT);
	serv_addr1.sin_addr.s_addr = INADDR_ANY;
	len1 = sizeof(serv_addr1);

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(32164);
	dest_addr.sin_addr.s_addr = inet_addr(dest_ip);


	if(bind(sockfd1, (struct sockaddr *)&serv_addr1, len1) < 0)
	{
		perror("Socket1 bind");
		exit(1);
	}

	// S1 <- IPHDR_INCL
	if(setsockopt(sockfd1, IPPROTO_IP, IP_HDRINCL, &(int){1}, sizeof(int)) < 0)
	{
		perror("setsockopt iphdr_incl");
		close(sockfd1);
		exit(1);	
	}
	if (setsockopt(sockfd1, SOL_SOCKET, IP_TTL, 
		&TTL, sizeof(TTL)) != 0) 
	{ 
		perror("setsockopt ttl");
		close(sockfd1);
		exit(1); 
	}

	struct timeval tv_out; 
	tv_out.tv_sec = TIMEOUT; 
	tv_out.tv_usec = 0; 

	if(setsockopt(sockfd1, SOL_SOCKET, SO_RCVTIMEO, 
				(const char*)&tv_out, sizeof tv_out) < 0)
	{
		perror("setsockopt timeout");
		close(sockfd1);
		exit(1);
	}


    signal(SIGINT, handler);	// to display final results
    
    if(w > 0)	// for deadline flag
    {
    	signal(SIGALRM, handler);
    	alarm(w);
    }

	struct timespec tfe, tfs;
	timespec_get(&tfs, TIME_UTC);

	int id = 0;
	int received = 0;

	long double max = -1.0;
	long double total = 0.0;
	long double min = 10000000.0;
	printf("PING %s (h: %s) (%s) %d data bytes\n", reverse_hostname, domain, ipaddr, PACKET_SIZE);

	while(loop && count!=0)
	{
		count--;
		// char* datagram = (char*)malloc(PACKET_SIZE);
		char datagram[PACKET_SIZE];
		memset(datagram, '\0', PACKET_SIZE);
		struct iphdr* hdrip = (struct iphdr *)datagram;
		struct icmphdr *hdricmp = (struct icmphdr *)(datagram + sizeof(struct iphdr));

		char *payload = datagram + sizeof(struct iphdr) + sizeof(struct icmphdr);

		// filling payload of packet with some data
		memset(payload, 'z', PACKET_SIZE - sizeof(struct icmphdr) - sizeof(struct iphdr));
		

		//setting up ip header
		hdrip->ihl = 5;
		hdrip->ttl = TTL;
		hdrip->version = 4;
		hdrip->id = 0;
		hdrip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr) 
							+ strlen(payload));
		hdrip->protocol = IPPROTO_ICMP;
		hdrip->saddr = 0;
		hdrip->daddr = inet_addr(ipaddr);
		// printf("Dest IP: %s\n", ipaddr);		

		// setting up icmp header
		hdricmp->type = ICMP_ECHO;
		hdricmp->code = 0;
		hdricmp->un.echo.id = getpid(); 
		hdricmp->un.echo.sequence = id++; 
		hdricmp->checksum = checksum(hdricmp, PACKET_SIZE - sizeof(struct iphdr));

		sleep(PING_INTERVAL);
		int bytes;
		struct timespec send_time = {0, 0}, recv_time = {0,0};
		timespec_get(&send_time, TIME_UTC);

		if((bytes = sendto(sockfd1, &datagram, PACKET_SIZE, 0,
			(struct sockaddr*) &dest_addr, sizeof(dest_addr))) < 0)
		{
			perror("send failed");
			close(sockfd1);
			exit(1);
		}
		
		
		struct timeval t;
		t.tv_sec = 5;
    	t.tv_usec = 0;	
		fd_set readfds;
		int r;
		
		FD_ZERO(&readfds);
		FD_SET(sockfd1, &readfds);
		// sockfd2 = sockfd1;
		r = select(sockfd1+1, &readfds, 0, 0, &t);
		if(r < 0)
		{
			perror("select");
			close(sockfd1);
			exit(1);
		}
		if(r == 0)
		{
			printf("select timeout....continuing\n");
		}
		else
		{
			if(FD_ISSET(sockfd1, &readfds))
			{
				socklen_t len = sizeof(cli_addr);
				char buf[PACKET_SIZE];
				memset(buf, '\0', PACKET_SIZE);

				if((bytes = recvfrom(sockfd1, (char*)buf, PACKET_SIZE, 0, (struct sockaddr *) &cli_addr, &len)) < 0)
				{
					perror("recv error");
				}
				else
				{
					timespec_get(&recv_time, TIME_UTC);
					double ping = ((double)recv_time.tv_sec + 1.0e-9*recv_time.tv_nsec) - 
	       					((double)send_time.tv_sec + 1.0e-9*send_time.tv_nsec);
	   				ping*=1000.0;

	    			struct iphdr *recv_hdrip = (struct iphdr *)buf;
	    			int proto = recv_hdrip->protocol;
	    			char *recv_ip_dest = inet_ntoa(*(struct in_addr*)&(recv_hdrip->daddr));
	    			char *recv_ip_src = inet_ntoa(*(struct in_addr*)&(recv_hdrip->saddr));
	    			
	    			// printf("IP daddr: %s\n", inet_ntoa(*(struct in_addr*)&(recv_hdrip->daddr)));

	    			struct icmphdr *recv_hdricmp = (struct icmphdr *)(buf + sizeof(struct iphdr));
	    			int recv_type = recv_hdricmp->type;
	    			
	    			if(recv_type == ICMP_ECHOREPLY)
	    			{
	    				printf("%d bytes from %s (h: %s)(%s): icmp_seq=%d ttl=%d rtt = %f ms.\n", 
							bytes, reverse_hostname, domain, 
							ipaddr, recv_hdricmp->un.echo.sequence, 
							recv_hdrip->ttl, ping); 
	    				received++;
	    				if(ping > max)
	    				{
	    					max = ping;
	    				}
	    				if(ping < min)
	    				{
	    					min = ping;
	    				}
	    				total += ping;
	    			}
	    			else if(recv_type == ICMP_TIME_EXCEEDED)	// ICMP Time Limit Exceeded
	    			{
	    				printf("%d bytes from %s (h: %s)(%s): icmp_seq=%d Time exceeded: Hop limit\n", 
							bytes, reverse_hostname, domain, 
							ipaddr, recv_hdricmp->un.echo.sequence);
	    			}
	    			else
	    			{
	    				printf("Error - ICMP Type %d message received\n", recv_type);
	    			}

				}
			}
		}
    }
	timespec_get(&tfe, TIME_UTC);
	double timeElapsed = ((double)(tfe.tv_nsec - 
						tfs.tv_nsec))/1000000.0; 
	
	long double total_msec = (tfe.tv_sec-tfs.tv_sec)*1000.0+timeElapsed ;
	printf("\n--- %s ping statistics ---\n", domain); 
	printf("%d packets transmitted, %d packets received, %f%% packet loss, time %Lf ms.\n", 
		id, received, ((id - received)/id) * 100.0, total_msec);
	if(received > 0)
		printf("rtt min/avg/max = %Lf %Lf %Lf ms\n", min, total/1.0*received, max);
	close(sockfd1);
}