#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <vector>
#include <pthread.h>
#include <sys/types.h>
#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <string>
#include <dirent.h>
// #include <pthread_np.h>

using std::vector;
using std::cout;
using std::endl;
using std::string;
// using std::pair;
#define PORT 5000
#define N_CLIENTS 10000
#define BUFF_SIZE 8192

vector<pthread_t> threads;
vector<int> client_fd;
pthread_mutex_t fd_lock;

struct pair {
    int first;
    int second;
};

int get_client_id()
{
    int j = 0;
	int child_state; 
    pthread_mutex_lock(&fd_lock);
    bool available = false;
	do
	{
		if(client_fd[j] == -1)
        {
            available = true;
            client_fd[j] = 0; // mark as unavailable, assign value later
            break;
        }
        j = (j+1)%N_CLIENTS;
	}
	while(j != 0);
    
    pthread_mutex_unlock(&fd_lock);
	
    if(available)
        return j;
    return -1;
}

void sendErrorMessage(int sockfd, int error)
{
    cout << "sending error" << error << endl;
    string reply = "\r\nConnection: Keep-Alive\r\nContent-Type: text/html\r\nServer: Server\r\n\r\n";
    string html;
    string start;
	switch(error)
	{
		case 400: 
            html = "<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n<BODY><H1>400 Bad Rqeuest</H1>\n</BODY></HTML>"; 
            start = "HTTP/1.1 400 Bad Request\r\nContent-Length: ";
            reply = start + std::to_string(html.size()) + reply + html;
            send(sockfd, reply.c_str(), strlen(reply.c_str()), 0);
            break;

		case 403: 
            html = "<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\n<BODY><H1>403 Forbidden</H1><br>Permission Denied\n</BODY></HTML>";
            start = "HTTP/1.1 403 Forbidden\r\nContent-Length: ";
            reply = start + std::to_string(html.size()) + reply + html;
            send(sockfd, reply.c_str(), strlen(reply.c_str()), 0);
            break;

		case 404: 
            html = "<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n<BODY><H1>404 Not Found</H1>\n</BODY></HTML>";
            start = "HTTP/1.1 404 Not Found\r\nContent-Length: ";
            reply = start + std::to_string(html.size()) + reply + html;
            send(sockfd, reply.c_str(), strlen(reply.c_str()), 0);
            break;

        case 500: 
            html = "<HTML><HEAD><TITLE>500 Internal Service Error</TITLE></HEAD>\n<BODY><H1>500 Internal Service Error</H1>\n</BODY></HTML>";
            start = "HTTP/1.1 500 Internal Service Error\r\nContent-Length: ";
            reply = start + std::to_string(html.size()) + reply + html;
            send(sockfd, reply.c_str(), strlen(reply.c_str()), 0);
            break;

		case 501: 
            html = "<HTML><HEAD><TITLE>501 Not Implemented</TITLE></HEAD>\n<BODY><H1>501 Not Implemented</H1>\n</BODY></HTML>";
            start = "HTTP/1.1 501 Not Implemented\r\nContent-Length: ";
            reply = start + std::to_string(html.size()) + reply + html;
            send(sockfd, reply.c_str(), strlen(reply.c_str()), 0);
            break;

        case 505:
            html = "<HTML><HEAD><TITLE>505 HTML Version Not Supported</TITLE></HEAD>\n<BODY><H1>505 HTTP Version Not Supported</H1>\n</BODY></HTML>";
            start = "HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Length: ";
            reply = start + std::to_string(html.size()) + reply + html;
            send(sockfd, reply.c_str(), strlen(reply.c_str()), 0);
            break;
		default:  ;	  
	}

}

string MIME_type(string path)
{
	size_t found = path.find_last_of(".");
	
	string extension = path.substr(found + 1); 

	if(extension.compare(0, 3, "htm") == 0)
		return "text/html";
    if(extension.compare(0, 3, "gif") == 0)
		return "image/gif";
    if(extension.compare(0, 3, "pdf") == 0)
		return "application/pdf";
    if(extension.compare(0, 2, "jp") == 0)
		return "image/jpeg";
    if(extension.compare(0, 3, "txt") == 0)
		return "text/plain";
    return "application/octet-stream";

} 

int requestType(char *data)
{	
	int type = -1;
	if(strncmp(data, "GET\0",4) == 0)
		type = 1;
	else if(strncmp(data, "POST\0", 5) == 0)
		type = 2;
	else if(strncmp(data, "HEAD\0", 5) == 0)
		type = 3;
    else if(strncmp(data, "DELETE\0", 7) == 0)
        type = 4;
    else if(strncmp(data, "PUT\0", 4) == 0)
        type = 5;
	return type;
}

void sendHeader(int sockfd, string header, string type, int _size)
{
    string length = std::to_string(_size);
    header += "\r\nContent-Type: " + type;
    header += "\r\nConnection: Keep-Alive";
    header += "\r\nContent-Length: " + length;
    header += "\r\n\r\n";
    if(send(sockfd, header.c_str(), header.size(), 0) < 0)
        perror("Error in sending header");
}

void open_dir(int sockfd, string path, bool head=false)
{
    DIR* dirp = opendir(path.c_str());
    struct dirent * dp;
    string data = "Files in Dir\n";
    while ((dp = readdir(dirp)) != NULL) 
    {
        if((dp->d_name)[0] != '.')
            data += string(dp->d_name) + "\n";
    }
    cout << data << endl;
    sendHeader(sockfd, "HTTP/1.1 200 OK", "text/plain", data.size());
    
    closedir(dirp);

    if(head)
        return;
    if(send(sockfd, data.c_str(), data.size(), 0) < 0)
    {
        perror("Sending directory structure failed");
        sendErrorMessage(sockfd, 500);
    }
    
}

void send_file(int sockfd, string path, bool head=false)
{
    int fd = open(path.c_str(), O_RDONLY);
    if(fd < 0)
	{
		if(errno == EACCES)
		{
			perror("Permission Denied");
			sendErrorMessage(sockfd, 403);
		}
		else
		{
			perror("File does not exist");
			sendErrorMessage(sockfd, 404);
		}
	}

	string type = MIME_type(path);	
    
    struct stat st;
	fstat(fd, &st);
	int file_size = st.st_size;
    sendHeader(sockfd, "HTTP/1.1 200 OK", type, file_size);
    if(head)
    {
        close(fd);
        return;
    }
    int bytes = sendfile(sockfd, fd, NULL, file_size);	
    
    if(bytes <= 0)
    {
        perror(("Couldn't send file: " + path).c_str());
        sendErrorMessage(sockfd, 500);
        return;
    }

	printf("Total bytes Sent : %d\n", bytes);
    close(fd);

}

void GET(int sockfd, string msg, bool head=false)
{
    string dir = "web";
    string path;

    if(head)
        cout << "HEAD " << msg << endl;
    else
        cout << "GET " << msg << endl;

    if(msg.size() == 0)
    {
        sendErrorMessage(sockfd, 400);
        return;
    }
    if(msg.size()==1)
    {
        if(msg != "/")
        {
            sendErrorMessage(sockfd, 400);
            return;
        }
        path = "web/index.html";
    }
    else
    {
        path = msg;
    }
    if(path[0] == '/')
        path = path.substr(1);
    auto found = path.find("?");
    string params;
    if(found != string::npos)
    {
        params = path.substr(found);
        path = path.substr(0, found);
    }
    // cout << path << endl;
    struct stat s;
    if(stat(path.c_str(), &s) == 0)
    {
        if( s.st_mode & S_IFDIR)
        {
            // cout << "IFDIR" << endl;
            open_dir(sockfd, path, head);
        }
        else if( s.st_mode & S_IFREG)
        {
            cout << "Sending file " << path << endl;
            send_file(sockfd, path, head);
        }
        else
        {
            cout << "File not found" << endl;
            sendErrorMessage(sockfd, 400);
        }
    }
    else
    {
        cout << "File not found" << endl;
        sendErrorMessage(sockfd, 400);
    }
}

void HEAD(int sockfd, string msg)
{
    GET(sockfd, msg, true);
}

void* server_thread(void *args)
{
    pair *temp = (pair *)args;

    int sockfd = temp->first;
    int client_num = temp->second;

    cout << "Client serviced using threadid: " << pthread_self() << endl;
    // struct timeval tv;
    // tv.tv_sec = 15;
    // tv.tv_usec = 0;

    // if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0)
    //     perror("setsockopt for serving client");

    char buffer[BUFF_SIZE];
    bzero(buffer, BUFF_SIZE);

    int bytes = read(sockfd, buffer, BUFF_SIZE);
    cout << buffer << endl;
    while(bytes > 0)
    {
        char *msg[3];
        if(strlen(buffer) > 0)
        {
            msg[0] = strtok(buffer, " \t\n");					// stores Request Method

			int type = requestType(msg[0]);
			if(type == 1)
			{	
				msg[1] = strtok(NULL, " \t\n");
				msg[2] = strtok(NULL, " \t\n");
                GET(sockfd, string(msg[1]));
			}
			else if(type == 2)
			{
				cout << "POST: Not implemented" << endl;
                cout << buffer << endl;
				sendErrorMessage(sockfd, 501);
			}
			else if(type == 3)
			{
                msg[1] = strtok(NULL, " \t\n");
				msg[2] = strtok(NULL, " \t\n");
                HEAD(sockfd, string(msg[1]));
			}
            else    // Unknown Method Request
			{
				printf("Unknown Method %s: Not implemented\n", msg[0]);
				sendErrorMessage(sockfd, 501);
			}
        }
        bzero(buffer, BUFF_SIZE);
		bytes = read(sockfd, buffer, sizeof(buffer));
    }
    if(bytes == 0)
        cout << "Client timed out" << endl;
    else
    {
        if(errno == EAGAIN)
        {
            cout << "Ending connection with client\n";
        }
        else
            perror("Error in reading from client");
    }
    
    pthread_mutex_lock(&fd_lock);
    client_fd[client_num] = -1;
    pthread_mutex_unlock(&fd_lock);
    pthread_exit(NULL);
    cout << "closing socket " << sockfd << endl;
    close(sockfd);
}

int main(int argc, char const *argv[])
{
    int server_fd;
    long valread;
    struct sockaddr_in serv_addr, client_addr;
    auto addrlen = sizeof(serv_addr);
    auto client_len = sizeof(client_addr);
    threads = vector<pthread_t>(N_CLIENTS);
    client_fd = vector<int>(N_CLIENTS, -1);
    
    if (pthread_mutex_init(&fd_lock, NULL) != 0) 
    { 
        printf("mutex init has failed\n"); 
        exit(EXIT_FAILURE); 
    } 
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("In socket");
        exit(EXIT_FAILURE);
    }
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    
    memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));
    
    if (bind(server_fd, (struct sockaddr *)&serv_addr, addrlen)<0)
    {
        perror("In bind");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0)
    {
        perror("In listen");
        exit(EXIT_FAILURE);
    }
    while(1)
    {
        int new_sockfd;
        bzero((char*)&client_addr, sizeof(client_addr));
		client_len = sizeof(client_addr);
        
        if((new_sockfd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t*)&client_len)) < 0)
        {
            perror("In accept");
            exit(EXIT_FAILURE);
        }
        struct sockaddr_in* client_pt = (struct sockaddr_in*)&client_addr;
		struct in_addr ip_addr = client_pt->sin_addr;
		char str[INET_ADDRSTRLEN];	
		inet_ntop(AF_INET, &ip_addr, str, INET_ADDRSTRLEN);	
        int client_num = get_client_id();

        if(client_num == -1)
        {
            printf("Server full!\n");
            close(new_sockfd);
            continue;
        }

		printf("Client connected at %s:%d\n", str, ntohs(client_addr.sin_port));


        pthread_mutex_lock(&fd_lock);
        client_fd[client_num] = new_sockfd;
        pthread_mutex_unlock(&fd_lock);

        pair thread_args;
        thread_args.first = new_sockfd;
        thread_args.second = client_num;

        pthread_create(&threads[client_num], NULL, server_thread, &thread_args);
    }
    pthread_mutex_destroy(&fd_lock); 
    return 0;
}