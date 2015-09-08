CC=gcc
CFLAGS=-m32 -static-libgcc -Os

all:
	$(CC) $(CFLAGS) blast.c unZ.c -o unZ

clean:
	rm -f unZ.exe