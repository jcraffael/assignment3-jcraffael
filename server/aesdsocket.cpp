#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/unistd.h>
#include <unistd.h>

#define PORT 9000
#define DefaultSize 8192
const char* fileName = "/var/tmp/aesdsocketdata";

class ServerSocket
{
	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof(client_addr);
	int opt;
	int server_fd, new_socket;
	int total_read_size;
	char ip_addr[17];
	struct sigaction new_action;
	FILE* fptr;
	char buff[DefaultSize];

	ServerSocket()
	{
		memset(&new_action,0,sizeof(struct sigaction));
		/*memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;  // Either IPv4 or IPv6
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE; */
		opt = 1, total_read_size = 0;
		fptr = NULL;
		client_addr.sin_family = AF_INET;
		client_addr.sin_addr.s_addr = INADDR_ANY;
		client_addr.sin_port = htons(PORT);
	}


	inline void errHandleExit(const char* str)
	{
		fprintf(stderr, str);
		exit(-1);
	}

	int recv_total()
	{
		int total_size = 0;
		int count = 0, bufSize = DefaultSize;
		//char *recBuf = (char *)malloc(DefaultSize);
		char *totBuf = (char *)malloc(DefaultSize);
		memset(totBuf, 0, DefaultSize);
		do
		{
			bufSize += count;
			totBuf = (char *)realloc(totBuf, bufSize); 
			if(totBuf == NULL)
				break;
			memset(buff, 0, DefaultSize);
			count = recv(new_socket, buff, DefaultSize, 0);
			if(count < 0)
				break;
			printf("Received buf: %c, received count is %d\n", buff[0], count);
			memcpy(totBuf + bufSize - DefaultSize, buff, sizeof(buff));
			//strcat(totBuf, buff);
			/*fptr = fopen(fileName, "a+");
			if(fptr == NULL)
			{
				perror("File open");
				break;	
			}
			
			if(fwrite(recBuf, 1, count, fptr) < 0)
			{
				fprintf(stderr, "Write error!");
				break;
			}
			fflush(fptr);*/
			total_size += count;
			
		}while(count >= DefaultSize);
		printf("Received size: %d, with last count is %d\n", total_size, count);
		fptr = fopen(fileName, "a+");
		if(fptr == NULL)
		{
			perror("File open");
			exit(-1);	
		}
		
		if(fwrite(totBuf, 1, total_size, fptr) < 0)
		{
			fprintf(stderr, "Write error!");
			
		}
		fflush(fptr);
		fclose(fptr);
		free(totBuf);
		return total_size;
	}

	int send_all()
	{
		fptr = fopen(fileName, "r");
		if(fptr == NULL)
			errHandleExit("File open error!\n");
		printf("File opened again\n");
		size_t n = 0, len = 0;
		char *totBuf = (char *)malloc(DefaultSize) ;
		size_t bufSize = DefaultSize, total_size = 0, sent = 0;
		//memset(recBuf, 0, bufSize);
		memset(totBuf, 0, bufSize);
		/*while((n = fread(recBuf, 1, bufSize, fptr)) > 0){
			total_size += n;
			printf("Read size is %ld\n", n);
			if(n < bufSize)
				break;
			bufSize += n;
			recBuf = (char *)realloc(ptr, bufSize);
			if(recBuf == NULL)
			{
				perror("Realloc");
				recBuf = ptr;
				break;
			}
			memset(recBuf, 0, bufSize);
		}*/
		do{
			bufSize += n;
			totBuf = (char *)realloc(totBuf, bufSize);
			memset(buff, 0, sizeof(buff));
			if(totBuf == NULL)
				break;
			printf("To realloc to %ld size\n", bufSize);
                         /*recBuf = (char *)realloc(recBuf, bufSize);
                         if(recBuf == NULL)
                         {
                                 perror("Realloc");
                                 break; 
                         }*/
			 n = fread(buff, 1, DefaultSize, fptr);
			 printf("Read bytes %ld, read buf is %c\n", n, buff[0]);
			total_size += n;
			memcpy(totBuf + bufSize - DefaultSize, buff, sizeof(buff));
			if(n < DefaultSize)
                                 break;
		}while(n > 0);
		
		printf("The read buffer is %s with size %ld\n", totBuf, total_size);
		sent = send(new_socket, totBuf, total_size, 0);
		printf("Sent buf with size %ld to socket %d\n", total_size, new_socket);
		if(sent < 0)
		{
			perror("Sent");
			return total_size;
		
		}
		//free(recBuf);
		free(totBuf);
		fflush(fptr);
		fclose(fptr);
		return sent;
	}

	static void signal_handler (int signal_number)
	{
		if ( signal_number == SIGINT || signal_number == SIGTERM) {
			printf("Caught sigint.\n");
			close(getInstance().new_socket);
			close(getInstance().server_fd);
			//fclose(getInstance().fptr);
			closelog();
			remove(fileName);
			syslog(LOG_DEBUG, "Caught signal, exiting...");
			exit(0);
		}
	}

	public:
	static ServerSocket& getInstance()
	{
		static ServerSocket ss;
		return ss;
	}

	void initConfig()
	{
		/*if( sigaction(SIGTERM, &new_action, NULL) != 0 )
		errHandleExit("Reg SIGTERM error.\n");
		if( sigaction(SIGINT, &new_action, NULL) )
		errHandleExit("Reg SIGINT error.\n");
		new_action.sa_handler=signal_handler;*/
		signal(SIGINT, signal_handler);
		signal(SIGTERM, signal_handler);
		openlog(NULL, 0, LOG_USER);
		//if ((getaddrinfo(NULL, PORT, &(this->hints), &(this->res))) != 0)
		// errHandleExit("getaddrinfo error\n");

		if((server_fd = socket(AF_INET, SOCK_STREAM, 0/*res->ai_family, res->ai_socktype, res->ai_protocol*/)) < 0)
			errHandleExit("socket error\n");

		//if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
		//	errHandleExit("setsockopt error\n");

		/*fptr = fopen(fileName, "w");
		if(fptr == NULL)
		errHandleExit("File open error!\n");*/
		system("rm /var/tmp/aesdsocketdata 2>/dev/null");
	}

	void operate()
	{
		if(bind(server_fd, (struct sockaddr*)&client_addr, sizeof(client_addr)/*res->ai_addr, res->ai_addrlen*/) < 0)
		{errHandleExit("bind error\n");
		printf("Bond!\n");}
		//freeaddrinfo(res); // free the linked list

		if (listen(server_fd, 5) < 0)
			errHandleExit("listen error\n");

		for(;;)
		{
			printf("Waiting for new connection...\n");
			if ((new_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen)) < 0)
			{
				fprintf(stderr, "Accept fault!\n");
				continue;
			}

			inet_ntop(AF_INET,(struct sockaddr_in *)&client_addr.sin_addr,ip_addr,16);
			syslog(LOG_DEBUG, "Accepted connection from %s", ip_addr);
			printf("Accepted connection from %s with socket %d\n", ip_addr, new_socket);
			if(!(total_read_size = recv_total()))
			{
				fprintf(stderr, "Recv data error!\n");
				continue;
			}
			printf("Total read size is %d\n", total_read_size);
			send_all();
			//printf("Not sent all.\n");
			/*const char *str = "Hahaha~";
			int len = strlen(str);
			int sent = send(new_socket, str, len, 0);
                        printf("Sent buf: %s with size %d to socket %d\n", str, sent, new_socket);
			*/
			syslog(LOG_DEBUG, "Closed connection from %s", ip_addr);
			//shutdown(new_socket, SHUT_WR);
			printf("Closed connection from %s\n", ip_addr);
		}
	}
};


int main(int argc, char *argv[])
{
    /*if(argc > 1)
	{
	int opt;
	while((opt = getopt(argc, argv, “:if:lrx”)) != -1)
	{
	switch(opt)
	{
	case 'd':
	break;
	default:
	break;
	}
	}
	}*/
	ServerSocket::getInstance().initConfig();
	ServerSocket::getInstance().operate();
    return 0;
}
