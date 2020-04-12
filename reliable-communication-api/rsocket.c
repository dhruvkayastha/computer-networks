#include "rsocket.h"

#define N 105
#define T 2

int transmissions = 0;
int id = 0;

pthread_t X;
pthread_mutex_t trans_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t unack_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buff_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t msgid_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_buff = PTHREAD_COND_INITIALIZER;

struct _unack_msg{
	int id;
	bool isUnack;
	char msg[100];
	time_t msg_time;
	struct sockaddr dest_addr;
	int flags;
	socklen_t addrlen;
	size_t len;
};
typedef struct _unack_msg unack_msg;

struct _message {
	int id;
	char msg[100];
	struct sockaddr src_addr;
};
typedef struct _message message;

struct _messageQ {
	int in;
	int out;
	message Q[N];
};
typedef struct _messageQ messageQ;


void push(messageQ *msgQ, message msg)
{
	if((msgQ->in+1)%N == msgQ->out)
		return;
	memcpy(&(msgQ->Q[msgQ->in]), &msg, sizeof(msg));
	msgQ->in = (msgQ->in+1)%N;
	if(msgQ->in==(msgQ->out+1)%N)
		pthread_cond_signal(&cond_buff);
}

void pop(messageQ *msgQ, message *recv_msg)
{
	message temp = msgQ->Q[msgQ->out];
	msgQ->out = (msgQ->out+1)%N;
	*recv_msg = temp;
}

int isEmpty(messageQ *msgQ)
{
	if(msgQ->in==msgQ->out)
		return 1;
	return 0;
}


unack_msg *unack_msg_table;
messageQ *recv_buffer;
int *recv_msg_id;


// static short id = 0;

void HandleRetransmit(int sockfd)
{
	int i;
	pthread_mutex_lock(&unack_lock);
	time_t ts = time(NULL);
	// printf("loop enter\n");
	for(i = 0; i<N; i++)
	{
		// printf("id = %d, i = %d\n", unack_msg_table[i].id, i);
		// if(i == unack_msg_table[i].id)
		if(unack_msg_table[i].isUnack)
		{
			// printf("unack %d\n", i);
			if(ts - unack_msg_table[i].msg_time >= T)
			{
				char temp[102];
				memset(temp, '\0', sizeof(temp));
				temp[0] = 'M';
				temp[1] = i%256;

				// memcpy(temp+2, unack_msg_table[i].msg, strlen(unack_msg_table[i].msg)+1);
				memcpy(temp+2, unack_msg_table[i].msg, unack_msg_table[i].len);


				// printf("Retransmitting %s\n", unack_msg_table[i].msg);


				int ret = 0;
				ret = sendto(sockfd, temp, unack_msg_table[i].len+2, 0, 
					&unack_msg_table[i].dest_addr, unack_msg_table[i].addrlen);
				if(ret < 0)
				{
					perror("sendto HandleRetransmit");
					exit(EXIT_FAILURE);
				}

				pthread_mutex_lock(&trans_lock);
				transmissions++;
				pthread_mutex_unlock(&trans_lock);

				unack_msg_table[i].msg_time = time(NULL);
			}
		}
	}
	pthread_mutex_unlock(&unack_lock);
}

void HandleACKMsgRecv(int sockfd, int i)
{
	// printf("Acknowledge %d\n", i);
	pthread_mutex_lock(&unack_lock);
	// if(unack_msg_table[i].id == i)
	if(unack_msg_table[i].isUnack)
	{
		unack_msg_table[i].isUnack = false;
	}
	pthread_mutex_unlock(&unack_lock);

}

void HandleAppMsgRecv(int sockfd, char* buf, const struct sockaddr *src_addr, socklen_t addrlen, int len)
{
	int id = buf[1];
	char ACK[2];
	ACK[0] = 'A';
	ACK[1] = id%256;
	// ACK[2] = '\0';
	pthread_mutex_lock(&msgid_lock);
	if(recv_msg_id[id]==1) 	//duplicate
	{	
		sendto(sockfd, ACK, sizeof(ACK), 0, src_addr, addrlen);
	}
	else 
	{
		message temp;
		temp.id = id;
		// temp.src_addr = *src_addr;
		memcpy(&temp.src_addr, src_addr, addrlen);
		memcpy(temp.msg, buf+2, strlen(buf+2)+1);
		
		sendto(sockfd, ACK, sizeof(ACK), 0, src_addr, addrlen);

		pthread_mutex_lock(&buff_lock);
		push(recv_buffer, temp);
		pthread_mutex_unlock(&buff_lock);

		recv_msg_id[id] = 1;
		
	}
	pthread_mutex_unlock(&msgid_lock);

}

void HandleReceive(int sockfd)
{
	char buf[102];
	memset(buf, '\0', sizeof(buf));
	struct sockaddr src_addr;
	socklen_t addrlen = sizeof(src_addr);
	// printf("drp %d\n", flag);
	
	int n = recvfrom(sockfd, buf, 102, 0, &src_addr, &addrlen);
	// printf("Received in rocket: %s\n", buf);
	if(dropMessage(p))
		return;
	if(n<0)
	{
		perror("Recvfrom error");
		exit(EXIT_FAILURE);
	}
	int id = buf[1];
	if(buf[0]=='A')
		HandleACKMsgRecv(sockfd, id);
	else HandleAppMsgRecv(sockfd, buf, &src_addr, addrlen, n);
}

void* thread_handle(void* arg)
{
	// sleep(2);
	int sockfd = *((int*)arg);
	fd_set readfs;
	struct timeval *t = (struct timeval *)malloc(sizeof(struct timeval));
	t->tv_sec = T;
	t->tv_usec = 0;

	while(1)
	{
		// printf("Selecting\n");
		FD_ZERO(&readfs);
    	FD_SET(sockfd, &readfs);
    	int r = select(sockfd+1, &readfs, NULL, 0, t);
    	if(r<0)
    	{
    		perror("Select failed");
    		// fflush(1);
    		exit(EXIT_FAILURE);
    	}
    	if(r==0)
    	{
    		// printf("Rocket select r = %d, sockfd = %d\n", r, sockfd);
    		HandleRetransmit(sockfd);
			t->tv_sec = T;
			t->tv_usec = 0;
    	}
    	else if(FD_ISSET(sockfd, &readfs))
    	{
    		// printf("Rocket select r = %d, sockfd = %d\n", r, sockfd);
    		HandleReceive(sockfd);
    	}

	}
}

int dropMessage(float P)
{
	// srand(time(NULL));
	float x = (rand()*1.0)/(float)RAND_MAX;
	// printf("x = %f\n", x);
	if(x < P) return 1;
	return 0;
}

int r_socket(int domain, int type, int protocol)
{
	int sockfd = socket(domain, SOCK_DGRAM, protocol);
	
	unack_msg_table = (unack_msg*)malloc(sizeof(unack_msg)*N);
	int i;
	for(i = 0; i<N; i++)
	{
		unack_msg_table[i].isUnack = false;
		// unack_msg_table[i].id = -1;
	}
	recv_msg_id = (int*)calloc(N, sizeof(int));
	recv_buffer = (messageQ*)malloc(sizeof(messageQ));
	
	return sockfd;
}

int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	// if(ret = bind(sockfd, addr, addrlen) < 0)
	//  	printf("Error in rocket\n");
	int sock = sockfd;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	if(pthread_create(&X, &attr, thread_handle, &sock) < 0)
		return -1;
	
	return bind(sockfd, addr, addrlen);
}

int r_close(int fd)
{
	free(unack_msg_table);
	free(recv_msg_id);
	free(recv_buffer);
	printf("No of transmissions = %d for %d messages. Avg = %f\n", transmissions, id, 1.0*transmissions/(float)id);
	if(close(fd) || pthread_cancel(X))
		return -1;
	return 0;
}

ssize_t r_sendto(int sockfd, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen)
{
	char buffer[102];
	buffer[0] = 'M';
	// buffer[0] = 'A';
	buffer[1] = id%256;
	memcpy(buffer+2, buf, len);
	buffer[len+2] = '\0';

	pthread_mutex_lock(&unack_lock);
	
	// unack_msg_table[id].id = id;
	unack_msg_table[id].isUnack = true;
	// unack_msg_table[id].dest_addr = *dest_addr;
	memcpy(&unack_msg_table[id].dest_addr, dest_addr, addrlen);
	unack_msg_table[id].msg_time = time(NULL);
	memcpy(unack_msg_table[id].msg, buf, len);
	unack_msg_table[id].flags = flags;
	unack_msg_table[id].addrlen = addrlen;
	unack_msg_table[id].len = len;


	pthread_mutex_unlock(&unack_lock);

	id = (id+1)%N;
	pthread_mutex_lock(&trans_lock);
	transmissions++;
	pthread_mutex_unlock(&trans_lock);
	return sendto(sockfd, buffer, len+2, 0, dest_addr, addrlen);
}

ssize_t r_recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen)
{
	pthread_mutex_lock(&buff_lock);
	while(isEmpty(recv_buffer))
	{
        pthread_cond_wait(&cond_buff, &buff_lock);
		// usleep(5);
	}
	// message recv_msg = pop(recv_buffer);
	message recv_msg;
	pop(recv_buffer, &recv_msg);
	pthread_mutex_unlock(&buff_lock);


	*src_addr = recv_msg.src_addr;
	*addrlen = sizeof(struct sockaddr);
	strncpy(buf, recv_msg.msg, len);
	
	return sizeof(buf);

}
