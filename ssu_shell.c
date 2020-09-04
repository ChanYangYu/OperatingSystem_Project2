#include  <stdio.h>
#include  <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

int backup;

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

int main(int argc, char* argv[]) {
	struct stat statbuf;
	char  line[MAX_INPUT_SIZE];            
	char *args[MAX_TOKEN_SIZE];
	char  **tokens;              
	int i, status, fd;
	FILE* fp;

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
		//args배열 채움
		for(i=0;tokens[i]!=NULL;i++)
			args[i] = tokens[i];
		args[i] = NULL;
		//에러 체크 임시파일 생성
		if((fd = open(".error.txt",O_RDWR|O_CREAT|O_TRUNC, 0644)) < 0){
			fprintf(stderr,"file create error\n");
			exit(1);
		}
		//redirection STDERR
		backup = dup(2);
		dup2(fd, 2);
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
		//roll back
		dup2(backup, STDERR_FILENO);
		close(backup);
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
