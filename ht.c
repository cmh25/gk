#include "ht.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "timer.h"

static int hs(char *k, int n) {
  int i,h=0;
  for(i=0;i<n;i++) h=((h<<5)+h)^*k++;
  return h;
}

HT* htnew(size_t m) {
  HT *ht=calloc(1,sizeof(HT));
  if(m) {
    for(ht->m=2;(ht->m<<=1)<m;);
    if(!(ht->kh=calloc(ht->m,sizeof(char*)))){fprintf(stderr,"htset");exit(1);}
    if(!(ht->nh=calloc(ht->m,sizeof(int)))){fprintf(stderr,"htset");exit(1);}
    if(!(ht->vh=calloc(ht->m,sizeof(int)))){fprintf(stderr,"htset");exit(1);}
  }
  return ht;
}

void htfree(HT *ht) {
  size_t i;
  for(i=0;i<ht->m;++i) if(ht->kh[i]) free(ht->kh[i]);
  free(ht->kh);
  free(ht->nh);
  free(ht->vh);
  free(ht);
}

void htset(HT *ht, char *k, int n, int v) {
  char *t,**kh;
  int *vh,*nh;
  size_t i,h=hs(k,n);
  if(ht->m)
    for(;(t=ht->kh[h&(ht->m-1)]);++h)
      if(ht->nh[h&(ht->m-1)]==n && !memcmp(k,t,n)) {
        ht->vh[h&(ht->m-1)]=v;
        return;
      }
  if(ht->i==ht->m) {
    ht->m=ht->m?ht->m*2:2;
    if(!(kh=calloc(ht->m,sizeof(char*)))){fprintf(stderr,"htset");exit(1);}
    if(!(nh=calloc(ht->m,sizeof(int)))){fprintf(stderr,"htset");exit(1);}
    if(!(vh=calloc(ht->m,sizeof(int)))){fprintf(stderr,"htset");exit(1);}
    for(i=0;i<ht->i;i++) {
      if((t=ht->kh[i])) {
        for(h=hs(t,ht->nh[i]);(kh[h&(ht->m-1)]);++h){};
        kh[h&(ht->m-1)]=t;
        nh[h&(ht->m-1)]=ht->nh[i];
        vh[h&(ht->m-1)]=ht->vh[i];
      }
    }
    if(ht->i) { free(ht->kh); free(ht->nh); free(ht->vh); }
    ht->kh=kh; ht->nh=nh; ht->vh=vh;
    for(h=hs(k,n);(ht->kh[h&(ht->m-1)]);++h);
  }
  if(!(t=malloc(n))){fprintf(stderr,"htset");exit(1);}
  ht->i+=2;
  memcpy(t,k,n);
  ht->kh[h&(ht->m-1)]=t;
  ht->nh[h&(ht->m-1)]=n;
  ht->vh[h&(ht->m-1)]=v;
}

int htget(HT *ht, char *k, int n) {
  char *t;
  int i,h;
  if(ht->m) {
    i=ht->m-1;
    for(h=hs(k,n);(t=ht->kh[h&i]);++h)
      if(ht->nh[h&i]==n && !memcmp(k,t,n))
        return ht->vh[h&i];
  }
  return -1;
}
