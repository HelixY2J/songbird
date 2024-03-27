CFLAGS= -Wall -pedantic -std=gnu99
CORE_PATH=../core
CORE_LIB= -I $(CORE_PATH)

all: libsong nightswatch song

libsong: $(CORE_PATH)/libsong.c
	gcc -c $(CFLAGS) $(CORE_LIB) $(CORE_PATH)/libsong.c -o $(CORE_PATH)/libsong.o

nightswatch: nightswatch.c
	gcc $(CFLAGS) $(CORE_LIB) nightswatch.c $(CORE_PATH)/libsong.o -o build/nightswatchd

song: song.c
	gcc $(CFLAGS) $(CORE_LIB) `pkg-config --cflags --libs libnotify` song.c $(CORE_PATH)/libsong.o -o build/songd
