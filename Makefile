CFLAGS=-std=c99 -pedantic -Wall -Wextra -O -D_POSIX_SOURCE -D_GNU_SOURCE -D_POSIX_C_SOURCE=199309L
LDFLAGS=-lrt

all: magicSmoke

magicSmoke: magicSmoke.o

clean:
	rm -f *.o magicSmoke
