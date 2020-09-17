all : ssu_shell pps ttop
CC=gcc
CFLGGS=-Wall

ssu_shell : ssu_shell.c
	$(CC) -o $@ $< $(CFLGGS)

pps : pps.c
	$(CC) -o $@ $< $(CFLGGS) -lncurses 

ttop : ttop.c
	$(CC) -o $@ $< $(CFLGGS) -lncurses -lrt

clean:
	rm -r ssu_shell
	rm -r pps
	rm -r ttop
