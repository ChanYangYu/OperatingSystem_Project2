#include "ssu_shell.h"

//전역변수
int is_aOption, is_uOption, is_xOption;
int MAX_WIDTH;
long memtotal;
time_t start_time,cpu_time;
double cpu_usage;
char tokens[25][MAX_TOKEN_SIZE];

//함수
int isNum(char* name);
void get_info(unsigned long long totaltime, unsigned long long starttime);
void get_memtotal();
void print_no_option();
void print_not_uOption();
void print_uOption();

//메인
int main(int argc, char* argv[])
{
	FILE* fp;
	DIR* dirp;
	int user, process_uid;
	struct dirent* dp;
	struct stat statbuf;
	struct passwd *pw;
	int i, j, c;

	//터미널 너비받아옴
	initscr();
	getmaxyx(stdscr, i, j);
	endwin();
	MAX_WIDTH = j;
	
	//옵션처리
	if(argc > 1){
		for(i = 0; argv[1][i] !='\0'; i++)
			switch(argv[1][i]){
				case 'a' : is_aOption = 1; break;
				case 'u' : is_uOption = 1; break;
				case 'x' : is_xOption = 1; break;
				default  : fprintf(stderr,"option error\n");exit(1);break;
			}
	}
	//u옵션이 있으면
	if(is_uOption){
		printf("USER        PID %%CPU %%MEM    VSZ   RSS TTY      STAT START   TIME COMMAND\n");
		//창이 작은경우
		if(MAX_WIDTH <= 67)
			//커맨드 길이 두배늘림
			MAX_WIDTH *= 2;
		//memory총크기 전역변수에 저장
		get_memtotal();
	}
	//u옵션이 없는경우
	else if(is_aOption || is_xOption)
		printf("  PID TTY      STAT   TIME COMMAND\n");
	//옵션이 없는경우
	else{
		printf("  PID TTY          TIME CMD\n");
		//함수에서 처리후 종료
		print_no_option();
		exit(0);
	}

	//process stat parsing
	if((dirp = opendir("/proc")) == NULL){
		fprintf(stderr,"/proc open fail\n");
		exit(1);
	}
	user = getuid();	
	chdir("/proc");
	while((dp = readdir(dirp)) != NULL){
		memset(tokens, 0, sizeof(tokens));
		if(isNum(dp->d_name)){
			if(stat(dp->d_name, &statbuf) < 0){
				fprintf(stderr,"%s stat error\n", dp->d_name);
				exit(1);
			}
			//소유자 아이디 문자열화
			process_uid = statbuf.st_uid;
			pw = getpwuid(process_uid);
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
			//24개의 stat정보 저장
			for(i = 1; i < 25; i++){
				j = 0;
				while((c = fgetc(fp)) != ' '){
					if(c == '('){
						while((c = fgetc(fp)) != ')')
							tokens[i][j++] = c;
					}
					else
						tokens[i][j++] = c;
				}
				tokens[i][j] = '\0';
			}
			//a옵션 단독일경우
			if(is_aOption && !is_xOption){
				if(!strcmp(tokens[7], "0")){
					fclose(fp);
					chdir("..");
					continue;
				}
			}
			//x옵션 단독일 경우
			else if(is_xOption && !is_aOption){
				if(user != process_uid){
					fclose(fp);
					chdir("..");
					continue;
				}
			}
			//u옵션 단독일 경우
			else if(is_uOption && !is_aOption && !is_xOption){
				if(user != process_uid || !strcmp(tokens[7], "0")){
					fclose(fp);
					chdir("..");
					continue;
				}
			}
			//print 옵션확인
			if(is_uOption)
				print_uOption();
			//u옵션이 없는경우
			else if(is_aOption || is_xOption)
				print_not_uOption();
			fclose(fp);
			chdir("..");
		}
	}
}

//디렉토리 이름이 숫자인지 확인
int isNum(char* name)
{
	int i;

	for(i = 0; name[i] != '\0'; i++){
		if(name[i] < '0' || name[i] > '9')
			return 0;
	}
	return 1;
}

//cpu_usage, start_time, cpu_time을 구하여 전역변수에 저장
void get_info(unsigned long long totaltime, unsigned long long starttime)
{
	FILE* fp;
	double uptime, ps_uptime;
	int c, i, tmp;
	char token[MAX_TOKEN_SIZE];
	time_t t;

	if((fp = fopen("/proc/uptime", "r")) == NULL){
		fprintf(stderr,"/prco/uptime open error\n");
		exit(1);
	}
	i = 0;
	while((c = fgetc(fp)) != ' ')
		token[i++] = c;
	token[i] = '\0';
	uptime = atof(token);
	ps_uptime = uptime - (starttime/HZ);
	cpu_usage = 100 * ((totaltime/HZ) / ps_uptime);

	//CPU 점유율
	tmp = (int)(cpu_usage * 10);
	cpu_usage = tmp/10.0;
	//CPU 사용시간
	cpu_time = (time_t)(totaltime/HZ);

	//프로세스 시작시간
	t = time(NULL);
	tmp = (int)uptime;
	tmp = tmp - (int)(starttime/HZ);
	t -= (time_t)tmp;
	start_time = t;

	fclose(fp);
}

// /proc/meminfo에 total memory를 구하여 전역변수에 저장
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

//u옵션 print 처리
void print_uOption()
{
	long nice, num_thread, rss, vsz;
	struct tm *tmbuf;
	time_t t;
	unsigned long long starttime,totaltime;
	char path[MAX_INPUT_SIZE], *ptr;
	char s_time[MAX_TOKEN_SIZE], c_time[MAX_TOKEN_SIZE];
	char line[MAX_WIDTH];
	int i, j, tmp, tty_nr, fd;
	int cur;

	nice = atol(tokens[19]);
	//high-priority
	if(nice < 0)
		strcat(tokens[3], "<");
	//low-priority
	else if(nice > 0)
		strcat(tokens[3], "N");
	//session leader
	if(!strcmp(tokens[1], tokens[6]))
		strcat(tokens[3], "s");
	num_thread = atol(tokens[20]);
	//multithread
	if(num_thread > 1)
		strcat(tokens[3], "l");
	//foreground
	if(!strcmp(tokens[5], tokens[8]))
		strcat(tokens[3], "+");
	//is tty0~
	tty_nr = atoi(tokens[7]);
	if(tty_nr >= 1024 && tty_nr <= 1087){
		memset(tokens[7], 0, sizeof(tokens[7]));
		sprintf(tokens[7],"tty%d",tty_nr-1024);
	}
	//is ttyS
	else if(tty_nr >= 1088 && tty_nr <= 1119){
		memset(tokens[7], 0, sizeof(tokens[7]));
		sprintf(tokens[7],"ttyS%d",tty_nr-1088);
	}
	//is tty
	else if(tty_nr == 1280){
		memset(tokens[7], 0, sizeof(tokens[7]));
		strcpy(tokens[7], "tty");
	}
	//is ptmx
	else if(tty_nr == 1282){
		memset(tokens[7], 0, sizeof(tokens[7]));
		strcpy(tokens[7], "/ptmx");
	}
	//is ttyprintk
	else if(tty_nr == 1283){
		memset(tokens[7], 0, sizeof(tokens[7]));
		strcpy(tokens[7], "ttyprintk");
	}
	//is pts
	else if(tty_nr >= 34816){
		memset(tokens[7], 0, sizeof(tokens[7]));
		sprintf(tokens[7],"pts/%d",tty_nr-34816);
	}
	//else
	else{
		memset(tokens[7], 0, sizeof(tokens[7]));
		strcpy(tokens[7], "?");
	}
	//COMMAND
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
	close(fd);
	totaltime = 0;
	//totaltime
	for(i = 14; i < 16; i++){
		totaltime += strtoul(tokens[i], NULL, 10);
	}
	//get time info
	starttime = strtoull(tokens[22], NULL, 10);
	get_info(totaltime, starttime);
	//start time
	t = time(NULL);
	//하루가 지난 경우
	if(t - start_time >= 86400){
		tmbuf = localtime(&start_time);
		memset(s_time, 0, sizeof(s_time));
		strftime(s_time, sizeof(s_time), "%b %d",tmbuf);
	}
	//24시간 이전
	else{
		tmbuf = localtime(&start_time);
		memset(s_time, 0, sizeof(s_time));
		strftime(s_time, sizeof(s_time), "%H:%M",tmbuf);
	}
	//TIME 값 저장
	if(cpu_time >= 60){
		memset(c_time, 0, sizeof(c_time));
		sprintf(c_time,"%ld:%02ld",cpu_time/60, cpu_time%60);
	}
	else{
		memset(c_time, 0, sizeof(c_time));
		sprintf(c_time,"0:%02ld",cpu_time);
	}
	//vsz rss 값 연산
	vsz = atol(tokens[23]) / 1024;
	rss = atol(tokens[24]) * 4;
	tmp = (int)(((double)rss / memtotal) * 1000);

	//print
	cur = 0;
	memset(line, 0, sizeof(line));
	sprintf(line,"%-8s %6s  %.1f  %.1f %6ld %5ld %s ",tokens[0], tokens[1], cpu_usage, tmp/10.0,vsz,rss, tokens[7]);
	printf("%s",line);
	//1차 print분기점
	cur += strlen(line);
	while(cur++ < 48)
		printf(" ");
	memset(line, 0, sizeof(line));
	sprintf(line,"%-4s %s %6s ",tokens[3], s_time, c_time);
	printf("%s",line);
	cur += strlen(line);
	//2차 print분기점
	memset(line, 0, sizeof(line));
	strncpy(line, path, MAX_WIDTH-cur+1);
	printf("%s\n",line);
}

//u옵션이 없는경우 print처리
void print_not_uOption(){
	long nice, num_thread;
	unsigned long long starttime,totaltime;
	char path[MAX_INPUT_SIZE], *ptr;
	char c_time[MAX_TOKEN_SIZE], line[MAX_WIDTH];
	int i, j, tty_nr, fd, cur;

	nice = atol(tokens[19]);
	//high-priority
	if(nice < 0)
		strcat(tokens[3], "<");
	//low-priority
	else if(nice > 0)
		strcat(tokens[3], "N");
	//session leader
	if(!strcmp(tokens[1], tokens[6]))
		strcat(tokens[3], "s");
	num_thread = atol(tokens[20]);
	//multithread
	if(num_thread > 1)
		strcat(tokens[3], "l");
	//foreground
	if(!strcmp(tokens[5], tokens[8]))
		strcat(tokens[3], "+");
	//is tty0~
	tty_nr = atoi(tokens[7]);
	if(tty_nr >= 1024 && tty_nr <= 1087){
		memset(tokens[7], 0, sizeof(tokens[7]));
		sprintf(tokens[7],"tty%d",tty_nr-1024);
	}
	//is ttyS
	else if(tty_nr >= 1088 && tty_nr <= 1119){
		memset(tokens[7], 0, sizeof(tokens[7]));
		sprintf(tokens[7],"ttyS%d",tty_nr-1088);
	}
	//is tty
	else if(tty_nr == 1280){
		memset(tokens[7], 0, sizeof(tokens[7]));
		strcpy(tokens[7], "tty");
	}
	//is ptmx
	else if(tty_nr == 1282){
		memset(tokens[7], 0, sizeof(tokens[7]));
		strcpy(tokens[7], "/ptmx");
	}
	//is ttyprintk
	else if(tty_nr == 1283){
		memset(tokens[7], 0, sizeof(tokens[7]));
		strcpy(tokens[7], "ttyprintk");
	}
	//is pts
	else if(tty_nr >= 34816){
		memset(tokens[7], 0, sizeof(tokens[7]));
		sprintf(tokens[7],"pts/%d",tty_nr-34816);
	}
	//else
	else{
		memset(tokens[7], 0, sizeof(tokens[7]));
		strcpy(tokens[7], "?");
	}
	//COMMAND
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
	close(fd);
	totaltime = 0;
	//totaltime
	for(i = 14; i < 16; i++){
		totaltime += strtoul(tokens[i], NULL, 10);
	}
	//get time info
	starttime = strtoull(tokens[22], NULL, 10);
	get_info(totaltime, starttime);
	//TIME값 처리
	if(cpu_time >= 60){
		memset(c_time, 0, sizeof(c_time));
		sprintf(c_time,"%ld:%02ld",cpu_time/60, cpu_time%60);
	}
	else{
		memset(c_time, 0, sizeof(c_time));
		sprintf(c_time,"0:%02ld",cpu_time);
	}

	//print
	memset(line, 0, sizeof(line));
	sprintf(line, "%5s %-8s %-6s %s ", tokens[1], tokens[7], tokens[3], c_time);
	printf("%s",line);
	cur = strlen(line);
	//print분기점
	memset(line, 0, sizeof(line));
	strncpy(line, path, MAX_WIDTH - cur);
	printf("%s\n",line);

}

//옵션이 없는경우 print처리
void print_no_option()
{
	FILE* fp;
	DIR* dirp;
	struct dirent* dp;
	char path[MAX_INPUT_SIZE], line[MAX_WIDTH];
	char tty_num[MAX_TOKEN_SIZE],c_time[MAX_TOKEN_SIZE];
	pid_t pid;
	int i, j, c, tty_nr, cur;
	unsigned long long totaltime, starttime;

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
			if(c == '(')
				while((c = fgetc(fp)) != ')');
			else if(i == 7)
				tty_num[j++] = c;
		}
	}
	tty_num[j] = '\0';
	fclose(fp);

	//process stat parsing
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
			//stat의 24개 정보를 받아옴
			for(i = 1; i < 25; i++){
				j = 0;
				while((c = fgetc(fp)) != ' '){
					if(c == '('){
						while((c = fgetc(fp)) != ')')
							tokens[i][j++] = c;
					}
					else
						tokens[i][j++] = c;
				}
				tokens[i][j] = '\0';
			}
			//같은 터미널이 아니면
			if(strcmp(tokens[7], tty_num) != 0){
				chdir("..");
				fclose(fp);
				continue;
			}
			//is tty
			if(tokens[7][0] == '1'){
				tty_nr = atoi(tokens[7]);
				memset(tokens[7], 0, sizeof(tokens[7]));
				sprintf(tokens[7],"tty%d",tty_nr-1024);
			}
			//is pts
			else if(tokens[7][0] == '3'){
				tty_nr = atoi(tokens[7]);
				memset(tokens[7], 0, sizeof(tokens[7]));
				sprintf(tokens[7],"pts/%d",tty_nr-34816);
			}
			//else
			else{
				memset(tokens[7], 0, sizeof(tokens[7]));
				strcpy(tokens[7], "?");
			}
			totaltime = strtoul(tokens[14], NULL, 10) + strtoul(tokens[15], NULL, 10);
			starttime = strtoull(tokens[22], NULL, 10);
			get_info(totaltime, starttime);
			//TIME 연산처리
			if(cpu_time >= 3600){
				memset(c_time, 0, sizeof(c_time));
				sprintf(c_time,"%02ld:%02ld:%02ld",cpu_time/3600, (cpu_time%3600)/60, cpu_time%60);
			}
			else if(cpu_time >= 60){
				memset(c_time, 0, sizeof(c_time));
				sprintf(c_time,"00:%02ld:%02ld",cpu_time/60, cpu_time%60);
			}
			else
				sprintf(c_time,"00:00:%02ld",cpu_time%60);

			//print
			memset(line, 0, sizeof(line));
			sprintf(line, "%5s %-8s %s ",tokens[1], tokens[7], c_time);
			printf("%s",line);
			cur = strlen(line);
			//print분기점
			memset(line, 0, sizeof(line));
			strncpy(line, tokens[2], MAX_WIDTH-cur);
			printf("%s\n",line);
			fclose(fp);
			chdir("..");
		}
	}
}
