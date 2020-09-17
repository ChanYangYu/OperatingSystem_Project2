//headers
#include  <stdio.h>
#include  <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <sys/param.h>
#include <curses.h>
#include <sys/time.h>

//constants
#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

//struct
struct pid_stat{
    int pid;
    char user[10];
    char stat;
    char pr[5];
    long ni;
    unsigned long virt;
    unsigned long res;
    unsigned long shr;
    unsigned long utime;
    unsigned long stime;
    double cpu_usage;
    char TIME[10];
    char command[100];
    struct pid_stat *next;
    struct pid_stat *prev;
};

