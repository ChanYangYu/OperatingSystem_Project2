#include "ssu_shell.h"

struct pid_stat{
	int pid;
	unsigned long utime;
	unsigned long stime;
	struct pid_stat *next;
	struct pid_stat *prev;
};

struct pid_stat* head;
struct pid_stat* tail;
int MEMTOT;
int POS;
int MAX_WIDTH;
int MAX_HEIGHT;
int MAX_PROC = 80;

void set_memtotal();
void get_data();

int main()
{
	char ch;
	int x, y, i;

	head = NULL;
	tail = NULL;
	set_memtotal();
	initscr();
	getmaxyx(stdscr,y, x);
	MAX_WIDTH = x;
	MAX_HEIGHT = y; 
	
    chdir("/proc");
	y = 0;
	for(i = 0; i < MAX_PROC; i++){
		if(y >= MAX_HEIGHT)
			break;
		if(i >= POS){
			printw("process : %d\n",i+1);
			y++;
		}
	}

	keypad(stdscr, TRUE);
	while((ch = getch()) != 'q'){
		if(ch == 3 && POS >0) 
			POS--;
		else if(ch == 2 && POS < MAX_PROC-1)
			POS++;
		else
			continue;
		clear();
		y = 0;
		for(i = 0; i < MAX_PROC; i++){
			if(y >= MAX_HEIGHT)
				break;
			if(i >= POS){
				printw("process : %d\n",i+1);
				y++;
			}
		}
	}
	endwin();
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

void set_memtotal()
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
	MEMTOT = atol(token);
}

void get_data(){
	int i,j,c, process_uid;
	int is_null = 0, ps_cnt = 0, run_cnt = 0, stp_cnt = 0;
	int sle_cnt = 0, zom_cnt = 0;
	unsigned long long tmp, totaltime, starttime;
	double uptime, ps_uptime, cpu_usage;
	long virt, res, shr;
	FILE* fp;
	DIR* dirp;
	char tokens[26][MAX_TOKEN_SIZE], token[MAX_TOKEN_SIZE];
	char buffer[MAX_TOKEN_SIZE], print_buffer[1024];
	struct stat statbuf;
	struct dirent* dp;
	struct pid_stat *new_data, *cur;
	struct passwd *pw;

	//처음실행인지 확인
	if(head == NULL && tail == NULL)
		is_null = 1;
	cur = head;
    //process stat parsing
    if((dirp = opendir("/proc")) == NULL){
        fprintf(stderr,"/proc open fail\n");
        exit(1);
    }   
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
            //22개의 stat정보 저장
            for(i = 1; i < 23; i++){
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
			fclose(fp);
			if((fp = fopen("statm","r")) == NULL){
				fprintf(stderr,"statm open error\n");
				endwin();
			}
			//3개 memory정보 저장
            for(i = 23; i < 26; i++){
				j = 0;
                while((c = fgetc(fp)) != ' ')
                       tokens[i][j++] = c;
                tokens[i][j] = '\0';
            }
			virt = strtoul(tokens[23], NULL, 10) * 4;
			res = strtoul(tokens[24], NULL, 10) * 4; 
			shr = strtoul(tokens[25], NULL, 10) * 4;
			fclose(fp);
			chdir("..");

			memset(print_buffer, 0, sizeof(print_buffer));
			sprintf(print_buffer,"%7s %-9s %2s %3s %7ld %6ld %6ld %c", tokens[1], tokens[0], tokens[18], tokens[19], virt, res, shr, tokens[3][0]);
			new_data = (struct pid_stat*)malloc(sizeof(struct pid_stat));
			new_data->prev = NULL;
			new_data->next = NULL;
			new_data->pid = atoi(tokens[1]);
			new_data->utime = strtoul(tokens[14], NULL, 10);
			new_data->stime = strtoul(tokens[15], NULL, 10);
			if(is_null){
				if(head == NULL){
					head = new_data;
					tail = new_data;
					cur = new_data;
				}
				else{
					cur->next = new_data;
					cur = new_data;
					tail = new_data;
				}
				//totaltime
				totaltime = new_data->utime + new_data->stime;
				//starttime
				starttime = strtoull(tokens[22], NULL, 10);
				//uptime
				if((fp = fopen("/proc/uptime", "r")) == NULL){
					fprintf(stderr,"/prco/uptime open error\n");
					exit(1);
				}
				i = 0;
				while((c = fgetc(fp)) != ' ')
					token[i++] = c;
				token[i] = '\0';
				uptime = atof(token);
				fclose(fp);
				//calculation
				ps_uptime = uptime - (starttime/HZ);
				cpu_usage = 100 * ((totaltime/HZ) / ps_uptime);
				//CPU 점유율
				c = (int)(cpu_usage * 10);
				cpu_usage = tmp/10.0;
				memset(buffer, 0, sizeof(buffer));
				sprintf(buffer,"  %2.1f",cpu_usage);
				strcat(print_buffer,buffer);
			}
			//여기서부터
			else{
				if(new_data->pid == cur->pid){
				}
			}
			if(tokens[3][0] == 'R')
				run_cnt++;
			else if(tokens[3][0] == 'S')
				sle_cnt++;
			else if(tokens[3][0] == 'Z')
				zom_cnt++;
			else if(tokens[3][0] == 'T')
				stp_cnt++;
			ps_cnt++;
			tmp = new_data->utime + new_data->stime;
			//sprintf(new_data->TIME,"%ld:%02ld.%02d",(tmp/HZ)/60, (tmp/HZ)%60, tmp%100);
			//strcpy(new_data->command, tokens[2]);
		}
	}
}

