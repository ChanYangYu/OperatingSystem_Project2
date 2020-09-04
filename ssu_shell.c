#include  <stdio.h>
#include  <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64
int err_backup;
int out_backup;
int in_backup;

/* Splits the string by space and returns the array of tokens
*
*/

char **tokenize(char *line)
{
  	char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  	char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  	int i, tokenIndex = 0, tokenNo = 0;

  	for(i =0; i < strlen(line); i++){
   	 	char readChar = line[i];
    	if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
			token[tokenIndex] = '\0';
      		if (tokenIndex != 0){
				tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
				strcpy(tokens[tokenNo++], token);
				tokenIndex = 0; 
    		}
    	}
		else 
			token[tokenIndex++] = readChar;
  	}
  	free(token);
 	tokens[tokenNo] = NULL ;
 	return tokens;
}

void signal_handler(int signo)
{
	remove(".error.txt");
	printf("\n");
	exit(0);
}

int main(int argc, char* argv[]) {
	struct stat statbuf;
	char  line[MAX_INPUT_SIZE];            
	char *args[MAX_TOKEN_SIZE];
	char  **tokens;              
	int i, status, fd, pipe_point;
	int is_pipe_after;
	int pipe_out, pipe_in;
	FILE* fp;

	//시그널처리
	signal(SIGINT, signal_handler);
	//모드 확인
	if(argc == 2) {
		fp = fopen(argv[1],"r");
		if(fp < 0) {
			printf("File doesn't exists.");
			return -1;
		}
	}
	while(1) {			
		/* BEGIN: TAKING INPUT */
		bzero(line, sizeof(line));
		// batch mode
		if(argc == 2) { 
			if(fgets(line, sizeof(line), fp) == NULL) { // file reading finished
				break;	
			}
			line[strlen(line) - 1] = '\0';
		} 
		// interactive mode
		else { 
			printf("$ ");
			scanf("%[^\n]", line);
			getchar();
		}

		/* END: TAKING INPUT */
		line[strlen(line)] = '\n'; //terminate with new line
		//한자리도 입력 안됐으면
		if(line[0] == '\n')
			continue;
		//line 토큰화
		tokens = tokenize(line);
		//에러 체크 임시파일 생성
		if((fd = open(".error.txt",O_RDWR|O_CREAT|O_TRUNC, 0644)) < 0){
			fprintf(stderr,"file create error\n");
			exit(1);
		}
		//redirection STDERR
		err_backup = dup(STDERR_FILENO);
		dup2(fd, STDERR_FILENO);
		pipe_point = 0;
		is_pipe_after = 0;
		//args배열 채움
		for(i=0;tokens[i] != NULL;i++){
			if(!strcmp(tokens[i], "|")){
				//args cut
				args[pipe_point] = NULL;
				// |문자가 나오면 STDOUT을  Redirection
				if(!strcmp(tokens[i], "|")){
					//이전 pipe파일이 있으면( | 명령어 | 인 경우)
					if(access("pipe.txt", F_OK) == 0 )
						pipe_out = open("pipe2.txt",O_WRONLY|O_CREAT|O_TRUNC, 0644);
					else
						//pipe파일 생성
						pipe_out = open("pipe.txt",O_WRONLY|O_CREAT|O_TRUNC, 0644);
					out_backup = dup(STDOUT_FILENO);
					dup2(pipe_out, STDOUT_FILENO);
				}
				// |문자가 이후이면 STDIN을 Redirection 
				if(is_pipe_after){
					//파이프는 반드시 아래에서 rename되므로 pipe.txt로 연다.
					pipe_in = open("pipe.txt",O_RDONLY);
					in_backup = dup(STDIN_FILENO);
					dup2(pipe_in, STDIN_FILENO);
				}
				//자식프로세스 생성
				if(fork() == 0){
					if(execvp(args[0],args) < 0){
						fprintf(stderr,"exec error\n");
						exit(1);
					}
				}
				//자식프로세스 대기
				if(wait(&status) < 0){
					fprintf(stderr,"wait error\n");
					exit(1);
				}
				//pipe_point 0으로 초기화
				pipe_point = 0;

				//파이프 사이에 낀 경우도 있으므로 is_pipe_after순서변경
				//STDIN 롤백
				if(is_pipe_after){
					dup2(in_backup, STDIN_FILENO);
					close(in_backup);
					//pipe처리 완료
					is_pipe_after = 0;
					//파일디스크립터 close
					close(pipe_in);
					//이미 다읽었으므로 파일삭제
					remove("pipe.txt");
				}
				//STDOUT 롤백
				if(!strcmp(tokens[i], "|")){
					dup2(out_backup, STDOUT_FILENO);
					close(out_backup);
					//pipe처리 필요
					is_pipe_after = 1;
					close(pipe_out);
					//이전 pipe는 사용됐으므로 pipe rename
					if(access("pipe2.txt", F_OK) == 0)
						rename("pipe2.txt", "pipe.txt");
				}
			}
			else
				args[pipe_point++] = tokens[i];
		}
		// |문자가 이후이면 STDIN을 Redirection 
		if(is_pipe_after){
			//파이프는 반드시 아래에서 rename되므로 pipe.txt로 연다.
			pipe_in = open("pipe.txt",O_RDONLY);
			in_backup = dup(STDIN_FILENO);
			dup2(pipe_in, STDIN_FILENO);
		}
		//자식프로세스 생성
		if(fork() == 0){
			if(execvp(args[0],args) < 0){
				fprintf(stderr,"exec error\n");
				exit(1);
			}
		}
		//자식프로세스 대기
		if(wait(&status) < 0){
			fprintf(stderr,"wait error\n");
			exit(1);
		}
		//STDIN 롤백
		if(is_pipe_after){
			dup2(in_backup, STDIN_FILENO);
			close(in_backup);
			//pipe처리 완료
			is_pipe_after = 0;
			//파일디스크립터 close
			close(pipe_in);
			//이미 다읽었으므로 파일삭제
			remove("pipe.txt");
		}
		//pipe_point 0으로 초기화
		dup2(err_backup, STDERR_FILENO);
		close(err_backup);
		close(fd);
		//에러파일 체크
		if(stat(".error.txt",&statbuf) < 0){
			fprintf(stderr,"stat error\n");
			exit(1);
		}
		//error가 났는지 확인
		if(statbuf.st_size != 0)
			fprintf(stderr,"SSUShell : Incorrect command\n");
		// Freeing the allocated memory	
		for(i=0;tokens[i]!=NULL;i++)
			free(tokens[i]);
		free(tokens);
	}
	return 0;
}
