CFLAGS= -Wall -pedantic -std=gnu99 -g

all: libsong songtest

libsong: libsong.c
	gcc -c $(CFLAGS) ./libsong.c -o libsong.o

flextest: songtest.c
	gcc $(CFLAGS) -pthread songtest.c libsong.o -o songtest
