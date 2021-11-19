#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include <netdb.h>
//#include <sys/wait.h>
//#include <sys/stat.h>
//#include <math.h>
//#include <dirent.h>
//#include <ctype.h>
//#include <setjmp.h>
//#include <sys/time.h>
//#include <sys/mman.h>
//#include <pthread.h>
//#include <semaphore.h>

#define MAXLINE 1000024

struct {
	char *ext;
	char *filetype;
} extensions [] = {
	{"gif", "image/gif" },
	{"jpg", "image/jpeg"},
	{"jpeg","image/jpeg"},
	{"png", "image/png" },
	{"tar", "image/tar" },
	{"htm", "text/html" },
	{"html","text/html" },
	{"txt", "text/plain" },
	{"exe", "text/plain" },
	{"mp3", "audio/mp3" },
	{0,0} };

void handle_socket(int connfd);
void recv_file(int connfd , char* buffer , int ret);

int main(int argc, char **argv){
	int pid , listenfd , connfd , len;
	struct sockaddr_in servaddr;
	struct sockaddr_in clientaddr;

	signal(SIGCLD , SIG_IGN);

//	if((listenfd = socket(AF_INET , SOCK_STREAM , 0)) < 0)
//		exit(1);
	listenfd = socket(AF_INET , SOCK_STREAM , 0);

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(8080);

	bind(listenfd , (struct sockaddr *) &servaddr , sizeof(servaddr));
	listen(listenfd , 128);

//	if(bind(listenfd , (struct sockaddr *) &servaddr , sizeof(servaddr)) < 0)
//		exit(1);

//	if(listen(listenfd , 128) < 0)
//		exit(1);

	fprintf(stderr , "listen start\n");

	while(1){
		len = sizeof(clientaddr);
		connfd = accept(listenfd , (struct sockaddr *) &clientaddr , &len);
//		if((connfd = accept(listenfd , (struct sockaddr *) &clientaddr , &len)) < 0)
//			exit(1);

		fprintf(stderr , "accepted\n");
		if((pid = fork()) < 0)
			exit(1);
		else{
			if(pid == 0){		// chlid
				close(listenfd);
				handle_socket(connfd);
			}
			else{				//parent
				close(connfd);
			}
		}
	}
}

void recv_file(int connfd , char* buffer , int ret){
	printf("============start============\n");
	fprintf(stderr , "%s\n" , buffer);
	printf("============finish===========\n");
	int index;
	char *ptr , *tmp;
	char name[MAXLINE + 5] , data[MAXLINE + 5] , boundary[MAXLINE + 5];

	ptr = strstr(buffer , "boundary=");
	if(ptr == 0) return;
	ptr += 9;
	index = 0;
	while(1){
		if((*ptr) != '\r' && (*ptr) != '\n'){
			boundary[index] = *ptr;
		}
		else{
			name[index] = '\0';
			break;
		}
		ptr++;
		index++;
	}
	
	ptr = strstr(buffer , "filename=\"");
	if(ptr == 0) return;

	ptr += 10;
	strcpy(name , "recv_files/");
	index = 11;
	while(1){
		if((*ptr) != '\"'){
			name[index] = *ptr;
		}
		else{
			name[index] = '\0';
			break;
		}
		ptr++;
		index++;
	}

	ptr = strstr(ptr , "\n");
	ptr = strstr(++ptr , "\n");
	ptr = strstr(++ptr , "\n");
	ptr++;

	tmp = strstr(ptr , boundary);
	if(tmp == 0) return;
	tmp -= 4;
	(*tmp) = '\0';

	int file_fd = open(name , O_CREAT | O_WRONLY | O_TRUNC | O_SYNC , S_IRWXO | S_IRWXU | S_IRWXG);
	write(file_fd , ptr , strlen(ptr));
	close(file_fd);

	return;
}

void handle_socket(int connfd){
	int ret , file_fd , bufferlen , i , index_shift;
	static char buffer[MAXLINE + 5] , tmp[MAXLINE + 5];
	char* fstr;

	ret = read(connfd , buffer , MAXLINE);

	if(ret <= 0)
		exit(1);

	recv_file(connfd , buffer , ret);

	if(ret < MAXLINE)
		buffer[ret] = '\0';
	else
		buffer[0] = '\0';

	for(i = 0;i < ret;i++)
		if(buffer[i] == '\r' || buffer[i] == '\n')
			buffer[i] = '\0';

	if(!strncmp(buffer , "GET" , 3) || !strncmp(buffer , "get" , 3))
		index_shift = 4;
	else if(!strncmp(buffer , "POST" , 4) || !strncmp(buffer , "post" , 4))
		index_shift = 5;
	else
		exit(1);

	for(i = index_shift;i < MAXLINE;i++){
		if(buffer[i] == ' '){
			buffer[i] = '\0';
			break;
		}
	}
	bufferlen = strlen(buffer);
	fstr = (char *) 0;
	for(i = 0;extensions[i].ext != 0;i++){
		int len = strlen(extensions[i].ext);
		if(bufferlen < len)
			continue;
		if(!strncmp(&buffer[bufferlen - len] , extensions[i].ext , len)){
			fstr = extensions[i].filetype;
			break;
		}
	}

	if(fstr == 0)
		fstr = extensions[i - 1].filetype;

	for(i = index_shift + 1;i < MAXLINE;i++){
		if(buffer[i] != ' ')
			tmp[i - (index_shift + 1)] = buffer[i];
		else
			tmp[i - (index_shift + 1)] = '\0';
	}
	file_fd = open(tmp , O_RDONLY);
	sprintf(buffer , "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n" , fstr);
	write(connfd , buffer , strlen(buffer));

	while((ret = read(file_fd , buffer , MAXLINE)) > 0){
		write(connfd , buffer , ret);
	}

	exit(1);

	return;
}
