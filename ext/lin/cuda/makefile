GKDIR   ?= ../../..
GK      := $(GKDIR)/gk

CUDA    ?= /usr/local/cuda
NVCC    ?= nvcc
CC      ?= cc
NVFLAGS ?= -O3 -shared -Xcompiler -fPIC -I$(GKDIR)
CFLAGS  ?= -O2 -fPIC -Wall -I$(GKDIR) -I$(CUDA)/include
LDLIBS  := -lcublas -lnvrtc -lcuda

all: cuda.so

# cuda.cu: f32 matmul via cuBLAS (gmul, ginit).  exec.c: a scalar lambda as a kernel via NVRTC (exec, src, stats, info).
cuda.so: cuda.cu exec.o $(GKDIR)/gk.h
	$(NVCC) $(NVFLAGS) cuda.cu exec.o -o $@ $(LDLIBS)

exec.o: exec.c $(GKDIR)/gk.h
	$(CC) $(CFLAGS) -c exec.c -o $@

test: cuda.so
	$(GK) -q test.k

clean:
	rm -f cuda.so exec.o

.PHONY: all test clean
