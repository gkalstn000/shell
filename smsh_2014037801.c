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
void comls();
void history_cmd(char* history[]);
void comcd(int argc ,char* argv[]);

int cnt=0;
int ISREDIR,ISBACK=0;
char *history[512];

void child_handler(int signo){
	pid_t pid;
	int stat;
	while((pid=waitpid(-1,&stat,WNOHANG))>0){
		printf("(%d)child terminated\n",pid);
	}
}

int cmd_check(int argc, char *argv[]){
	pid_t pid;
	if(argc==0){
		return 0;
	}
	if(!strcmp(argv[0],"cd")){
		comcd(argc,argv);
		return 1;
	}
	else if(!strcmp(argv[0],"exit")){
		exit(1);
		return 1;
	}
	else if(!strcmp(argv[0],"clear")){
		system("clear");
		return 1;
	}
	else if(!strcmp(argv[0],"ls"))
	{
		if (argc > 1 && !strcmp(argv[1], "-la")) system("ls -la");
		else if (argc > 1 && !strcmp(argv[1], "/")) system("ls /");
		else comls();

		return 1;
	}
	else if(!strcmp(argv[0],"history")){
		printf("histroy\n");
		history_cmd(history);
		return 1;
	}
	else if(!strcmp(argv[0],"ps")){
		if(argc>1&&!strcmp(argv[1],"-ef")){
			system("ps -ef");
			return 1;
		}
		system("ps");
		return 1;
	}
	else if(!strcmp(argv[0],"bg")){
		system("bg");
		return 1;
	}
	else if(!strcmp(argv[0],"jobs")){
		system("jobs");
		return 1;
	}
	else if(!strcmp(argv[0],"./")){
		pid_t pid;
		if(ISBACK==1){
			printf("BACK\n");
			if(pid=(fork())==0){//Child
				sigset_t set;
				setpgid(0,0);
				sigfillset(&set);
				sigprocmask(SIG_UNBLOCK,&set,NULL);
				system("./");
			}
			else{ //Parent{
				waitpid(pid,NULL,0);
				tcsetpgrp(STDIN_FILENO,getpid());
				fflush(stdout);
			}
				
		}
		else if(ISBACK==0)
			system("./");

		return 1;
	}
	else{	
		pid_t pid;
		if((pid=fork())==0){
			execvp(argv[0],argv);
			exit(1);
		}
		else{
			waitpid(pid,NULL,0);
			return 1;
		}
	}	
	return 0;
}
void _isbackend(char* buffer){
	int pos=strlen(buffer)-1;
	while(pos!=0){
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
					perror("Error : \n");
				dup(fd);
				buffer[pos-1]='\0';
			}
			else if(buffer[pos+1]=='|'){
				ISREDIR=1;
				argvp=strtok(&buffer[pos+2]," ");
				close(STDOUT_FILENO);
				if((fd=open(argvp,O_WRONLY|O_CREAT|O_TRUNC,0755)<0))
					perror("Error : \n");
				dup(fd);
				buffer[pos]='\0';
			}
			else{
				ISREDIR=1;
				argvp=strtok(&buffer[pos+1]," ");
				close(STDOUT_FILENO);
				if((fd=open(argvp,O_WRONLY|O_CREAT|O_TRUNC|O_EXCL,0755)<0))
					perror("Error : \n");
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
		_isbackend(buffer);
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

	while(1){
		getcwd(cwd,sizeof(cwd));
		pid=getpid();

		printf("(%d)myshell:%s/$",pid,cwd);
		fgets(buffer, BSIZE, stdin);
		buffer[strlen(buffer)-1]='\0';
		int index=0;
		char *cpy=(char *)malloc(sizeof(char)*(strlen(buffer)+1));
		strcpy(cpy,buffer);
		history[cnt]=cpy;
		char *semi_buffer[10];
		int semi=makeargv(buffer,semi_buffer,";");

		while(index < semi){
			execute_cmd(semi_buffer[index]);
			index++;
		}
		cnt++;
	}
	
	return 0;
}

void history_cmd(char *history[]){
	int i=0;
	while(history[i]!=NULL){
		printf("histroy[%d] : %s\n",i,history[i]);
		i++;
	}
}
void comcd(int argc, char* argv[]){
	char cwd[1024];

	if(argc<2){
		printf("you must more write command\n");
	}
	else{
		if(chdir(argv[1])){
			printf("Not Exist Directory\n");
		}
		getcwd(cwd,sizeof(cwd));

	}
	
}
void comls(){
	char cwd[1024];

	DIR* dir=NULL;
	struct dirent* entry;
	struct stat buf;

	getcwd(cwd,sizeof(cwd));
	if((dir=opendir(cwd))==NULL)
		printf("opendir Error\n");
	while((entry=readdir(dir))!=NULL){
		lstat(entry->d_name, &buf);
		printf("%s	\n",entry->d_name);
	}
}


