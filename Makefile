CC ?= cc
CFLAGS += -g -std=c99 -pedantic -Wall
LIBS = -lSDL2 -lSDL2_ttf -lSDL2_image

sslide: *.c
	$(CC) $^ -o $@ $(CFLAGS) $(LIBS)

