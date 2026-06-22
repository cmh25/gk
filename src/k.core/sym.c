#include "sym.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "x.h"

static char **sh;
static int j, k;

static inline unsigned hs(const char *s) {
  unsigned int h=0,c;
  while((c=(unsigned char)*s++)) h=(h<<5)-h+c;
  return 16807u*h;
}

char *sp(char *s) {
  char *t,**p;
  int i;
  unsigned int h=hs(s);
  size_t len;

  // 1. Lookup existing symbol
  if(k) {
    for(;;++h) {
      t=sh[h&(k-1)];
      if(!t) break;                 // empty slot -> not found
      if(!strcmp(s,t)) return t;    // found
    }
  }

  // 2. Grow table if needed (same j/k logic as before)
  if(j==k) {
    int oldk=k;
    k=k?k*2:2;
    p=xcalloc((size_t)k,sizeof(char*));
    if(!p) { fprintf(stderr, "sp\n"); exit(1); }

    // rehash old entries
    for(i=0;i<j;++i) {
      t=sh?sh[i]:0;
      if(t) {
        unsigned int hh=hs(t);
        while(p[hh&(k-1)]) ++hh;
        p[hh&(k-1)]=t;
      }
    }

    if(oldk) xfree(sh);
    sh=p;

    // recompute probe position for s on new table
    h=hs(s);
    while(sh[h&(k-1)]) ++h;
  }

  // 3. Insert new symbol
  len=strlen(s);
  t=xmalloc(len+1);
  if(!t) { fprintf(stderr, "sp\n"); exit(1); }
  memcpy(t,s,len+1);

  j+=2;
  sh[h&(k-1)]=t;
  return t;
}

void sf(void) {
  int i;
  if(!sh) return;
  for(i=0;i<k;++i)
    if(sh[i]) xfree(sh[i]);
  xfree(sh);
  sh=0;
  j=k=0;
}
