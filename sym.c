#include "sym.h"
#include <string.h>
#include <stdio.h>
#include "x.h"

int hs(char *s) {
  int h=0;
  while(*s) h=(h<<5)-h+*s++;
  return 16807*h;
}

char* sp(char *s) {
  static char **sh;
  static int i,j,k;
  int h = hs(s);
  char **p,*t;
  if(k) for(;(t=sh[h&(k-1)]);++h) if(!strcmp(s,t)) return t;
  if(j==k) {
    k=k?k*2:2;
    if(!(p=xmalloc(k*sizeof(char*)))){fprintf(stderr,"sp");exit(1);}
    for(i=0;i<k;i++) p[i]=0;
    for(i=0;i<j;i++) { if((t=sh[i])) { for(h=hs(t);(p[h&(k-1)]);++h){}; p[h&(k-1)]=t; } }
    if(j) xfree(sh);
    sh=p;
    for(h=hs(s);(sh[h&(k-1)]);++h);
  }
  if(!(t=xmalloc(1+strlen(s)))){fprintf(stderr,"sp");exit(1);}
  return j+=2,sh[h&(k-1)]=strcpy(t,s);
}
