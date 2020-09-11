#include "ssu_shell.h"
#include <dirent.h>

int isNum(char* name);
int main(int argc, char* argv[])
{
	FILE* fp;
	char tokens[31][MAX_TOKEN_SIZE];
	DIR* dirp;
	struct dirent* dp;
	int i, j, c;
	long nice, num_thread;


	if((dirp = opendir("/proc")) == NULL){
		fprintf(stderr,"/proc open fail\n");
		exit(1);
	}

	chdir("/proc");
	while((dp = readdir(dirp)) != NULL){
		memset(tokens, 0, sizeof(tokens));
		if(isNum(dp->d_name)){
			chdir(dp->d_name);
			if((fp = fopen("stat", "r")) == NULL){
				fprintf(stderr,"stat open error\n");
				exit(1);
			}
			for(i = 1; i < 31; i++){
				j = 0;
				while((c = fgetc(fp)) != ' ')
					tokens[i][j++] = c;
				tokens[i][j] = '\0';
			}
			nice = atol(tokens[19]);
			if(nice < 0)
				strcat(tokens[3], "<");
			else if(nice > 0)
				strcat(tokens[3], "N");
			if(!strcmp(tokens[1], tokens[6]))
				strcat(tokens[3], "s");
			num_thread = atol(tokens[20]);
			if(num_thread > 1)
				strcat(tokens[3], "l");
			if(strcmp(tokens[8], "-1"))
				strcat(tokens[3], "+");

			printf("%s\t%s\n",tokens[1], tokens[3]);
			chdir("..");
		}
	}
}

int isNum(char* name)
{
	int i;

	for(i = 0; name[i] != '\0'; i++){
		if(name[i] < '0' || name[i] > '9')
			return 0;
	}
	return 1;
}

