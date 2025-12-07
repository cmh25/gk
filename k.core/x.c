#include "x.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>

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

void* xstrdup(const char *s) {
  void *p=0;
  size_t n=1+strlen(s);
  if(!(p=xmalloc(n))) {
    printf("error: xstrdup(): memory allocation failed\n");
    exit(1);
  }
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
