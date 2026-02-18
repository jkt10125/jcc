CC=gcc
CFLAGS=-std=c99 -O2
SRCS=$(wildcard src/*.c)
OBJS=$(SRCS:.c=.o)

all: jcc

jcc: $(SRCS)
	$(CC) $(CFLAGS) -o jcc $(SRCS)

clean:
	rm -f jcc prog.s prog.o rt.o a.out

test: jcc
	./jcc -m 1024 examples/add.j
	./a.out
	echo exit:$?

