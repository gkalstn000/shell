#include<errno.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<unistd.h>
#include<dirent.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>

#define BSIZE 1024

struct sigaction act;
void my_cd(int argc ,char* argv[]);
int my_history(void);


char buffer[BSIZE];
char cwd[512];


int main(){
	pid_t pid;
	sigset_t set;
	sigfillset(&set);
	sigdelset(&set,SIGCHLD);
	sigdelset(&set, SIGINT);
	sigdelset(&set, SIGQUIT);
	sigprocmask(SIG_SETMASK,&set,NULL);


    FILE *history_write;


	while(1){
		getcwd(cwd,sizeof(cwd));
		pid=getpid();

		printf("(%d)myshell:%s/$",pid,cwd);
		fgets(buffer, BSIZE, stdin);
		buffer[strlen(buffer)-1]='\0';
		int index=0;
		char *cpy=(char *)malloc(sizeof(char)*(strlen(buffer)+1));
		strcpy(cpy,buffer);


        history_write = fopen("history.txt", "a");
        strcat(cpy, "\n");
        fputs(cpy, history_write);
        fclose(history_write);


	}

	return 0;
}


int my_history(void)
{
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("history.txt", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    printf("========== history ==========\n");
    while ((read = getline(&line, &len, fp)) != -1) {
        printf("%s", line);
    }
    printf("========== history ==========\n");

    fclose(fp);
    if (line)
        free(line);
}

void my_cd(int argc, char* argv[]){
	char cwd[1024];
	if(argc<2){
		printf("invalid command.\n");
	}
	else{
		if(chdir(argv[1]) == -1){
			printf("cd: no such file or directory: %s\n", argv[1]);
		}
		getcwd(cwd,sizeof(cwd));

	}

}
