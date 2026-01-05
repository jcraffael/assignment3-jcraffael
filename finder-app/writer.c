#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	openlog(NULL, 0, LOG_USER);
	if(argc < 3){
	       syslog(LOG_ERR, "Invalid number of arguments %d. A file path and a string are needed", argc);
		exit(1);
	}
	char *fileName = argv[1];
	char *str = argv[2];
	FILE* fptr=fopen(fileName, "w");
	if(fptr == NULL){
		syslog(LOG_ERR, "The path is not a valid file");
		exit(1);
	}
	if(!fputs(str, fptr))
	{
		syslog(LOG_ERR, "Write to file failed");
	}
	syslog(LOG_DEBUG, "Write %s to file %s", str, fileName);
	fclose(fptr);
	closelog();
	return 0;
}

