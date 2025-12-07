#include "ms.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static char *b;
static size_t l, c;

int mprintf(const char *f, ...) {
  va_list a,a2;
  int n;
  size_t nc;
  char *t=0;
  va_start(a,f);
  va_copy(a2,a);
  n=vsnprintf(0,0,f,a2);
  va_end(a2);
  if(n<0) { va_end(a); return -1; }
  if(l+n+1>c) {
    nc=c?c*2:32;
    while(nc<l+n+1) nc*=2;
    t=realloc(b,nc);
    if(!t) { va_end(a); return -1; }
    b=t;
    c=nc;
  }
  vsnprintf(b+l,c-l,f,a);
  va_end(a);
  l+=n;
  return n;
}

const char *mbuffer(void) { return b?b:""; }
void mreset(void) { l=0; if(b) b[0]=0; }
void mfree(void) { free(b); b=0; l=c=0; }
