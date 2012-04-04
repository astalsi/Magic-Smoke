CFLAGS=-std=c99 -pedantic -Wall -Wextra -O

all: magicSmoke.o
	
clean:
	rm -f *.o magicSmoke
