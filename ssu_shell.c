#include "ssu_shell.h" 

//global variable
int err_backup;
int out_backup;
int in_backup;

//local func
char **tokenize(char *line);
void execute_pipe_command(char **tokens);
void execute_normal_command(char **tokens);
void signal_handler(int signo);

//main
int main(int argc, char* argv[]) {
	struct stat statbuf;
	char  line[MAX_INPUT_SIZE];            
	char  **tokens;              
	int i, fd;
	int in_pipe;
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
			if(fgets(line, sizeof(line), fp) == NULL) { 
				remove(".error.txt");
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
		// |검사
		in_pipe = 0;
		for(i=0;tokens[i] != NULL;i++){
			if(!strcmp(tokens[i], "|")){
				in_pipe = 1;
				break;
			}
		}
		//파이프 명령어 실행
		if(in_pipe)
			execute_pipe_command(tokens);
		//일반 명령어 실행
		else
			execute_normal_command(tokens);
		//STDERR 롤백
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

void signal_handler(int signo)
{
	remove(".error.txt");
	printf("\n");
	exit(0);
}

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


void execute_pipe_command(char **tokens)
{
	char *args[MAX_TOKEN_SIZE/2][MAX_TOKEN_SIZE];
	int pipes[MAX_TOKEN_SIZE/2][2], i;
	int pipe_num = 0;
	int j = 0;
	for(i = 0; tokens[i] != NULL; i++){
		//파이프가 나오면
		if(!strcmp(tokens[i], "|")){
			//마지막 arg NULL처리
			args[pipe_num][j] = NULL;
			//파이프 생성
			if(pipe(pipes[pipe_num]) < 0){
				fprintf(stderr,"pipe create error\n");
				exit(1);
			}
			pipe_num++;
			j = 0;
		}
		//grep 옵션 설정
		else if(!strcmp(tokens[i], "grep")){
			args[pipe_num][j++] = tokens[i];
			args[pipe_num][j++] = "--color=auto";
		}
		else
			args[pipe_num][j++] = tokens[i];
	}
	//마지막 arg NULL처리
	args[pipe_num][j] = NULL;
	//명령어는 pipe_num보다 1개 더크므로 <=
	for(i = 0; i <= pipe_num; i++){
		if(fork() == 0){
			//첫번째 명령어 일때
			if(i == 0){
				dup2(pipes[i][1], STDOUT_FILENO);
				//나머지 파이프 close
				close(pipes[i][0]);
				for(j = 1; j < pipe_num; j++){
					close(pipes[j][0]);
					close(pipes[j][1]);
				}
			}
			//마지막 명령어 일때
			else if(i == pipe_num){
				//pipe개수는 1개 더 적으므로 i-1
				dup2(pipes[i-1][0], STDIN_FILENO);
				//나머지 파이프 close
				close(pipes[i-1][1]);
				for(j = 0; j < pipe_num-1; j++){
					close(pipes[j][0]);
					close(pipes[j][1]);
				}
			}
			//파이프 사이의 명령어 일때
			else{
				dup2(pipes[i-1][0], STDIN_FILENO);
				dup2(pipes[i][1], STDOUT_FILENO);
				//나머지 파이프 close
				for(j = 0; j < pipe_num; j++){
					if(j == i-1)
						close(pipes[j][1]);
					else if(j == i)
						close(pipes[j][0]);
					else{
						close(pipes[j][1]);
						close(pipes[j][0]);
					}
				}
			}
			if(execvp(args[i][0], args[i]) < 0){
				fprintf(stderr,"exec error\n");
				exit(1);
			}
		}
	}
	//부모프로세스의 모든 파이프 close
	for(i = 0; i < pipe_num; i++){
		close(pipes[i][0]);
		close(pipes[i][1]);
	}
	//자식프로세스 대기
	while(wait((int*)0) != -1);
}

void execute_normal_command(char **tokens)
{
	char *arg[MAX_TOKEN_SIZE];
	int i;
	for(i = 0; tokens[i] != NULL; i++){
		//grep 명령어 옵션추가
		if(!strcmp(tokens[i], "grep")){
			arg[i] = tokens[i];
			arg[i] = "--color=auto";
		}
		//arg에 추가
		else
			arg[i] = tokens[i];
	}
	//마지막 arg NULL처리
	arg[i] = NULL;
	if(fork() == 0){
		if(execvp(arg[0], arg) < 0){
			fprintf(stderr, "exec error\n");
			exit(1);
		}
	}
	//자식프로세스 대기
	if(wait((int*)0) < 0){
		fprintf(stderr,"wait error\n");
		exit(1);
	}
}
