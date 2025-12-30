#include "k.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dict.h"
#include "fn.h"
#include "io.h"
#include "b.h"
#include "fe.h"
#include "irecur.h"

static K vlookup(K x) {
  K r=scope_get(cs,x);
  return r?r:KERR_VALUE;
}

static inline K cp(K x) {
  if(x&&s(x)&&T(x)==0&&((ko*)(b(48)&x))->r) {
    K p=kcp(x); if(E(p)) return 0;
    _k(x); x=p;
  }
  return x;
}
K atcb(K a,K x) {
  K r=KERR_TYPE,e,*prk,*pxk,t,*pt;
  char **pxs,s[2];
  static i32 d=0;
  if(++d>maxr) { --d; return KERR_STACK; }
  if(!a||!x) { --d; return KERR_TYPE; }
  if(4==ta&&!s(a)) { /* `d"a"  `f"x" */
    if(4==(a=vlookup(a))) {
       if(3==tx) { --d; return tn(0,0); }
       else { --d; return KERR_TYPE; }
     }
    _k(a);
  }
  if(0x80==s(a)) { /* dict */
    if(s(x)) { --d; return KERR_TYPE; }
    switch(tx) {
    case  3: s[0]=ck(x); s[1]=0; r=cp(dget(a,sp(s))); if(!r) r=null; break;
    case -3: r=cp(dget(a,sp((char*)px(x)))); if(!r) r=null; break;
    case  4: r=cp(dget(a,sk(x))); if(!r) r=null; break;
    case -4: PRK(nx); PXS; i(nx,prk[i]=cp(dget(a,pxs[i]));if(!prk[i]) prk[i]=null; EC(prk[i])) break;
    case  6: r=dvals(a); break;
    case  0: PRK(nx); PXK; i(nx,prk[i]=atcb(a,pxk[i]);if(!prk[i]) prk[i]=null; EC(prk[i])) break;
    default: r=KERR_TYPE;
    }
    --d;
    return knorm(r);
  }
  else if(0xc3==s(a)) {
    t=tn(0,1); pt=px(t);
    pt[0]=k_(x);
    r=fne(k_(a),t,0);
  }
  else if(0xc4==s(a)) {
    t=tn(0,1); pt=px(t);
    pt[0]=k_(x);
    r=fne(k_(a),t,0);
  }
  else if(0xc0==s(a)) {
    u8 c=ck(a);
    i32 w=c%32;
    r=k(w,0,k_(x));
  }
  else if(0xc6==s(a) || 0xc8==s(a)) {
    r=builtin(a,0,k_(x));
  }
  else if(0xc5==s(a)) {
    r=fc(a,0,k_(x),"");
  }
  else if(0xc1==s(a)) {
    r=fe(k_(a),0,k_(x),"");
  }
  else if(0xd7==s(a)) {
    r=fe(k_(a),0,k_(x),"");
  }
  //else r=k(13,k_(a),k_(x)); // symbol could have resolved to a primitive type
  --d;
  return r;
cleanup:
  --d;
  if(r) _k(r);
  return e;
}

K formatcb(K x) {
  K r=0;
  char *prc;
  switch(s(x)) {
  case 0x80: r=tn(3,5); prc=px(r); snprintf(prc,6,".(..)"); break;
  default: r=fivecolon(0,x);
  }
  return r;
}

extern K dot0(K a,K x);
K dotcb(K a,K x) {
  K r=0,e,t,*pr,*pt,v,*pv,x2,*px;
  char **pxs,b[2];
  static i32 d=0;
  if(++d>maxr) { --d; return KERR_STACK; }

  if(s(x)) { --d; return KERR_TYPE; }

  if(4==ta&&!s(a)) {
    K a2=scope_get(cs,a);
    if(E(a2)) { --d; return 0; }
    else if(4==T(a2)||3==T(a2)||-3==T(a2)) { --d; return a2; }
    else if(a2) {
      r=k(11,a2,k_(x)); // symbol could have resolved to a primitive type
      --d;
      return r;
    }
    else { --d; return a2; }
  }
  else if(3==ta&&!s(a)) {
    b[0]=ck(a); b[1]=0;
    K a2=scope_get(cs,t(4,sp(b)));
    if(E(a2)) { --d; return 0; }
    else if(4==T(a2)||3==T(a2)||-3==T(a2)) { --d; return a2; }
    else if(a2) {
      r=k(11,a2,k_(x)); // symbol could have resolved to a primitive type
      --d;
      return r;
    }
    else { --d; return a2; }
  }
  else if(-3==ta&&!s(a)) {
    K a2=scope_get(cs,t(4,sp((char*)px(a))));
    if(E(a2)) { --d; return 0; }
    else if(4==T(a2)||3==T(a2)||-3==T(a2)) { --d; return a2; }
    else if(a2) {
      r=k(11,a2,k_(x)); // symbol could have resolved to a primitive type
      --d;
      return r;
    }
    else { --d; return a2; }
  }

  if(0x80==s(a)) { /* dict */
    switch(tx) {
    case  6: r=atcb(a,x); break;
    case  4:
      if(!strlen(sk(x))) { r=KERR_DOMAIN; break; }
      r=dget(a,sk(x));
      if(!r) r=null;
      break;
    case -4:
      PXS;
      t=k_(a);
      for(size_t i=0;i<nx;i++) {
        if(!*pxs[i]) { _k(t); r=KERR_DOMAIN; break; }
        if(0x80!=s(t)) { _k(t); r=KERR_RANK; break; }
        r=dget(t,pxs[i]);_k(t);
        if(!r) r=null;
        t=r;
      }
      break;
    case  0:
      px=px(x);
      if(nx>=1 && (px[0]==null || px[0]==inull)) {
        /* d[;i] - apply remaining indices to each value, return list */
        v=dvals(a);
        if(E(v)) { --d; return v; }
        pv=px(v);
        u64 n=n(v);
        r=tn(0,n); pr=px(r);
        for(u64 j=0;j<n;j++) {
          if(nx==1) pr[j]=k_(pv[j]);  /* d[;] -> copy values */
          else if(nx==2) pr[j]=k(13,k_(pv[j]),k_(px[1]));  /* d[;i] -> value@i */
          else {  /* d[;i;j;...] */
            x2=k(16,t(1,1),k_(x));
            if(E(x2)) { _k(v); --d; return x2; }
            pr[j]=k(11,k_(pv[j]),x2);
          }
          if(E(pr[j])) { e=pr[j]; _k(r); _k(v); --d; return e; }
        }
        _k(v);
      }
      else r=dot0(a,x);
      break;
    default: r=KERR_TYPE;
    }
    --d; return knorm(r);
  }
  else if(0xc3==s(a)||0xc4==s(a)) {
    switch(tx) {
    case  0: r=fne(k_(a),k_(x),0); break;
    case -1: case -2: case -3: case -4:
      t=kmix(x); if(E(t)) { --d; return t; }
      r=fne(k_(a),t,0);
      break;
    default: t=tn(0,1); pt=px(t); pt[0]=k_(x); r=fne(k_(a),t,0); break;
    }
  }
  else if(0xc0==s(a)) {
    u8 c=ck(a);
    i32 w=c%32;
    switch(tx) {
    case -1: case -2: case -3: case -4: case 0:
      t=kmix(x); if(E(t)) { --d; return t; }
      pt=px(t);
      switch(n(t)) {
      case 1: r=k(w,0,k_(pt[0])); break;
      case 2: r=k(w,k_(pt[0]),k_(pt[1])); break;
      case 3: r=kamend3(k_(pt[0]),k_(pt[1]),k_(pt[2])); break;
      case 4: r=kamend4(k_(pt[0]),k_(pt[1]),k_(pt[2]),k_(pt[3])); break;
      default: r=KERR_VALENCE;
      }
      _k(t);
      break;
    default: r=k(w,0,k_(x));
    }
  }
  else if(0xc6==s(a)) {
    r=builtin(a,0,k_(x));
  }
  else if(0xc7==s(a)||0xc8==s(a)) {
    switch(tx) {
    case -1: case -2: case -3: case -4: case 0:
      t=kmix(x); if(E(t)) { --d; return t; }
      pt=px(t);
      switch(n(t)) {
      case 1: r=builtin(a,0,k_(pt[0])); break;
      case 2: r=builtin(a,k_(pt[0]),k_(pt[1])); break;
      default: r=KERR_VALENCE;
      }
      _k(t);
      break;
    default: r=builtin(a,0,k_(x));
    }
  }
  else if(0xc5==s(a)) {
    switch(tx) {
    case -1: case -2: case -3: case -4: case 0:
      t=kmix(x); if(E(t)) { --d; return t; }
      pt=px(t);
      switch(n(t)) {
      case 1: r=fc(a,0,k_(pt[0]),""); break;
      case 2: r=fc(a,k_(pt[0]),k_(pt[1]),""); break;
      default: r=KERR_VALENCE;
      }
      _k(t);
      break;
    default: r=fc(a,0,k_(x),"");
    }
  }
  else if(0xc1==s(a)) {
    switch(tx) {
    case -1: case -2: case -3: case -4: case 0:
      t=kmix(x); if(E(t)) { --d; return t; }
      pt=px(t);
      switch(n(t)) {
      case 1: r=fe(k_(a),0,k_(pt[0]),""); break;
      case 2: r=fe(k_(a),k_(pt[0]),k_(pt[1]),""); break;
      default: r=KERR_VALENCE;
      }
      _k(t);
      break;
    default: r=fe(k_(a),0,k_(x),"");
    }
  }
  else if(0xd0==s(a)) {
    switch(tx) {
    case -1: case -2: case -3: case -4: case 0:
      t=kmix(x); if(E(t)) { --d; return t; }
      pt=px(t);
      switch(n(t)) {
      case 1: r=fe(k_(a),0,k_(pt[0]),""); break;
      default: r=KERR_VALENCE;
      }
      _k(t);
      break;
    default: r=fe(k_(a),0,k_(x),"");
    }
  }
  else r=KERR_TYPE;
  --d;
  return r;
}
K enumeratecb(K x) {
  K r=0,v;
  if(0x80==s(x)) r=dkeys(x);
  else if(tx==4) {
    v=scope_get(ks,x); // ktree_get()
    if(0x80==s(v)) r=dkeys(v);
    else r=null;
    _k(v);
  }
  else r=KERR_TYPE;
  return r;
}
#define DMAX 100
K valuecb(K x) {
  K r=0,e,q,*pr,*px,kk,v,*pv,*t,m;
  char *pxc,*h,**pk,**ts;
  K i;
  size_t n;
  int opencode0;
  if(0x80==s(x)) {
    px=px(x);
    v=px[2]; px[2]=null;
    r=k(1,0,k_(b(48)&x));
    px[2]=v;
    return r;
  }
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case  3:
    h=xmalloc(3);
    h[0]=ck(x); h[1]='\n'; h[2]=0;
    opencode0=opencode; opencode=1;
    q=pgparse(h,0,0);
    opencode=opencode0;
    if(E(q)) r=q;
    else if(q) { r=pgreduce(q,0); prfree(q); }
    else r=KERR_PARSE;
    break;
  case  4:
    pxc=sk(x);
    n=strlen(pxc);
    if(!n) { r=k_(ktree); break; }
    h=xmalloc(n+2);
    i(n,h[i]=pxc[i]) h[n]='\n'; h[n+1]=0;
    opencode0=opencode; opencode=1;
    q=pgparse(h,0,0);
    opencode=opencode0;
    if(E(q)) r=q;
    else if(q) { r=pgreduce(q,0); prfree(q); }
    else r=KERR_PARSE;
    break;
  case -3:
    pxc=px(x);
    h=xcalloc(1,nx+2);
    i(nx,h[i]=pxc[i]) h[nx]='\n'; h[nx+1]=0;
    opencode0=opencode; opencode=1;
    q=pgparse(h,0,0);
    opencode=opencode0;
    if(E(q)) r=q;
    else if(q) {
      r=pgreduce(q,0); prfree(q);
      if(quiet) { _k(r); r=null; }
    }
    else r=KERR_PARSE;
    break;
  case  0:
    px=px(x);
    if(nx==2 && ISF(px[0])) {
      return k(11,k_(px[0]),k_(px[1]));
    }
    if(nx==2 && (T(px[0])==4 || T(px[0])==-3 || T(px[0])==3)) {
      if(T(px[0])==4) {
        K a=scope_get(cs,px[0]);
        if(E(a)) return KERR_VALUE;
        else if(4==T(a)||3==T(a)||-3==T(a)) return a;
        return k(11,a,k_(px[1]));
      }
      else if(T(px[0])==-3) {
        K a=scope_get(cs,t(4,sp((char*)px(px[0]))));
        if(E(a)) return KERR_VALUE;
        else if(4==T(a)||3==T(a)||-3==T(a)) return a;
        return k(11,a,k_(px[1]));
      }
      else if(T(px[0])==3) {
        char b[2];
        b[0]=ck(px[0]); b[1]=0;
        K a=scope_get(cs,t(4,sp(b)));
        if(E(a)) return KERR_VALUE;
        else if(4==T(a)||3==T(a)||-3==T(a)) return a;
        return k(11,a,k_(px[1]));
      }
    }
    int recur=1;
    i(nx,if(-3!=T(px[i])&&0!=T(px[i])){recur=0;break;})
    for(u64 i=0;i<nx;++i) {
      if(0!=T(px[i])&&-4!=T(px[i])) { /* dict? */
        if(recur) { /* .("10";"11") */
          //r=irecur1(valuecb,x); /* ref count issues due to k() freeing args, but valuecb doesn't */
          r=tn(0,nx); pr=px(r);
          for(i64 i=nx-1;i>=0;--i) {
            pr[i]=valuecb(px[i]);
            if(E(pr[i])) { e=pr[i]; _k(r); return e; }
          }
          return r;
        }
        return KERR_RANK;
      }
    }
    i(nx,if(2!=n(px[i])&&3!=n(px[i])) return KERR_LENGTH)
    r=st(0x80,tn(0,3));
    if(nx<DMAX) { kk=tn(4,DMAX); v=tn(0,DMAX); m=t(1,DMAX); }
    else { kk=tn(4,nx); v=tn(0,nx); m=t(1,nx); }
    n(kk)=nx;
    n(v)=nx;
    pr=px(r);
    pk=(char**)px(kk);
    pv=px(v);
    pr[0]=kk;
    pr[1]=v;
    pr[2]=m;
    for(i=0;i<nx;i++) {
      if(0==T(px[i])) {
        t=px(px[i]);
        if(3==n(px[i])&&6!=T(t[2])) { _k(r); return KERR_TYPE; }
        if(4!=T(t[0])) { _k(r); return KERR_TYPE; }
        pk[i]=sk(t[0]);
        pv[i]=k_(t[1]);
      }
      else if(-4==T(px[i])) {
        ts=px(px[i]);
        if(2!=(n(px[i]))) { _k(r); return KERR_VALENCE; }
        pk[i]=ts[0];
        pv[i]=t(4,ts[1]);
      }
      else { _k(r); return KERR_TYPE; }
    }
    break;
  default: r=KERR_TYPE;
  }
  return r;
}

static K form2w(char *t, i32 w, i32 z) {
  K r=0;
  i32 i,n,l,m;
  char *prc;
  l=strlen(t); m=abs(w); n=abs(l-m);
  if(m>l) {
    PRC(m);
    if(w<0) { for(i=0;i<l;i++) prc[i]=*t++; for(;i<m;i++) prc[i]=' '; }
    else if(w>0) { for(i=0;i<n;i++) prc[i]=' '; for(;i<m;i++) prc[i]=*t++; }
  } else if(m<l) {
    PRC(m);
    if(z) { i(m,*prc++='*') *prc=0; }
    else { if(w<0) { i(m,prc[i]=t[i]) } else { i(m,prc[i]=*(t+l-m+i)) } }
  }
  else { PRC(l); i(m,prc[i]=t[i]) }
  return r;
}
K formcb(K a, K x) {
  K r=0,*prk,t,p;
  i32 *pai;
  if((s(a)==0xc3||s(a)==0xc4)&&!s(x)) {
    switch(tx) {
    case -3: r=fnnew(px(x)); break;
    case  0: r=irecur2(formcb,a,x); break;
    default: r=KERR_TYPE; break;
    }
  }
  else if(!s(a)&&s(x)) {
    switch(ta) {
    case  1:
      VSIZE(abs(ik(a)));
      switch(s(x)) {
      case 0x80:
      case 0xc3:
      case 0xc4: t=formatcb(x); r=form2w(px(t),ik(a),1); _k(t); break;
      default: r=KERR_TYPE;
      } break;
    case -1:
      switch(s(x)) {
      case 0x80:
      case 0xc3:
      case 0xc4:
        PAI;
        i(na,VSIZE(abs(pai[i])))
        PRK(na);
        i(na,t=formatcb(x); p=form2w(px(t),pai[i],1); _k(t); prk[i]=p)
        break;
      default: r=KERR_TYPE;
      } break;
    default: r=KERR_TYPE;
    }
  }
  else r=KERR_TYPE;
  return r;
}

i32 vstcb(K x) {
  return VST(x);
}

K enlistcb(K x) {
  K r,*prk,t;
  if(!VST(x)) return KERR_TYPE;
  t=kcp(x); if(E(t)) return t;
  PRK(1);
  *prk=t;
  return r;
}
