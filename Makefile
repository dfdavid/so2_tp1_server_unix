CC=gcc
CFLAGS= -Werror -Wall -pedantic

all: tp1_server_u

tp1_server_u: main.c
	cppcheck --enable=all --inconclusive --std=posix main.c
	$(CC) $(CFLAGS) -o tp1_server_u main.c

clean:
	rm tp1_server_u
