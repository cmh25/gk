GKDIR  ?= ../../..
GK     := $(GKDIR)/gk

CC     ?= cc
CFLAGS ?= -O2 -shared -fPIC -I$(GKDIR)
LDLIBS := -lgmp

all: bignum.so

bignum.so: bignum.c $(GKDIR)/gk.h
	$(CC) $(CFLAGS) bignum.c -o $@ $(LDLIBS)

test: bignum.so
	$(GK) -q test.k

clean:
	rm -f bignum.so

.PHONY: all test clean
