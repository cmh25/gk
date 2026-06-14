GKDIR   ?= ../../..
GK      := $(GKDIR)/gk

NVCC    ?= nvcc
NVFLAGS ?= -O3 -shared -Xcompiler -fPIC -I$(GKDIR)
LDLIBS  := -lcublas

all: cuda.so

cuda.so: cuda.cu $(GKDIR)/gk.h
	$(NVCC) $(NVFLAGS) cuda.cu -o $@ $(LDLIBS)

test: cuda.so
	$(GK) -q test.k

clean:
	rm -f cuda.so

.PHONY: all test bench clean
