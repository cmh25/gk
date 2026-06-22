SRC=src
CORE=$(SRC)/k.core
MEM=-DBUDDY

# MSYS2 CLANG64/CLANG32/CLANGARM64
CCBIN ?= gcc
ifneq (,$(filter CLANG%,$(MSYSTEM)))
CCBIN=clang
endif

LTO := $(shell $(CCBIN) -flto=auto -E -x c /dev/null >/dev/null 2>&1 && echo -flto=auto || echo -flto)
CC=$(CCBIN) -O3 -march=native $(LTO) $(MEM) -I.
CCD=gcc -g -Wall -Wformat=2 -Wextra -Wformat-security -Wno-format-nonliteral -Wpedantic -I.
CCA=afl-clang-fast -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls -shared-libasan -I.
CCF=afl-clang-fast -g -O2 -I.
CFILES=$(addprefix $(SRC)/,p.c lex.c timer.c k.c main.c repl.c dict.c scope.c fn.c b.c v.c av.c ms.c h.c pnp.c fe.c lzw.c md5.c sha1.c sha2.c aes256.c io.c irecur.c la.c nt.c ipc.c tmr.c watch.c ffi.c)
COREFILES=$(CORE)/k.c $(CORE)/v.c $(CORE)/av.c $(CORE)/sort.c $(CORE)/rand.c $(CORE)/sym.c $(CORE)/x.c

ifeq ($(shell uname -s),Linux)
DL=-ldl
DLEXPORT=-Wl,--dynamic-list=ffi.list
endif

ifeq ($(shell uname -s),Darwin)
DLEXPORT=-Wl,-export_dynamic
endif

UNAME_S:=$(shell uname -s)
ifneq (,$(findstring CYGWIN,$(UNAME_S))$(findstring MSYS,$(UNAME_S))$(findstring MINGW,$(UNAME_S)))
DLEXPORT=-Wl,--out-implib,libgk.dll.a -Wl,--stack,0x800000
endif

ifneq (,$(findstring MINGW,$(UNAME_S)))
CFILES += $(SRC)/systime.c
DL = -lws2_32
MEM=
endif

all: gk

gk:
	$(CC) -ogk $(CFILES) $(COREFILES) -lm $(DL) $(DLEXPORT)

# CR-tolerant comparator used by tests
ndiff: $(SRC)/ndiff.c
	cc -O2 -o ndiff $(SRC)/ndiff.c

gkd:
	$(CCD) -ogk $(CFILES) $(COREFILES) -lm $(DL) $(DLEXPORT)

test:
	@test -f gk || $(MAKE) gk
	@test -f ndiff || $(MAKE) ndiff
	cd t && ./t.sh

testv:
	@test -f gk || $(MAKE) gkd
	cd t && ./v.sh

testp:
	@test -f gk || $(MAKE) gk
	cd t && ./p.sh

clean:
	rm -f t/[0-9]* t/[dev][0-9]*
	rm -f t64/[0-9]* t64/[de][0-9]*
	rm -f gk ndiff libgk.dll.a *.o

.PHONY: all gk gkd test testv testp clean
