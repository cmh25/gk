CC=gcc -O3 -march=native -flto -DBUDDY
CCD=gcc -g -Wall -Wformat=2 -Wextra -Wformat-security -Wno-format-nonliteral -Wpedantic
CFILES=k.c v.c av.c sort.c rand.c sym.c x.c
OFILES=k.o v.o av.o sort.o rand.o sym.o x.o

all: k

k:
	$(CC) -c $(CFILES) -fPIC
	ar r k.a $(OFILES)

kd:
	$(CCD) -c $(CFILES) -fPIC
	ar r kd.a $(OFILES)

clean:
	rm -f *.a *.o

.PHONY: all k kd clean
