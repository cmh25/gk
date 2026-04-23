CC=gcc -O3 -march=native -flto
CCD=gcc -g -Wall -Wformat=2 -Wextra -Wformat-security -Wno-format-nonliteral -Wpedantic
CCA=afl-clang-fast -DASAN_ENABLED -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls -shared-libasan
CCF=afl-clang-fast -g -O2
CFILES=k.c v.c av.c sort.c rand.c sym.c x.c
OFILES=k.o v.o av.o sort.o rand.o sym.o x.o

all: k

k:
	$(CC) -c $(CFILES) -fPIC
	ar r k.a $(OFILES)

kd:
	$(CCD) -c $(CFILES) -fPIC
	ar r kd.a $(OFILES)

ka:
	$(CCA) -c $(CFILES) -fPIC
	ar r ka.a $(OFILES)

kf:
	$(CCF) -c $(CFILES) -fPIC
	ar r kf.a $(OFILES)

analyze:
	@for file in $(CFILES); do \
		echo "clang --analyze -o /dev/null $$file"; \
		clang --analyze -o /dev/null $$file; \
	done

clean:
	rm -f *.a *.o

.PHONY: all k kd ka kf analyze clean
