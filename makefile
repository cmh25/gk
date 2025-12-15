CORE=k.core
export CORE
CC=gcc -O3 -march=native -flto
CCD=gcc -g -Wall -Wformat=2 -Wextra -Wformat-security -Wno-format-nonliteral -Wpedantic
CFILES=p.c lex.c timer.c k.c main.c repl.c dict.c scope.c fn.c b.c v.c av.c ms.c h.c pnp.c fe.c lzw.c md5.c sha1.c sha2.c aes256.c io.c irecur.c la.c

all: gk

gk:
	$(MAKE) -C $(CORE) k
	$(CC) -ogk $(CFILES) $(CORE)/k.a -lm

gkd:
	$(MAKE) -C $(CORE) kd
	$(CCD) -ogk $(CFILES) $(CORE)/kd.a -lm

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
	$(MAKE) -C $(CORE) clean
	rm -f gk *.o

.PHONY: all gk gkd test testv testp clean
