CFLAGS=-std=c99 -pedantic -Wall -Wextra -O -D_POSIX_SOURCE

all: magicSmoke

magicSmoke: magicSmoke.o

clean:
	rm -f *.o magicSmoke
