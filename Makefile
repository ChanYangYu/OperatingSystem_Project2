all : ssu_shell pps
CC=gcc
CFLGGS=-Wall

ssu_shell : ssu_shell.c
	$(CC) -o $@ $< $(CFLGGS)

pps : pps.c
	$(CC) -o $@ $< $(CFLGGS) -lncurses 
clean:
	rm  ssu_shell
	rm  pps
