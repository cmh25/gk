#include "av.h"
#include <string.h>
#include "fn.h"
#include "b.h"
#include "fe.h"
#include <stdio.h>

static char *P=":+-*%&|<>=~.!@?#_^,$'/\\";

static K avdoi(K f, K a, K x, int ai, int xi, char *av) {
  K r=0,*pak,*pxk,a_=0;
  char *pac,*pxc,**pas,**pxs,Ta,Tx;
  int *pai,*pxi;
  double *paf,*pxf;
  static int d=0;
  Ta=ta; if(s(a)) { if(!VST(a)) return kerror("type"); Ta=15; }
  Tx=tx; if(s(x)) { if(!VST(x)) return kerror("type"); Tx=15; }
  if(++d>maxr) { --d; return kerror("stack"); } /* stack */
  if(ai==-1) a_=k_(a);
  else {
    switch(Ta) {
    case -1: PAI; a_=t(1,(u32)pai[ai]); break;
    case -2: PAF; a_=t2(paf[ai]); break;
    case -3: PAC; a_=t(3,(u8)pac[ai]); break;
    case -4: PAS; a_=t(4,pas[ai]); break;
    case  0: PAK; a_=k_(pak[ai]); break;
    }
  }
  if(xi==-1) r=avdo(k_(f),a_,k_(x),av);
  else {
    switch(Tx) {
    case -1: PXI; r=avdo(k_(f),a_,t(1,(u32)pxi[xi]),av); break;
    case -2: PXF; r=avdo(k_(f),a_,t2(pxf[xi]),av); break;
    case -3: PXC; r=avdo(k_(f),a_,t(3,(u8)pxc[xi]),av); break;
    case -4: PXS; r=avdo(k_(f),a_,t(4,pxs[xi]),av); break;
    case  0: PXK; r=avdo(k_(f),a_,k_(pxk[xi]),av); break;
    }
  }
  --d;
  return r;
}

static K each(K f, K a, K x, char *av) {
  K r=0,e,*prk;
  char Ta,Tx;
  Ta=ta; if(s(a)) { if(!VST(a)) return kerror("type"); Ta=15; }
  Tx=tx; if(s(x)) { if(!VST(x)) return kerror("type"); Tx=15; }
  if(a&&Ta<=0) {
    PRK(na);
    if(Tx>0) i(na,prk[i]=avdoi(f,a,x,i,-1,av);EC(prk[i]))
    else if(na!=nx) { _k(r); return kerror("length"); }
    else i(na,prk[i]=avdoi(f,a,x,i,i,av);EC(prk[i]))
  }
  else {
    if(Tx>0) r=avdoi(f,a,x,-1,-1,av);
    else { PRK(nx); i(nx,prk[i]=avdoi(f,a,x,-1,i,av);EC(prk[i])) }
  }
  return r;
cleanup:
  _k(r);
  return e;
}

static K eachright(K f, K a, K x, char *av) {
  K r=0,e,*prk;
  char Tx;
  Tx=tx; if(!Tx&&s(x)) { if(!VST(x)) return kerror("type"); Tx=15; }
  if(Tx>0) r=avdoi(f,a,x,-1,-1,av);
  else { PRK(nx); i(nx,prk[i]=avdoi(f,a,x,-1,i,av);EC(prk[i])) }
  return r;
cleanup:
  _k(r);
  return e;
}

static K eachleft(K f, K a, K x, char *av) {
  K r=0,e,*prk;
  char Ta;
  Ta=ta; if(s(a)) { if(!VST(a)) return kerror("type"); Ta=15; }
  if(Ta>0) r=avdoi(f,a,x,-1,-1,av);
  else { PRK(na); i(na,prk[i]=avdoi(f,a,x,i,-1,av);EC(prk[i])) }
  return r;
cleanup:
  _k(r);
  return e;
}

static K over7(K f, K x, char *av) {
  K r=0,e,p=0,q=0,*pp,*pq,*pxk;
  i64 m=-1;

  if(s(x)!=0x81) return kerror("type");
  if(nx!=(u32)ik(val(f))) return kerror("valence");

  pxk=px(x);
  int allatoms=1; i1(nx,if(T(pxk[i])<=0&&!s(pxk[i])){allatoms=0;break;})
  if(allatoms) { r=fne_(k_(f),k_(x),av); return r; }

  q=tn(0,nx); pq=px(q);
  pq[0]=k_(pxk[0]);
  for(u64 i=1;i<nx;i++) {
    p=pxk[i];
    if(m==-1&&T(p)<=0) m=n(p);
    else if(m>=0&&T(p)<=0&&!s(p)&&m!=(i64)n(p)) { _k(q);  return kerror("valence"); }
    if(T(p)<0&&!s(p)) { pq[i]=kmix(p); EC(pq[i]); }
    else pq[i]=k_(p);
  }
  if(m==1) r=fne_(k_(f),k_(x),av);
  else {
    p=tn(0,nx); pp=px(p);
    r=k_(pxk[0]);
    pp[0]=null;
    for(i64 i=0;i<m;i++) {
      _k(pp[0]); pp[0]=k_(r);
      j1(nx,pp[j]=T(pq[j])>0||s(pq[j])?pq[j]:((K*)px(pq[j]))[i])
      _k(r);
      r=fne_(k_(f),k_(p),av);
      if(E(r)) { _k(pp[0]); e=r; goto cleanup; }
    }
    _k(pp[0]);
    n(p)=0; _k(p);
  }
  _k(q);
  return r;
cleanup:
  n(p)=0; _k(p);
  _k(q);
  _k(r);
  return e;
}

static K scan7(K f, K x, char *av) {
  K r=0,e,p=0,q=0,*pp,*pq,*pxk,*prk;
  i64 m=-1;

  if(s(x)!=0x81) return kerror("type");
  if(nx!=(u32)ik(val(f))) return kerror("valence");

  pxk=px(x);
  int allatoms=1; i1(nx,if(T(pxk[i])<=0&&!s(pxk[i])){allatoms=0;break;})
  if(allatoms) { r=fne_(k_(f),k_(x),av); return r; }

  q=tn(0,nx); pq=px(q);
  pxk=px(x);
  pq[0]=k_(pxk[0]);
  for(u64 i=1;i<nx;i++) {
    p=pxk[i];
    if(m==-1&&T(p)<=0) m=n(p);
    else if(m>=0&&T(p)<=0&&!s(p)&&m!=(i64)n(p)) { _k(q);  return kerror("valence"); }
    if(T(p)<0) { pq[i]=kmix(p); EC(pq[i]); }
    else pq[i]=k_(p);
  }
  if(m==1) r=fne_(k_(f),k_(x),av);
  else {
    r=tn(0,m+1); prk=px(r);
    p=tn(0,nx); pp=px(p);
    prk[0]=k_(pxk[0]);
    for(i64 i=0;i<m;i++) {
      pp[0]=prk[i];
      j1(nx,pp[j]=T(pq[j])>0||s(pq[j])?pq[j]:((K*)px(pq[j]))[i])
      prk[i+1]=fne_(k_(f),k_(p),av); EC(prk[i+1]);
    }
    n(p)=0; _k(p);
  }
  _k(q);
  return r;
cleanup:
  n(p)=0; _k(p);
  _k(q);
  _k(r);
  return e;
}

static K overm7(K f, K x, char *av) {
  K r=0,e,p,q=0,xx,*pxx;
  xx=tn(0,1); pxx=px(xx); pxx[0]=x;
  p=x;  /* first */
  q=fne_(k_(f),k_(xx),av); EC(q); /* previous */
  if(!ik(k(10,k_(p),k_(q)))) {
    pxx[0]=q;
    K z=fne_(k_(f),k_(xx),av); EC(z); r=z;
    while(!ik(k(10,k_(r),k_(p)))&&!ik(k(10,k_(r),k_(q)))) {
      _k(q); q=r;
      pxx[0]=q;
      z=fne_(k_(f),k_(xx),av); EC(z); r=z;
      if(STOP) { STOP=0; e=kerror("stop"); goto cleanup; }
      if(EXIT) { e=kerror("abort"); goto cleanup; }
    }
  }
  _k(r);
  pxx[0]=0; _k(xx);
  return q;
cleanup:
  pxx[0]=0; _k(xx);
  _k(r);
  if(q!=r) _k(q);
  return e;
}

static K overm(K f, K x, char *av) {
  K r=0,e,p,q;
  int b=0,n=strlen(av);
  if(0xc3==s(f)||0xc4==s(f)) return overm7(f,x,av);
  if(n==0) b=1;
  p=x;  /* first */
  q=b?fe(k_(f),0,k_(x),av):avdo(k_(f),0,k_(x),av); EC(q); /* previous */
  if(!ik(k(10,k_(p),k_(q)))) {
    K z=b?fe(k_(f),0,k_(q),av):avdo(k_(f),0,k_(q),av); EC(z); r=z;
    while(!ik(k(10,k_(r),k_(p)))&&!ik(k(10,k_(r),k_(q)))) {
      _k(q); q=r;
      z=b?fe(k_(f),0,k_(q),av):avdo(k_(f),0,k_(q),av); EC(z); r=z;
      if(STOP) { STOP=0; e=kerror("stop"); goto cleanup; }
      if(EXIT) { e=kerror("abort"); goto cleanup; }
    }
  }
  _k(r);
  return q;
cleanup:
  _k(r);
  if(q!=r) _k(q);
  return e;
}

static K scanm7(K f, K x, char *av) {
  K r=0,t=0,e,q=0,*prk,xx,*pxx;
  int zi=0,zm=32;
  PRK(zm);
  xx=tn(0,1); pxx=px(xx); pxx[0]=x;
  prk[zi]=kcp2(x); if(E(prk[zi])) { e=prk[zi]; goto cleanup; } ++zi; /* first */
  q=fne_(k_(f),k_(xx),av); EC(q); /* previous */
  if(EXIT) { e=kerror("abort"); goto cleanup; }
  if(!ik(k(10,k_(x),k_(q)))) {
    prk[zi]=kcp2(q); if(E(prk[zi])) { e=prk[zi]; goto cleanup; } ++zi;
    if(E(prk[zi-1])) { e=prk[zi-1]; goto cleanup; }
    pxx[0]=q;
    t=fne_(k_(f),k_(xx),av); EC(t);
    if(EXIT) { e=kerror("abort"); goto cleanup; }
    while(!ik(k(10,k_(t),k_(x)))&&!ik(k(10,k_(t),k_(q)))) {
      if(zi==zm) { zm<<=1; r=kresize(r,zm); prk=px(r); }
      prk[zi]=kcp2(t); if(E(prk[zi])) { e=prk[zi]; goto cleanup; } ++zi;
      _k(q); q=t;
      pxx[0]=q;
      t=fne_(k_(f),k_(xx),av); EC(t);
      if(STOP) { STOP=0; e=kerror("stop"); goto cleanup; }
      if(EXIT) { e=kerror("abort"); goto cleanup; }
    }
  }
  _k(q);
  _k(t);
  pxx[0]=0; _k(xx);
  n(r)=zi;
  return r;
cleanup:
  _k(q);
  _k(t);
  pxx[0]=0; _k(xx);
  n(r)=zi;
  _k(r);
  return e;
}

static K scanm(K f, K x, char *av) {
  K r=0,t=0,e,q=0,*prk;
  int b=0,n=strlen(av),zi=0,zm=32;
  if(0xc3==s(f)||0xc4==s(f)) return scanm7(f,x,av);
  if(n==0) b=1;
  PRK(zm);
  prk[zi]=kcp2(x); if(E(prk[zi])) { e=prk[zi]; goto cleanup; } ++zi; /* first */
  q=b?fe(k_(f),0,k_(x),av):avdo(k_(f),0,k_(x),av); EC(q); /* previous */
  if(!ik(k(10,k_(x),k_(q)))) {
    prk[zi]=kcp2(q); if(E(prk[zi])) { e=prk[zi]; goto cleanup; } ++zi;
    t=b?fe(k_(f),0,k_(q),av):avdo(k_(f),0,k_(q),av); EC(t);
    while(!ik(k(10,k_(t),k_(x)))&&!ik(k(10,k_(t),k_(q)))) {
      if(zi==zm) { zm<<=1; r=kresize(r,zm); prk=px(r); }
      prk[zi]=kcp2(t); if(E(prk[zi])) { e=prk[zi]; goto cleanup; } ++zi;
      _k(q); q=t;
      t=b?fe(k_(f),0,k_(q),av):avdo(k_(f),0,k_(q),av); EC(t);
      if(STOP) { STOP=0; e=kerror("stop"); goto cleanup; }
      if(EXIT) { e=kerror("abort"); goto cleanup; }
    }
  }
  _k(q);
  _k(t);
  n(r)=zi;
  return r;
cleanup:
  _k(t);
  _k(q);
  n(r)=zi;
  _k(r);
  return e;
}

K overdfe(K f, K x, char *av) {
  K r=0,e,p,*px,t=0;
  if(tx>0||s(x)) return k_(x);
  if(!nx) return kerror("length");
  if(tx<0) { t=kmix(x); if(E(t)) return t; x=t; }
  px=px(x);
  r=k_(px[0]);
  i1(nx,p=fe(k_(f),r,k_(px[i]),av);r=0;EC(p);r=p)
  _k(t);
  return r;
cleanup:
  _k(t);
  _k(r);
  return e;
}

K scandfe(K f, K x, char *av) {
  K r=0,e,*px,*prk,t=0;
  if(tx>0||s(x)) return k_(x);
  if(!nx) return tn(0,0);
  if(tx<0) { t=kmix(x); if(E(t)) return t; x=t; }
  px=px(x);
  r=tn(0,nx);
  prk=px(r);
  prk[0]=k_(px[0]);
  i1(nx,prk[i]=fe(k_(f),k_(prk[i-1]),k_(px[i]),av);EC(prk[i]))
  _k(t);
  return r;
cleanup:
  _k(t);
  _k(r);
  return e;
}

static K overd7(K f, K x, char *av) {
  K r=0,e,p,*px,xx,*pxx,t=0;
  if(tx>0||s(x)) return k_(x);
  if(!nx) return kerror("length");
  if(tx<0) { t=kmix(x); if(E(t)) return t; x=t; }
  px=px(x);
  r=k_(px[0]);
  xx=tn(0,2); pxx=px(xx);
  i1(nx,pxx[0]=r;pxx[1]=px[i];p=fne_(k_(f),k_(xx),av);EC(p);_k(r);r=p)
  pxx[0]=0; pxx[1]=0; _k(xx);
  _k(t);
  return r;
cleanup:
  pxx[0]=0; pxx[1]=0; _k(xx);
  _k(t);
  _k(r);
  return e;
}

static K scand7(K f, K x, char *av) {
  K r=0,e,*px,xx,*pxx,*prk,t=0;
  if(tx>0||s(x)) return k_(x);
  if(!nx) return tn(0,0);
  if(tx<0) { t=kmix(x); if(E(t)) return t; x=t; }
  px=px(x);
  r=tn(0,nx);
  prk=px(r);
  prk[0]=k_(px[0]);
  xx=tn(0,2); pxx=px(xx);
  i1(nx,pxx[0]=prk[i-1];pxx[1]=px[i];prk[i]=fne_(k_(f),k_(xx),av);EC(prk[i]))
  pxx[0]=0; pxx[1]=0; _k(xx);
  _k(t);
  return r;
cleanup:
  pxx[0]=0; pxx[1]=0; _k(xx);
  _k(r);
  _k(t);
  return e;
}

static K eachparam7(K f, K x) {
  K r=0,p=0,q=0,*pp,*pq,*px,*pr,t;
  u64 m=0;
  i32 b=0;
  u32 valence=val(f);
  char tp;
  if(nx!=valence) return kerror("valence");
  q=tn(0,nx); pq=px(q);
  px=px(x);
  for(u64 i=0;i<nx;++i) {
    p=px[i];
    tp=s(p)?10:T(p); /* subtypes are atoms */
    if(b==0&&tp<=0) { b=1; m=n(p); }
    else if(b==0&&tp>0) m=1;
    else if(tp<=0&&m!=n(p)) { _k(q); return kerror("length"); }
    else if(tp==6) { _k(q); return kerror("valence"); } /* TODO: projection */
    if(tp<0) { t=kmix(p); if(E(t)) { _k(q); return t; } pq[i]=t; }
    else pq[i]=k_(p);
  }
  r=tn(0,m); pr=px(r);
  p=tn(0,nx); pp=px(p);
  for(u64 i=0;i<n(r);++i) {
    for(u64 j=0;j<n(p);++j) {
      _k(pp[j]);
      pp[j]=T(pq[j])||s(pq[j]) ? k_(pq[j]) : k_(((K*)px(pq[j]))[i]);
    }
    t=fne_(k_(f),k_(p),0);
    if(E(t)) { _k(p); _k(q); _k(r); return kerror("valence"); }
    pr[i]=t;
  }
  _k(p); _k(q);
  if(b==0&&m==1) { p=pr[0]; n(r)=0; _k(r); r=p; }
  return r;
}

static K each7(K f, K a, K x) {
  K r=0,e,*prk,p,*pp;
  i8 Ta, Tx;
  u64 n;
  Ta=ta; if(s(a)) { if(!VST(a)) return kerror("type"); Ta=15; }
  Tx=tx; if(s(x)) { if(!VST(x)) return kerror("type"); Tx=15; }
  p=params[paramsi++];
  pp=px(p);
  if(a) { /* dyadic */
    if(Ta<=0 && Tx<=0 && nx!=na) { --paramsi; return kerror("length"); }
    n=Ta>0?1:na;
    if(Tx>0 && Ta<=0) n=na;
    else if(Ta>0 && Tx<=0) n=nx;
    n(p)=2;
    if(Ta>0 && Tx>0) { /* both atoms */
      pp[0]=a; pp[1]=x;
      r=fne_(k_(f),k_(p),0);
      EC(r);
    }
    else {
      PRK(n);
      for(u64 i=0;i<n;i++) {
        pp[0]=Ta>0?a:xi(a,i,Ta); pp[1]=Tx>0?x:xi(x,i,Tx);
        prk[i]=fne_(k_(f),k_(p),0);
        if(Ta==-2) _k(pp[0]);
        if(Tx==-2) _k(pp[1]);
        EC(prk[i]);
      }
    }
  }
  else { /* monadic */
    n=(Tx>0)?1:nx;
    n(p)=1;
    if(Tx>0) { pp[0]=x; r=fne_(k_(f),k_(p),0); EC(r); }
    else { PRK(n); i(n,pp[0]=xi(x,i,Tx); prk[i]=fne_(k_(f),k_(p),0); if(Tx==-2) _k(pp[0]); EC(prk[i])) }
  }
  --paramsi;
  return r;
cleanup:
  --paramsi;
  _k(r);
  return e;
}

static K eachright7(K f, K a, K x) {
  K r=0,e,p,*pp,*prk;
  i8 Tx=tx; if(!Tx&&s(x)) { if(!VST(x)) return kerror("type"); Tx=15; }
  p=params[paramsi++]; n(p)=2; pp=px(p); pp[0]=a;
  if(Tx>0) { pp[1]=x; r=fne_(k_(f),k_(p),0); EC(r); }
  else { PRK(nx); i(nx,pp[1]=xi(x,i,Tx);prk[i]=fne_(k_(f),k_(p),0);if(Tx==-2)_k(pp[1]);EC(prk[i])) }
  n(p)=0; paramsi--;
  return r;
cleanup:
  n(p)=0; paramsi--;
  _k(r);
  return e;
}

static K eachleft7(K f, K a, K x) {
  K r=0,e,p,*pp,*prk;
  i8 Ta=ta; if(s(a)) { if(!VST(a)) return kerror("type"); Ta=15; }
  p=params[paramsi++]; n(p)=2; pp=px(p); pp[1]=x;
  if(Ta>0) { pp[0]=a; r=fne_(k_(f),k_(p),0); EC(r); }
  else { PRK(na); i(na,pp[0]=xi(a,i,Ta);prk[i]=fne_(k_(f),k_(p),0);if(Ta==-2)_k(pp[0]);EC(prk[i])) }
  n(p)=0; paramsi--;
  return r;
cleanup:
  n(p)=0; paramsi--;
  _k(r);
  return e;
}

static K eachfe(K f, K a, K x, char *av) {
  K r=0,e,*prk;
  i8 Ta,Tx;
  u64 n;
  Ta=ta; if(s(a)) { if(!VST(a)) return kerror("type"); Ta=15; }
  Tx=tx; if(s(x)) { if(!VST(x)) return kerror("type"); Tx=15; }
  if(a) { /* dyadic */
    if(Ta<=0 && Tx<=0 && nx!=na) return kerror("length");
    n=Ta>0?Tx>0?1:nx:na;
    if(Ta>0 && Tx>0) { /* both atoms */
      r=fe(k_(f),k_(a),k_(x),av);
      EC(r);
    }
    else {
      PRK(n);
      i(n,K a_=Ta>0?k_(a):xi_(a,i,Ta); K x_=Tx>0?k_(x):xi_(x,i,Tx); prk[i]=fe(k_(f),a_,x_,av); EC(prk[i]))
    }
  }
  else { /* monadic */
    n=Tx>0?1:nx;
    if(Tx>0) { r=fe(k_(f),0,k_(x),av); EC(r); }
    else { PRK(n); i(n,K x_=xi_(x,i,Tx); prk[i]=fe(k_(f),0,x_,av); EC(prk[i])) }
  }
  return r;
cleanup:
  _k(r);
  return e;
}

static K eachrightfe(K f, K a, K x, char *av) {
  K r=0,e,*prk;
  i8 Tx=tx; if(!Tx&&s(x)) { if(!VST(x)) return kerror("type"); Tx=15; }
  if(Tx>0) { r=fe(k_(f),k_(a),k_(x),av); EC(r); }
  else { PRK(nx); i(nx,prk[i]=fe(k_(f),k_(a),xi_(x,i,Tx),av);EC(prk[i])) }
  return r;
cleanup:
  _k(r);
  return e;
}

static K eachleftfe(K f, K a, K x, char *av) {
  K r=0,e,*prk;
  i8 Ta=ta; if(s(a)) { if(!VST(a)) return kerror("type"); Ta=15; }
  if(Ta>0) { r=fe(k_(f),k_(a),k_(x),av); EC(r); }
  else { PRK(na); i(na,prk[i]=fe(k_(f),xi_(a,i,Ta),k_(x),av);EC(prk[i])) }
  return r;
cleanup:
  _k(r);
  return e;
}

K avdo(K f, K a, K x, char *av) {
  K r=0;
  int w,n=strlen(av);
  char av2[256];
  if(n>32) { _k(f); _k(a); _k(x); r=kerror("length"); }
  else if(0xd1==s(f)||0xd2==s(f)||0xd3==s(f)) { _k(f); _k(a); _k(x); r=kerror("type"); }
  else if(n==1) {
    if(a) {
      if(!x) { r=kerror("type"); _k(f); _k(a); }
      else if(0xc3==s(f) || 0xc4==s(f)) {
        if(*av=='\'') r=each7(f,a,x);
        else if(*av=='/') r=eachright7(f,a,x);
        else if(*av=='\\') r=eachleft7(f,a,x);
        else r=kerror("type");
        _k(f); _k(a); _k(x);
      }
      else if(0xc7==s(f)||0xcd==s(f)) { /* builtin dyad */
        if(*av=='\'') r=eachfe(f,a,x,"");
        else if(*av=='/') r=eachrightfe(f,a,x,"");
        else if(*av=='\\') r=eachleftfe(f,a,x,"");
        else r=kerror("type");
        _k(f); _k(a); _k(x);
      }
      else if(0xc5==s(f)) {  /* a+/x */
        if(*av=='\'') r=eachfe(f,a,x,"");
        else if(*av=='/') r=eachrightfe(f,a,x,"");
        else if(*av=='\\') r=eachleftfe(f,a,x,"");
        else r=kerror("type");
        _k(f); _k(a); _k(x);
      }
      else {
        if(*av=='\'') r=k(32+(i32)f,a,x);
        else if(*av=='/') r=k(64+(i32)f,a,x);
        else if(*av=='\\') r=k(96+(i32)f,a,x);
        else { r=kerror("type"); _k(a); _k(x); }
      }
    }
    else {
      if(!x) { _k(f); r=kerror("type"); }
      else if(0x81==s(x)&&nx!=(u32)ik(val(f))&&*av!='\'') { r=kerror("valence"); _k(f); _k(x); }
      else if(0xc3==s(f) || 0xc4==s(f)) {
        if(*av=='\'') {
          if(0x81==s(x)) r=eachparam7(f,x);
          else r=each7(f,0,x);
        }
        else if(*av=='/') {
          if(ik(val(f))==1) r=overm7(f,x,"");
          else if(ik(val(f))==2) r=overd7(f,x,"");
          else if(0x81==s(x)&&nx==(u32)ik(val(f))) r=over7(f,x,"");
          else r=kerror("valence");
        }
        else if(*av=='\\') {
          if(ik(val(f))==1) r=scanm7(f,x,"");
          else if(ik(val(f))==2) r=scand7(f,x,"");
          else if(0x81==s(x)&&nx==(u32)ik(val(f))) r=scan7(f,x,"");
          else r=kerror("valence");
        }
        else r=kerror("type");
        _k(f); _k(x);
      }
      else if(0xc6==s(f)||0xcc==s(f)) { /* builtin monad */
        if(*av=='\'') r=eachfe(f,0,x,"");
        else if(*av=='/') r=overm(f,x,"");
        else if(*av=='\\')r=scanm(f,x,"");
        else r=kerror("type");
        _k(f); _k(x);
      }
      else if(0xc5==s(f)||0xc7==s(f)) {
        /* f'[a;x]; f/[a;x];   f\[a;x] */
        /* each     eachright  eachleft */
        if(*av=='\'') {
          if(0x81==s(x)&&nx==2) r=eachfe(f,((K*)px(x))[0],((K*)px(x))[1],"");
          else r=eachfe(f,0,x,"");
        }
        else if(*av=='/') {
          if(0x81==s(x)&&nx==2) r=eachrightfe(f,((K*)px(x))[0],((K*)px(x))[1],"");
          else r=overdfe(f,x,"");
        }
        else if(*av=='\\') {
          if(0x81==s(x)&&nx==2) r=eachleftfe(f,((K*)px(x))[0],((K*)px(x))[1],"");
          else r=scandfe(f,x,"");
        }
        else r=kerror("type");
        _k(f); _k(x);
      }
      else if(0xd4==s(f)||0xd0==s(f)) {
        if(*av=='\'') r=eachfe(f,0,x,"");
        else if(*av=='/') r=overm(f,x,"");
        else if(*av=='\\') r=scanm(f,x,"");
        else r=kerror("type");
        _k(f); _k(x);
      }
      else {
        w=strchr(P,*av)-P;
        if(0x81==s(x)) {
          if(nx==0) { r=k(w,(i32)f,null); _k(x); }
          else if(nx==1) { r=k(w,(i32)f,k_(((K*)px(x))[0])); _k(x); }
          else r=k(w,(i32)f,b(48)&x);
        }
        else r=k(w,(i32)f,x);
      }
    }
  }
  else {
    if(n>32) { _k(f); _k(a); _k(x); return kerror("length"); }
    snprintf(av2,sizeof(av2),"%s",av);
    av2[n-1]=0;
    if(a) {
      if(!x) r=kerror("type");
      else if(av[n-1]=='\'') r=each(f,a,x,av2);
      else if(av[n-1]=='/') {
        if(n>1 && ta==1 && 0xc3!=s(f)) r=overmonadn(k_(f),k_(a),k_(x),av2);
        else r=eachright(f,a,x,av2);
      }
      else if(av[n-1]=='\\') {
        if(n>1 && ta==1 && 0xc3!=s(f)) r=scanmonadn(k_(f),k_(a),k_(x),av2);
        else r=eachleft(f,a,x,av2);
      }
      _k(f); _k(a); _k(x);
    }
    else {
      if(!x) r=kerror("type");
      else if(av[n-1]=='\'') r=each(f,0,x,av2);
      else if(av[n-1]=='/') r=overm(f,x,av2);
      else if(av[n-1]=='\\') r=scanm(f,x,av2);
      _k(f); _k(x);
    }
  }
  return knorm(r);
}

K overmonadn(K f, K a, K x, char *av) {
  K r=0,e,p=0;
  i32 i=ik(a);
  if(i<0) { e=kerror("domain"); goto cleanup; }
  r=k_(x);
  while(i>0) {
    p=fe(k_(f),0,k_(r),av); EC(p);
    _k(r); r=p;
    --i;
    if(STOP) { STOP=0; e=kerror("stop"); goto cleanup; }
    if(EXIT) { e=kerror("abort"); goto cleanup; }
  }
  _k(f); _k(a); _k(x);
  return knorm(r);
cleanup:
  _k(f); _k(r); _k(a); _k(x);
  return e;
}

K scanmonadn(K f, K a, K x, char *av) {
  K r=0,e,p=0,*prk;
  i32 i=ik(a),j=0;
  if(i<0) { e=kerror("domain"); goto cleanup; }
  r=tn(0,1+i); prk=px(r);
  prk[j]=kcp2(x); if(E(prk[j])) { e=prk[j]; goto cleanup; } ++j;
  while(i>0) {
    p=fe(k_(f),0,k_(prk[j-1]),av); EC(p);
    prk[j++]=p;
    --i;
    if(STOP) { STOP=0; e=kerror("stop"); goto cleanup; }
    if(EXIT) { e=kerror("abort"); goto cleanup; }
  }
  _k(f); _k(a); _k(x);
  return knorm(r);
cleanup:
  _k(f); _k(r); _k(a); _k(x);
  return e;
}

K overmonadb(K f, K a, K x, char *av) {
  K r=0,e,p=0,b;
  r=k_(x);
  p=fe(k_(a),0,k_(r),av); EC(p);
  if(T(p)==1) b=ik(p);
  else b=1;
  _k(p);
  while(ik(b)) {
    p=fe(k_(f),0,k_(r),av); EC(p);
    _k(r); r=p;
    p=fe(k_(a),0,k_(r),av); //EC(p);
    if(E(p)) { e=p; goto cleanup; }
    if(T(p)==1) b=ik(p);
    else b=1;
    _k(p);
    if(STOP) { STOP=0; e=kerror("stop"); goto cleanup; }
    if(EXIT) { e=kerror("abort"); goto cleanup; }
  }
  _k(f); _k(a); _k(x);
  return knorm(r);
cleanup:
  _k(f);
  _k(r);
  _k(a);
  if(r!=x) _k(x);
  return e;
}

K scanmonadb(K f, K a, K x, char *av) {
  K r=0,e,p=0,*prk,b;
  i32 j=0,m=32;
  r=tn(0,m); prk=px(r); n(r)=0;
  prk[j]=kcp2(x); if(E(prk[j])) { e=prk[j]; goto cleanup; } ++j;
  n(r)++;
  p=fe(k_(a),0,k_(prk[j-1]),av); EC(p);
  if(T(p)==1) b=ik(p);
  else b=1;
  _k(p);
  while(ik(b)) {
    p=fe(k_(f),0,k_(prk[j-1]),av); EC(p);
    if(j==m) { m<<=1; r=kresize(r,m); n(r)=j; prk=px(r); }
    prk[j++]=p;
    n(r)++;
    p=fe(k_(a),0,k_(prk[j-1]),av); EC(p);
    if(T(p)==1) b=ik(p);
    else b=1;
    _k(p);
    if(STOP) { STOP=0; e=kerror("stop"); goto cleanup; }
    if(EXIT) { e=kerror("abort"); goto cleanup; }
  }
  _k(f); _k(a); _k(x);
  return knorm(r);
cleanup:
  _k(f); _k(r); _k(a); _k(x);
  return e;
}
