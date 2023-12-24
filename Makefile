CC=gcc
#CFLAGS=-g -O2 -Wall -Wextra -Wformat=2
CFLAGS=-O3

all: gk

gk: main.c p.o timer.o node.o x.o ops.o sym.o sort.o k.o dict.o av.o sys.o scope.o fn.o avopt.o io.o adcd.o interp.o
	$(CC) $(CFLAGS) -o gk main.c p.o timer.o node.o x.o ops.o sym.o sort.o k.o dict.o av.o sys.o scope.o fn.o avopt.o io.o adcd.o interp.o -lm

gkd:
	$(MAKE) CFLAGS="-g -Wunused-variable -Wunused-but-set-parameter"

test: gk
	$(MAKE) -C test

testv: gk
	$(MAKE) -C test testv

clean:
	rm -f gk *.o

.PHONY: test testv
