CORE=k.core
CC=gcc -O3 -march=native -flto -DBUDDY
CCD=gcc -g -Wall -Wformat=2 -Wextra -Wformat-security -Wno-format-nonliteral -Wpedantic
CFILES=p.c lex.c timer.c k.c main.c repl.c dict.c scope.c fn.c b.c v.c av.c ms.c h.c pnp.c fe.c lzw.c md5.c sha1.c sha2.c aes256.c io.c irecur.c la.c nt.c ipc.c tmr.c watch.c
COREFILES=$(CORE)/k.c $(CORE)/v.c $(CORE)/av.c $(CORE)/sort.c $(CORE)/rand.c $(CORE)/sym.c $(CORE)/x.c

all: gk

gk:
	$(CC) -ogk $(CFILES) $(COREFILES) -lm

gkd:
	$(CCD) -ogk $(CFILES) $(COREFILES) -lm

test:
	@test -f gk || $(MAKE) gk
	$(MAKE) -C t

testv:
	@test -f gk || $(MAKE) gkd
	$(MAKE) -C t testv

testp:
	@test -f gk || $(MAKE) gk
	$(MAKE) -C t testp

clean:
	$(MAKE) -C t clean
	rm -f gk *.o $(CORE)/*.o $(CORE)/*.a

.PHONY: all gk gkd test testv testp clean
