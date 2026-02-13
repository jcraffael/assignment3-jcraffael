#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdbool.h>
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

typedef struct ServerSocket
{
struct sockaddr_in client_addr;
socklen_t addrlen;
int opt;
int server_fd, new_socket;
int total_read_size;

struct sigaction new_action;
FILE* fptr;
char buff[DefaultSize];
char *totBuf;
} ServerSocket;
/*ServerSocket()
{
memset(&new_action,0,sizeof(struct sigaction));
opt = 1, total_read_size = 0;
fptr = NULL;
totBuf = NULL;
client_addr.sin_family = AF_INET;
client_addr.sin_addr.s_addr = INADDR_ANY;
client_addr.sin_port = htons(PORT);
}*/


static ServerSocket ss;
void errHandleExit(const char* str)
{
	fprintf(stderr, str);
	exit(-1);
}

int recv_total()
{
	int total_size = 0;
	int count = 0, bufSize = DefaultSize;
	ss.totBuf = (char *)malloc(DefaultSize);
	memset(ss.totBuf, 0, DefaultSize);
	do
	{
		bufSize += count;
		ss.totBuf = (char *)realloc(ss.totBuf, bufSize);
		if(ss.totBuf == NULL)
			break;
		memset(ss.buff, 0, DefaultSize);
		count = recv(ss.new_socket, ss.buff, DefaultSize, 0);
		if(count < 0)
			break;
		printf("Received buf count is %d\n", count);
		memcpy(ss.totBuf + bufSize - DefaultSize, ss.buff, sizeof(ss.buff));
		total_size += count;

	}while(count >= DefaultSize);
	printf("Received size: %d, with last count is %d\n", total_size, count);
	ss.fptr = fopen(fileName, "a+");
	if(ss.fptr == NULL)
	{
		perror("File open");
		exit(-1);
	}

	if(fwrite(ss.totBuf, 1, total_size, ss.fptr) < 0)
	{
		fprintf(stderr, "Write error!");
	}
	fflush(ss.fptr);
	fclose(ss.fptr);
	free(ss.totBuf);
	return total_size;
}

int send_all()
{
	ss.fptr = fopen(fileName, "r");
	if(ss.fptr == NULL)
		errHandleExit("File open error!\n");
	printf("File opened again\n");
	size_t n = 0;
	ss.totBuf = (char *)malloc(DefaultSize) ;
	size_t bufSize = DefaultSize, total_size = 0, sent = 0;
	memset(ss.totBuf, 0, bufSize);
	do{
		bufSize += n;
		ss.totBuf = (char *)realloc(ss.totBuf, bufSize);
		if(ss.totBuf == NULL)
			break;
		printf("To realloc to %ld size\n", bufSize);
		memset(ss.buff, 0, sizeof(ss.buff));
		n = fread(ss.buff, 1, DefaultSize, ss.fptr);
		printf("Read bytes %ld\n", n);
		total_size += n;
		memcpy(ss.totBuf + bufSize - DefaultSize, ss.buff, sizeof(ss.buff));
		if(n < DefaultSize)
			break;
	}while(n > 0);

	printf("The read buffer with size %ld\n", total_size);
	sent = send(ss.new_socket, ss.totBuf, total_size, 0);
	printf("Sent buf with size %ld\n", total_size);
	if(sent < 0)
	{
		perror("Sent");
		return total_size;
	}
	free(ss.totBuf);
	fflush(ss.fptr);
	fclose(ss.fptr);
	return sent;
}

static void signal_handler (int signal_number)
{
if ( signal_number == SIGINT || signal_number == SIGTERM) {
	printf("Caught sigint.\n");
	close(ss.new_socket);
	close(ss.server_fd);
	closelog();
	remove(fileName);
	syslog(LOG_DEBUG, "Caught signal, exiting...");
	exit(0);
	}
}


/*static ServerSocket& getInstance()
{
static ServerSocket ss;
return ss;
}*/

void initConfig()
{
	memset(&(ss.new_action),0,sizeof(struct sigaction));
	ss.opt = 1, ss.total_read_size = 0;
	ss.fptr = NULL;
	ss.addrlen = sizeof(ss.client_addr);
	ss.totBuf = NULL;
	ss.client_addr.sin_family = AF_INET;
	ss.client_addr.sin_addr.s_addr = INADDR_ANY;
	ss.client_addr.sin_port = htons(PORT);
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	openlog(NULL, 0, LOG_USER);

	if((ss.server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		errHandleExit("socket error\n");

	system("rm /var/tmp/aesdsocketdata 2>/dev/null");
	if(bind(ss.server_fd, (struct sockaddr*)&(ss.client_addr), sizeof(ss.client_addr)) < 0)
		errHandleExit("bind error\n");
	printf("Bond!\n");

	if (listen(ss.server_fd, 5) < 0)
		errHandleExit("listen error\n");

}

void operate()
{
	char ip_addr[17];
	for(;;)
	{
		printf("Waiting for new connection...\n");
		if ((ss.new_socket = accept(ss.server_fd, (struct sockaddr*)&(ss.client_addr), &(ss.addrlen))) < 0)
		{
			fprintf(stderr, "Accept fault!\n");
			continue;
		}

		inet_ntop(AF_INET,(struct sockaddr_in *)&(ss.client_addr).sin_addr,ip_addr,16);
		syslog(LOG_DEBUG, "Accepted connection from %s", ip_addr);
		printf("Accepted connection from %s\n", ip_addr);
		if(!(ss.total_read_size = recv_total()))
		{
			fprintf(stderr, "Recv data error!\n");
			continue;
		}
		printf("Total read size is %d\n", ss.total_read_size);
		send_all();
		syslog(LOG_DEBUG, "Closed connection from %s", ip_addr);
		printf("Closed connection from %s\n", ip_addr);
	}
}



int main(int argc, char *argv[])
{
	initConfig();
	if(argc > 1)
	{
		if(!strcmp(argv[1], "-d"))
		{
			printf("Into Daemon mode!\n");
			pid_t pid = fork();
			if ( pid < 0 ) {
			perror("Fork error"); return false;
			}else
			{
			if(pid == 0)
			{
			  operate();
			}
			else
			  return 0;
		}
	}
    }
    operate();
    return 0;
}
