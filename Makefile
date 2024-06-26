CC=gcc
CFLAGS=-O3

all: gk

gk: main.c p.o timer.o node.o x.o ops.o sym.o sort.o k.o dict.o av.o sys.o scope.o fn.o avopt.o io.o adcd.o interp.o rand.o os.o md5.o sha1.o sha2.o
	$(CC) $(CFLAGS) -o gk main.c p.o timer.o node.o x.o ops.o sym.o sort.o k.o dict.o av.o sys.o scope.o fn.o avopt.o io.o adcd.o interp.o rand.o os.o md5.o sha1.o sha2.o -lm

gkd:
	$(MAKE) CFLAGS="-g -Wall -Wformat=2 -Wextra -Wformat-security -Wformat-nonliteral -Wpedantic"

test: gk
	$(MAKE) -C test

testv: gk
	$(MAKE) -C test testv

clean:
	rm -f gk *.o

.PHONY: test testv
