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

int ISREDIR,ISBACK=0;

void child_handler(int signo){
	pid_t pid;
	int stat;
	while((pid=waitpid(-1,&stat,WNOHANG))>0){
		printf("(%d)child terminated\n",pid);
	}
}
void cmd_excution(int argc, char* argv[])
{
	if(argc==0){
		printf("\n");
	}

	if(!strcmp(argv[0],"cd")){
		my_cd(argc,argv);
	}
	else if(!strcmp(argv[0],"exit")){
		exit(1);
	}
	else if(!strcmp(argv[0],"clear")){
		system("clear");
	}
	else if(!strcmp(argv[0],"history")){
        my_history();
	}
	else if(!strcmp(argv[0],"./"))
	{
		system("./");
	}
	else if(!strcmp(argv[0],"ps")){
		if(argc>1&&!strcmp(argv[1],"-ef")){
			system("ps -ef");
		}
		system("ps");
	}
	else if(!strcmp(argv[0],"bg")){
		system("bg");
	}
	else if(!strcmp(argv[0],"jobs")){
		system("jobs");
	}
    else if(!strcmp(argv[0],"exit")){
		exit(1);
	}
	else{
		pid_t pid;
		if((pid=fork())==0){
			execvp(argv[0],argv);
			exit(1);
		}
		else{
			waitpid(pid,NULL,0);
		}
	}

}

int cmd_check(int argc, char *argv[]){
	pid_t pid;
	if(ISBACK == 1){
		if((pid=fork())==0){//Child
			cmd_excution(argc, argv);
			exit(1);
		}
		else // Parent
		{
			// waitpid(pid,NULL,0);
			tcsetpgrp(STDIN_FILENO,getpid());
			fflush(stdout);
			ISBACK = 0;
			return 1;
		}
	}
	else
	{
		cmd_excution(argc, argv);
		return 1;
	}
}
void DeleteChar(char* str, char ch1, char ch2, char ch3)
{
	if (str[0] != '\0');
	{
		for (; *str != '\0'; str++)
	    {
	        if (*str == ch1)
	        {
	            strcpy(str, str + 1);
	            str--;
	        }
	        else if (*str == ch2)
	    	{
			    strcpy(str, str + 1);
			    str--;
	        }
	        else if (*str == ch3)
	    	{
			    strcpy(str, str + 1);
			    str--;
	        }
	    }
	}
}

void _isbackend(char* buffer){

	int pos=strlen(buffer)-1;
	while(pos > 0){
		if(buffer[pos]=='&'){
			printf("Delete\n");
			ISBACK=1;
			buffer[pos]='\0';
		}
		pos--;
	}

}
void _redirection(char* buffer){
	int fd;
	char *argvp;

	int pos=strlen(buffer);
	char *cpy=(char *)malloc(sizeof(char)*(strlen(buffer)+1));
	strcpy(cpy,buffer);
	while(pos!=0){
		if(buffer[pos]=='<'){
			ISREDIR=1;
			argvp=strtok(&cpy[pos+1]," ");
			close(STDIN_FILENO);
			if((fd=open(argvp,O_RDONLY|O_CREAT))<0)
				printf("Error : Not open\n");
			dup(fd);
			buffer[pos]=' ';
		}
		else if(buffer[pos]=='>'){
			if(buffer[pos-1]=='>'){
				ISREDIR=1;
				argvp=strtok(&buffer[pos+1]," ");
				close(STDOUT_FILENO);
				if((fd=open(argvp,O_WRONLY|O_CREAT|O_APPEND,0755)<0))
					perror("Error : 1\n");
				dup(fd);
				buffer[pos-1]='\0';
			}
			else if(buffer[pos+1]=='|'){
				ISREDIR=1;
				argvp=strtok(&buffer[pos+2]," ");
				close(STDOUT_FILENO);
				if((fd=open(argvp,O_WRONLY|O_CREAT|O_TRUNC,0755)<0))
					perror("Error : 2\n");
				dup(fd);
				buffer[pos]='\0';
			}
			else{
				ISREDIR=1;
				argvp=strtok(&buffer[pos+1]," ");
				close(STDOUT_FILENO);
				if((fd=open(argvp,O_WRONLY|O_CREAT|O_TRUNC|O_EXCL,0755)<0))
					perror("Error : 3\n");
				dup(fd);
				buffer[pos]='\0';
			}
		}
		pos--;
	}
}
int makeargv(char* buffer,char* argv[],char* tok){
	int argc=0;
	//char* cpy=(char *)malloc(sizeof(char)*(strlen(buffer)+1));
	char *ptr = strtok(buffer,tok);
	while(ptr !=NULL){
		argv[argc] = (ptr);
		argc++;
		ptr = strtok(NULL,tok);
	}
	return argc;
}
void execute_cmdline(char* buffer, char* argv[]){
	pid_t pid;
	if((pid=fork())==0){
		if(ISREDIR==0){
			exit(1);
		}
		else if(ISREDIR==1){
			int argc=makeargv(buffer,argv," ");
			if(!cmd_check(argc,argv)){
				printf("Error\n");
			}
			exit(1);
		}
	}
	else if(pid>1){
		if(ISREDIR==0){
			waitpid(pid,NULL,0);
			int argc=makeargv(buffer,argv," ");
			if(!cmd_check(argc,argv)){
				printf("Error\n");
			}
		}

	}
}

void execute_cmd(char* buffer){
	int ISPIPE=0;
	int status;
	int count,i=0;
	int pfd[2];
	char *argv[10]={NULL,};
	char *pip_argv[10]={NULL,};

	if(fork()==0){
		_redirection(buffer);
		if( (count = makeargv(buffer, pip_argv, "|") ) > 1){
			ISPIPE=1;
			pipe(pfd);
			buffer=pip_argv[1];
			if(fork()==0){
				close(STDOUT_FILENO);
				dup(pfd[1]);
				close(pfd[1]);
				close(pfd[0]);
				execute_cmdline(pip_argv[0],argv);
				exit(1);
			}
			close(STDIN_FILENO);
			dup(pfd[0]);
			close(pfd[0]);
			close(pfd[1]);
		}
		if(ISREDIR==1 && ISPIPE==0){
			execute_cmdline(buffer,argv);
			exit(1);
		}
		else if(ISREDIR==0 && ISPIPE==1){
			execute_cmdline(buffer,argv);
			exit(1);
		}
		else if(ISREDIR==1 && ISPIPE==1){
			execute_cmdline(buffer,argv);
			exit(1);
		}
		exit(1);
	}
	wait(&status);
	if(ISPIPE==0 && ISREDIR==0){
		execute_cmdline(buffer,argv);
	}
}

char buffer[BSIZE];
char cwd[512];

int main(){
	pid_t pid;
	sigset_t set;
	sigfillset(&set);
	sigdelset(&set,SIGCHLD);
	sigprocmask(SIG_SETMASK,&set,NULL);

	sigemptyset(&act.sa_mask);
	act.sa_handler = child_handler;
	sigaction(SIGCHLD,&act,0);

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

		char *semi_buffer[10];

		_isbackend(buffer);
		DeleteChar(buffer, '(', ')', '&');
		int semi=makeargv(buffer,semi_buffer,";");

		while(index < semi){
			execute_cmd(semi_buffer[index]);
			index++;
		}
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
