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
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "queue.h"
#define PORT 9000
#define DefaultSize 8192
#define TimeSize 50
#define MaxClient 20
const char* fileName = "/var/tmp/aesdsocketdata";
typedef struct thread_data
{
	int socketId;
	FILE* fptr;
	pthread_mutex_t* mutexPtr;
	bool *done;
}thread_data;

struct node
{
    pthread_t *pt;
	thread_data* td;
	bool complete;
    TAILQ_ENTRY(node) nodes;
};

typedef struct ServerSocket
{
	struct sockaddr_in client_addr;
	socklen_t addrlen;
	int opt;
	int server_fd;// new_socket;
	struct sigaction new_action;
	FILE* fptr;

	pthread_mutex_t mutex;
	TAILQ_HEAD(head_s, node) head;

} ServerSocket;


static ServerSocket ss;
time_t t ;
struct tm *tmp ;

pthread_t threads[MaxClient];

void errHandleExit(const char* str)
{
	fprintf(stderr, str);
	exit(-1);
}

void destroyThread(bool final)
{
	struct node *n = NULL; //TAILQ_FIRST(&(ss.head));
	if(final)
	{
		printf("Final clean up\n");
		while (!TAILQ_EMPTY(&(ss.head)))
		{
			n = TAILQ_FIRST(&(ss.head));
			TAILQ_REMOVE(&(ss.head), n, nodes);
			int s;
			if(( s = pthread_cancel(*(n->pt))) < 0)
				perror("Thread cancel");
			pthread_join(*(n->pt), NULL);
			free(n->td);
			free(n);
			n = NULL;
		}
	}else{
		struct node * next = NULL;
		if(!TAILQ_EMPTY(&(ss.head)))
		{
			TAILQ_FOREACH_SAFE(n, &(ss.head), nodes, next)
			{
				if(n->complete)
				{
					TAILQ_REMOVE(&(ss.head), n, nodes);
					pthread_join(*(n->pt), NULL);
					free(n->td);
					free(n);
					n = NULL;
				}
			}
			
		}
	}
}

void cleanAll()
{
    close(ss.server_fd);
    closelog();
    remove(fileName);
    pthread_mutex_destroy(&(ss.mutex));
    destroyThread(true);
    printf("Cleaned all\n");
}   
 
static void signal_handler (int signal_number)
{
     if ( signal_number == SIGINT || signal_number == SIGTERM) {
     printf("Caught sigint.\n");
     cleanAll();
     syslog(LOG_DEBUG, "Caught signal, exiting...");
     exit(0);
     }
 }   

void initConfig()
{
	memset(&(ss.new_action),0,sizeof(struct sigaction));
	memset(threads, 0, sizeof(threads));
	ss.opt = 1;
	ss.fptr = NULL;
	ss.addrlen = sizeof(ss.client_addr);

	ss.client_addr.sin_family = AF_INET;
	ss.client_addr.sin_addr.s_addr = INADDR_ANY;
	ss.client_addr.sin_port = htons(PORT);
	if(pthread_mutex_init(&(ss.mutex), NULL)!=0)
		exit(-1);
	TAILQ_INIT(&(ss.head));
	
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	openlog(NULL, 0, LOG_USER);

	if((ss.server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		errHandleExit("socket error\n");

	system("rm /var/tmp/aesdsocketdata 2>/dev/null");
	if(bind(ss.server_fd, (struct sockaddr*)&(ss.client_addr), sizeof(ss.client_addr)) < 0)
		errHandleExit("bind error\n");
	printf("Bond!\n");

	if (listen(ss.server_fd, MaxClient) < 0)
		errHandleExit("listen error\n");
}

int recv_total(int socketId, FILE* fptr, pthread_mutex_t *mutexPtr)
{
	int total_size = 0;
	int count = 0, bufSize = DefaultSize;
	char *totBuf = (char *)malloc(DefaultSize);
	char buff[DefaultSize];
	memset(totBuf, 0, DefaultSize);
	do
	{
		bufSize += count;
		totBuf = (char *)realloc(totBuf, bufSize);
		if(totBuf == NULL)
			break;
		memset(buff, 0, DefaultSize);
		count = recv(socketId, buff, DefaultSize, 0);
		if(count < 0)
			break;

		memcpy(totBuf + bufSize - DefaultSize, buff, sizeof(buff));
		total_size += count;

	}while(count >= DefaultSize);

	pthread_mutex_lock(mutexPtr);
	fptr = fopen(fileName, "a+");
	if(fptr == NULL)
	{
		perror("File open");
		return -1;
	}

	if(fwrite(totBuf, 1, total_size, fptr) < 0)
	{
		fprintf(stderr, "Write error!");
	}
	fflush(fptr);
	fclose(fptr);
	pthread_mutex_unlock(mutexPtr);
	free(totBuf);
	return total_size;
}

int send_all(int socketId, FILE* fptr, pthread_mutex_t *mutexPtr)
{
	size_t n = 0;
	char *totBuf = (char *)malloc(DefaultSize);
	char buff[DefaultSize];
	size_t bufSize = DefaultSize, total_size = 0, sent = 0;
	memset(totBuf, 0, bufSize);

	pthread_mutex_lock(mutexPtr);
	fptr = fopen(fileName, "r");
	if(fptr == NULL)
	{
		perror("File open error!\n");
		return -1;
	}

	do{
		bufSize += n;
		totBuf = (char *)realloc(totBuf, bufSize);
		if(totBuf == NULL)
			break;

		memset(buff, 0, sizeof(buff));
		n = fread(buff, 1, DefaultSize, fptr);
		//printf("Read bytes %ld\n", n);
		total_size += n;
		memcpy(totBuf + bufSize - DefaultSize, buff, sizeof(buff));
		if(n < DefaultSize)
			break;
	}while(n > 0);
	fflush(fptr);
	fclose(fptr);
	pthread_mutex_unlock(mutexPtr);

	sent = send(socketId, totBuf, total_size, 0);
	//printf("Sent buf with size %ld\n", total_size);
	if(sent < 0)
	{
		perror("Sent");
		return -1;
	}

	free(totBuf);
	return sent;
}

void* threadfuncData(void* thread_param)
{
	int total_read_size;
	thread_data *thread_func_args = (thread_data *) thread_param;

	if(!(total_read_size = recv_total(thread_func_args->socketId, thread_func_args->fptr, thread_func_args->mutexPtr)))
	{
		fprintf(stderr, "Recv data error!\n");
		goto RETURN;
	}
	printf("Total read size is %d\n", total_read_size);
	send_all(thread_func_args->socketId, thread_func_args->fptr, thread_func_args->mutexPtr);

	//syslog(LOG_DEBUG, "Closed connection from %s", thread_func_args->ip_addr);
	//printf("Closed connection from %s\n", thread_func_args->ip_addr);
	RETURN:
	*(thread_func_args->done) = true;
	printf("Mark thread done\n");
}

void* threadfuncTime(void* thread_param)
{
	char MY_TIME[TimeSize];
	char writeString[TimeSize + 20];
	thread_data *thread_func_args = (thread_data *) thread_param;

	for(;;)
	{
		sleep(10);
		time( &t );
		tmp = localtime( &t );
		memset(MY_TIME, 0, TimeSize);
		strftime(MY_TIME, sizeof(MY_TIME), "%Y-%m-%d - %H:%M:%S", tmp);
		memset(writeString, 0, TimeSize + 20);
		snprintf(writeString, sizeof(writeString), "timestamp:%s\n", MY_TIME);
		printf("%s", writeString);
		
		pthread_mutex_lock(thread_func_args->mutexPtr);
		thread_func_args->fptr = fopen(fileName, "a+");
		if(fwrite(writeString, 1, strlen(writeString), thread_func_args->fptr) < 0)
		{
			fprintf(stderr, "Write error!");
		}
		fflush(thread_func_args->fptr);
		fclose(thread_func_args->fptr);
		pthread_mutex_unlock(thread_func_args->mutexPtr);		
	}
}


void createThread(int sock, int num)
{
	thread_data* tData = malloc(sizeof(thread_data));
	if(tData == NULL)
	{
	  perror("Dynamic allocation failed.");
	  return;
	}
	tData->socketId = sock;
	tData->fptr = ss.fptr;
	tData->mutexPtr = &(ss.mutex);

	struct node *e = malloc(sizeof(struct node));
	if (e == NULL)
	{
		fprintf(stderr, "malloc failed");
		return;
	}

	e->complete = false;
	e->td = tData;
	tData->done = &(e->complete);
	int rc = pthread_create(&(threads[num]), NULL, num == 0 ? threadfuncTime : threadfuncData, (void*)tData);
	if(rc != 0)
	{
		perror("Pthread creation failed.");
  		return;
	}
	e->pt = &(threads[num]);

	TAILQ_INSERT_TAIL(&(ss.head), e, nodes);
	e = NULL;
	printf("Created thread %d\n", num);
}

void operate()
{
	char ip_addr[17];
	createThread(-1, 0);

	int num = 1;
	while(num < MaxClient)
	{
		printf("Waiting for new connection...\n");
		int new_socket = -1;
		//destroyThread(false);
		
		if (( new_socket = accept(ss.server_fd, (struct sockaddr*)&(ss.client_addr), &(ss.addrlen))) < 0)
		{
			fprintf(stderr, "Accept fault!\n");
			continue;
		}
		inet_ntop(AF_INET,(struct sockaddr_in *)&(ss.client_addr).sin_addr,ip_addr,16);
		syslog(LOG_DEBUG, "Accepted connection from %s", ip_addr);
		printf("Accepted connection from %s\n", ip_addr);

		createThread(new_socket, num);
		num++;
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

	cleanAll();
    return 0;
}
