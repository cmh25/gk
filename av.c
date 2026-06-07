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
    default: r=kerror("type");
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

/* Pass 5: each-conformable fold (over) on a multi-arg callable f
   with x being a 0x81 plist of N args.  Was over7 (0xc3-only via
   fne_); now type-agnostic through fe(). */
static K over_fe(K f, K x, char *av) {
  K r=0,e,p=0,q=0,*pp,*pq,*pxk;
  i64 m=-1;

  if(s(x)!=0x81) return kerror("type");
  if(nx!=(u32)ik(val(f))) return kerror("valence");

  pxk=px(x);
  int allatoms=1; i1(nx,if(T(pxk[i])<=0&&!s(pxk[i])){allatoms=0;break;})
  if(allatoms) { r=fe(k_(f),0,k_(x),av); return r; }

  q=tn(0,nx); pq=px(q);
  pq[0]=k_(pxk[0]);
  for(u64 i=1;i<nx;i++) {
    p=pxk[i];
    if(m==-1&&T(p)<=0) m=n(p);
    else if(m>=0&&T(p)<=0&&!s(p)&&m!=(i64)n(p)) { _k(q);  return kerror("valence"); }
    if(T(p)<0&&!s(p)) { pq[i]=kmix(p); EC(pq[i]); }
    else pq[i]=k_(p);
  }
  if(m==1) r=fe(k_(f),0,k_(x),av);
  else {
    p=st(0x81,tn(0,nx)); pp=px(p);
    r=k_(pxk[0]);
    pp[0]=null;
    for(i64 i=0;i<m;i++) {
      _k(pp[0]); pp[0]=k_(r);
      j1(nx,pp[j]=T(pq[j])>0||s(pq[j])?pq[j]:((K*)px(pq[j]))[i])
      _k(r);
      r=fe(k_(f),0,k_(p),av);
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

/* Pass 5: each-conformable scan on a multi-arg callable f with x
   a 0x81 plist of N args.  Was scan7 (0xc3-only); now via fe(). */
static K scan_fe(K f, K x, char *av) {
  K r=0,e,p=0,q=0,*pp,*pq,*pxk,*prk;
  i64 m=-1;

  if(s(x)!=0x81) return kerror("type");
  if(nx!=(u32)ik(val(f))) return kerror("valence");

  pxk=px(x);
  int allatoms=1; i1(nx,if(T(pxk[i])<=0&&!s(pxk[i])){allatoms=0;break;})
  if(allatoms) { r=fe(k_(f),0,k_(x),av); return r; }

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
  if(m==1) r=fe(k_(f),0,k_(x),av);
  else {
    r=tn(0,m+1); prk=px(r);
    p=st(0x81,tn(0,nx)); pp=px(p);
    prk[0]=k_(pxk[0]);
    for(i64 i=0;i<m;i++) {
      pp[0]=prk[i];
      j1(nx,pp[j]=T(pq[j])>0||s(pq[j])?pq[j]:((K*)px(pq[j]))[i])
      prk[i+1]=fe(k_(f),0,k_(p),av); EC(prk[i+1]);
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

/* overm7 retired in Pass 5 -- overm now handles 0xc3 too. */

static K overm(K f, K x, char *av) {
  K r=0,e,p,q;
  int b=0,n=strlen(av);
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

/* scanm7 retired in Pass 5 -- scanm now handles 0xc3 too. */

static K scanm(K f, K x, char *av) {
  K r=0,t=0,e,q=0,*prk;
  int b=0,n=strlen(av),zi=0,zm=32;
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

/* overd7/scand7 retired in Pass 5 -- overdfe/scandfe handle 0xc3 too. */

/* Pass 5: each-conformable apply on a multi-arg callable f with x
   a 0x81 plist of args.  Was eachparam7 (0xc3-only via fne_); now
   type-agnostic through fe(). */
static K eachparamfe(K f, K x) {
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
  /* Pass 5: tag plist as 0x81 so fe()'s 0xc3 path routes through
     fne(f, plist, "") rather than wrapping in a 1-elt plist (which
     would project a multi-arg lambda instead of fully applying it). */
  p=st(0x81,tn(0,nx)); pp=px(p);
  for(u64 i=0;i<n(r);++i) {
    for(u64 j=0;j<n(p);++j) {
      _k(pp[j]);
      pp[j]=T(pq[j])||s(pq[j]) ? k_(pq[j]) : k_(((K*)px(pq[j]))[i]);
    }
    t=fe(k_(f),0,k_(p),"");
    if(E(t)) { _k(p); _k(q); _k(r); return kerror("valence"); }
    pr[i]=t;
  }
  _k(p); _k(q);
  if(b==0&&m==1) { p=pr[0]; n(r)=0; _k(r); r=p; }
  return r;
}

/* each7/eachright7/eachleft7 retired in Pass 5 -- eachfe / eachrightfe
   / eachleftfe handle 0xc3 too. */

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

/* Pass 5 -- avdo unification.  Dispatch is on the adverb string, not
   on s(f).  Per-element calls go through fe() uniformly, so f can be
   any callable subtype (0xc3, 0xc5, 0xc6, 0xc7, 0xcc, 0xcd, 0xd0,
   0xd9, 0xda) or a primitive verb (numeric ff). */
K avdo(K f, K a, K x, char *av) {
  K r=0;
  int w,n=strlen(av);
  char av2[256];
  /* Force monad: a {Vx} lambda is semantically the bare MONADIC primitive V,
     so a monadic adverb application (each/over/scan over one list) must
     dispatch identically -- and in bulk, not by iterating the lambda per
     element.  Swap it for V's raw verb code here (the choke point all adverb
     dispatch funnels through), taking the isprim path below.  Guard with !a &&
     x-not-a-plist: a left arg (seed/count, e.g. 5{,x}\1) or a 0x81 multi-arg
     plist means the form is dyadic/iterate-n where V's bare-primitive default
     valence (dyadic for `,`) would differ from the monadic force -- leave
     those on the lambda path.  pf[3]'s high bits cache V's FM[]/P index (0 = not a force
     monad). */
  if(0xc3==s(f) && !a && 0x81!=s(x)) {
    K *pf=px(f); int fm=FN_FMIDX(pf[3]); if(fm) { _k(f); f=(K)(i64)fm; }
  }
  int isfn=VST(f) && (s(f)&0x40); /* f is a callable subtype */
  int isprim=!s(f) && !T(f);      /* numeric primitive verb code */
  static int d=0;

  /* Issue #3 fuzz fix: avdo<->overm/scanm can recurse tightly without
     going through fne_'s depth guard (each cycle peels one av char +
     does fixed-point iter), so long adverb strings on a recursive
     lambda blow the stack before maxr fires.  Track depth here too. */
  if(++d>maxr) { --d; _k(f); _k(a); _k(x); return kerror("stack"); }

  if(0x85==s(a)||0x85==s(x)) { _k(f); _k(a); _k(x); r=kerror("type"); }
  else if(n>32) { _k(f); _k(a); _k(x); r=kerror("length"); }
  else if(0xd1==s(f)||0xd2==s(f)||0xd3==s(f)) { _k(f); _k(a); _k(x); r=kerror("type"); }
  else if(n==1) {
    if(a) {
      /* dyadic: iterate (a,x) in lockstep, leftward, or rightward */
      if(!x) { r=kerror("type"); _k(f); _k(a); }
      else if(isfn) {
        if(*av=='\'') r=eachfe(f,a,x,"");
        else if(*av=='/') r=eachrightfe(f,a,x,"");
        else if(*av=='\\') r=eachleftfe(f,a,x,"");
        else r=kerror("type");
        _k(f); _k(a); _k(x);
      }
      else if(isprim) {
        /* primitive dyadic verb -- dispatch to k() with adverb-encoded
           verb id (32 = each, 64 = right, 96 = left). */
        if(*av=='\'') r=k(32+(i32)f,a,x);
        else if(*av=='/') r=k(64+(i32)f,a,x);
        else if(*av=='\\') r=k(96+(i32)f,a,x);
        else { r=kerror("type"); _k(a); _k(x); }
      }
      else { _k(f); _k(a); _k(x); r=kerror("type"); }
    }
    else {
      /* monadic */
      if(!x) { _k(f); r=kerror("type"); }
      else if(isfn && 0x81==s(x) && nx!=(u32)ik(val(f)) && *av!='\'') {
        /* over/scan with packed-args plist must match valence */
        r=kerror("valence"); _k(f); _k(x);
      }
      else if(isfn) {
        i32 vf = ik(val(f));
        /* In monadic avdo context, builtin-monad subtypes report
           val=2 (because they also support the dyadic call) but
           the effective monadic valence is 1.  Override here so
           over/scan dispatch picks fixed-point monad form. */
        u64 sf=s(f);
        if(sf==0xc6 || sf==0xcc || sf==0xc9 || sf==0xce) vf=1;
        if(*av=='\'') {
          /* each: if x is a packed-args plist of length val(f), do
             conformable each-multi-arg (works for any vf, val=1
             included -- it just iterates the single inner list).
             Issue #2 Pass 5 step 3: when f is a projection-like
             (0xd9/0xda) and x is a partial-arg shape (atom, or
             0x81 plist with nx<vf), don't iterate -- forward to
             fapply with the each preserved.  fapply's NEST branch
             lifts av onto an outer 0xda so chained `'[args]` keep
             composing. */
          if(0x81==s(x) && nx==(u32)vf) r=eachparamfe(f,x);
          else if((0xd9==s(f) || 0xda==s(f))
                  && (tx>0 || (0x81==s(x) && nx<(u32)vf))) {
            K xx;
            if(0x81==s(x)) xx=k_(x);
            else { K t=tn(0,1); ((K*)px(t))[0]=k_(x); xx=st(0x81,t); }
            r=fapply(k_(f),xx,av);
          }
          else r=eachfe(f,0,x,"");
        }
        else if(*av=='/') {
          if(vf==1) r=overm(f,x,"");
          else if(vf==2) {
            /* val=2: if x is {a,b} plist, use each-right; else fold */
            if(0x81==s(x) && nx==2) r=eachrightfe(f,((K*)px(x))[0],((K*)px(x))[1],"");
            else r=overdfe(f,x,"");
          }
          else if(0x81==s(x) && nx==(u32)vf) r=over_fe(f,x,"");
          else r=kerror("valence");
        }
        else if(*av=='\\') {
          if(vf==1) r=scanm(f,x,"");
          else if(vf==2) {
            if(0x81==s(x) && nx==2) r=eachleftfe(f,((K*)px(x))[0],((K*)px(x))[1],"");
            else r=scandfe(f,x,"");
          }
          else if(0x81==s(x) && nx==(u32)vf) r=scan_fe(f,x,"");
          else r=kerror("valence");
        }
        else r=kerror("type");
        _k(f); _k(x);
      }
      else if(isprim) {
        w=strchr(P,*av)-P;
        if(0x81==s(x)) {
          if(nx==0) { r=k(w,(i32)f,null); _k(x); }
          else if(nx==1) { r=k(w,(i32)f,k_(((K*)px(x))[0])); _k(x); }
          else r=k(w,(i32)f,b(48)&x);
        }
        else r=k(w,(i32)f,x);
      }
      else { _k(f); _k(x); r=kerror("type"); }
    }
  }
  else {
    /* multi-char adverb -- peel last char and recurse */
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
      else if(av[n-1]=='\'') {
        /* Issue #2 Pass 7/9: multi-char each on a multi-arg plist
           iterates the newest arg.  Each chained `'` iterates one
           arg slot.  Pre-Pass-9 used pxk[n-1] (av-indexed) which
           broke when a projection had merged-in scalar slots
           between bound and new args (e.g. `f[;2;]'[3 4]'[5 6]`
           merges to [vec(3 4), 2, vec(5 6)] with av "''"; the
           outer `'` should iterate vec(5 6) at slot 2, not the
           scalar at slot 1).  Pass 9: scan backward from
           pxk[nx-1] for the first iterable (non-atom, non-
           subtype, n>1) slot -- skip scalar slots from inner
           projections (e.g. `f[;2;]` binds slot 1 to 2, a non-
           iterable scalar) and resolved-scalar slots from
           previous outer iterations.  Per-iter: replace that
           slot with a scalar and recurse with av shorter by one.
           When n drops to 1, eachparamfe handles conform-zip
           over the remaining slots.  For non-plist x or nx<n,
           fall back to simple each. */
        if(0x81==s(x) && nx>=(u32)n) {
          K *pxk=px(x);
          i64 ti=-1;
          K target=0;
          char Tt=0;
          u64 nt=0;
          for(i64 k=(i64)nx-1; k>=0; --k) {
            K c=pxk[k];
            if(!s(c) && T(c)<=0 && n(c)>1) {
              ti=k; target=c; Tt=T(c); nt=n(c);
              break;
            }
          }
          if(ti>=0) {
            r=tn(0,nt); K *pr=px(r);
            K err=0;
            for(u64 i=0;i<nt;++i) {
              K xnew=tn(0,nx); K *pxn=px(xnew);
              for(u64 j=0;j<nx;++j) {
                if(j==(u64)ti) {
                  switch(Tt) {
                  case -1: { i32 *p=(i32*)px(target); pxn[j]=t(1,(u32)p[i]); break; }
                  case -2: { double *p=(double*)px(target); pxn[j]=t2(p[i]); break; }
                  case -3: { char *p=(char*)px(target); pxn[j]=t(3,(u8)p[i]); break; }
                  case -4: { char **p=(char**)px(target); pxn[j]=t(4,p[i]); break; }
                  case  0: { K *p=(K*)px(target); pxn[j]=k_(p[i]); break; }
                  default: pxn[j]=k_(target);
                  }
                }
                else pxn[j]=k_(pxk[j]);
              }
              K xn=st(0x81,xnew);
              pr[i]=avdo(k_(f),0,xn,av2);
              if(E(pr[i])) { err=pr[i]; n(r)=i; _k(r); r=0; break; }
            }
            if(err) r=err;
          }
          else r=avdo(k_(f),0,k_(x),av2);
        }
        else r=each(f,0,x,av2);
      }
      else if(av[n-1]=='/') r=overm(f,x,av2);
      else if(av[n-1]=='\\') r=scanm(f,x,av2);
      _k(f); _k(x);
    }
  }
  --d;
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
  _k(x);
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
