#include "lzw.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ht.h"

//#define MAX 1024
//#define MAX 2048
//#define MAX 4096
//#define MAX 8192
//#define MAX 16384
#define MAX 32768
//#define MAX 65536
//#define MAX 131072
//#define MAX 262144
//#define MAX 524288

static char *D[MAX];
static int L[MAX],N=257;

static void bfa(LZW *b, int n, int w) {
  size_t i=b->i;
  unsigned char *p;
  b->i+=w;
  if(b->n<b->i/8+(b->i%8?1:0)) {
    p=calloc(b->n<<=1,1);
    memcpy(p,b->b,b->n>>1);
    free(b->b);
    b->b=p;
  }
  for(;i<b->i;i++) {
    if(n&1<<--w) b->b[i/8]|=(1<<(i%8));
    else b->b[i/8]&=~(1<<(i%8));
  }
  b->c++;
}

static int bfr(LZW *b, int i, int w) {
  int r;
  for(r=0;0<=--w;++i) if(b->b[i/8]&(1<<(i%8))) r |= 1<<w;
  return r;
}

LZW* lzwnew(void) {
  LZW *r=calloc(sizeof(LZW),1);
  r->b=calloc(32,1);
  r->n=32;
  r->v=0;
  return r;
}

void lzwfree(LZW *b) {
  free(b->b);
  free(b);
}

LZW* lzwc(char *s, size_t n) {
  size_t i;
  int j,k,w=9,p=0;
  LZW *r=lzwnew();
  HT *ht=htnew(MAX*2);
  N=257;
  while(n) {
    j=(unsigned char)*s;
    for(i=1;i+1<=n;++i) { if(-1==(k=htget(ht,s,i+1))) break; else j=k; }
    if(i==1) ++p;
    if(N>1<<w) ++w;
    bfa(r,j,w);
    n-=i;
    if(n) {
      htset(ht,s,1+i,N++);
      s+=i;
      if(N==MAX||p==512) {
        bfa(r,256,w);
        htfree(ht);
        ht=htnew(MAX*2);
        N=257;
        w=9;
        p=0;
      }
    }
  }
  htfree(ht);
  return r;
}

LZW* lzwd(LZW *z) {
  int j,w=9;
  size_t i,rn=0,k=0,pl=0;
  char *p=0;
  LZW *r=lzwnew();
  N=257;
  if(!z) { fprintf(stderr,"corrupted input!\n"); exit(1); }
  if(!z->b) { fprintf(stderr,"corrupted input!\n"); exit(1); }
  if(z->v) { fprintf(stderr,"corrupted input!\n"); exit(1); }
  for(i=0;i<z->c;++i) {
    if(N==1<<w) ++w;
    j=bfr(z,k,w);
    k+=w;
    if(j==256) {
      while(N>257) free(D[--N]);
      w=9;
      if(p) free(p);
      p=0;
    }
    else if(j<N) {
      if(j<256 && !D[j]) { D[j]=malloc(1); D[j][0]=j; L[j]=1; }
      if(r->n<rn+L[j]+1) r->b=realloc(r->b,r->n<<=1);
      memcpy(r->b+rn,D[j],L[j]);
      rn+=L[j];
      if(p) {
        p[pl]=D[j][0];
        D[N]=p;
        L[N++]=++pl;
      }
      p=malloc(2+L[j]);
      memcpy(p,D[j],L[j]);
      pl=L[j];
    }
    else {
      if(!p) { fprintf(stderr,"corrupted input!\n"); exit(1); }
      p[pl++]=p[0];
      p=realloc(p,pl+2);
      D[N]=malloc(pl);
      memcpy(D[N],p,pl);
      L[N++]=pl;
      if(r->n<rn+pl) r->b=realloc(r->b,r->n<<=1);
      memcpy(r->b+rn,p,pl);
      rn+=pl;
    }
  }
  while(N>257) free(D[--N]);
  if(p) free(p);
  r->n=rn;
  return r;
}
