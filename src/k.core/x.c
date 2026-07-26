#include "x.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>

/* FUZZING-only per-eval allocation budget (companion to gk_budget's loop cap).
 * Charges bytes against gk_alloc_budget (defined in p.c, reset per top-level
 * eval in repl.c); on overrun, take xmalloc's normal OOM path -- printf+exit(1),
 * just reached in ms instead of after seconds of allocating toward the OS cap,
 * so big-structure inputs (88888888#x etc.) become fast exits, not AFL hangs.
 * Compiles to nothing without FUZZING.  exit(1) (not return 0): xmalloc never
 * returns NULL, callers don't null-check, so returning 0 would be a false crash. */
#ifdef FUZZING
extern long gk_alloc_budget;
#define GK_CHARGE(n) do{ gk_alloc_budget-=(long)(n); if(gk_alloc_budget<0){ \
  printf("wsfull\n"); exit(1); } }while(0)
#else
#define GK_CHARGE(n) ((void)0)
#endif

/*
 * compile with -DBUDDY to enable buddy allocator
 * default: use system malloc/free
 */

#ifdef BUDDY

#ifndef _WIN32
#include <sys/mman.h>
#endif

/* ASan cannot see inside this allocator: it interposes malloc/free, and the
   arena is one anonymous mmap, so every block up to 32MB -- which is every K
   object -- would have no redzone and no use-after-free trap.  The manual
   poisoning API restores both, which is what makes an ASan+BUDDY build worth
   fuzzing rather than merely running.  Detect via the COMPILER's macro, not
   gk's -DASAN_ENABLED: make.bat compiles the core files without that flag, and
   what matters here is whether the sanitizer runtime is actually linked. */
#if defined(__has_feature)
# if __has_feature(address_sanitizer)
#  define GK_ASAN 1
# endif
#endif
#ifdef __SANITIZE_ADDRESS__
# define GK_ASAN 1
#endif

#ifdef GK_ASAN
#include <sanitizer/asan_interface.h>
#define GK_POISON(p,n)   __asan_poison_memory_region((p),(n))
#define GK_UNPOISON(p,n) __asan_unpoison_memory_region((p),(n))
#else
#define GK_POISON(p,n)   ((void)0)
#define GK_UNPOISON(p,n) ((void)0)
#endif

/*
 * buddy allocator: power-of-2 blocks starting at 32 bytes
 * level 0 = 32B, level 1 = 64B, level 2 = 128B, ... level 24 = 512MB
 * stores level in first 8 bytes, returns pointer offset by 8
 * level 31 = marker for system malloc fallback
 */
/* Overridable from the command line (like BIGV) so a fuzz build can shrink the
   arena.  At the shipped 1GB the carve granule is 32MB -- buddy_alloc recurses
   to the top level before touching the arena -- so exhaustion takes 32 carves,
   while -DFUZZING caps one top-level eval at GK_ALLOC_BUDGET (64MB).  An AFL
   exec therefore never reaches the exhaustion branch, the level-degradation
   ladder below it, or the BUDDY_SYS fallback: they are dead code under the
   fuzzer.  -DBUDDY_SIZE=1048576 makes all three hot in every input; see the
   gkfb target.  BUDDY_SYS is protocol, not a tunable -- xfree tells a level
   from the system-malloc marker by value, hence the check below. */
#ifndef BUDDY_MIN
#define BUDDY_MIN 32UL
#endif
#ifndef BUDDY_LEVELS
#define BUDDY_LEVELS 21        /* max 32MB per allocation */
#endif
#ifndef BUDDY_SIZE
#define BUDDY_SIZE (1L << 30)  /* 1GB total pool */
#endif
#define BUDDY_SYS 31

#if BUDDY_MIN < 16
#error "BUDDY_MIN must hold the 8-byte header plus the 8-byte freelist link"
#endif
#if BUDDY_LEVELS < 1 || BUDDY_LEVELS > BUDDY_SYS
#error "BUDDY_LEVELS must be in 1..BUDDY_SYS-1 so xfree can distinguish a level from BUDDY_SYS"
#endif
#if BUDDY_SIZE < BUDDY_MIN
#error "BUDDY_SIZE cannot hold a single smallest block"
#endif

static uint64_t buddy_fl[BUDDY_LEVELS];  /* freelists */
static char *buddy_arena;
static size_t buddy_used;

/* compute level from size (including 8-byte header) */
/* returns BUDDY_LEVELS if size exceeds max buddy allocation */
static inline uint32_t buddy_level(size_t s) {
  /* s+8 must not wrap: for s in [SIZE_MAX-7, SIZE_MAX] it wraps to 0..7, which
     picks level 0 and hands back a 24-byte block for a ~2^64-byte request --
     silently UNDER-allocating instead of failing.  BUDDY_LEVELS routes it to
     the system-malloc arm, which fails cleanly.  This matters because -DBUDDY
     is the PRODUCTION build while gka/gkf/gkabig build without it, so ASan and
     AFL never exercise this allocator at all. */
  if(s > SIZE_MAX - 8) return BUDDY_LEVELS;
  size_t sz = s + 8;
  uint32_t lv = 0;
  while(((size_t)BUDDY_MIN << lv) < sz && lv < BUDDY_LEVELS - 1) lv++;
  /* check if size actually fits in this level */
  if(((size_t)BUDDY_MIN << lv) < sz) return BUDDY_LEVELS;
  return lv;
}

#ifdef GK_ASAN
/* The redzone is the slack between the request and the bucket, so an exact fit
   leaves no redzone at all -- and that is the common case here, not a corner:
   sizeof(ko) is 24, so EVERY K object asks for 24 and 24+8 == 32 == BUDDY_MIN
   exactly.  Likewise K lists at n = 3,7,15..., i32 vectors at n = 6,14,30...,
   and char vectors at n = 23,55,119...  Take the next bucket when that happens
   so there is always something to overflow into.  It doubles the block for
   those sizes, in ASan builds only, where memory is already multiplied.  lv may
   reach BUDDY_LEVELS, which just routes to the system-malloc arm -- ASan
   supplies its own redzone there, so the guarantee holds either way. */
static inline uint32_t buddy_level_rz(size_t s) {
  uint32_t lv = buddy_level(s);
  if(lv < BUDDY_LEVELS && ((size_t)BUDDY_MIN << lv) - 8 - s == 0) lv++;
  return lv;
}
#define BUDDY_LEVEL(s) buddy_level_rz(s)
#else
#define BUDDY_LEVEL(s) buddy_level(s)
#endif

/* allocate from level lv */
static uint64_t buddy_alloc(uint32_t lv) {
  uint64_t x;

  /* check freelist */
  if(buddy_fl[lv]) {
    x = buddy_fl[lv];
    buddy_fl[lv] = *(uint64_t*)x;   /* the link stays open while the block is
                                       free -- see buddy_free */
    return x;
  }

  /* try splitting from higher level */
  if(lv + 1 < BUDDY_LEVELS) {
    uint64_t block = buddy_alloc(lv + 1);
    if(block) {
      /* put first half on freelist, return second half */
      GK_UNPOISON((void*)block, 8);   /* allocator metadata, not user memory */
      *(uint64_t*)block = buddy_fl[lv];
      buddy_fl[lv] = block;
      return block + (BUDDY_MIN << lv);
    }
  }

  /* allocate from arena */
  if(!buddy_arena) {
#ifdef _WIN32
    buddy_arena = (char*)malloc(BUDDY_SIZE);
#else
    buddy_arena = (char*)mmap(0,BUDDY_SIZE,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(buddy_arena == MAP_FAILED) buddy_arena = 0;
#endif
    buddy_used = 0;
  }

  size_t sz = BUDDY_MIN << lv;
  if(buddy_arena && buddy_used + sz <= BUDDY_SIZE) {
    x = (uint64_t)(buddy_arena + buddy_used);
    buddy_used += sz;
    GK_POISON((void*)x, sz);   /* carve poisoned; xmalloc opens exactly the
                                  bytes it hands out.  Per-carve, not once over
                                  the whole arena: the shadow write is then
                                  proportional to memory actually used. */
    return x;
  }

  return 0;  /* out of arena memory */
}

/* free to level lv */
static inline void buddy_free(uint32_t lv, uint64_t x) {
  GK_POISON((void*)x, BUDDY_MIN << lv); /* the whole block is dead: a read or
                                           write anywhere in the user region
                                           now traps as use-after-free */
  GK_UNPOISON((void*)x, 8);             /* except the freelist link */
  *(uint64_t*)x = buddy_fl[lv];
  buddy_fl[lv] = x;
}

void* xmalloc(size_t s) {
  if(!s) s=1;
  /* The +8 header must not wrap.  buddy_level() already refuses such a size
     (returning BUDDY_LEVELS), but that only routes it to the system-malloc arm
     BELOW, which then computes `malloc(s + 8)` and wraps there instead --
     s = SIZE_MAX-3 became malloc(4) and handed back a 4-byte block for a
     ~2^64-byte request.  Guard once, here, so BOTH arms are covered. */
  if(s > SIZE_MAX - 8) { printf("wsfull\n"); exit(1); }
  GK_CHARGE(s);

  uint32_t lv = BUDDY_LEVEL(s);

  /* large allocations go to system malloc */
  if(lv >= BUDDY_LEVELS) {
    void *p = malloc(s + 8);
    if(!p) {
      printf("wsfull\n");
      exit(1);
    }
    *(uint32_t*)p = BUDDY_SYS;
    return (char*)p + 8;
  }

  uint64_t block = buddy_alloc(lv);
  if(!block) {
    /* fallback to system malloc */
    void *p = malloc(s + 8);
    if(!p) {
      printf("wsfull\n");
      exit(1);
    }
    *(uint32_t*)p = BUDDY_SYS;
    return (char*)p + 8;
  }

  GK_UNPOISON((void*)block, 8 + s);   /* header + exactly the request */
  *(uint32_t*)block = lv;
#ifdef GK_ASAN
  /* Only the low 4 bytes of the 8-byte header carry the level, so the upper 4
     are free to record the true request size.  Two uses, both ASan-only: the
     slack between the request and the bucket becomes a redzone (below), and
     xrealloc can copy just the live bytes instead of the whole bucket, which
     would otherwise read straight through that redzone.  s fits in 32 bits
     because this arm only runs when s+8 <= BUDDY_MIN<<(BUDDY_LEVELS-1). */
  *(uint32_t*)(block + 4) = (uint32_t)s;
  GK_POISON((char*)block + 8 + s, (BUDDY_MIN << lv) - 8 - s);
#endif
  return (void*)(block + 8);
}

void xfree(void *p) {
  if(!p) return;

  uint64_t base = (uint64_t)p - 8;
  uint32_t lv = *(uint32_t*)base;

  if(lv == BUDDY_SYS) {
    free((void*)base);
    return;
  }

  if(lv < BUDDY_LEVELS) {
    buddy_free(lv, base);
  }
  /* else: corrupted or invalid - ignore */
}

void* xcalloc(size_t n, size_t s) {
  /* n*s must not wrap.  libc calloc checks this, so the NON-BUDDY builds
     (gka/gkf/gkabig, MSVC) were already safe and only the BUDDY production
     build was exposed: xcalloc((size_t)1<<62,8) wrapped to 0 and returned a
     valid pointer with a matching quiet memset.  tn(0,n) reaches this for
     n >= 2^61. */
  size_t sz;
  if(s && n > SIZE_MAX / s) { printf("wsfull\n"); exit(1); }
  sz=n*s;
  void *p=xmalloc(sz);
  memset(p,0,sz);
  return p;
}

void* xrealloc(void *p, size_t s) {
  if(!p) return xmalloc(s);
  if(!s) { xfree(p); return xmalloc(1); }
  if(s > SIZE_MAX - 8) { printf("wsfull\n"); exit(1); }  /* see xmalloc: the
                              system arm below computes realloc(base, s+8) */
  GK_CHARGE(s);

  uint64_t base = (uint64_t)p - 8;
  uint32_t old_lv = *(uint32_t*)base;

  if(old_lv == BUDDY_SYS) {
    /* system malloc - use system realloc */
    void *p2 = realloc((void*)base, s + 8);
    if(!p2) {
      printf("wsfull\n");
      exit(1);
    }
    *(uint32_t*)p2 = BUDDY_SYS;
    return (char*)p2 + 8;
  }

  if(old_lv >= BUDDY_LEVELS) {
    /* corrupted - treat as new alloc */
    return xmalloc(s);
  }

  size_t old_sz = (BUDDY_MIN << old_lv) - 8;
  uint32_t new_lv = BUDDY_LEVEL(s);   /* must use the same rule as xmalloc, or
                              the same-bucket test below would disagree with the
                              level actually recorded in the header */
#ifdef GK_ASAN
  old_sz = (size_t)*(uint32_t*)(base + 4);  /* the true old request: copying the
                              whole bucket would read this block's own redzone */
#endif

  /* same bucket - no realloc needed */
  if(new_lv == old_lv) {
#ifdef GK_ASAN
    GK_UNPOISON(p, s);                      /* the live region moved, so the
                                               redzone has to move with it */
    *(uint32_t*)(base + 4) = (uint32_t)s;
    GK_POISON((char*)p + s, (BUDDY_MIN << old_lv) - 8 - s);
#endif
    return p;
  }

  /* different bucket - alloc, copy, free */
  void *p2 = xmalloc(s);
  memcpy(p2, p, old_sz < s ? old_sz : s);
  xfree(p);
  return p2;
}

#else /* !BUDDY - use system malloc */

/* A zero size is not an allocation failure.  malloc(0)/calloc(0,n) may return
   NULL, and realloc(p,0) frees p and returns NULL on glibc and MSVC alike --
   all three would trip the OOM check below and exit("wsfull").  The BUDDY
   allocator above already rounds zero up to one byte; mirror that here so an
   empty vector behaves the same in both builds (gk is BUDDY, but gka/gkf/
   gkabig and the whole MSVC build are not). */
void* xmalloc(size_t s) {
  void *p=0;
  if(!s) s=1;
  GK_CHARGE(s);
  if(!(p=malloc(s))) {
    printf("wsfull\n");
    exit(1);
  }
  return p;
}

void xfree(void *p) {
  if(p) free(p);
}

void* xcalloc(size_t n, size_t s) {
  void *p=0;
  if(!n||!s) { n=1; s=1; }
  GK_CHARGE(n*s);
  if(!(p=calloc(n,s))) {
    printf("wsfull\n");
    exit(1);
  }
  return p;
}

void* xrealloc(void *p, size_t s) {
  void *p2=0;
  if(!p) return xmalloc(s);
  if(!s) { xfree(p); return xmalloc(1); }
  GK_CHARGE(s);
  if(!(p2=realloc(p,s))) {
    printf("wsfull\n");
    exit(1);
  }
  return p2;
}

#endif /* BUDDY */

void* xstrdup(const char *s) {
  size_t n=1+strlen(s);
  void *p=xmalloc(n);
  memcpy(p,s,n);
  return p;
}

void* xstrndup(const char *s, size_t n) {
  void *p=xcalloc(n+1,1);
  memcpy(p,s,strnlen(s,n));
  return p;
}

void* xmemdup(const void *s, size_t n) {
  void *p=xmalloc(n);
  memcpy(p,s,n);
  return p;
}

/* dup for char-vector payloads: n bytes + the zero terminator cv's carry */
void* xmemdup0(const void *s, size_t n) {
  char *p=xmalloc(n+1);
  memcpy(p,s,n);
  p[n]=0;
  return p;
}

int xatoi(char *s) {
  int r;
  int64_t a;
  char *e;
  if(!s||!strlen(s)) r=INT32_MIN;
  else if(!strcmp(s,"0I")) r=INT32_MAX;
  else if(!strcmp(s,"0N")) r=INT32_MIN;
  else if(!strcmp(s,"-0N")) r=INT32_MIN;
  else if(!strcmp(s,"-0I")) r=INT32_MIN+1;
  else {
    a=strtol(s,&e,10);
    if((size_t)(e-s)!=strlen(s)) r=INT32_MIN;
    else if(a>INT32_MAX) r=INT32_MAX;
    else if(a<=INT32_MIN) r=INT32_MIN+1;  /* saturate to -0I: the null pattern is
                                             reserved for 0N and parse failure,
                                             and the lexer clamps the same way */
    else r=a;
  }
  return r;
}

int64_t xatol(char *s) {
  int64_t r;
  char *e;
  if(!s||!strlen(s)) r=INT64_MIN;
  else if(!strcmp(s,"0I")) r=INT64_MAX;
  else if(!strcmp(s,"0N")) r=INT64_MIN;
  else if(!strcmp(s,"-0N")) r=INT64_MIN;
  else if(!strcmp(s,"-0I")) r=INT64_MIN+1;
  else {
    r=strtoll(s,&e,10);
    if((size_t)(e-s)!=strlen(s)) r=INT64_MIN;
    else if(r==INT64_MIN) r=INT64_MIN+1;  /* saturate to -0I, as xatoi does:
                                             INT64_MIN is 0N / parse failure */
  }
  return r;
}

double xstrtod(char *s) {
  double r;
  char *e;
  if(!s||!strlen(s)) r=NAN;
  else if(!strcmp(s,"0i")||!strcmp(s,"0I")) r=INFINITY;
  else if(!strcmp(s,"0n")||!strcmp(s,"0N")) r=NAN;
  else if(!strcmp(s,"-0n")||!strcmp(s,"-0N")) r=NAN;
  else if(!strcmp(s,"-0i")||!strcmp(s,"-0I")) r=-INFINITY;
  else {
    r=strtod(s,&e);
    if((size_t)(e-s)!=strlen(s)) r=NAN;
  }
  return r;
}

/* f32 text: what xstrtod accepts, plus the f32 spellings the lexer takes and
   the printer emits -- 0ne/0ie/-0ie, and a trailing `e` suffix reached through
   a decimal point or exponent ("3.5e", "1e0e", "2.147484e+09e"), never off a
   bare integer: "5e" is 5 then the name e, and stays a parse failure here
   too.  Failure is NaN (0ne), like xstrtod. */
float xstrtoe(char *s) {
  double r;
  size_t l;
  char *m;
  if(!s||!(l=strlen(s))) return NAN;
  if(!strcmp(s,"0ne")||!strcmp(s,"-0ne")) return NAN;
  if(!strcmp(s,"0ie")) return INFINITY;
  if(!strcmp(s,"-0ie")) return -INFINITY;
  r=xstrtod(s);
  if(isnan(r)&&'e'==s[l-1]&&(memchr(s,'.',l-1)||memchr(s,'e',l-1)||memchr(s,'E',l-1))) {
    m=xmalloc(l);
    memcpy(m,s,l-1);
    m[l-1]=0;
    r=xstrtod(m);
    xfree(m);
  }
  return (float)r;
}

char* xesc(char *p) {
  char *ss;
  int n,i,j=0;
  if(!p) return 0;
  n=strlen(p);
  ss=xmalloc(1+strlen(p)*4);
  for(i=0;i<n;i++) {
    if(p[i]<32||p[i]>126) {
      if(p[i]=='\b') { ss[j++]='\\'; ss[j++]='b'; }
      else if(p[i]=='\t') { ss[j++]='\\'; ss[j++]='t'; }
      else if(p[i]=='\n') { ss[j++]='\\'; ss[j++]='n'; }
      else if(p[i]=='\r') { ss[j++]='\\'; ss[j++]='r'; }
      else j+=sprintf(&ss[j],"\\%03o",(unsigned char)p[i]);
    }
    else {
      if(p[i]=='"') { ss[j++]='\\'; ss[j++]='"'; }
      else if(p[i]=='\\') { ss[j++]='\\'; ss[j++]='\\'; }
      else ss[j++]=p[i];
    }
  }
  ss[j]=0;
  return ss;
}

char* xunesc(char *p) {
  char *ss;
  int n,i,j=0,s=0;
  unsigned char o;
  if(!p) return 0;
  n=strlen(p);
  ss=xmalloc(1+n);
  for(i=0;i<n;i++) {
    switch(s) {
    case 0:
      if(*p=='\\') s=1;
      else ss[j++]=*p;
      break;
    case 1:
      switch(*p) {
        case 'b': ss[j++]='\b'; s=0; break;
        case 't': ss[j++]='\t'; s=0; break;
        case 'n': ss[j++]='\n'; s=0; break;
        case 'r': ss[j++]='\r'; s=0; break;
        case '"': ss[j++]='"';  s=0; break;
        case '\\': ss[j++]='\\'; s=0; break;
        default:
          if(isdigit((unsigned char)*p) && *p<='7') { o=*p-'0'; s=2; }
          else { ss[j++]=*p; s=0; }
          break;
      }
      break;
    case 2: /* octal */
      if(isdigit((unsigned char)*p)&&*p<='7') { o*=8; o+=*p-'0'; s=3; }
      else if(*p=='\\') { ss[j++]=o; s=1; }
      else { ss[j++]=o; ss[j++]=*p; s=0; }
      break;
    case 3: /* octal */
      if(isdigit((unsigned char)*p)&&*p<='7') { o*=8; o+=*p-'0'; ss[j++]=o; s=0; }
      else if(*p=='\\') { ss[j++]=o; s=1; }
      else { ss[j++]=o; ss[j++]=*p; s=0; }
      break;
    default: return 0; /* error */
    }
    ++p;
  }
  if(s==2||s==3) ss[j++]=o;
  ss[j]=0;
  return ss;
}

char* xeqs(char *p) {
  if(!*p++) return 0;
  while(*p) {
    if(*p=='\\') { ++p; if(*p) ++p; }
    else if(*p=='"') { ++p; break; }
    else ++p;
  }
  return p;
}

const char* xstrerror(int e) {
#ifdef _WIN32
  static char b[256];
  strerror_s(b,sizeof(b),e);
  return b;
#else
  return strerror(e);
#endif
}

uint64_t xfnv1a(char *v, uint64_t n) {
  uint64_t h=0xcbf29ce484222325;
  while(n--) {
    h^=*v++;
    h*=0x00000100000001b3;
  }
  return h;
}
