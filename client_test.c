
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/select.h>

#include <string.h>


void console();
void fork_console(int fd[2]);

int main(int argc, char *argv[])
{
	int fd[2];

	fork_console(fd);

	return 0;
}
void fork_console(int fd[2])
{
	pid_t pid;
	if(pipe(fd) == -1){
		exit(1);
	}
	pid = fork();
	if (pid == 0){
		close(fd[0]);
		dup2(fd[1], 1);
		close(fd[1]);
		console();
	
	}else if(pid != -1){
		char word[5];
		fd_set fdset;
		struct timeval timeout;

		close(fd[1]);
		dup2(fd[0],0);
		close(fd[0]);
		fprintf(stderr,"Ready to read child");
		while(1){
			FD_ZERO(&fdset);
    		FD_SET(0, &fdset);
			timeout.tv_usec = 0;
			timeout.tv_sec = 2;
    		if(select(8, &fdset, NULL, NULL, &timeout) == 0){
				fprintf(stderr, "No s'ha rebut res de consola\n");
			}else{
				fgets(word, 5, stdin);
				if(strcmp(word,"quit")){
					
				}	
			}
		}
		
	}else{
		perror("Error amb el fork del procés de la consola");
		exit(1);
	}
}
void console()
{
	int quit = 0;
	char word[5];
	fprintf( stderr, "Console activated\n");
	while(quit == 0){
		fgets(word, sizeof(word), stdin);
		if(strcmp(word,"quit") == 0){
			quit = 1;
		}
	}
	fprintf( stderr, "Console deactivated\n");
	write(1, word, strlen(word)+1);
	fprintf( stderr, "Exitting child process\n");
	exit(1);
}