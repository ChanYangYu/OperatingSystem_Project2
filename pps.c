#include "ssu_shell.h"
#include <dirent.h>
#include <time.h>
#include <pwd.h>

int is_aOption, is_uOption, is_xOption;
long memtotal;
char tokens[31][MAX_TOKEN_SIZE];

int isNum(char* name);
void no_option();
void print_uOption();
double cpuUsage(unsigned long long totaltime, unsigned long long starttime);
void get_memtotal();

int main(int argc, char* argv[])
{
	FILE* fp;
	DIR* dirp;
	struct dirent* dp;
	struct stat statbuf;
	struct passwd *pw;
	int i, j, c;
	long nice, num_thread;

	if(argc > 1){
		for(i = 0; argv[1][i] !='\0'; i++)
			switch(argv[1][i]){
				case 'a' : is_aOption = 1; break;
				case 'u' : is_uOption = 1; break;
				case 'x' : is_xOption = 1; break;
				default  : fprintf(stderr,"option error\n");break;
			}
	}
	if(is_uOption){
		printf("USER         PID %%CPU %%MEM    VSZ   RSS TTY      STAT START   TIME COMMAND\n");
		get_memtotal();
	}
	else if(is_aOption || is_xOption)
		printf("    PID TTY      STAT   TIME COMMAND\n");
	else{
		printf("    PID TTY          TIME CMD\n");
		no_option();
		exit(0);
	}
	if((dirp = opendir("/proc")) == NULL){
		fprintf(stderr,"/proc open fail\n");
		exit(1);
	}
	chdir("/proc");
	while((dp = readdir(dirp)) != NULL){
		memset(tokens, 0, sizeof(tokens));
		if(isNum(dp->d_name)){
			if(stat(dp->d_name, &statbuf) < 0){
				fprintf(stderr,"%s stat error\n", dp->d_name);
				exit(1);
			}
			//소유자 아이디 문자열화
			pw = getpwuid(statbuf.st_uid);
			if(strlen(pw->pw_name) > 8){
				strncpy(tokens[0],pw->pw_name, 7);
				strcat(tokens[0],"+");
			}
			else
				strcpy(tokens[0], pw->pw_name);
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
			if(is_uOption)
				print_uOption();
			fclose(fp);
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

double cpuUsage(unsigned long long totaltime, unsigned long long starttime)
{
	FILE* fp;
	double uptime, ps_uptime, cpu_usage;
	int c, i, tmp;
	long clktick;
	char token[MAX_TOKEN_SIZE];

	if((fp = fopen("/proc/uptime", "r")) == NULL){
		fprintf(stderr,"/prco/uptime open error\n");
		exit(1);
	}
	i = 0;
	while((c = fgetc(fp)) != ' ')
		token[i++] = c;
	token[i] = '\0';
	uptime = atof(token);
	clktick = sysconf(_SC_CLK_TCK);
	ps_uptime = uptime - (starttime/clktick);
	cpu_usage = 100 * ((totaltime/clktick) / ps_uptime);

	tmp = (int)(cpu_usage * 10);
	printf(" %.1f ", tmp/10.0);

	fclose(fp);
	return cpu_usage;
}

void print_uOption()
{
	long nice, num_thread, rss, vsz;
	unsigned long long starttime,totaltime;
	int i, tmp;

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
	//수정필요
	if(strcmp(tokens[8], "-1"))
		strcat(tokens[3], "+");
	totaltime = 0;
	//14~15
	for(i = 14; i < 16; i++){
		totaltime += strtoul(tokens[i], NULL, 10);
	}
	vsz = atol(tokens[23]) / 1024;
	rss = atol(tokens[24]) * 4;
	tmp = (int)(((double)rss / memtotal) * 1000);
	starttime = strtoull(tokens[22], NULL, 10);
	printf("%-9s %6s ",tokens[0], tokens[1]);
	cpuUsage(totaltime, starttime);
	printf(" %.1f ", tmp/10.0);
	printf("%6ld ",vsz);
	printf("%5ld\n",rss);
	//printf("%s\t%s\n",tokens[1], tokens[3]);


}
void get_memtotal()
{
	FILE* fp;
	char token[MAX_TOKEN_SIZE];
	int c, i;
	if((fp = fopen("/proc/meminfo", "r")) == NULL){
		fprintf(stderr,"meminfo open error\n");
		exit(1);
	}
	while((c = fgetc(fp)) != '\n'){
		if(c >= '0' && c <= '9'){
			i = 0;
			token[i++] = c;
			c = fgetc(fp);
			while(c >= '0' && c <= '9'){
				token[i++] = c;
				c = fgetc(fp);
			}
			token[i] = '\0';
			break;
		}
		
	}
	memtotal = atol(token);
}
void no_option()
{
	FILE* fp;
	DIR* dirp;
	struct dirent* dp;
	char tokens[31][MAX_TOKEN_SIZE];
	char path[MAX_INPUT_SIZE];
	char buffer[MAX_INPUT_SIZE*2];
	char tty_num[MAX_TOKEN_SIZE];
	char *ptr;
	pid_t pid;
	int i, j, c, fd;
	long cmdsize;
	time_t t;

	pid = getpid();
	memset(path, 0, sizeof(path));
	sprintf(path, "/proc/%d/stat", pid);
	//현재 pid의 stat파일 오픈
	if((fp = fopen(path, "r")) == NULL){
		fprintf(stderr,"%s open error\n",path);
		exit(1);
	}
	//실행된pid의 터미널 정보가져옴
	memset(tty_num, 0, sizeof(path));
	j = 0;
	for(i = 1; i <= 7; i++){
		while((c = fgetc(fp)) != ' '){
			if(i == 7)
				tty_num[j++] = c;
		}
	}
	tty_num[j] = '\0';
	fclose(fp);

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
				while((c = fgetc(fp)) != ' '){
					if(c == '(' || c == ')')
						continue;
					tokens[i][j++] = c;
				}
			}
			//같은 터미널이 아니면
			if(strcmp(tokens[7], tty_num) != 0){
				chdir("..");
				fclose(fp);
				continue;
			}
			printf("%7s ",tokens[1]);
			printf("%s\n",tokens[2]);
			//COMMAND
			/*
			if((fd = open("cmdline", O_RDONLY)) < 0){
				fprintf(stderr,"cmdline open error\n");
				exit(1);
			}
			memset(path, 0, sizeof(path));
			//임시로 cmdline이 1024자 이하라고 가정
			j = read(fd, path, sizeof(path));
			if(j != 0){
				if(strlen(path) < j){
					ptr = path;
					while(ptr-path <= j &&(ptr = strchr(ptr, '\0')) != NULL)
						*ptr++ = ' ';
					path[j-1] = '\0';
				}
			}
			else
				sprintf(path,"[%s]",tokens[2]);
			printf("%s\n",path);
			*/
			close(fd);
			fclose(fp);
			chdir("..");
		}
	}
}
