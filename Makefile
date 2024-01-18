CC=gcc
CFLAGS=-O3

all: gk

gk: main.c p.o timer.o node.o x.o ops.o sym.o sort.o k.o dict.o av.o sys.o scope.o fn.o avopt.o io.o adcd.o interp.o
	$(CC) $(CFLAGS) -o gk main.c p.o timer.o node.o x.o ops.o sym.o sort.o k.o dict.o av.o sys.o scope.o fn.o avopt.o io.o adcd.o interp.o -lm

gkd:
	$(MAKE) CFLAGS="-g -Wall -Wformat=2 -Wextra -Wformat-security -Wformat-nonliteral -Wno-unused-parameter"

test: gk
	$(MAKE) -C test

testv: gk
	$(MAKE) -C test testv

clean:
	rm -f gk *.o

.PHONY: test testv
