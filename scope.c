#include "scope.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#ifdef _WIN32
#include "systime.h"
#include "unistd.h"
#define getpid() _getpid()
#else
#include "sys/time.h"
#include <unistd.h>
  static int __gettimeofday(struct timeval * tp, struct timezone * tzp) {
    return gettimeofday(tp,tzp);
  }
#endif
#include "fn.h"

#define SM 1000

#ifdef _WIN32
#define strtok_r strtok_s
#endif

static K scopea[SM];
static char *spt,*spT;
K ks,gs,cs;
K ktree,C,Z,D;
K locals[LOCALSMAX];
int localsi;

void scope_init(int argc, char **argv) {
  char hn[256];
  K a;
  ks=scope_newk(null,t(4,sp("")));
  gs=scope_newk(ks,t(4,sp(".k")));
  cs=k_(gs); /* cs starts out in `.k */
  K *pks=px(ks);
  K *pgs=px(gs);
  ktree=pks[1];
  Z=dnew();
  K *pk=px(ktree); char **pkk=px(pk[0]); K *pkv=px(pk[1]);
  pkk[0]=sp("k"); pkv[0]=k_(pgs[1]);
  pkk[1]=sp("z"); pkv[1]=k_(Z);
  n(pk[0])=2; n(pk[1])=2; pk[2]=t(1,2);
  C=dnew();
  D=t(4,sp(".k"));
#ifdef _WIN32
  DWORD size=256;
  GetComputerNameA(hn,&size);
#else
  gethostname(hn,256);
#endif
  if(argc>1) {
    a=tn(0,argc-2); K *pa=px(a);
    i(argc-2,char *a_=argv[i+2];pa[i]=tnv(3,strlen(a_),xmemdup(a_,1+strlen(a_))))
  }
  else a=tn(0,0);
  spt=sp("t");
  spT=sp("T");
  K *pz=px(Z); char **pzk=px(pz[0]); K *pzv=px(pz[1]);
  pzk[0]=sp("h"); pzv[0]=t(4,sp(hn));
  pzk[1]=sp("P"); pzv[1]=t(1,(u32)getpid());
  pzk[2]=sp("i"); pzv[2]=a;
  pzk[3]=spt; pzv[3]=null;
  pzk[4]=spT; pzv[4]=null;
  pzk[5]=sp("f"); pzv[5]=null;
  n(pz[0])=6; n(pz[1])=6; pz[2]=t(1,6);
  i(LOCALSMAX,locals[i]=tn(0,DMAX))
}

static K scope_new_(K p, K k) {
  K r,*prk;
  int i;
  PRK(6);
  prk[0]=k_(p);
  prk[1]=dnew();
  prk[2]=k?k:null;
  prk[3]=t(1,0); /* closure? */
  prk[4]=t(1,0); /* version */
  prk[5]=t(1,0); /* borrowed */
  for(i=0;i<SM;i++) if(!scopea[i]) break;
  if(i==SM) {
    printf("error: scope_new() i==SM\n");
    exit(1);
  }
  if(k) scopea[i]=k_(r);
  return r;
}
K scope_new(K p) { return scope_new_(p,0); }
K scope_newk(K p, K k) { return scope_new_(p,k); }

void scope_free(K s) {
  /* note: variable dict has to be freed first since scopes can have circular references */
  K *psu=px(s);
  dfree(psu[1]);
  psu[1]=null;
  _k(s);
}

void scope_free_all(void) {
  i(SM,if(!scopea[i]) break; scope_free(scopea[i]))
  i(LOCALSMAX,n(locals[i])=0;_k(locals[i]))
}

int scope_vktp(char *x) {
  int v=1,s=0;
  while(*x) {
    if(!v) break;
    switch(s) {
    case 0:
      if(*x=='.') s=1;
      else if(isalpha(*x)) s=2;
      else v=0;
      break;
    case 1:
      if(isalpha(*x)) s=2;
      else v=0;
      break;
    case 2:
      if(*x=='.') s=1;
      else if(!isalnum(*x)) v=0;
      break;
    }
    ++x;
  }
  if(s==1) v=0; // .k.
  return v;
}

/* n is a global reference. ex: .k.a.b.c */
static K ktree_get(char *n) {
  K r=0,q;
  char s[256],*p,*t;
  if(!*n) return k_(ktree);
  if(!scope_vktp(n)) return KERR_DOMAIN;
  memcpy(s,n,1+strlen(n));
  p=strtok_r(s,".",&t);
  r=dget(ktree,sp(p));
  while((p=strtok_r(0,".",&t))) {
    if(!r) return KERR_VALUE;
    if(0x80!=s(r)) { _k(r); return KERR_VALUE; }
    q=dget(r,sp(p));
    _k(r); r=q;
  }
  return r?r:KERR_VALUE;
}

static K t0(void) { /* ts */
  return t(1,(u32)(time(0)-2051222400l)); /* 2035-1970 */
}

static K tt0(void) { /* TS */
  struct timeval tv;
  __gettimeofday(&tv,0);
  double d = tv.tv_sec - 2051222400l; /* 2035-1970 */
  return t2(d/60/60/24);
}

static K scope_get_local(K s, int n) {
  K *ps=px(s);
  K *pd=px(ps[1]);
  K *pv=px(pd[1]);
  return k_(pv[n]);
}
static void setz(void) {
  K t;
  t=t0(); (void)dset(Z,spt,t); _k(t);
  t=tt0(); (void)dset(Z,spT,t); _k(t);
  if(fnestacki>=0) (void)dset(Z,sp("f"),fnestack[fnestacki]);
  else (void)dset(Z,sp("f"),null);
}
static K scope_get_(K s, char *n) {
  K r=0,t=0,*ps=px(s),q;
  char *p,*rp;
  char nn[256];
  if(!*n) { setz(); return k_(ktree); }
  if(strlen(n)>255) return KERR_LENGTH;
  if(n[0]=='.') {
    if(n[1]=='z' && (n[2]==0 || n[2]=='.')) setz();
    return ktree_get(n);
  }
  if(ps[2]!=null) { /* refresh scope dict from ktree */
    q=ktree_get(sk(ps[2]));
    if(E(q)) return q;
    _k(ps[1]); ps[1]=q;
  }
  if(strchr(n,'.')) {
    memcpy(nn,n,1+strlen(n));
    p=strtok_r(nn,".",&rp);
    if(p) {
      r=scope_get_(s,sp(p));
      while((p=strtok_r(0,".",&rp))) {
        if(!p) return KERR_VALUE;
        if(0x80!=s(r)) { _k(r); return KERR_VALUE; }
        t=dget(r,sp(p));
        _k(r); r=t;
      }
    }
    return r?r:KERR_VALUE;
  }
  else {
    // this can happen like this:
    // .[`;nul;,;"0"]
    // .[`;nul;,]
    // pretty sure this can't happen anymore
    //if(0x80!=s(ps[1])) return KERR_VALUE;
    r=dget(ps[1],n);
    if(!r && ps[0]!=null && s!=gs) r=scope_get_(ps[0],n);
    if(!r && s!=gs) r=scope_get_(gs,n);
    if(!r) r=dget(C,n);
    return r?r:KERR_VALUE;
  }
}

K scope_get(K s, K n) {
  if(4==T(n)) return scope_get_(s,sk(n));
  else return scope_get_local(s,ik(n));
}
static K scope_set_local(K s, int n, K v) {
  K *ps,*pd,*pv;
  char **pk;
  if(s==gs) {
    /* handle a situation like {x::1} */
    ps=px(cs);
    pd=px(ps[1]);
    pk=px(pd[0]);
    return scope_set(s,t(4,pk[n]),v);
  }
  ps=px(s);
  pd=px(ps[1]);
  pv=px(pd[1]);
  _k(pv[n]);
  pv[n]=k_(v);
  return 0;
}
static K scope_set_(K s, char *n, K v) {
  K d,e=0,m,*psu,w;
  char *rp,nn[256],*t,*ss,*u;
  K es=cs,*pes;
  ko *kd;
  int copy=0, gcopy=0;
  if(strlen(n)>255) return KERR_LENGTH;
  psu=px(s);
  if(0x80==s(v)&&(s==gs||s==ks)) { // d.c:.k
    kd=(ko*)(b(48)&v); if(kd->r>0) gcopy=1;
  }
  if(s==ks && strlen(n)==1 && *n!='k') { // top level, single letters reserved, except k
    fprintf(stderr,"reserved: %s\n",n);
    return KERR_RESERVED;
  }
  if(kreserved(sp(n))) { // reserved names
    fprintf(stderr,"reserved: %s\n",n);
    return KERR_RESERVED;
  }
  if(strchr(n,'.')) {
    if(n[0]=='.') {
      // cannot change current directory path
      char *skd=sk(D);
      if(skd==strstr(skd,n)) return KERR_DOMAIN;
      es=ks;
    }
    if(!scope_vktp(n)) return KERR_DOMAIN;
    memcpy(nn,n,1+strlen(n));
    u=t=ss=strtok_r(nn,".",&rp);

    pes=px(es);
    d=dget(pes[1],sp(ss));
    if(!d) d=dnew();
    else if(0x80!=s(d)) { _k(d); return KERR_VALUE; }
    kd=(ko*)(b(48)&d);
    if(kd->r>1) { --kd->r; d=kcp(d); if(E(d)) { return d; } } /* copy on write */
    e=m=k_(d);
    while((ss=strtok_r(0,".",&rp))) {
      t=ss; e=d; _k(d);
      if(v==e) copy=1;  /* v in traversal path - need to copy */
      d=dget(e,sp(ss));
      if(d) { if(0x80!=s(d)) break; }
      else d=dnew();
      kd=(ko*)(b(48)&e);
      if(kd->r>1 && v!=e) { --kd->r; e=kcp(e); if(E(e)) { _k(m); return e;} }
      (void)dset(e,sp(ss),d);
    }
    _k(d);
    if(!strtok_r(0,".",&rp)) {
      if(copy||gcopy) { w=kcp(v); if(E(w)) { _k(m); return w; } (void)dset(e,sp(t),w); _k(w); }
      else (void)dset(e,sp(t),v);
      scope_set_(es,sp(u),m);
    }
    else  { _k(m); return KERR_VALUE; }
    _k(m);
  }
  else {
    // this can happen like this:
    // .[`;nul;,;"0"]
    // .[`;nul;,]
    if(0x80!=s(psu[1])) return KERR_VALUE;
    if(gcopy) { w=kcp(v); if(E(w)) return w; (void)dset(psu[1],sp(n),w); _k(w); }
    else (void)dset(psu[1],sp(n),v);
  }
  return 0;
}

K scope_set(K s, K n, K v) {
  K *ps=px(s);
  if(1==ik(ps[5])) { /* scope dict vals are borrowed, make copy */
    K d=ps[1]; K *pd=px(d);
    K v=kcp(pd[1]); if(E(v)) return v;
    v=kresize(v,ik(pd[2]));
    n(v)=n(pd[1]);
    pd[1]=v;
    ps[5]=t(1,0);
  }
  if(4==T(n)) return scope_set_(s,sk(n),v);
  else return scope_set_local(s,ik(n),v);
}

K scope_cp(K s) {
  int i;
  K e, s2=tn(0,6); K *ps2=px(s2);
  K *ps=px(s);
  ps2[0]=s==ks?k_(ks):scope_cp(ps[0]); if(E(ps2[0])) { e=ps2[0]; _k(s2); return e; }
  ps2[1]=dcp(ps[1]); if(E(ps2[1])) { e=ps2[1]; _k(s2); return e; }
  ps2[2]=ps[2];
  ps2[3]=ps[3];
  ps2[4]=t(1,0);
  ps2[5]=t(1,0);

  for(i=0;i<SM;i++) if(!scopea[i]) break;
  if(i==SM) {
    printf("error: scope_new() i==SM\n");
    exit(1);
  }
  scopea[i]=k_(s2);
  return s2;
}

K scope_find(char *x) {
  int i;
  for(i=0;i<SM;i++) {
    if(!scopea[i]) break;
    K *ps=px(scopea[i]);
    if(ps[2]==t(4,sp(x))) break;
  }
  if(i==SM) {
    printf("error: scope_find() i==SM\n");
    exit(1);
  }
  return scopea[i];
}
