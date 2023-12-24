#include "x.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

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
  if(!(p=calloc(n, s))) {
    printf("error: xcalloc(): memory allocation failed\n");
    exit(1);
  }
  return p;
}

void* xrealloc(void *p, size_t s) {
  void *p2=0;
  if(!(p2=realloc(p, s))) {
    printf("error: xrealloc(): memory allocation failed\n");
    exit(1);
  }
  return p2;
}

void* xstrdup(const char *s) {
  void *p=0;
  if(!(p=strdup(s))) {
    printf("error: xstrdup(): memory allocation failed\n");
    exit(1);
  }
  return p;
}

void* xstrndup(const char *s, size_t n) {
  void *p=xcalloc(n+1,1);
  memcpy(p,s,n);
  return p;
}

int xatoi(char *s) {
  int r;
  long a;
  if(!strcmp(s,"0I")) r=INT_MAX;
  else if(!strcmp(s,"0N")) r=INT_MIN;
  else if(!strcmp(s,"-0N")) r=INT_MIN;
  else if(!strcmp(s,"-0I")) r=INT_MIN+1;
  else {
    a=strtol(s,0,10);
    if(a>INT_MAX) r=INT_MAX;
    else if(a<INT_MIN) r=INT_MIN;
    else r=a;
  }
  return r;
}

double xstrtod(char *s, char **e) {
  double r;
  if(!strcmp(s,"0i")||!strcmp(s,"0I")) r=INFINITY;
  else if(!strcmp(s,"0n")||!strcmp(s,"0N")) r=NAN;
  else if(!strcmp(s,"-0n")||!strcmp(s,"-0N")) r=NAN;
  else if(!strcmp(s,"-0i")||!strcmp(s,"-0I")) r=-INFINITY;
  else r = strtod(s,e);
  return r;
}

uint64_t xfnv1a(char *v, uint64_t n) {
  uint64_t h=0xcbf29ce484222325;
  while(n--) {
    h^=*v++;
    h*=0x00000100000001b3;
  }
  return h;
}

char* xesc(char *s) {
  int i,j=0,n;
  char *esc=xmalloc(1+strlen(s)*2);
  if(!s) return 0;
  n=strlen(s);
  for(i=0;i<n;i++) {
    if(s[i]<32||s[i]>126) {
      if(s[i]=='\b') { esc[j++]='\\'; esc[j++]='b'; }
      else if(s[i]=='\t') { esc[j++]='\\'; esc[j++]='t'; }
      else if(s[i]=='\n') { esc[j++]='\\'; esc[j++]='n'; }
      else if(s[i]=='\r') { esc[j++]='\\'; esc[j++]='r'; }
      else { esc[j++]='\\'; esc[j++]=s[i]; }
    } else {
      if(s[i]=='"') { esc[j++]='\\'; esc[j++]='"'; }
      else if(s[i]=='\\') { esc[j++]='\\'; esc[j++]='\\'; }
      else esc[j++]=s[i];
    }
  }
  esc[j]=0;
  return esc;
}
char* xunesc(char *s) {
  int i,j=0,n;
  char *esc=xmalloc(1+strlen(s));
  if(!s) return 0;
  n=strlen(s);
  for(i=0;i<n;i++) {
    if(s[i]=='\\') {
      i++;
      if(s[i]=='b') esc[j++]='\b';
      else if(s[i]=='t') esc[j++]='\t';
      else if(s[i]=='n') esc[j++]='\n';
      else if(s[i]=='r') esc[j++]='\r';
      else if(s[i]=='"') esc[j++]='\"';
      else if(s[i]=='\\') esc[j++]='\\';
    }
    else esc[j++]=s[i];
  }
  esc[j]=0;
  return esc;
}
