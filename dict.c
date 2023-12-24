#include "dict.h"
#include <stdio.h>
#include <string.h>
#include "k.h"
#include "x.h"
#include "sym.h"
#include "fn.h"

dict* dnew() {
  dict *d = xmalloc(sizeof(dict));
  d->c = 0;
  d->k = 0;
  d->v = 0;
  d->r = 0;
  return d;
}

void dfree(dict *d) {
  if(!d) return;
  if(d->r) { d->r--; return; }
  DO(d->c,kfree(d->v[i]))
  xfree(d->k);
  xfree(d->v);
  xfree(d);
}

dict* l2d(K *l) {
  int i;
  K *p;
  dict *d = xmalloc(sizeof(dict));
  d->c = l->c;
  d->k = xmalloc(sizeof(char*)*d->c);
  d->v = xmalloc(sizeof(K*)*d->c);
  d->r = 0;
  for(i=0;i<d->c;i++) {
    p = v0(l)[i];
    if(p->t == 0) {
      d->k[i] = v3(v0(p)[0]);
      d->v[i] = kref(v0(p)[1]);
    } else if(p->t == -4) {
      d->k[i] = v4(p)[0];
      d->v[i] = k4(v4(p)[1]);
    } else {
      fprintf(stderr, "l2d\n");
      exit(1);
    }
  }
  return d;
}

K* d2l(dict *d) {
  int i;
  K *r = kv0(d->c);
  K *q = 0;
  for(i=0;i<r->c;i++) {
    v0(r)[i] = kv0(3);
    v0(v0(r)[i])[0] = k4(d->k[i]);
    q = kref(d->v[i]);
    v0(v0(r)[i])[1] = q;
    v0(v0(r)[i])[2] = null;
  }
  return r;
}

dict *dcp(dict *d) {
  dict *d2 = xmalloc(sizeof(dict));
  d2->c = d->c;
  d2->k = xmalloc(sizeof(char*)*d2->c);
  d2->v = xmalloc(sizeof(K*)*d2->c);
  d2->r = 0;
  DO(d->c, d2->k[i]=d->k[i]; d2->v[i]=kref(d->v[i]))
  return d2;
}

K* dget(dict *d, char *key) {
  K *r = 0;
  char *kk=sp(key);
  DO(d->c,if(kk == d->k[i]) { r=kref(d->v[i]); break; })
  return r;
}

void dset(dict *d, char *key, K *v) {
  K *r = 0;
  char *kk=sp(key);
  kref(v);
  DO(d->c,if(kk == d->k[i]) { r=d->v[i]; kfree(r); d->v[i]=v; break; })
  if(!r) {
    d->k = xrealloc(d->k, sizeof(char*)*(d->c+1));
    d->v = xrealloc(d->v, sizeof(K*)*(d->c+1));
    d->k[d->c]=kk;
    d->v[d->c]=v;
    d->c++;
  }
}

K* dvals(dict *d) {
  K *r = kv0(d->c);
  DO(d->c, v0(r)[i]=kref(d->v[i]))
  return knorm(r);
}

K* dkeys(dict *d) {
  K *r = 0;
  r = kv4(d->c);
  DO(d->c,v4(r)[i] = d->k[i])
  return r;
}

int dcmp(dict *d0, dict *d1) {
  int i=0,r=0;
  for(;;) {
    if(i==d0->c && i==d1->c) break;
    else if(i==d0->c && i<d1->c) {r=r?r:-1;break;}    /* count */
    else if(i<d0->c && i==d1->c) {r=r?r:1;break;}     /* count */
    else if(strcmp(d0->k[i],d1->k[i])<0) r=-1;        /* key */
    else if(strcmp(d0->k[i],d1->k[i])>0) r=1;         /* key */
    else if(kcmp(d0->v[i],d1->v[i])<0) r=-1;
    else if(kcmp(d0->v[i],d1->v[i])>0) r=1;
    i++;
  }
  return r;
}

uint64_t dhash(dict *d) {
  uint64_t r=0;
  DO(d->c,r^=xfnv1a(d->k[i],strlen(d->k[i])))
  DO(d->c,r^=khash(d->v[i]))
  return r;
}
