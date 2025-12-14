#include "x.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>

/*
 * Compile with -DUSE_BUDDY to enable buddy allocator
 * Default: use system malloc/free
 */

#ifdef USE_BUDDY

#ifndef _WIN32
#include <sys/mman.h>
#endif

/*
 * Buddy allocator: power-of-2 blocks starting at 32 bytes
 * Level 0 = 32B, level 1 = 64B, level 2 = 128B, ... level 24 = 512MB
 * Stores level in first 8 bytes, returns pointer offset by 8
 * Level 31 = marker for system malloc fallback
 */
#define BUDDY_MIN 32UL
#define BUDDY_LEVELS 21              /* max 32MB per allocation */
#define BUDDY_SYS 31
#define BUDDY_ARENA_SIZE (1L << 30)  /* 1GB total pool */

static uint64_t buddy_fl[BUDDY_LEVELS];  /* freelists */
static char *buddy_arena = NULL;
static size_t buddy_used = 0;

/* Compute level from size (including 8-byte header) */
static inline uint32_t buddy_level(size_t s) {
  size_t sz = s + 8;
  uint32_t lv = 0;
  while(((size_t)BUDDY_MIN << lv) < sz && lv < BUDDY_LEVELS - 1) lv++;
  return lv;
}

/* Allocate from level lv */
static uint64_t buddy_alloc(uint32_t lv) {
  uint64_t x;

  /* Check freelist */
  if(buddy_fl[lv]) {
    x = buddy_fl[lv];
    buddy_fl[lv] = *(uint64_t*)x;
    return x;
  }

  /* Try splitting from higher level */
  if(lv + 1 < BUDDY_LEVELS) {
    uint64_t block = buddy_alloc(lv + 1);
    if(block) {
      /* Put first half on freelist, return second half */
      *(uint64_t*)block = buddy_fl[lv];
      buddy_fl[lv] = block;
      return block + (BUDDY_MIN << lv);
    }
  }

  /* Allocate from arena */
  if(!buddy_arena) {
#ifdef _WIN32
    buddy_arena = (char*)malloc(BUDDY_ARENA_SIZE);
#else
    buddy_arena = (char*)mmap(NULL, BUDDY_ARENA_SIZE,
                              PROT_READ|PROT_WRITE,
                              MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if(buddy_arena == MAP_FAILED) buddy_arena = NULL;
#endif
    buddy_used = 0;
  }

  size_t sz = BUDDY_MIN << lv;
  if(buddy_arena && buddy_used + sz <= BUDDY_ARENA_SIZE) {
    x = (uint64_t)(buddy_arena + buddy_used);
    buddy_used += sz;
    return x;
  }

  return 0;  /* Out of arena memory */
}

/* Free to level lv */
static inline void buddy_free(uint32_t lv, uint64_t x) {
  *(uint64_t*)x = buddy_fl[lv];
  buddy_fl[lv] = x;
}

void* xmalloc(size_t s) {
  if(!s) s = 1;

  uint32_t lv = buddy_level(s);

  /* Large allocations go to system malloc */
  if(lv >= BUDDY_LEVELS) {
    void *p = malloc(s + 8);
    if(!p) {
      printf("error: xmalloc(): memory allocation failed\n");
      exit(1);
    }
    *(uint32_t*)p = BUDDY_SYS;
    return (char*)p + 8;
  }

  uint64_t block = buddy_alloc(lv);
  if(!block) {
    /* Fallback to system malloc */
    void *p = malloc(s + 8);
    if(!p) {
      printf("error: xmalloc(): memory allocation failed\n");
      exit(1);
    }
    *(uint32_t*)p = BUDDY_SYS;
    return (char*)p + 8;
  }

  *(uint32_t*)block = lv;
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
  size_t sz = n * s;
  void *p = xmalloc(sz);
  memset(p, 0, sz);
  return p;
}

void* xrealloc(void *p, size_t s) {
  if(!p) return xmalloc(s);
  if(!s) { xfree(p); return xmalloc(1); }

  uint64_t base = (uint64_t)p - 8;
  uint32_t old_lv = *(uint32_t*)base;

  if(old_lv == BUDDY_SYS) {
    /* System malloc - use system realloc */
    void *p2 = realloc((void*)base, s + 8);
    if(!p2) {
      printf("error: xrealloc(): memory allocation failed\n");
      exit(1);
    }
    *(uint32_t*)p2 = BUDDY_SYS;
    return (char*)p2 + 8;
  }

  if(old_lv >= BUDDY_LEVELS) {
    /* Corrupted - treat as new alloc */
    return xmalloc(s);
  }

  size_t old_sz = (BUDDY_MIN << old_lv) - 8;
  uint32_t new_lv = buddy_level(s);

  /* Same bucket - no realloc needed */
  if(new_lv == old_lv) return p;

  /* Different bucket - alloc, copy, free */
  void *p2 = xmalloc(s);
  memcpy(p2, p, old_sz < s ? old_sz : s);
  xfree(p);
  return p2;
}

#else /* !USE_BUDDY - use system malloc */

void* xmalloc(size_t s) {
  void *p=0;
  if(!(p=malloc(s))) {
    printf("error: xmalloc(): memory allocation failed\n");
    exit(1);
  }
  return p;
}

void xfree(void *p) {
  if(p) free(p);
}

void* xcalloc(size_t n, size_t s) {
  void *p=0;
  if(!(p=calloc(n,s))) {
    printf("error: xcalloc(): memory allocation failed\n");
    exit(1);
  }
  return p;
}

void* xrealloc(void *p, size_t s) {
  void *p2=0;
  if(!(p2=realloc(p,s))) {
    printf("error: xrealloc(): memory allocation failed\n");
    exit(1);
  }
  return p2;
}

#endif /* USE_BUDDY */

void* xstrdup(const char *s) {
  size_t n = 1 + strlen(s);
  void *p = xmalloc(n);
  memcpy(p, s, n);
  return p;
}

void* xstrndup(const char *s, size_t n) {
  void *p = xcalloc(n + 1, 1);
  memcpy(p, s, strnlen(s, n));
  return p;
}

void* xmemdup(const void *s, size_t n) {
  void *p = xmalloc(n);
  memcpy(p, s, n);
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
    else if(a<INT32_MIN) r=INT32_MIN;
    else r=a;
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
    } else {
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
          if(isdigit(*p) && *p<='7') { o=*p-'0'; s=2; }
          else { ss[j++]=*p; s=0; }
          break;
      }
      break;
    case 2: /* octal */
      if(isdigit(*p)&&*p<='7') { o*=8; o+=*p-'0'; s=3; }
      else if(*p=='\\') { ss[j++]=o; s=1; }
      else { ss[j++]=o; ss[j++]=*p; s=0; }
      break;
    case 3: /* octal */
      if(isdigit(*p)&&*p<='7') { o*=8; o+=*p-'0'; ss[j++]=o; s=0; }
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
