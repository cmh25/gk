#include "scope.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "x.h"
#include "sym.h"
#include "sys.h"

#define SM 1000000

#ifdef _WIN32
  #define strtok_r strtok_s
#endif

static scope *scopea[SM];
scope *ks,*gs,*cs;
dict *ktree,*C,*Z;

extern K *null;

static int reserved(char *s, char *n) {
  if(!s) return 0;
  if(strlen(s)>2)
    if(s[0]=='.'&&s[2]=='.'&&s[1]!='k')
      return 1;
  if(strlen(s)==1)
    if(s[0]=='.' && n && strlen(n)==1 && n[0]!='k')
      return 1;
  if(strlen(s)==2 && s[0]=='.' && s[1]!='k')
    return 1;
  return 0;
}

static scope* scope_new_(scope *p, char *k) {
  int i;
  scope *s = xmalloc(sizeof(scope));
  s->p = p;
  s->d = dnew();
  s->k = k;
  for(i=0;i<SM;i++) if(!scopea[i]) break;
  if(i==SM) {
    printf("error: scope_new() i==SM\n");
    exit(1);
  }
  scopea[i]=s;
  return s;
}
scope* scope_new(scope *p) { return scope_new_(p,0); }
scope* scope_newk(scope *p, char *k) { return scope_new_(p,k); }

void scope_free(scope *s) {
  int i;
  if(s->k) return;
  for(i=0;i<SM;i++) {
    if(!scopea[i]) break;
    if(scopea[i] && scopea[i] == s) scopea[i]=0;
  }
  dfree(s->d);
  xfree(s);
}

/* n is a global reference. ex: .k.a.b.c */
static K* ktree_get(char *n) {
  K *r=null,*q;
  char *s,*p,*sp;
  if(!strcmp(n,".")) { ktree->r++; return k5(ktree); }
  s=xstrdup(n);
  p=strtok_r(s,".",&sp);
  r=dget(ktree,p);
  while((p=strtok_r(0,".",&sp))) {
    if(!r) { xfree(s); return kerror("value"); }
    if(r->t!=5) { xfree(s); kfree(r); return kerror("value"); }
    q=dget(r->v,p);
    kfree(r);
    r=q;
  }
  xfree(s);
  return r ? r : kerror("value");
}
K* scope_get(scope *s, char *n) {
  K *r=0,*t=0,*q=0;
  char *p=0,*nn,*rp;
  if(!s) return 0;
  if(!n) return 0;
  t=t0(); dset(Z,"t",t); kfree(t);
  t=tt0(); dset(Z,"T",t); kfree(t);
  if(n[0]=='.') return ktree_get(n);
  if(s->k) { /* refresh scope dict from ktree */
    q=ktree_get(s->k);
    if(q->v!=s->d) { dfree(s->d); s->d=q->v; }
    kfree(q);
  }
  if(strchr(n,'.')) {
    nn=xstrdup(n);
    p=strtok_r(nn,".",&rp);
    r=scope_get(s,p);
    while((p=strtok_r(0,".",&rp))) {
      if(!r) { xfree(nn); return kerror("value"); }
      if(r->t!=5) { xfree(nn); kfree(r); return kerror("value"); }
      t=dget(r->v,p); kfree(r); r=t;
    }
    xfree(nn);
    return r ? r : kerror("value");
  }
  r = dget(s->d,n);
  if(!r && s->p && s!=gs) r = scope_get(s->p,n);
  if(!r) r = dget(C,n);
  return r ? r : kerror("value");
}

K* scope_set(scope *s, char *n, K *v) {
  if(reserved(s->k,n)) return kerror("reserved");
  dset(s->d,n,v);
  return null;
}

scope* scope_cp(scope *s) {
  int i;
  if(!s) return 0;
  scope *s2 = xmalloc(sizeof(scope));
  s2->p = s->p;
  s2->d = dcp(s->d);
  s2->k = s->k;
  for(i=0;i<SM;i++) if(!scopea[i]) break;
  if(i==SM) {
    printf("error: scope_new() i==SM\n");
    exit(1);
  }
  scopea[i]=s2;
  return s2;
}

scope* scope_find(dict *d) {
  int i;
  for(i=0;i<SM;i++) if(!scopea[i]||scopea[i]->d==d) break;
  if(i==SM) {
    printf("error: scope_find() i==SM\n");
    exit(1);
  }
  return scopea[i];
}
