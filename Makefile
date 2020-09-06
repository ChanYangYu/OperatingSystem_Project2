CC=gcc
CFLGGS=-g -Wall
TARGET=ssu_shell
OBJS=ssu_shell.o

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

ssu_shell.o : ssu_shell.c
	$(CC) -c -o ssu_shell.o ssu_shell.c

clean:
	rm -f *.o
	rm -f $(TARGET)
