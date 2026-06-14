GKDIR  ?= ../../..
GK     := $(GKDIR)/gk

CC     ?= cc
CFLAGS ?= -O2 -shared -fPIC -undefined dynamic_lookup -I$(GKDIR)
LDLIBS := -framework Accelerate

all: accelerate.dylib

accelerate.dylib: accelerate.c $(GKDIR)/gk.h
	$(CC) $(CFLAGS) accelerate.c -o $@ $(LDLIBS)

test: accelerate.dylib
	$(GK) -q test.k

clean:
	rm -f accelerate.dylib

.PHONY: all test clean
