#include "p.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "k.h"
#include "scope.h"
#include "lex.h"
#include "timer.h"
#include "repl.h"
#include "pnp.h"
#include "fn.h"
#include "b.h"
#include "av.h"
#include "fe.h"

/*
s > e se | se
e > o ez
se > ';' | '\n'
o > N | V | klist
ez > | e | plist
klist > '(' elist ')'
plist > '[' elist ']' pz
pz > | e | plist
elist > ee elistz
elistz > | se ee elistz
ee > | e
*/

static char *P=":+-*%&|<>=~.!@?#_^,$'/\\";
static int inkl,inpl;

int RETURN;
int quiet;
int STOP,EFLAG=1,SIGNAL;
K EXIT;
int opencode=1;
char *pfile="";
int gline,glinei,gline0,gline0i,fileline;
char *glinep,*gline0p;

#define PARAMSMAX 1024
K params[PARAMSMAX];
int paramsi;
void pinit(void) { i(PARAMSMAX,params[i]=st(0x81,tn(0,32))) }
void pexit(void) { i(PARAMSMAX,n(params[i])=0;_k(params[i])) }

#define GROW() do { \
  s->Sm<<=1; \
  s->S=xrealloc(s->S,s->Sm*sizeof(int)); \
  s->Rm<<=1; \
  s->R=xrealloc(s->R,s->Rm*sizeof(int)); \
  s->Vm<<=1; \
  s->V=xrealloc(s->V,s->Vm*sizeof(pn*)); } while(0);

#define LI 12
#define LJ 21
static int LL[12][21]={
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,0,0,0,0,-1,-1,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,2,2,1,1,1,-1,-1,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,3,3,3,-1,-1,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,4,5,-1,-1,-1,-1,-1,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,6,7,8,-1,-1,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,9,9,10,10,10,9,11,9,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,12,-1,-1,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,13,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,14,14,15,15,15,14,16,14,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,17,17,17,17,17,17,-1,17,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,19,19,-1,-1,-1,18,-1,18,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,20,20,21,21,21,20,-1,20,-1}
};

static int RT[22][4]={
{T001},
{T002,T003},
{T003},
{T004,T005},
{T012},
{T013},
{T014},
{T015},
{T006},
{-1},
{T002},
{T007},
{T016,T009,T017},
{T018,T009,T019,T008},
{-1},
{T002},
{T007},
{T011,T010},
{-1},
{T003,T011,T010},
{-1},
{T002}
};

static int RC[22]={1,2,1,2,1,1,1,1,1,0,1,1,3,4,0,1,1,2,0,3,0,1};

#define Vvi s->V[s->vi]

static K stripav(K x, char **av) {
  char a2[256];
  u8 t=tx;
  u8 st=s(x);
  **av=0;
  if(strlen(sk(x))>255) return KERR_LENGTH;
  snprintf(a2,sizeof(a2),"%s",sk(x));
  char *p=strpbrk(a2,"'/\\");
  if(p) { snprintf(*av,256,"%s",p); *p=0; }
  return t(t,st(st,sp(a2)));
}

static K vlookupav(K x) {
  K *pr,fav;
  ko *kr;
  char a2[256],avb[256],*av=avb;
  *av=0;
  if(strlen(sk(x))>255) return KERR_LENGTH;
  snprintf(a2,sizeof(a2),"%s",sk(x));
  char *p=strpbrk(a2,"'/\\");
  if(p) { snprintf(av,sizeof(avb),"%s",p); *p=0; }
  K r=scope_get(cs,t(4,sp(a2)));
  if(r&&*av) {
    switch(s(r)) {
    case 0xc3:
      kr=(ko*)(b(48)&r);
      kr->r--;
      r=kcp(r); if(E(r)) return r;
      pr=px(r);
      fav=pr[3];
      if(T(fav)==6) {
        pr[3]=tnv(3,strlen(av),xmemdup(av,1+strlen(av)));
      }
      else {
        u64 n0=n(fav);
        fav=kresize(fav,n(fav)+strlen(av));
        char *pfav=px(fav);
        snprintf(pfav+n0,strlen(av),"%s",av);
      }
      break;
    case 0xc5:
      kr=(ko*)(b(48)&r);
      kr->r--;
      r=kcp(r); if(E(r)) return r;
      pr=px(r);
      fav=pr[1];
      char *pfav=px(fav);
      u64 n=strlen(pfav)+strlen(av);
      if(n > 255) return KERR_LENGTH;
      char av2[256];
      snprintf(av2,256,"%s%s",pfav,av);
      _k(pr[1]);
      pr[1]=tnv(3,n,xmemdup(av2,1+strlen(av2)));
      break;
    }
  }
  return r?r:4;
}
static K vlookup(K x) {
  K r;
  if(4==T(x) && strpbrk(sk(x),"'/\\")) return vlookupav(x);
  else r=scope_get(cs,x);
  return r?r:KERR_VALUE;
}
static K vlookuprs(K x, K *rs) {
  K r=0,s=cs,*ps;
  char *n=sk(x);
  while(s!=null&&s!=ks) {
    ps=px(s);
    if(0x80!=s(ps[1])) break;
    r=dget(ps[1],n);
    if(r) { *rs=s; break; }
    if(null==ps[2]) s=ps[0];
    else break;
  }
  if(!r) {
    r=scope_get(gs,x);
    if(!E(r)) *rs=gs;
  }
  return r?r:KERR_VALUE;
}

/*  v: error
   x0: (values;index;stmt;line;file;gline;ggline)
    i: index */
static void printerror(K v, K x0, int i) {
  K *px0=px(x0),*ps;
  int *pindex=px(px0[1]);
  int *pline=px(px0[3]);
  char *pfile=px(px0[4]);
  char *ss0,*ss;
  int n;
  if(fnestacki>=0 && !opencode) { /* in lambda */
    K *pf=px(fnestack[fnestacki]);
    K ffs=ksplit(px(pf[0]),"\r\n");
    K f0=((K*)px(ffs))[0];
    char *ff0=px(f0);
    if(pline[i]) { /* multiline lambda */
      if(strlen(pfile)) {
        fprintf(stderr,"%s ... + %d in %s:%d\n",ff0,pline[i],pfile,1+pline[i]+ik(px0[6]));
        LOADLINE=1+pline[i]+ik(px0[6]);
      }
      else fprintf(stderr,"%s ... + %d\n",ff0,pline[i]);
      kprint(v,"","\n","");
      if(pline[i]<(int)n(px0[2])) {
        ps=px(px0[2]);
        ss=px(ps[pline[i]]);
        fprintf(stderr,"%s\n",ss);
      }
      i(pindex[i],putc(' ',stderr)); putc('^',stderr); putc('\n',stderr);
    }
    else { /* single line lambda */
      if(strlen(pfile)) {
        fprintf(stderr,"in %s:%d\n",pfile,1+ik(px0[6]));
        LOADLINE=1+ik(px0[6]);
      }
      kprint(v,"","\n","");
      fprintf(stderr,"%s\n",ff0);
      // if ff0 has args like {[a;b;c]a+b+c+`}
      // have to advance the error indicator by the length of [a;b;c]
      n=0;
      if(*ff0=='{') { ++n; ++ff0; }
      while(*ff0&&isspace(*ff0)) { ++n; ++ff0; }
      if(*ff0=='[') {
        ++n; ++ff0;
        while(*ff0 && *ff0!=']') { ++ff0; ++n; }
        ++n;
      }
      i(n+pindex[i],putc(' ',stderr)); putc('^',stderr); putc('\n',stderr);
    }
    _k(ffs);
  }
  else if((inkl&&!inpl)||inpl) {
    ss0="linenotfound";
    i32 i5=ik(px0[5]);
    if(i5>=0 && (u64)i5<n(px0[2])) {
      K s0=((K*)px(px0[2]))[i5];
      ss0=px(s0);
    }
    if(pline[i]!=i5) { /* multiline list */
      if(strlen(pfile)) {
        fprintf(stderr,"%s ... + %d in %s:%d\n",ss0,pline[i]-i5,pfile,1+pline[i]+ik(px0[6]));
        LOADLINE=1+pline[i];
      }
      else fprintf(stderr,"%s ... + %d\n",ss0,pline[i]);
      kprint(v,"","\n","");
      if(pline[i]<(int)n(px0[2])) {
        ps=px(px0[2]);
        ss=px(ps[pline[i]]);
        fprintf(stderr,"%s\n",ss);
      }
      i(pindex[i],putc(' ',stderr)); putc('^',stderr); putc('\n',stderr);
    }
    else { /* single line list */
      if(strlen(pfile)) {
        fprintf(stderr,"in %s:%d\n",pfile,1+ik(px0[6]));
        LOADLINE=1+pline[i];
      }
      kprint(v,"","\n","");
      fprintf(stderr,"%s\n",ss0);
      i(pindex[i],putc(' ',stderr)); putc('^',stderr); putc('\n',stderr);
    }
  }
  else { /* open code */
    if(strlen(pfile)) {
      fprintf(stderr,"in %s:%d\n",pfile,1+pline[i]+ik(px0[6]));
      LOADLINE=1+pline[i];
    }
    kprint(v,"","\n","");
    if(pline[i]<(int)n(px0[2])) {
      ps=px(px0[2]);
      ss=px(ps[pline[i]]);
      fprintf(stderr,"%s\n",ss);
    }
    i(pindex[i],putc(' ',stderr)); putc('^',stderr); putc('\n',stderr);
  }
}

static K rl(K x, K t) {
  K r,a,*pr,*px,p;
  int q;
  i64 i;
  px=px(x);
  if(0x41==s(x) && 0==nx) { /* [] -> [nul] */
    r=tn(0,1);
    ((K*)px(r))[0]=null;
    _k(x);
    RETURN=0;
    return st(t,r);
  }
  else if(0x42==s(x) && 1==nx) { /* (1) -> 1 */
    r=pgreduce_(px[0],&q);
    _k(x);
    RETURN=0;
    return r;
  }
  else {
    r=tn(0,nx);
    if(nx) {
      pr=px(r);
      for(i=nx-1;i>=0;--i) {
        a=pgreduce_(px[i],&q);
        if(!a&&t==0) a=null;
        else if(!a&&t==0x81) a=inull;
        if(a && E(a)&&!strcmp(sk(a),"value")) {
          if(EFLAG&&!SIGNAL&&strcmp(sk(a),"abort")) {
            printerror(a,px[i],0);
            ++ecount;
            a=repl();
          }
        } // note: when running like "./gk < file", repl() can return anything
        if(E(a)||EXIT){_k(r);_k(x);RETURN=0;return a;}
        if(s(a)&&T(a)==0&&((ko*)(b(48)&a))->r) {
          p=kcp(a); _k(a); a=p;
        }
        if(a<EMAX) a=kerror(E[a]);
        if(a && E(a)) {
          _k(x); _k(r);
          return a;
        }
        pr[nx-i-1]=a;
      }
    }
    _k(x);
    RETURN=0;
    return st(t,r);
  }
}

static K assign(K d, K i, K y) {
  K r=0,p=0,v=d,rs=0;
  if(!d||!i||!y) { _k(d); d=KERR_TYPE; goto cleanup; }
  if(0x40!=s(d)) { _k(d); d=KERR_TYPE; goto cleanup; }
  d=vlookuprs(d,&rs);
  if(d==KERR_VALUE) { /* d[`a]:1 */
    if(T(i)==4 || T(i)==-4) { rs=gs; d=dnew(); }
    else goto cleanup; /* value */
  }
  if(E(d)) goto cleanup;
  if(s(d)!=0x80 && T(d)>0) { _k(d); d=KERR_TYPE; goto cleanup; }
  if(0x40==s(i)) { i=vlookup(i); if(E(i)) { _k(d); d=i; goto cleanup; } }
  if(0x40==s(y)) { y=vlookup(y); if(E(y)) { _k(d); d=y; goto cleanup; } }
  if(s(y)&&s(y)!=0x80&&s(y)!=0xc3&&s(y)!=0xc4) { _k(d); d=KERR_TYPE; goto cleanup; }
  if(s(i)&&0x81!=s(i)) { _k(d); d=KERR_TYPE; goto cleanup; }
  ko *kd=(ko*)(b(48)&d);
  if(kd->r>1) { /* copy on write */
    --kd->r;
    d=kcp(d); if(E(d)) goto cleanup;
  }

  if(s(i)&&0x81==s(i)) r=kamend4(d,b(48)&i,0,k_(y));
  else r=kamendi4(d,i,0,k_(y));
  if(E(r)||EXIT) { _k(y); return r; }
  else {
    if(v) p=scope_set(rs,v,r);
    _k(r);
    if(p) { _k(y); return p; }
    return knorm(y);
  }

cleanup:
  _k(i); _k(y);
  return d;
}

/* resolve predefined */
static K rpd(K x) {
  K f,*pf;
  char *cf=sk(x);
  char avb[256],a2[256],*av=avb;
  *av=0;
  if(strlen(cf)>255) return KERR_LENGTH;
  snprintf(a2,sizeof(a2),"%s",cf);
  char *p=strpbrk(a2,"'/\\");
  if(p) { snprintf(av,sizeof(avb),"%s",p); *p=0; }
  if(!(f=dget(C,sp(a2)))) return KERR_VALUE;
  K f2=kcp(f); _k(f); if(E(f2)) return f2;
  f=f2;
  pf=px(f);
  pf[3]=tnv(3,strlen(av),xmemdup(av,1+strlen(av)));
  return f;
}

static inline K r41(K x);
static K rc5(K x);
static K r44(K x) {
  K r,t;
  char avb[256],f2[256],*av=avb;
  *av=0;

  K *px=px(x);
  K f=k_(px[0]);
  K x1=k_(px[1]);
  K x2=0,*px2;

  if(0x40==s(x1)) {
    K *px1=px(x1);
    x2=tn(0,n(x1)); px2=px(x2);
    for(u64 i=0;i<n(x1);++i) {
      t=0x41==s(px1[i])?r41(k_(px1[i])):k_(px1[i]);
      if(E(t)||EXIT) { _k(f); _k(x1); _k(x2); _k(x); return t; }
      px2[i]=t;
    }
  }
  else if(0x41==s(x1))  {
    x2=r41(k_(x1));
    if(E(x2)||EXIT) { _k(f); _k(x1); _k(x); return x2; }
    px2=px(x2);
  }
  else if(0x81==s(x1)) {
    x2=k_(x1);
    px2=px(x2);
  }

  /* resolve predefined */
  if(0xca==s(f) || 0xc9==s(f) || 0xcb==s(f)) {
    f=rpd(f);
    if(E(f)||EXIT) { _k(x1); _k(x2); _k(x); return f; }
  }

  /* consolidate adverbs and resolve f */
  if(0x40==s(f)) {
    if(4==T(f)) {
      char *skf=sk(f);
      if(strlen(skf)>255) { _k(x1); _k(x2); _k(x); return KERR_LENGTH; }
      if(!strchr("'/\\",*skf)) {
        f=vlookup(f);
      }
      else {
        snprintf(f2,sizeof(f2),"%s",skf);
        char *p=strpbrk(f2,"'/\\");
        if(p) { snprintf(av,sizeof(avb),"%s",p); *p=0; }
        f=vlookup(t(4,sp(f2)));
      }
    }
    else f=vlookup(f);
    if(E(f)) { _k(x1); _k(x2); _k(x); return f; }
  }
  else if(0xc5==s(f)) {
    f=rc5(f);
    if(E(f)) { _k(x1); _k(x2); _k(x); return f; }
  }
  else if(0x44==s(f)) {
    f=r44(f);
    if(E(f)) { _k(x1); _k(x2); _k(x); return f; }
  }

  if(f<256) f=t(1,st(0xc0,strchr(P,f)-P+32));
  if(0x40==s(x1)) { /* f[1][2] */
    r=f;
    for(u64 i=0;i<n(x1);++i) {
      K x3=px2[i];
      K *px3=px(x3);
      if(ISF(r)) {
        if(n(x3)==1) r=fe(r,0,k_(px3[0]),av);
        else if(n(x3)==2) r=fe(r,k_(px3[0]),k_(px3[1]),av);
        else r=fe(r,0,k_(x3),av);
      }
      else {
        if(n(x1)==1) r=k(13,r,k_(px3[0]));
        else r=k(11,r,b(48)&k_(x3)); /* f . x */
      }
      if(E(r)||EXIT) { _k(x1); _k(x2); _k(x); return r; }
    }
  }
  else if(0x41==s(x1)||0x81==s(x1))  { /* f[1] */
    if(ISF(f)) {
      if(n(x1)==1) r=fe(f,0,k_(px2[0]),av);
      else if(n(x1)==2) r=fe(f,k_(px2[0]),k_(px2[1]),av);
      else r=fe(f,0,k_(x2),av);
    }
    else {
      if(T(f)>0&&f!=null) { if(T(f)==2) _k(f); r=KERR_RANK; }
      else if(n(x1)==1) r=k(13,f,k_(px2[0])); /* f @ x */
      else r=k(11,f,b(48)&k_(x2)); /* f . x */
    }
  }
  else r=k(13,f,k_(x2)); /* f @ x */

  _k(x1);
  _k(x2);
  _k(x);
  return r;
}

static K rd0(K x) {
  K *px=px(x);
  if(0x42==s(px[0])) {
    K t=rl(k_(px[0]),0);
    if(E(t)||EXIT) { _k(x); return t; }
    K p=kcp(t); _k(t);
    if(E(p)) { _k(x); return p; }
    _k(px[0]);
    px[0]=p;
  }
  else if(0x40==s(px[0])) {
    K t=vlookup(px[0]);
    if(E(t)) { _k(x); return t; }
    K p=kcp(t); _k(t);
    if(E(p)) { _k(x); return p; }
    _k(px[0]);
    px[0]=p;
  }

  if(0xca==s(px[1]) || 0xc9==s(px[1]) || 0xcb==s(px[1])) {
    K f=rpd(px[1]);
    if(E(f)||EXIT) { _k(x); return f; }
    K p=kcp(f); _k(f);
    if(E(p)) { _k(x); return p; }
    _k(px[1]);
    px[1]=p;
  }
  else if(0xd1==s(px[1]) || 0xd2==s(px[1]) || 0xd3==s(px[1])) {
    _k(x);
    return KERR_PARSE;
  }
  return x;
}

static K rc5(K x) {
  K *px=px(x);
  K f=px[0];
  K *pf=px(f);
  for(i64 i=n(f)-1;i>=0;--i) {
    if(0xd0==s(pf[i])) {
      K t=rd0(k_(pf[i]));
      if(E(t)||EXIT) { _k(x); return t; }
      _k(pf[i]);
      pf[i]=t;
    }
    else if(0xca==s(pf[i])||0xc9==s(pf[i])||0xcb==s(pf[i])) {
      K t=rpd(pf[i]);
      if(E(t)||EXIT) { _k(x); return t; }
      _k(pf[i]);
      pf[i]=t;
    }
  }
  return x;
}

static inline K r41(K x) {
  inpl=1;
  K r=rl(x,0x81);
  inpl=0;
  return r;
}
static inline K r42(K x) {
  inkl=1;
  K r=knorm(rl(x,0));
  inkl=0;
  return r;
}
static inline K r40(K x) {
  return vlookup(x);
}

// one-step reduction for certain subtypes
// if no reduction is needed, returns x unchanged
// caller must check E(x)
static inline K reduce(K x) {
  K r;
  switch(s(x)) {
  case 0x40: // variable
    r=r40(x); if(E(r)||EXIT) return r;
    return s(r)?reduce(r):r;
  case 0x41: return r41(x); // plist (unreduced)
  case 0x42: return r42(x); // klist (unreduced)
  case 0x44: return r44(x); // plist+adverbs
  case 0xc5: return rc5(x); // verb composition
  case 0xd0: return rd0(x); // fixed dyad
  case 0xc9: return rpd(x); // predefined monad
  case 0xca: return rpd(x); // predefined dyad
  case 0xcb: return rpd(x); // predefined triad
  default: return x;
  }
}

static K indexconvergeover(K a, K x) {
  K r=0,e,p=0,q=0;
  if(s(a)||ta>0) { _k(a); _k(x); return KERR_RANK; }
  q=k_(x);                    /* first */
  p=k(13,k_(a),k_(x)); EC(p); /* previous */
  if(kcmpr(p,q)) {
    r=k(13,k_(a),k_(p)); EC(r);
    while(kcmpr(r,q)&&kcmpr(r,p)) {
      _k(p);
      p=r;
      r=k(13,k_(a),k_(p)); EC(r);
      if(STOP) { STOP=0; _k(p); _k(r); e=kerror("stop"); goto cleanup; }
    }
    _k(r);
    r=p;
  }
  else r=p;
  _k(q); _k(a); _k(x);
  return knorm(r);
cleanup:
  _k(p); _k(q); _k(a); _k(x);
  return e;
}
static K indexconvergescan(K a, K x) {
  K r=0,e,p=0,q=0,sr=0,*psr;
  u32 j=0,m=32;

  q=k_(x);                    /* first */
  p=k(13,k_(a),k_(x)); EC(p); /* previous */
  sr=tn(0,m); psr=px(sr);
  psr[j++]=k_(q);
  if(kcmpr(p,q)) {
    psr[j++]=k_(p);
    r=k(13,k_(a),p); EC(r);
    while(kcmpr(r,q)&&kcmpr(r,p)) {
      if(j==m) { m<<=1; sr=kresize(sr,m); psr=px(sr); }
      psr[j++]=k_(r);
      p=r;
      r=k(13,k_(a),p); EC(r);
      if(STOP) { STOP=0; e=kerror("stop"); goto cleanup; }
    }
    _k(q);
  }
  else { _k(p); _k(q); }
  _k(r);
  n(sr)=j; r=sr;
  _k(a); _k(x);
  return knorm(r);
cleanup:
  _k(q);
  _k(r);
  if(sr) { n(sr)=j; _k(sr); }
  _k(a); _k(x);
  return e;
}

static K A0[1024][256];
K pgreduce_(K x0, int *quiet) {
  char q,*mv,*s;
  u8 c;
  size_t ma=2;
  static int d;
  K *pA=A0[d],*A2=0,*A=A0[d],v=0,i,a,*pa,b,*pb,xx,*pxk,f,p,*pp,t,*pt,a_;
  K *px0=px(x0);
  K x=px0[0]; K *px=px(x); /* values */
  int w,valence;
  if(STOP) { STOP=0; return kerror("stop"); }
  if(++d>1+maxr) { --d; return kerror("stack"); } /* must be one greater than fne_() */
  if(nx*10>256) {
    ma=nx*10;
    pA=A2=xmalloc(sizeof(K)*ma);
    A=A2;
  }
  for(i=0;i<nx;i++) {
    *quiet=0; c=0; q=0;
    if(0xc0==s(px[i])) { v=px[i]; c=ck(px[i]); q=c>>5; }
    switch(q) {
    case 0:
      *pA++=v=px[i];
      switch(s(v)) {
      case 0xc3: case 0: case 0x40: case 0x41: case 0x81: k_(v); continue;
      case 0xc7: /* builtin dyad */
        if(pA<=A+1) { k_(v); continue; }
        --pA;
        b=*--pA;
        if(0x40==s(b)&&i+1<nx&&64==ik(px[i+1])&&pA==A) { /* f:draw */
          ++i;
          if((p=scope_set(cs,b,v))) { _k(v); *pA++=p; }
          else { *pA++=v; *quiet=1; }
          break;
        }
        if(s(b)) { b=reduce(b); if(E(b)||EXIT) { _k(v); *pA++=b; break; } }
        if(0x81==s(b)&&n(b)==2) { K *pb=px(b); *pA++=builtin(v,k_(pb[0]),k_(pb[1])); _k(b); }
        else if(0x81==s(b)) { _k(b); *pA++=KERR_VALENCE; } /* valence */
        else if(pA>A) {
          a=*--pA;
          if(0x40==s(a)&&i+1<nx&&64==ik(px[i+1])&&pA==A) { /* f:draw 3 */
            ++i;
            if(!VST(b)) { _k(v); _k(b); *pA++=KERR_TYPE; break; }
            t=builtin(v,0,b);
            if(E(t)||EXIT) *pA++=t;
            else if((p=scope_set(cs,a,t))) { _k(t); *pA++=p; }
            else { *pA++=t; *quiet=1; }
            break;
          }
          if(s(a)) { a=reduce(a); if(E(a)||EXIT) { _k(v); _k(b); *pA++=a; break; } }
          if(!VST(a)||!VST(b)) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
          if(0x85==s(a)||0x85==s(b)) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
          else *pA++=builtin(v,a,b);
        }
        else {
          if(!VST(b)) { _k(b); *pA++=KERR_TYPE; break; }
          if(0x85==s(b)) { _k(b); *pA++=KERR_TYPE; break; }
          t=builtin(v,0,b);
          *pA++=t;
        }
        break;
      case 0xc6: /* builtin monad */
        if(pA<=A+1) { k_(v); break; } // no argument available
        if(pA<=A+2&&i<nx-1&&ik(px[i+1])==0xff) { k_(v); break; } // {x} abs 2
        if(pA>A+2&&strpbrk(sk(v),"/\\")) { k_(v); break; }  // 5 sin/2; 5 sin\2
        --pA;
        a=*--pA;
        if(0x40==s(a)&&i+1<nx&&64==ik(px[i+1])&&pA==A) { /* f:+ */
          ++i;
          if((p=scope_set(cs,a,v))) { _k(a); *pA++=p; }
          else { *pA++=v; *quiet=1; }
          break;
        }
        if(s(a)) { a=reduce(a); if(E(a)||EXIT) { *pA++=a; break; } }
        if(0x81==s(a)&&n(a)==1) { K *pa=px(a); *pA++=builtin(v,0,k_(pa[0])); _k(a); }
        else if(0x81==s(a)) { _k(a); *pA++=KERR_VALENCE; } /* valence */
        else if(!VST(a)) { _k(a); *pA++=KERR_TYPE; break; }
        else *pA++=builtin(v,0,a);
        break;
      case 0xd1: case 0xd2: case 0xd3: /* do while if */
        if(pA<=A+1) { k_(v); break; }
        --pA;
        a=*--pA;
        if(0x41==s(a)||0x81==s(a)) *pA++=builtin(v,0,a);
        else { _k(a); *pA++=KERR_PARSE; } /* parse */
        break;
      case 0xcc: /* 5:x */
        if(pA<=A+1) { k_(v); break; }
        --pA;
        a=*--pA;
        if(s(a)) { a=reduce(a); if(E(a)||EXIT) { *pA++=a; break; } }
        if(s(a)==0x41) {
          a=r41(a); if(E(a)||EXIT) { *pA++=a; break; }
          if(n(a)==1) { K *pa=px(a); p=pa[0]; pa[0]=0; _k(a); a=p; }
          else if(n(a)==0) { _k(a); a=null; }
          *pA++=builtin(v,0,a);
        }
        else if(0x44==s(a)) {
          a=r44(a); if(E(a)||EXIT) { *pA++=a; break; }
          *pA++=builtin(v,0,a);
        }
        else if(!VST(a)) { _k(a); *pA++=KERR_TYPE; break; }
        else *pA++=builtin(v,0,a);
        break;
      case 0xcd: /* a 0:x */
        if(pA<=A+2) { *pA++=KERR_VALENCE; break; }
        --pA;
        b=*--pA;
        a=*--pA;
        if(!a||!b) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
        if(0x41==s(b)) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
        if(s(b)) { b=reduce(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
        if(s(a)) { a=reduce(a); if(E(a)||EXIT) { _k(b); *pA++=a; break; } }
        if(!VST(b)) { _k(a); _k(b); *pA++=KERR_TYPE; }
        else *pA++=builtin(v,a,b);
        break;
      case 0x82: /* a::1 */
        if(pA<=A+2) { k_(v); break; }
        --pA;
        b=*--pA;
        a=*--pA;
        if(s(b)) { b=reduce(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
        if(0x40!=s(a)) { _k(a); _k(b); *pA++=KERR_VALUE; break; }
        if(!VST(b)) { _k(b); *pA++=KERR_PARSE; break; }
        if((p=scope_set(gs,a,b))) { _k(a); _k(b); *pA++=p; }
        else { *pA++=b; *quiet=1; }
        break;
      case 0xcf: /* a+: */
        if(pA<=A+1) { k_(v); break; }
        --pA;
        a=*--pA;
        if(0x40!=s(a)) { _k(a); *pA++=KERR_VALUE; break; }
        if(KERR_VALUE==(a_=vlookup(a))) a_=null;
        if(E(a_)) { *pA++=a_; break; }
        t=k(strchr(P,ik(v))-P,0,a_);
        if(E(t)) { *pA++=t; break; };
        if((p=scope_set(cs,a,t))) { _k(t); *pA++=p; }
        else { *pA++=t; *quiet=1; }
        break;
      case 0xce: /* a+:1 */
        if(pA<=A+2) {
          if(pA<=A+1) { k_(v); break; }
          *pA++=KERR_PARSE; break;
        }
        --pA;
        b=*--pA;
        a=*--pA;
        if(s(b)) { b=reduce(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
        if(0x40==s(a)) {
          if(s(b)) { b=reduce(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
          if(!VST(b)) { _k(b); *pA++=KERR_PARSE; break; }
          // need to know which scope variable was resolved from
          K rs; if(KERR_VALUE==(a_=vlookuprs(a,&rs))) { a_=null; rs=gs; }
          if(E(a_)) { _k(b); *pA++=a_; break; }
          t=k(strchr(P,ik(v))-P,a_,b);
          if(E(t)||EXIT) { *pA++=t; break; };
          if((p=scope_set(rs,a,t))) { _k(t); *pA++=p; }
          else { *pA++=t; *quiet=1; }
        }
        else if(0x44==s(a)) {
          if(s(b)) { b=reduce(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
          if(!VST(b)) { _k(a); _k(b); *pA++=KERR_PARSE; break; }
          pa=px(a);
          K a_=k_(pa[0]); K i_=k_(pa[1]); _k(a);

          //if(0x41!=s(i_)) { _k(a_); _k(i_); _k(b); *pA++=KERR_TYPE; break; }
          if(0x41==s(i_)) {
            if(n(i_)) {
              i_=r41(i_); if(E(i_)) { _k(a_); _k(b); *pA++=i_; break; }
              if(0x81==s(i_)) i_=b(48)&i_;
            }
            else { _k(i_); i_=null; }
          }
          else if(0x81==s(i_)) {
            if(n(i_)) i_=b(48)&i_;
            else { _k(i_); i_=null; }
          }
          else { _k(a_); _k(i_); _k(b); *pA++=KERR_TYPE; break; }

          if(0x40!=s(a_)) { _k(a_); _k(i_); _k(b); *pA++=KERR_TYPE; break; }
          // need to know which scope variable was resolved from
          K rs; if(KERR_VALUE==(a_=vlookuprs(a_,&rs))) { a_=null; rs=gs; }
          if(E(a_)) { _k(b); _k(i_); *pA++=a_; break; }
          a=k(11,k_(a_),k_(i_));
          if(E(a)) { _k(b); _k(a_); _k(i_); *pA++=a; break; }
          if(T(i_)>0) {
            t=avdo(strchr(P,ik(v))-P,a,b,"'");
            if(E(t)||EXIT) { _k(a_); *pA++=t; break; };
          }
          else {
            K *pi=px(i_);
            K pi0=pi[0];
            if(T(pi0)<=0||pi0==null) t=avdo(strchr(P,ik(v))-P,a,b,"'");
            else t=k(strchr(P,ik(v))-P,a,b);
            if(E(t)||EXIT) { _k(a_); _k(i_); *pA++=t; break; };
          }
          K r=kamend4(a_,i_,0,k_(t));
          if(E(r)) { _k(t); *pA++=r; }
          else if((p=scope_set(rs,pa[0],r))) { _k(r); _k(t); *pA++=p; }
          else { _k(r); *pA++=t; *quiet=1; }
        }
        else { _k(a); _k(b); *pA++=KERR_TYPE; break; }
        break;
      case 0xc1: /* +/x */
        if(pA<=A+1) { k_(v); break; }
        mv=px(v);
        s=strchr(P,*mv);
        --pA;
        b=*--pA;
        if(0x42==s(b)) { b=r42(b); if(E(b)||EXIT) { *pA++=b; break; } }
        if(0x40==s(b)&&i+1<nx&&64==ik(px[i+1])) {
          if(pA<=A) { /* b:+' */
            ++i;
            if((p=scope_set(cs,b,v))) *pA++=p;
            else { *pA++=k_(b); *quiet=1; }
          }
          else { /* +'b */
            if(s(b)) { b=reduce(b); if(E(b)||EXIT) { *pA++=b; break; } }
            t=fe(k_(v),0,b,"");
            *pA++=t;
          }
        }
        else {
          if(s(b)) { b=reduce(b); if(E(b)||EXIT) { *pA++=b; break; } }
          if(0x81==s(b)) {
            if(n(b)==1) { K *pb=px(b); p=pb[0]; pb[0]=0; _k(b); b=p; }
            else if(n(b)==0) { _k(b); b=null; }
          }
          if(*++mv && s-P<=20) {
            if(s(b)&&s(b)!=0x80&&s(b)!=0xc3&&s(b)!=0xc4) { _k(b); *pA++=KERR_TYPE; }
            else *pA++=avdo(s-P,0,b,mv);
          }
          else { _k(b); *pA++=KERR_PARSE; } /* parse */
        }
        break;
      case 0xc2: /* a+/x */
        if(pA<=A+2) {
          k_(v);
          *pA++=KERR_PARSE; /* parse */
          break;
        }
        mv=px(v);
        s=strchr(P,*mv);
        --pA;
        b=*--pA;
        a=*--pA;
        if(!a||!b) { _k(a); _k(b); *pA++=KERR_PARSE; break; } /* parse */
        if(s(b)) { b=reduce(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
        if(s(a)) { a=reduce(a); if(E(a)||EXIT) { _k(b); *pA++=a; break; } }
        if(*++mv && s-P<=20) {
          if(!VST(a)||!VST(b)) { _k(a); _k(b); *pA++=KERR_TYPE; }
          else *pA++=avdo(s-P,a,b,mv);
        }
        else { _k(a); _k(b); *pA++=KERR_PARSE; } /* parse */
        break;
      case 0xca: /* predefined dyad (in lin dv dvl ...) */
      case 0xc9: /* predefined monad (gtime ltime ...) */
      case 0xcb: /* predefined triad (ssr) */
        f=rpd(v);
        if(E(f)||EXIT) { *pA++=f; break; }
        if(pA>A+1) {
          --pA;
          b=*--pA;
          if(0x40==s(b)&&i+1<nx&&64==ik(px[i+1])&&pA==A) { /* f:gtime */
            ++i;
            if((p=scope_set(cs,b,f))) { _k(f); *pA++=p; }
            else { *pA++=f; *quiet=1; }
            break;
          }
          if(0x40==s(b)) { b=r40(b); if(E(b)||EXIT) { _k(f); *pA++=b; break; } }
          if(0x42==s(b)) { b=r42(b); if(E(b)||EXIT) { _k(f); *pA++=b; break; } }
          if(0x41==s(b)) { b=r41(b); if(E(b)||EXIT) { _k(f); *pA++=b; break; } }
          if(0x81==s(b)) { *pA++=fne(f,b,0); break; }
          else if(0x43==s(b)) { /* a[]'x */
            pb=px(b);
            K b0=k_(pb[0]); K b1=pb[1]; K b2=pb[2];
            if(!VST(b2)) { _k(f); _k(b); *pA++=KERR_TYPE; break; }
            if(0x41==s(b0)) { b0=r41(b0); if(E(b0)||EXIT) { *pA++=b0; _k(f); _k(b); break; } }
            if(0==n(b0)) { t=tn(0,1); pt=px(t); pt[0]=null; p=fne(f,t,0); }
            else p=fne(f,b0,0);
            if(0xc3==s(p) || 0xc4==s(p)) *pA++=avdo(p,0,k_(b2),(char*)px(b1));
            else { _k(p); *pA++=KERR_TYPE; }
            _k(b);
            break;
          }
          else if(0xd0==s(b)) { _k(f); _k(b); *pA++=KERR_TYPE; break; }
          else if(0xc5==s(b)) { _k(f); _k(b); *pA++=KERR_TYPE; break; }
          else if(0x45==s(b)) { _k(f); _k(b); *pA++=KERR_TYPE; break; }
          else if(0x44==s(b)) { b=r44(b); if(E(b)||EXIT) { _k(f); *pA++=b; break; } }
          valence=val(f);
          if(valence==1) {
            xx=params[paramsi++]; pxk=px(xx); pxk[0]=b; n(xx)=1;
            *pA++=fne(f,k_(xx),0);
            _k(b); --paramsi;
          }
          else if(valence==2) {
            if(0==pA-A) {
              xx=params[paramsi++]; pxk=px(xx); pxk[0]=b; n(xx)=1;
              *pA++=fne(f,k_(xx),0);
              _k(b); --paramsi;
            }
            else {
              a=*--pA;
              if(0x40==s(a)&&i+1<nx&&64==ik(px[i+1])&&pA==A) { /* f:lin 1 2 3 */
                ++i;
                xx=params[paramsi++]; pxk=px(xx); pxk[0]=b; n(xx)=1;
                t=fne(f,k_(xx),0);
                _k(b); --paramsi;
                if(E(t)||EXIT) *pA++=t;
                else if((p=scope_set(cs,a,t))) { _k(t); *pA++=p; }
                else { *pA++=t; *quiet=1; }
                break;
              }
              if(s(a)) { a=reduce(a); if(E(a)||EXIT) { _k(f); _k(b); *pA++=a; break; } }
              if(!VST(b)) { _k(f); _k(a); _k(b); *pA++=KERR_TYPE; break; }
              if(!VST(a)) { _k(f); _k(a); _k(b); *pA++=KERR_TYPE; break; }
              xx=params[paramsi++]; pxk=px(xx); pxk[0]=a; pxk[1]=b; n(xx)=2;
              *pA++=fne(f,k_(xx),0);
              _k(a); _k(b); --paramsi;
            }
          }
          else if(valence==3) {
            *pA++=KERR_PARSE; /* parse, ssr always handled in r44 */
            _k(f); _k(v); _k(b);
          }
        }
        else *pA++=f;
        break;
      case 0xd8:  /* trace */
        if(pA>A+1) {
          --pA;
          b=*--pA;
          if(s(b)) { b=reduce(b); if(E(b)||EXIT) { *pA++=b; break; } }
          kprint(k_(b),"","\n","");
          *pA++=b;
        }
        break;
      default: k_(v);
      }
      break;
    case 1: /* 32 33 34 ... */
      if(pA<=A) {*pA++=v; break; }
      a=*--pA;
      if(!a) { *pA++=KERR_TYPE; break; }
      if(0x42==s(a)) { a=r42(a); if(E(a)||EXIT) { *pA++=a; break; } }
      if((0x41==s(a)||0x81==s(a))&&c%32!=19) { /* except for cond, $[1;2;3] */
        K jn=tn(0,2); K *pjn=px(jn);
        pjn[0]=v;
        pjn[1]=a;
        *pA++=st(0x44,jn);
        break;
      }
      if(0x40==s(a)&&i+1<nx&&64==ik(px[i+1])&&pA==A) { /* f:+ */
        ++i;
        if((p=scope_set(cs,a,v))) { _k(a); *pA++=p; }
        else { *pA++=a; *quiet=1; }
      }
      else {
        if(s(a)) {
          if(0x40==s(a)) { a=r40(a); if(E(a)||EXIT) { *pA++=a; break; } }
          if(0x44==s(a)) { a=r44(a); if(E(a)||EXIT) { *pA++=a; break; } }
          if(0xc5==s(a)) { a=rc5(a); if(E(a)||EXIT) { *pA++=a; break; } }
          if(0xd0==s(a)) { a=rd0(a); if(E(a)||EXIT) { *pA++=a; break; } }
        }
        if(c==32) { *pA++=a; RETURN=1; break; } /* return */
        else if(19==c%32&&(0x41==s(a)||0x81==s(a))&&n(a)>2) { *pA++=cond_(a); _k(a); }
        else if(0x41==s(a)||0x81==s(a)) {
          if(0x41==s(a)) { a=rl(a,0x81); if(E(a)||EXIT) { *pA++=a; break; } }
          pa=px(a);
          if(na==1) *pA++=k(c%32,0,k_(pa[0]));
          else if(na==2) *pA++=k(c%32,k_(pa[0]),k_(pa[1]));
          else if(na==3&&c%32==13) *pA++=kamendi3(k_(pa[0]),k_(pa[1]),k_(pa[2]));
          else if(na==4&&c%32==13) *pA++=kamendi4(k_(pa[0]),k_(pa[1]),k_(pa[2]),k_(pa[3]));
          else if(na==3&&c%32==11) *pA++=kamend3(k_(pa[0]),k_(pa[1]),k_(pa[2]));
          else if(na==4&&c%32==11) *pA++=kamend4(k_(pa[0]),k_(pa[1]),k_(pa[2]),k_(pa[3]));
          else *pA++=KERR_VALENCE; /* valence */
          _k(a);
        }
        else if(!VST(a)) { _k(a); *pA++=KERR_TYPE; }
        else *pA++=k(c%32,0,a);
      }
      break;
    case 2: /* 64 64 66 ... */
      if(pA<=A+1) { *pA++=KERR_VALENCE; break; }
      b=*--pA;
      a=*--pA;

      if(!a||!b) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
      if(0x42==s(b)) { b=r42(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
      if(0x42==s(a)) { a=r42(a); if(E(a)||EXIT) { _k(b); *pA++=a; break; } }
      if(0x41==s(b)) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
      if(c==64) {
        if(0x40==s(a)) { /* a:1 */
          if(s(b)) { b=reduce(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
          if(!VST(b)) { _k(b); *pA++=KERR_PARSE; break; }
          // if this is a closure, where to set?
          // if the variable is defined in the parent lexical scope, set it there
          // otherwise, set it in cs
          // under no circumstances set it in gs here.
          K rs; K a_=vlookuprs(a,&rs);
          if(KERR_VALUE==a_) rs=cs;
          else if(E(a_)) { _k(b); *pA++=a_; break; }
          else if(rs==gs) { _k(a_); rs=cs; }
          else _k(a_);
          if(rs!=cs) {
            K *prs=px(rs);
            if(!ik(prs[3])) { /* if not closure scope */
              // resolved in parent, but parent is not a closure scope
              // so we can't write to it.
              rs=cs;
            }
          }
          if((p=scope_set(rs,a,b))) { _k(a); _k(b); *pA++=p; }
          else { *pA++=b; *quiet=1; }
        }
        else if(0x44==s(a)) {
          if(0x44==s(b)) { b=r44(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
          pa=px(a);
          //if(0x41!=s(pa[1])) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
          if(0x41==s(pa[1])) { p=r41(k_(pa[1])); if(E(p)||EXIT) { _k(a); _k(b); *pA++=p; break; } }
          else if(0x81==s(pa[1])) p=k_(pa[1]);
          else { _k(a); _k(b); *pA++=KERR_TYPE; break; }
          K *pp=px(p);
          if(n(p)==1) *pA++=assign(k_(pa[0]),k_(pp[0]),b);
          else if(n(p)) *pA++=assign(k_(pa[0]),k_(p),b);
          else *pA++=assign(k_(pa[0]),null,b);
          if(!E(pA[-1])) *quiet=1;
          _k(a); _k(p);
        }
        else if(0x40!=s(a)) { _k(a); _k(b); *pA++=KERR_TYPE; } /* 1:1 (type) */
      }
      else {
        if(0x43==s(a)||0x43==s(b)) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
        if(0x41==s(a)||0x41==s(b)) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
        if(s(b)) { b=reduce(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
        if(s(a)) { a=reduce(a); if(E(a)||EXIT) { _k(b); *pA++=a; break; } }
        if(0xd0==s(a) && 0xd0!=s(b)) *pA++=fe(a,0,b,0);
        else if(0xd7==s(a)) *pA++=fe(a,0,b,0);
        else {
          int w=c%32;
          if((w==11||w==13)&&0x04==T(a)) { /* `a . 0; `a @ 0 */
            a=vlookup(a);
            if(KERR_VALUE==a) a=null;
            else if(E(a)||EXIT) { _k(b); *pA++=a; break; }
          }
          *pA++=k(w,a,b);
        }
      }
      break;
    case 7: /* juxtaposition */
      if(pA<=A+1) break;
      b=*--pA;
      a=*--pA;
      if(!a||!b) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
      if(0x42==s(b)) { b=r42(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
      if(0x42==s(a)) { a=r42(a); if(E(a)||EXIT) { _k(b); *pA++=a; break; } }
      char avb[256],*av=avb;
      *av=0;

      if(0x41==s(b)||0x81==s(b)) {
        if(0x85==s(a)) { t=tn(0,3); pt=px(t); pt[1]=a; pt[2]=b; *pA++=st(0x45,t); }
        else { K jn=tn(0,2); K *pjn=px(jn); pjn[0]=a; pjn[1]=b; *pA++=st(0x44,jn); }
        break;
      }

      if(s(b)) { b=reduce(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
      if(0x40==s(a)) {
        if(4==T(a)) {
          if(strlen(sk(a))>255) { _k(b); *pA++=KERR_LENGTH; break; }
          char *p=strpbrk(sk(a),"'/\\");
          if(p) {
            snprintf(av,sizeof(avb),"%s",p);
            char c=*p; *p=0;
            a=vlookup(t(4,sp(sk(a))));
            *p=c;
          }
          else a=vlookup(a);
        }
        else a=vlookup(a);
        if(E(a)) { _k(b); *pA++=a; break; }
      }
      if(!*av) av=0;
      if(s(a)) {
        if(0x44==s(a)) { a=r44(a); if(E(a)||EXIT) { _k(b); *pA++=a; break; } }
        if(0xd0==s(a)) { a=rd0(a); if(E(a)||EXIT) { _k(b); *pA++=a; break; } }
        if(0xc5==s(a)) { a=rc5(a); if(E(a)||EXIT) { _k(b); *pA++=a; break; } }
        if(!VST(a)) { _k(b); _k(a); *pA++=KERR_TYPE; break; }
      }

      if(0xc6==s(b)) { /* 5 sin\2 */
        char *pav=avb; /* TODO: this block is a mess */
        K b2=stripav(b,&pav);
        if((!strcmp(pav,"/") || !strcmp(pav,"\\"))
           &&pA>A&&i<nx&&0xc0==s(px[i])&&ik(px[i])==0xff) {  /* dyadic juxtaposition */
          t=*--pA;
          if(i<nx&&0xc0==s(px[i+1])&&ik(px[i+1])==64&&pA==A) {  /* f:{x>0.5}sin/ */
            if(0x40!=s(t)) { _k(t); _k(a); _k(b); *pA++=KERR_TYPE; break; }
            ++i;
            K d0=tn(0,2); K *pd0=px(d0);
            pd0[0]=a; pd0[1]=b;
            if((p=scope_set(cs,t,st(0xd0,d0)))) { _k(d0); *pA++=p; }
            else { *pA++=d0; *quiet=1; }
          }
          else {
            /* primitive do/while */
            if(s(t)) { t=reduce(t); if(E(t)||EXIT) { _k(a); *pA++=t; break; } }
            if(!VST(t)) { _k(a); *pA++=KERR_TYPE; break; }
            if(s(t)==0 && T(t)==1 && !strcmp(pav,"/")) { *pA++=overmonadn(b2,t,a,""); *quiet=0; }
            else if(s(t)==0xc3 && !strcmp(pav,"/")) { *pA++=overmonadb(b2,t,a,""); *quiet=0; }
            else if(s(t)==0 && T(t)==1 && !strcmp(pav,"\\")) { *pA++=scanmonadn(b2,t,a,""); *quiet=0; }
            else if(s(t)==0xc3 && !strcmp(pav,"\\")) { *pA++=scanmonadb(b2,t,a,""); *quiet=0; }
            else { _k(a); _k(t); *pA++=KERR_TYPE; }
          }
          break;
        }
      }

      switch(s(a)) {
      case 0xc3: case 0xc4:
        if(0x81==s(b)) { *pA++=fne(a,b,av); } /* f[x] */
        else if(ISF(b)&&0xc3!=s(b)&&0xc4!=s(b)&&(!av||!strlen(av))) {  /* {x} val */
          p=tn(0,2); pp=px(p);
          pp[0]=a; pp[1]=b;
          K q=tn(0,2); K *pq=px(q);
          pq[0]=p;
          pq[1]=tn(3,0); /* adverbs */
          *pA++=st(0xc5,q);
        }
        else if(0x45==s(b)) {
          pb=px(b);
          K b1=k_(pb[1]);
          K b2=k_(pb[2]);
          char *pb1=px(b1);
          if(0x44==s(b2)) { b2=r44(b2); if(E(b2)||EXIT) { _k(b1); _k(a); _k(b); *pA++=b2; break; } }
          if(0x41==s(b2)) { b2=r41(b2); if(E(b2)||EXIT) { _k(b1); _k(a); _k(b); *pA++=b2; break; } }
          if(0x81==s(b2)) *pA++=fne(a,b2,pb1);
          else {
            if(!VST(b2)) { _k(b1); _k(b2); _k(a); _k(b); *pA++=KERR_TYPE; break; }
            if(strlen(pb1)) *pA++=avdo(a,0,b2,pb1);
            else {
              xx=params[paramsi++]; pxk=px(xx); pxk[0]=b2; n(xx)=1;
              *pA++=fne(a,k_(xx),0);
              _k(b2); --paramsi;
            }
          }
          _k(b1); _k(b);
        }
        else if(0xc3==s(b)||0xc4==s(b)) {
          K *pb=px(b);
          K fav=0;
          if(0xc3==s(b)) fav=pb[3];
          else fav=pb[2];
          char *favp=-3==T(fav)?(char*)px(fav):"";
          if((!strcmp(favp,"/") || !strcmp(favp,"\\"))) {
            t=tn(0,2); pt=px(t);
            pt[0]=a; pt[1]=b;
            *pA++=st(0xd7,t);
          }
          else if(0x45==s(b)) {
            pb=px(b);
            K b1=pb[1];
            K b2=pb[2];
            char *pb1=px(b1);
            if(0x81==s(b2)) { *pA++=fne(a,k_(b2),pb1); _k(b); }
            else {
              xx=params[paramsi++]; pxk=px(xx); pxk[0]=b2; n(xx)=1;
              *pA++=fne(a,k_(xx),pb1);
              --paramsi;
              _k(b);
            }
          }
          else {
            xx=params[paramsi++]; pxk=px(xx); pxk[0]=b; n(xx)=1;
            *pA++=fne(a,k_(xx),av);
            _k(b); --paramsi;
          }
        }
        else {
          if(0x44==s(b)) b=r44(b);
          if(E(b)||EXIT) { _k(a); *pA++=b; break; }
          if(0x43==s(b)) { _k(a); _k(b); *pA++=KERR_PARSE; break; }  /* TODO */
          valence=val(a);
          if(valence==0||valence==1) { /* f x */
            K *pa=px(a);
            K fav=0;
            if(0xc3==s(a)) fav=pa[3];
            else if(0xc4==s(a)) fav=pa[2];
            else { _k(a); _k(b); *pA++=KERR_PARSE; break; }
            char *favp=-3==T(fav)?(char*)px(fav):"";
            if(av && *av) favp=av;
            if((!strcmp(favp,"/") || !strcmp(favp,"\\"))
               &&pA>A&&i<nx-1&&0xc0==s(px[i+1])&&ik(px[i+1])==0xff) {  /* dyadic juxtaposition */
              /* primitive do/while */
              ++i;
              t=*--pA;
              if(0x43==s(b)) { *pA++=0; _k(a); _k(b); break; } /* TODO: f[]'[] */
              if(s(t)) { t=reduce(t); if(E(t)||EXIT) { _k(a); _k(b); *pA++=t; break; } }
              if(!VST(t)) { _k(a); _k(b); _k(t); *pA++=KERR_TYPE; break; }
              K f=kcp(a); _k(a);
              if(E(f)) { _k(b); _k(t); *pA++=f; break; }
              K *pf=px(f);
              if(0xc3==s(f)) { _k(pf[3]); pf[3]=null; }
              else if(0xc4==s(f)) { _k(pf[2]); pf[2]=null; }
              if(s(t)==0 && T(t)==1 && !strcmp(favp,"/")) { *pA++=overmonadn(f,t,b,""); *quiet=0; }
              else if(s(t)==0xc3 && !strcmp(favp,"/")) { *pA++=overmonadb(f,t,b,""); *quiet=0; }
              else if(s(t)==0 && T(t)==1 && !strcmp(favp,"\\")) { *pA++=scanmonadn(f,t,b,""); *quiet=0; }
              else if(s(t)==0xc3 && !strcmp(favp,"\\")) { *pA++=scanmonadb(f,t,b,""); *quiet=0; }
              else { _k(f); _k(b); _k(t); *pA++=KERR_TYPE; break; }
            }
            else if(!strcmp(favp,"'")&&0==ik(val(a))) {
              K f=kcp(a); _k(a);
              if(E(f)) { _k(b); *pA++=f; break; }
              K *pf=px(f);
              if(0xc3==s(f)) { _k(pf[3]); pf[3]=null; }
              else if(0xc4==s(f)) { _k(pf[2]); pf[2]=null; }
              char av2[256];
              snprintf(av2,256,"%s%s",favp,av?av:"");
              *pA++=avdo(f,0,b,av2);
            }
            else if(0x45==s(b)) {
              pb=px(b);
              K b1=pb[1];
              K b2=pb[2];
              char *pb1=px(b1);
              if(0x81==s(b2)) { *pA++=fne(a,k_(b2),pb1); _k(b); }
              else {
                xx=params[paramsi++]; pxk=px(xx); pxk[0]=b2; n(xx)=1;
                *pA++=fne(a,k_(xx),pb1);
                --paramsi;
                _k(b);
              }
            }
            else {
              xx=params[paramsi++]; pxk=px(xx); pxk[0]=b; n(xx)=1;
              *pA++=fne(a,k_(xx),av);
              _k(b); --paramsi;
            }
          }
          else if(valence==2) { /* a f x */
            if(pA>A&&i<nx-1&&0xc0==s(px[i+1])&&ik(px[i+1])==0xff) {  /* dyadic juxtaposition */
              ++i;
              t=*--pA;
              if(0x43==s(b)) { *pA++=0; _k(a); _k(b); break; } /* TODO: f[]'[] */
              if(s(t)) { t=reduce(t); if(E(t)||EXIT) { _k(a); _k(b); *pA++=t; break; } }
              if(!VST(t)) { _k(a); _k(b); _k(t); *pA++=KERR_TYPE; break; }
              xx=params[paramsi++]; pxk=px(xx); pxk[0]=t; pxk[1]=b; n(xx)=2;
              *pA++=fne(a,k_(xx),av);
              _k(t); _k(b); --paramsi;
            }
            else if(0x43==s(b)) { *pA++=0; _k(a); _k(b); break; } /* TODO f[]'[] */
            else {
              xx=params[paramsi++]; pxk=px(xx); pxk[0]=b; n(xx)=1;
              *pA++=fne(a,k_(xx),av);
              _k(b); --paramsi;
            }
          }
          else { *pA++=KERR_VALENCE; _k(a); _k(b); }
        }
        break;
      case 0xc0:
        w=ck(a)%32;
        if(0x81==s(b)) { /* f[1;2] */
          if(n(b)==2) { pb=px(b); *pA++=k(w,k_(pb[0]),k_(pb[1])); }
          else *pA++=KERR_VALENCE;
          _k(b);
        }
        else if(0x45==s(b)) { // (,)'("012";"345";"678";"901")
          pb=px(b);
          mv=px(pb[1]);
          if(!VST(pb[2])) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
          if(pA>A&&i<nx-1&&0xc0==s(px[i+1])&&ik(px[i+1])==0xff) {  /* dyadic juxtaposition */
            ++i;
            t=*--pA;
            if(s(t)) { t=reduce(t); if(E(t)||EXIT) { _k(a); _k(b); *pA++=t; break; } }
            if(!VST(t)) { _k(a); _k(b); _k(t); *pA++=KERR_TYPE; break; }
            pb=px(b);
            mv=px(pb[1]);
            *pA++=avdo(w,t,k_(pb[2]),mv);
          }
          else *pA++=avdo(w,0,k_(pb[2]),mv);
          _k(b);
        }
        else *pA++=k(w,0,b);
        break;
      case 0xd0: /* fixed dyad */
        if(0x45==s(b)) {
          pb=px(b);
          mv=px(pb[1]);
          if(!VST(pb[2])) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
          *pA++=fe(a,0,k_(pb[2]),mv);
          _k(b);
        }
        else *pA++=fe(a,0,b,"");
        break;
      case 0: case 0x80:
        if(0x81==s(b)) {
          if(aa) { _k(a); _k(b); *pA++=KERR_RANK; break; } /* 0[] */
          if(n(b)==1) { pb=px(b); *pA++=k(13,a,k_(pb[0])); /* a[b] */ }
          else if(n(b)==0) *pA++=k(13,a,null); /* a[] */
          else *pA++=k(11,a,k_(b(48)&b)); /* a[;1] */
          _k(b);
        }
        else if(0x43==s(b)) { /* a[]'x */
          pb=px(b);
          K b0=pb[0]; K b1=pb[1]; K b2=pb[2];
          if(0x41==s(b0)) { b0=r41(k_(b0)); if(E(b0)||EXIT) { *pA++=b0; _k(a); _k(b); break; } }
          else b0=k_(b0);
          K *pb0=px(b0);

          if(n(b0)==1) { t=k(13,a,k_(pb0[0])); /* a[b] */ }
          else if(n(b0)==0) t=k(13,a,null); /* a[] */
          else t=k(11,a,k_(b(48)&b0)); /* a[;1] */

          if(E(t)||EXIT) *pA++=t;
          else if(0xc3==s(t) || 0xc4==s(t)) {
            K xx;
            char *pb1=px(b1);
            if(0x81==s(b2)) *pA++=fne(t,k_(b2),pb1);
            else {
              if(0x41==s(b2)) {
                b2=r41(k_(b2)); if(E(b2)||EXIT) { *pA++=b2; _k(t); _k(b); _k(b0); break; }
                *pA++=fne(t,b2,pb1);
              }
              else if(0x44==s(b2)) {
                b2=r44(k_(b2)); if(E(b2)||EXIT) { _k(t); _k(b); _k(b0); *pA++=b2; break; }
                xx=params[paramsi++]; pxk=px(xx); pxk[0]=b2; n(xx)=1;
                *pA++=fne(t,k_(xx),pb1);
                --paramsi;
              }
              else {
                xx=params[paramsi++]; pxk=px(xx); pxk[0]=b2; n(xx)=1;
                *pA++=fne(t,k_(xx),pb1);
                --paramsi;
              }
            }
          }
          else _k(t);
          _k(b);
          _k(b0);
        }
        else if(0x45==s(b)) {
          pb=px(b);
          K av1=pb[1]; char *pav1=av1?px(av1):"";
          if(s(pb[2])) { t=reduce(k_(pb[2])); if(E(t)) { _k(a); _k(b); *pA++=t; break; } _k(pb[2]); pb[2]=t; }
          if(pav1&&*pav1)  {
            if(!strcmp(pav1,"/")) *pA++=indexconvergeover(a,k_(pb[2]));
            else if(!strcmp(pav1,"\\")) *pA++=indexconvergescan(a,k_(pb[2]));
            else { _k(a); _k(b); *pA++=KERR_RANK; break; }
          }
          _k(b);
        }
        else if(0xc3==s(b)||0xc4==s(b)) {
          K *pb=px(b);
          K fav=0;
          if(0xc3==s(b)) fav=pb[3];
          else fav=pb[2];
          char *favp=-3==T(fav)?(char*)px(fav):"";
          if((!strcmp(favp,"/") || !strcmp(favp,"\\"))) {
            t=tn(0,2); pt=px(t);
            pt[0]=a; pt[1]=b;
            *pA++=st(0xd7,t);
          }
          else { _k(a); _k(b); *pA++=KERR_TYPE; break; }
        }
        else {
          if(0x04==T(a)) { /* `a 0 */
            a=r40(a);
            if(KERR_VALUE==a) a=null;
            else if(E(a)||EXIT) { _k(b); *pA++=a; break; }
          }
          if(av&&*av)  {
            if(!strcmp(av,"/")) *pA++=indexconvergeover(a,b);
            else if(!strcmp(av,"\\")) *pA++=indexconvergescan(a,b);
            else { _k(a); _k(b); *pA++=KERR_RANK; break; }
          }
          else *pA++=k(13,a,b); /* a b */
        }
        break;
      case 0xc1: /* +/x */
        mv=px(a);
        s=strchr(P,*mv);
        if(0x40==s(b)) { b=r40(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
        if(*++mv && s-P<=20) {
          if(0x45==s(b)) { /* adverbs with args, '1 2 */
            if(pA>A&&i<nx-1&&0xc0==s(px[i+1])&&ik(px[i+1])==0xff) {  /* dyadic juxtaposition */
              ++i;
              t=*--pA;
              if(s(t)) { t=reduce(t); if(E(t)||EXIT) { _k(a); _k(b); *pA++=t; break; } }
              if(!VST(t)) { _k(a); _k(b); _k(t); *pA++=KERR_TYPE; break; }
              pb=px(b);
              p=k(18,k_(a)&0xff00ffffffffffff,k_(pb[1])&0xff00ffffffffffff); /* join */
              if(E(p)) { _k(a); _k(b); _k(t); *pA++=p; break; }
              mv=px(p);
              ++mv;
              if(!VST(pb[2])) { _k(a); _k(b); _k(t); *pA++=KERR_TYPE; break; }
              *pA++=avdo(s-P,t,k_(pb[2]),mv);
              _k(p);
            }
            else {
              pb=px(b);
              p=k(18,k_(a)&0xff00ffffffffffff,k_(pb[1])&0xff00ffffffffffff); /* join */
              if(E(p)) { _k(a); _k(b); *pA++=p; break; }
              mv=px(p);
              ++mv;
              if(!VST(pb[2])) { _k(a); _k(b); _k(p); *pA++=KERR_TYPE; break; }
              *pA++=avdo(s-P,0,k_(pb[2]),mv);
              _k(p);
            }
          }
          else if(pA>A&&i<nx-1&&0xc0==s(px[i+1])&&ik(px[i+1])==0xff) {  /* dyadic juxtaposition */
            ++i;
            t=*--pA;
            if(s(t)) { t=reduce(t); if(E(t)||EXIT) { _k(a); _k(b); *pA++=t; break; } }
            if(!VST(t)) { _k(a); _k(b); _k(t); *pA++=KERR_TYPE; break; }
            *pA++=avdo(s-P,t,k_(b),mv);
          }
          else if(s(b)) *pA++=KERR_TYPE;
          else *pA++=avdo(s-P,0,k_(b),mv);
        }
        else { *pA++=KERR_PARSE; }
        _k(a); _k(b);
        break;
      case 0x85: /* adverbs alone */
        if(0x43==s(b)) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
        t=tn(0,3); pt=px(t);
        pt[1]=a; pt[2]=b;
        *pA++=st(0x45,t);
        break;
      case 0x41: case 0x81:
        if(0x45==s(b)) { /* adverbs with args. ex: '1 2 3 */
          pb=px(b);
          pb[0]=a;
          *pA++=st(0x43,b);
        }
        else *pA++=k(13,a,b); /* a b */
        break;
      case 0xc6:
        if(T(b)==0 && n(b)==1) { K *pb=px(b); p=k_(pb[0]); _k(b); b=p; }
        else if(T(b)==0 && n(b)==0) { _k(b); b=null; }
        else if(!VST(b)) { _k(b); *pA++=KERR_TYPE; break; }
        *pA++=builtin(a,0,b);
        break;
      case 0xc7:
        if(T(b)==0 && n(b)==1) { K *pb=px(b); p=k_(pb[0]); _k(b); b=p; }
        else if(T(b)==0 && n(b)==0) { _k(b); b=null; }
        else if(!VST(b)) { _k(b); *pA++=KERR_TYPE; break; }
        *pA++=builtin(a,0,b);
        break;
      case 0xc5:
        if(0x45==s(b)) {
          pb=px(b);
          K av1=pb[1]; char *pav1=av1?px(av1):"";
          t=0;
          if(pA>A&&i<nx-1&&0xc0==s(px[i+1])&&ik(px[i+1])==0xff) {  /* dyadic juxtaposition */
            ++i;
            t=*--pA;
            if(s(t)) { t=reduce(t); if(E(t)||EXIT) { _k(a); _k(b); *pA++=t; break; } }
            if(!VST(t)) { _k(a); _k(b); _k(t); *pA++=KERR_TYPE; break; }
          }
          if(s(pb[2])) { p=reduce(k_(pb[2])); if(E(p)||EXIT) { _k(a); _k(b); _k(t); *pA++=p; break; } }
          else p=k_(pb[2]);
          *pA++=fe(a,t,p,pav1);
          _k(b);
        }
        else if(!VST(b)) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
        else if(0x85==s(b)) {
          K *pa=px(a);
          K av0=pa[1];
          char *pav0=px(av0);
          char *pav1=px(b);
          u64 n=strlen(pav0)+strlen(pav1);
          if(n > 255) { _k(a); _k(b); *pA++=KERR_LENGTH; break; }
          char av2[256];
          snprintf(av2,256,"%s%s",pav0,pav1);
          _k(pa[1]);
          pa[1]=tnv(3,n,xmemdup(av2,1+strlen(av2)));
          _k(b);
          *pA++=a;
        }
        else {
          t=0;
          if(pA>A&&i<nx-1&&0xc0==s(px[i+1])&&ik(px[i+1])==0xff) {  /* dyadic juxtaposition */
            ++i;
            t=*--pA;
            if(s(t)) { t=reduce(t); if(E(t)||EXIT) { _k(a); _k(b); *pA++=t; break; } }
            if(!VST(t)) { _k(a); _k(b); _k(t); *pA++=KERR_TYPE; break; }
          }
          *pA++=fe(a,t,b,"");
        }
        break;
      case 0xd4:
        if(0x45==s(b)) {
          pb=px(b);
          mv=px(pb[1]);
          if(!VST(pb[2])) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
          *pA++=avdo(a,0,k_(pb[2]),mv);
          _k(b);
        }
        else *pA++=fe(a,0,b,"");
        break;
      case 0xd5:
        *pA++=fe(a,0,b,"");
        break;
      case 0xd6:
        *pA++=fe(a,0,b,"");
        break;
      case 0xd7: *pA++=fe(a,0,b,""); break;
      default: *pA++=k(13,a,b); /* a b */
      } break;
    }
    /* suspend console, invoke repl */
    if(pA>A&&pA[-1]) {
      if(pA[-1]<EMAX) pA[-1]=kerror(E[pA[-1]]);
      if(E(pA[-1])) {
        if(EFLAG&&!EXIT&&!SIGNAL&&strcmp(sk(pA[-1]),"abort")) {
          printerror(pA[-1],x0,i);
          ++ecount;
          pA[-1]=repl();
        }
        break;
      }
    }
    if(RETURN) break;
    if(EXIT) { *quiet=1; break; }
  }
  v=pA>A?*--pA:0;
  if(s(v)) {
    if(0x42==s(v)) v=r42(v);
    else if(0x83==s(v)) { /* handle things like b:!10;@[`b;0;:;5];b */
      if(pA>A&&0x44==s(pA[-1])) {
        K x=r44(*--pA);
        if(E(x)) v=x;
        else _k(x); /* discard result */
        if(v<EMAX) v=kerror(E[v]);
      }
    }
  }
  while(pA>A) _k(*--pA);
  if(s(v)) {
    if(0x44==s(v)) {
      v=r44(v);
      if(v && v<EMAX) v=kerror(E[v]);
      if(v && E(v) && EFLAG && !SIGNAL && strcmp(sk(v),"abort")) {
        printerror(v,x0,i?i-1:i);
        ++ecount;
        v=repl();
      }
    }
    else if(0x40==s(v)) {
      v=r40(v);
      if(v && v<EMAX) v=kerror(E[v]);
      if(v && E(v) && EFLAG && !SIGNAL && strcmp(sk(v),"abort")) {
        printerror(v,x0,i?i-1:i);
        ++ecount;
        v=repl();
      }
    }
    else {
      v=reduce(v);
      if(v&&v<EMAX) v=kerror(E[v]);
    }
    if(0x45==s(v)) { /* '"error" */
      K *pv=px(v);
      K av=pv[1];
      K e=pv[2];
      s=px(av);
      if(!strcmp(s,"'") && T(e)==-3 && !s(e)) {
        SIGNAL=1;
        p=kerror(px(e));
        _k(v);
        v=p;
      }
      else { _k(v); v=kerror("parse"); }
    }
    else if(0x85==s(v)) { /* ' (signal null) */
      if(DEPTH==1&&!ecount) { _k(v); v=kerror("parse"); }
      else {
        s=px(v);
        if(!strcmp(s,"'")) { /* signal */
          SIGNAL=1;
          _k(v);
          v=kerror("");
        }
      }
    }
    else if(!VST(v)||0x81==s(v)) { _k(v); v=kerror("parse"); }
  }
  --d;
  if(A2) xfree(A2);
  return v;
}

K pgreduce(K x, int p) {
  K r=null,a,*px;
  int n,timer=0;
  u64 i;
  if(!x) return r;
  if(6==tx) return r;
  if(!nx) return r;
  px=px(x);
  for(i=0;i<nx;i++) {
    K z=px[i]; K *pz=px(z); /* (values;index;stmt;line;file;gline;ggline) */
    K *values=px(pz[0]);
    if(!(n=n(pz[0]))) { r=null; if(timer) { timer=0; printf("%f\n",timer_stop()); r=null; } continue; }
    if(1<n&&0xc0==s(values[1])) {
      if(ck(values[1])==254) { EXIT=t(1,0); return null; }
      else if(ck(values[1])==253) { /* \t */
        timer=1;
        timer_start();
        r=null;
        continue;
      }
      else if(ck(values[1])==252) { /* \l */
        a=k_(values[0]);
        if(!a) { r=null; continue; }
        char *s=xstrndup((char*)px(a),na);
        r=load(s,2);
        _k(a);
        xfree(s);
        if(E(r)) return r;
        continue;
      }
      else if(ck(values[1])==251) { /* \p */
        a=values[0];
        if(ta==1) { precision=ik(a); if(precision>17) precision=17; if(precision<0) precision=0; }
        else if(ta==6) printf("%d\n",precision);
        else { r=kerror("parse"); continue; }
        _k(a);
        r=null;
        continue;
      }
      else if(ck(values[1])==250) { /* \v */
        kdump(0);
        r=null;
        continue;
      }
      else if(ck(values[1])==249) { /* \V */
        kdump(1);
        r=null;
        continue;
      }
      else if(ck(values[1])==248) { /* \p */
        a=values[0];
        if(ta==1) { zeroclamp=ik(a); if(zeroclamp!=0) zeroclamp=1; }
        else if(ta==6) printf("%d\n",zeroclamp);
        else { r=kerror("parse"); continue; }
        _k(a);
        r=null;
        continue;
      }
      else if(ck(values[1])==247) { /* \e */
        a=values[0];
        if(ta==1) EFLAG=ik(a);
        else if(ta==6) printf("%d\n",EFLAG);
        else { r=kerror("parse"); continue; }
        _k(a);
        r=null;
        continue;
      }
      else if(ck(values[1])==246) { /* \d */
        a=k_(values[0]);
        if(!a) { r=null; continue; }
        r=dir_(a);
        continue;
      }
    }
    r=pgreduce_(px[i],&quiet);
    if(E(r)) {
      if(cs!=gs) return r;
      if(!REPL && EFLAG) return r;
    }
    if(EXIT) { _k(r); return null; }
    if(timer) {
      if(E(r)) kprint(r,"","\n","");
      else { _k(r); printf("%f\n",timer_stop()); r=null; }
    }
    else if(p) {
      if(quiet) { _k(r); quiet=0; }
      else if(6==T(r)) ;
      else kprint(r,"","\n","");
    }
    else if(i<nx-1 && !RETURN) {
      _k(r);
    }
    timer=0;
    if(RETURN) {
      if(!REPL) RETURN=0;
      if(cs!=gs) break;
    }
  }
  return r;
}

static K listbc(pgs *s, pn *b, int t);
K list19(pgs *s, pn *a) {
  K r=0;
  char *t;
  K v=tn(0,2); K *pv=px(v);
  if(a->a[1]->t==40) {
    K w=tn(0,a->a[1]->m); K *pw=px(w);
    i(n(w),pw[i]=listbc(s,a->a[1]->a[i],0x41))
    pv[0]=k_(a->a[0]->t==1?a->a[0]->v:a->a[0]->n);
    pv[1]=st(0x40,w);
    r=st(0x44,v);
  }
  else if(a->a[1]->t==4) {
    if(a->a[0]->t==7) {

      pn *q=a->a[0];
      K v3=tn(0,q->m); K *pv3=px(v3);
      for(int i=0;i<q->m;++i) {
        if(q->a[i]->t==11) {
          K v2=tn(0,2); K *pv2=px(v2);
          if(q->a[i]->a[0]->t==6) pv2[0]=listbc(s,q->a[i]->a[0],0x42);
          else pv2[0]=k_(q->a[i]->a[0]->n);
          if(s(q->a[i]->v)) {
            pv2[1]=k_(q->a[i]->v);
          }
          else {
            t=strchr(P,q->a[i]->v);
            pv2[1]=t(1,st(0xc0,t-P));
          }
          pv3[i]=st(0xd0,v2);
        }
        else if(s(q->a[i]->v)) {
          pv3[i]=k_(q->a[i]->v);
        }
        else {
          t=strchr(P,q->a[i]->v);
          pv3[i]=t(1,st(0xc0,t-P));
        }
      }
      K w=tn(0,2); K *pw=px(w);
      pw[0]=v3;
      pw[1]=tn(3,0); /* adverbs */
      pv[0]=st(0xc5,w);

    }
    else if(a->a[0]->t==19) {
      pv[0]=list19(s,a->a[0]);
    }
    else pv[0]=k_(a->a[0]->t==1?a->a[0]->v:a->a[0]->n);
    pv[1]=listbc(s,a->a[1],0x41);
    r=st(0x44,v);
  }
  else { printf("fatal\n"); exit(1); }
  return r;
}

static void bc(pgs *s, pn *a, K values, K index, K line, int *vm) {
  char *t;
  pn **s0,**s1;
  int i0=0,i1=0,i0m=1024,i1m=1024,i,j=0;
  K *pvalues=px(values);
  int *pindex=px(index);
  int *pline=px(line);
  s0=xmalloc(sizeof(pn*)*i0m);
  s1=xmalloc(sizeof(pn*)*i1m);
  s0[++i0]=a;
  while(i0) {
    a=s0[i0--];
    if(++i1==i1m) { i1m<<=1; s1=xrealloc(s1,sizeof(pn*)*i1m); }
    s1[i1]=a;
    if(a->t==6) continue; /* klist () */
    if(a->t==4) continue; /* plist [] */
    if(a->t==40) continue; /* [][]... */
    if(a->t==19) continue; /* f[] */
    if(a->t==11) continue; /* 1+ */
    if(a->t==7) continue; /* +- */
    for(i=0;i<a->m;i++) {
      if(!a->a[i]) continue;
      if(++i0==i0m) { i0m<<=1; s0=xrealloc(s0,sizeof(pn*)*i0m); }
      s0[i0]=a->a[i];
    }
  }
  while(i1) {
    a=s1[i1--];
    pindex[j]=a->i;
    pline[j]=a->line;
    if(a->t==1) {
      if(0xc1==s(a->v)||0xc2==s(a->v)) {
        if(0xc1==s(a->v)&&a->a[0]) a->v=(a->v&~((K)0xff<<48))|(K)0xc2<<48;
        pvalues[j++]=k_(a->v);
      }
      else if(0xce==s(a->v)) {
        if(a->a[0]&&!a->a[1]) a->v=(a->v&~((K)0xff<<48))|(K)0xcf<<48;
        pvalues[j++]=k_(a->v);
      }
      else if(0xc6==s(a->v)||0xc7==s(a->v)||0xd1==s(a->v)||0xd2==s(a->v)
            ||0xd3==s(a->v)||0xcc==s(a->v)||0xcd==s(a->v)||0xca==s(a->v)
            ||0xc9==s(a->v)||0xcb==s(a->v)||0x85==s(a->v)||0x82==s(a->v)
            ||0xd8==s(a->v)) {
        pvalues[j++]=k_(a->v);
      }
      else if(a->v==0xff) pvalues[j++]=t(1,st(0xc0,(u8)0xff));
      else if((t=strchr(P,a->v))) pvalues[j++]=t(1,st(0xc0,(a->a[0]?64:32)+t-P));
      else pvalues[j++]=t(1,st(0xc0,a->v)); /* sys */
    }
    else if(a->t==2) { pvalues[j++]=a->n; a->n=0; }
    else if(a->t==6) { pvalues[j++]=listbc(s,a,0x42); } /* klist */
    else if(a->t==4) { pvalues[j++]=listbc(s,a,0x41); } /* plist */
    else if(a->t==19) { pvalues[j++]=list19(s,a); } /* f[x] aka 0x44 subtype */
    else if(a->t==11) {  /* 1+ */
      K v=tn(0,2); K *pv=px(v);
      if(a->a[0]->t==6) pv[0]=listbc(s,a->a[0],0x42);
      else pv[0]=k_(a->a[0]->n);
      if(s(a->v)) {
        pv[1]=k_(a->v);
      }
      else {
        t=strchr(P,a->v);
        pv[1]=t(1,st(0xc0,t-P));
      }
      pvalues[j++]=st(0xd0,v);
    }
    else if(a->t==7) {  /* +- */
      K v=tn(0,a->m); K *pv=px(v);
      for(int i=0;i<a->m;++i) {
        if(a->a[i]->t==11) {
          K v2=tn(0,2); K *pv2=px(v2);
          if(a->a[i]->a[0]->t==6) pv2[0]=listbc(s,a->a[i]->a[0],0x42);
          else pv2[0]=k_(a->a[i]->a[0]->n);
          if(s(a->a[i]->v)) {
            pv2[1]=k_(a->a[i]->v);
          }
          else {
            t=strchr(P,a->a[i]->v);
            pv2[1]=t(1,st(0xc0,t-P));
          }
          pv[i]=st(0xd0,v2);
        }
        else if(s(a->a[i]->v)) {
          pv[i]=k_(a->a[i]->v);
        }
        else {
          t=strchr(P,a->a[i]->v);
          pv[i]=t(1,st(0xc0,t-P));
        }
      }
      K w=tn(0,2); K *pw=px(w);
      pw[0]=v;
      pw[1]=tn(3,0); /* adverbs */
      pvalues[j++]=st(0xc5,w);
    }
    /* node will be freed in pgfree() */
    if(j==*vm) {
      *vm<<=1;
      values=kresize(values,*vm); pvalues=px(values);
      index=kresize(index,*vm); pindex=px(index);
      line=kresize(line,*vm); pline=px(line);
    }
  }
  n(values)=j;
  n(index)=j;
  n(line)=j;
  xfree(s0);
  xfree(s1);
}
static K listbc(pgs *s, pn *a, int t) {
  K *pz,stmt,*pv;
  int i,k=0,vm,constants=1;
  K z=prnew(a->m);
  pz=px(z);
  for(i=0;i<a->m;i++) {
    if(!a->a[i]) continue;
    vm=256;
    pz[k]=tn(0,7);
    K values=tn(0,vm); n(values)=0;
    K index=tn(1,vm); n(index)=0;

    char *q=s->p;
    if(opencode) stmt=ksplit(q,"\r\n");
    else {
      char q0=q[strlen(q)-1];
      q[strlen(q)-1]='}';
      stmt=ksplit(q,"\r\n");
      q[strlen(q)-1]=q0;
    }

    K line=tn(1,vm); n(line)=0;
    ((K*)px(pz[k]))[0]=values;
    ((K*)px(pz[k]))[1]=index;
    ((K*)px(pz[k]))[2]=stmt;
    ((K*)px(pz[k]))[3]=line;
    ((K*)px(pz[k]))[4]=tnv(3,strlen(s->file),xmemdup(s->file,1+strlen(s->file)));
    ((K*)px(pz[k]))[5]=t(1,(u32)a->line); // gline
    ((K*)px(pz[k]))[6]=t(1,(u32)fileline); // ggline
    bc(s,a->a[i],values,index,line,&vm);
    if(n(values)==1) {
      pv=px(values);
      if(0x40==s(pv[0])) constants=0;
      if(s(pv[0])) constants=0;
    }
    else constants=0;
    ++k;
  }
  n(z)=k;
  if(constants && 0x41==t) {
    K r,*pr,*pz;
    if(n(z)) {
      r=tn(0,n(z)); pr=px(r);
      pz=px(z);
      i(n(z),pr[i]=k_(((K*)px(((K*)px(pz[n(z)-i-1]))[0]))[0]))
      i(n(z),if(!pr[i]) pr[i]=inull)
    }
    else {
      r=tn(0,1); pr=px(r);
      pr[0]=null;
    }
    _k(z);
    return st(0x81,r);
  }
  else return st(t,z);
}

static int hasav(pn *a) {
  if(a->t==2 && 0xc3==s(a->n)) {
    K *pf=px(a->n);
    char *pc=px(pf[0]);
    if(strchr("'/\\",pc[strlen(pc)-1])) return 1;
    if(pf[3]!=null) {
      pc=px(pf[3]);
      if(strlen(pc)) return 1;
    }
  }
  else if(a->t==2 && 0x40==s(a->n) && 4==T(a->n)) {
    char *pc=sk(a->n);
    if(strchr("'/\\",pc[strlen(pc)-1])) return 1;
  }
  return 0;
}

/* parse node allocation with global tracking for memory management */
pn* pnnew(pgs *s, int t, K v, K n, int m, int f) {
  pn *r=xmalloc(sizeof(pn));
  r->t=t;
  r->v=v;
  r->n=n;
  r->m=m;
  r->f=f;
  r->i=-1;
  r->a=m?xcalloc(m,sizeof(pn*)):0;

  /* add to the global node array, grow if needed */
  if(s->node_count == s->node_max) {
    s->node_max = s->node_max ? s->node_max << 1 : 64;  /* start with 64, double when full */
    s->all_nodes = xrealloc(s->all_nodes, s->node_max * sizeof(pn*));
  }
  s->all_nodes[s->node_count++] = r;

  return r;
}
pn* pnnewi(pgs *s, int t, K v, K n, int m, int f, int i, int line) {
  pn *r=pnnew(s,t,v,n,m,f);
  r->i=i;
  r->line=line;
  r->file=pfile;
  r->gline=gline;
  return r;
}

void pnfree(pn *n) {
  if(n->n>>48) _k(n->n); /* if tagged */
  if(n->v>>48) _k(n->v); /* if tagged */
  if(n->m) {
    /* no recursive freeing - let pgfree() handle all nodes */
    xfree(n->a);
  }
  xfree(n);
}

pn* pnappend(pn *n0, pn *n1) {
  n0->a=xrealloc(n0->a,sizeof(pn*)*(1+n0->m));
  n0->a[n0->m++]=n1;
  return n0;
}

#define V(x) ((x)->t&1)

static void r000(pgs *s) { /* $a > s */
  bc(s,s->V[s->vi],s->values,s->index,s->line,&s->valuesmax);
}
static void r001(pgs *s) { /* s > e se */
  pn *a=s->V[s->vi];
  --s->vi;
  if(a->t==20) s->q=1;
  else s->q=0;
}
static void r002(pgs *s) { /* s > se */
  (void)s;
}
static void r003(pgs *s) { /* e > o ez */
  char *t;
  pn *q;
  pn *b=s->V[s->vi--];
  pn *a=s->V[s->vi];
  if(!b) return;
  if(a->t==2 && (b->t==4 || b->t==40)) {
    q=pnnewi(s,19,0,0,2,1,a->i,a->line);
    q->a[0]=a;
    q->a[1]=b;
    s->V[s->vi]=q;
  }
  else if(a->t==2 && b->a[0] && (b->a[0]->t==4 || b->a[0]->t==40)) {
    q=pnnewi(s,19,0,0,2,1,a->i,a->line);
    q->a[0]=a;
    q->a[1]=b->a[0];
    b->a[0]=q;
    s->V[s->vi]=b;
  }
  else if(V(a) && (b->t==4 || b->t==40)
          && 0xd1!=s(a->v) && 0xd2!=s(a->v) && 0xd3!=s(a->v)
          && 36!=a->v && 0x85!=s(a->v)) { /* 3 2 12 36 16 = do while if $ av */
    q=pnnewi(s,19,0,0,2,1,a->i,a->line);
    q->a[0]=a;
    q->a[1]=b;
    s->V[s->vi]=q;
  }
  else if(V(a) && V(b)) {
    if(b->a[0] && b->a[0]->t==40) {
      q=pnnewi(s,19,0,0,2,1,a->i,a->line);
      q->a[0]=a;
      q->a[1]=b->a[0];
      b->a[0]=q;
      s->V[s->vi]=b;
    }
    else if(0x85==s(a->v)) {
      q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
      q->a[0]=a;
      q->a[1]=b;
      s->V[s->vi]=q;
    }
    else if(b->v==0xff && b->a[0] && b->a[0]->t==4) {
      q=pnnewi(s,19,0,0,2,1,a->i,a->line);
      q->a[0]=a;
      q->a[1]=b->a[0];
      b->a[0]=q;
      s->V[s->vi]=b;
    }
    else if(b->a[0]&&(b->a[0]->t==4||b->a[0]->t==40)) { /* sqr[2]+sqr[3] */
      q=pnnewi(s,19,0,0,2,1,a->i,a->line);
      q->a[0]=a;
      q->a[1]=b->a[0];
      b->a[0]=q;
      s->V[s->vi]=b;
    }
    else if(a->t==7 && 0x85==s(b->v)) {
      q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
      q->a[0]=a;
      q->a[1]=b;
      s->V[s->vi]=q;
    }
    else if(a->t==7 && 0xff==b->v && b->a[0] && 0x85==s(b->a[0]->v)) {
      q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
      q->a[0]=a;
      q->a[1]=b;
      s->V[s->vi]=q;
    }
    else if(a->t==7 && b->t==11) { /* (|,)0, */
      a->m++;
      a->a=xrealloc(a->a,a->m*sizeof(pn*));
      a->a[a->m-1]=b;
      s->V[s->vi]=a;
    }
    else if(!a->a[1]) {
      if(b->a[0]&&b->a[0]->t==4&&b->v!=0xff) { /* v[] higher precedence (except for juxtaposition) */
        a->a[1]=b->a[0];
        b->a[0]=a;
        s->V[s->vi]=b;
      }
      else if(b->t==7 && a->v!=58 && b->v!=58  /* verb composition, no : */
             && 0xd1!=s(a->v) && 0xd2!=s(a->v) && 0xd3!=s(a->v)) {  /* no do while if */
        b->m++;
        b->a=xrealloc(b->a,b->m*sizeof(pn*));
        i(b->m-1,b->a[b->m-i-1]=b->a[b->m-i-2])
        b->a[0]=a;
        s->V[s->vi]=b;
      }
      else if(b->t==11 && a->v!=58 && b->v!=58  /* verb composition, no : */
             && 0xd1!=s(a->v) && 0xd2!=s(a->v) && 0xd3!=s(a->v)) {  /* no do while if */
        q=pnnewi(s,7,0,0,2,1,a->i,a->line);
        q->a[0]=a;
        q->a[1]=b;
        s->V[s->vi]=q;
      }
      else if(!b->a[0] && !b->a[1] && a->v!=58 && b->v!=58  /* verb composition, no : */
             && 0xd1!=s(a->v) && 0xd2!=s(a->v) && 0xd3!=s(a->v)  /* no do while if */
             && 0xd1!=s(b->v) && 0xd2!=s(b->v) && 0xd3!=s(b->v)) {  /* no do while if */
        q=pnnewi(s,7,0,0,2,1,a->i,a->line);
        q->a[0]=a;
        q->a[1]=b;
        s->V[s->vi]=q;
      }
      else if(b->v==58 && !b->a[0] && b->a[1] /* in:2 */
             && (0xc7==s(a->v) || 0xc6==s(a->v) || 0xc9==s(a->v) || 0xca==s(a->v) || 0xcb==s(a->v))) {
        b->a[0]=a;
        s->V[s->vi]=b;
      }
      else if(a->t==11 && b->v==0xff) {
        q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
        q->a[0]=a;
        q->a[1]=b;
        s->V[s->vi]=q;
      }
      else a->a[1]=b; /* 1+2*3 */
    }
    else { /* (1+2)*3 */
      b->a[0]=a;
      s->V[s->vi]=b;
    }
  }
  else if(V(a)) {
    if(0x85==s(a->v)) {
      q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
      q->a[0]=a;
      q->a[1]=b;
      s->V[s->vi]=q;
    }
    else if(a->a[1]) {
      q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
      q->a[0]=a;
      q->a[1]=b;
      s->V[s->vi]=q;
    }
    else a->a[1]=b;
  }
  else if((a->t==2||a->t==6) && V(b)) {
    if(b->t==7 && a->t==6 && a->m==1 && a->a[0] && a->a[0]->t==1) { /* =(,)(|)(|,)0, */
      b->m++;
      b->a=xrealloc(b->a,b->m*sizeof(pn*));
      i(b->m-1,b->a[b->m-i-1]=b->a[b->m-i-2])
      b->a[0]=a->a[0];
      s->V[s->vi]=b;
    }
    else if(b->t==7 && b->a[0]) {   /* 1+- */
      b->a[0]->a[0]=a;
      b->a[0]->t=11;
      s->V[s->vi]=b;
    }
    else if(b->a[0]) {
      if(b->v==0x27 && V(b->a[0])) { /* ' */
        t=strchr(P,b->a[0]->n);
        b->v=256+t-P;
        b->a[0]=a;
        s->V[s->vi]=b;
      }
      else if(b->v==0xce && V(b->a[0])) { /* / */
        t=strchr(P,b->a[0]->n);
        b->v=512+t-P;
        b->a[0]=a;
        s->V[s->vi]=b;
      }
      else if(b->v==0x5c && V(b->a[0])) { /* \ */
        t=strchr(P,b->a[0]->n);
        b->v=1024+t-P;
        b->a[0]=a;
        s->V[s->vi]=b;
      }
      else if(b->a[0] && (b->a[0]->t==4||b->a[0]->t==40) && b->v!=58 && b->v!=0xff) { /* d[],1   b->v != : */
        q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
        q->a[0]=a;
        q->a[1]=b->a[0];
        b->a[0]=q;
        s->V[s->vi]=b;
      }
      else if(b->a[0] && b->a[0]->v==0xff) {
        q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
        q->a[0]=a;
        q->a[1]=b->a[0];
        b->a[0]=q;
        s->V[s->vi]=b;
      }
      else {
        q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
        q->a[0]=a;
        q->a[1]=b;
        s->V[s->vi]=q;
      }
    }
    else {
      if(hasav(a)) {
        q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
        q->a[0]=a;
        q->a[1]=b;
        s->V[s->vi]=q;
      }
      else if(b->a[1]) {
        if(0xcc==s(b->v)) b->v = set_sx(b->v,0xcd); /* 0:x -> a 0:x */
        if(0xc6==s(b->v)) {
          q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
          q->a[0]=a;
          q->a[1]=b;
          s->V[s->vi]=q;
        }
        else if(b->a[1]->t==4) {
          q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
          q->a[0]=a;
          q->a[1]=b;
          s->V[s->vi]=q;
        }
        else {
          b->a[0]=a;
          s->V[s->vi]=b;
        }
      }
      else if(0xce==s(b->v)) { /* x+: */
        b->a[0]=a;
        s->V[s->vi]=b;
      }
      else if(0xc3==s(a->n)) {
        q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
        q->a[0]=a;
        q->a[1]=b;
        s->V[s->vi]=q;
      }
      else {
        b->a[0]=a;
        b->t=11;
        s->V[s->vi]=b;
      }
    }
  }
  else {
    q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
    q->a[0]=a;
    q->a[1]=b;
    s->V[s->vi]=q;
  }
}
static void r004(pgs *s) { /* se > ';' */
  pn *a=s->V[s->vi];
  a->t=20;
}
static void r005(pgs *s) { /* se > '\n' */
  pn *a=s->V[s->vi];
  a->t=21;
}
static void r006(pgs *s) { /* o > N */
  pn *a=s->V[s->vi];
  a->t=2;
}
static void r007(pgs *s) { /* o > V */
  pn *a=s->V[s->vi];
  a->t=1;
  a->v=a->n;
  a->n=0;
}
static void r008(pgs *s) { /* o > klist */
  (void)s;
}
static void r009(pgs *s) { /* ez > */
  if(++s->vi==s->Vm) GROW();
  s->V[s->vi]=0;
}
static void r010(pgs *s) { /* ez > e */
  (void)s;
}
static void r011(pgs *s) { /* ez > plist */
  (void)s;
}
static void r012(pgs *s) { /* klist > '(' elist ')' */
  s->vi--;
  pn *a=s->V[s->vi--];
  if(!a) a=pnnew(s,5,0,0,2,1);
  else if(a->t==3) a->t=6;
  a->i=s->V[s->vi]->i;
  a->line=s->V[s->vi]->line;
  s->V[s->vi]=a;
}
static void r013(pgs *s) { /* plist > '[' elist ']' pz */
  pn *b=s->V[s->vi--];
  s->vi--;
  pn *a=s->V[s->vi--];
  if(a) {
    a->i=s->V[s->vi]->i;
    a->line=s->V[s->vi]->line;
  }
  // []
  if(!a&&!b) {
    a=pnnewi(s,4,0,0,2,1,s->V[s->vi]->i,s->V[s->vi]->line);
    s->V[s->vi]=a;
    return;
  }
  // [a]
  if(a&&!b) {
    if(a->t==3) {
      a->t=4;
      s->V[s->vi]=a;
      return;
    }
    else {
      pn *q=pnnewi(s,4,0,0,2,1,a->i,a->line);
      q->a[0]=a;
      s->V[s->vi]=q;
      return;
    }
  }
  // [] b
  if(!a&&b) a=pnnewi(s,4,0,0,2,1,s->V[s->vi]->i,s->V[s->vi]->line);

  // elist -> plist
  if(a->t==3) a->t=4;

  if(a->t==4&&b->t==4) { /* [][] */
    pn *q=pnnewi(s,40,0,0,2,1,a->i,a->line);
    q->a[0]=a;
    q->a[1]=b;
    s->V[s->vi]=q;
  }
  else if(a->t==4&&b->t==40) { /* [] [][]... */
    b->m++;
    b->a=xrealloc(b->a,b->m*sizeof(pn*));
    i(b->m-1,b->a[b->m-i-1]=b->a[b->m-i-2])
    b->a[0]=a;
    s->V[s->vi]=b;
  }
  else if(a->t==4&&b->a[0]&&b->a[0]->t==40&&b->v==0xff) { /* [] [][]...v */
    b->a[0]->m++;
    b->a[0]->a=xrealloc(b->a[0]->a,b->a[0]->m*sizeof(pn*));
    i(b->a[0]->m-1,b->a[0]->a[b->a[0]->m-i-1]=b->a[0]->a[b->a[0]->m-i-2])
    b->a[0]->a[0]=a;
    s->V[s->vi]=b;
  }
  else if(a->t==4&&b->a[0]&&b->a[0]->t==4) { /* [] []v */
    pn *q=pnnewi(s,40,0,0,2,1,a->i,a->line);
    q->a[0]=a;
    q->a[1]=b->a[0];
    b->a[0]=q;
    s->V[s->vi]=b;
  }
  else if(a->t==4&&0x85==s(b->v)) { /* []' */
    pn *q=pnnewi(s,104,0,0,2,1,a->i,a->line);
    q->a[0]=a;
    q->a[1]=b;
    s->V[s->vi]=q;
  }
  else if(b->t==1) { /* [0]:5 */
    if(b->v==0xff) {
      pn *q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
      q->a[0]=a;
      q->a[1]=b;
      s->V[s->vi]=q;
    }
    else if(b->a[0]&&b->a[0]->v==0xff) {
      pn *q=pnnewi(s,1,0xff,0,2,1,b->a[0]->i,b->a[0]->line);
      q->a[0]=b->a[0];
      q->a[1]=a;
      b->a[0]=q;
      s->V[s->vi]=b;
    }
    else if(b->a[0]) {
      pn *q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
      q->a[0]=a;
      q->a[1]=b;
      s->V[s->vi]=q;
    }
    else {
      b->a[0]=a;
      s->V[s->vi]=b;
    }
  }
  else {
    pn *q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
    q->a[0]=a;
    q->a[1]=b;
    s->V[s->vi]=q;
  }
}
static void r014(pgs *s) { /* pz > */
  if(++s->vi==s->Vm) GROW();
  s->V[s->vi]=0;
}
static void r015(pgs *s) { /* pz > e */
  (void)s;
}
static void r016(pgs *s) { /* pz > plist */
  (void)s;
}
static void r017(pgs *s) { /* elist > ee elistz */
  pn *b=s->V[s->vi--];
  pn *a=s->V[s->vi];
  if(!b) {
    if(a->t==2&&!a->n) a->t=3;
    else if(a->t==2 || a->t==1) {
      b=pnnewi(s,3,0,0,1,1,a->i,a->line);
      b->a[0]=a;
      s->V[s->vi]=b;
    }
    return;
  }
  if(b->m==2&&!b->a[1]) b->a[1]=a;
  else {
    b->a=xrealloc(b->a,sizeof(K*)*(b->m+1));
    b->a[b->m++]=a;
  }
  s->V[s->vi]=b;
}
static void r018(pgs *s) { /* elistz > */
  if(++s->vi==s->Vm) GROW();
  s->V[s->vi]=0;
}
static void r019(pgs *s) { /* elistz > se ee elistz */
  pn *c=s->V[s->vi--];
  pn *b=s->V[s->vi--];
  pn *a=s->V[s->vi];
  if(!c) {
    a->a[0]=b;
    a->t=3;
  }
  else {
    c->t=3;
    if(c->m==2&&!c->a[1]) c->a[1]=b;
    else {
      c->a=xrealloc(c->a,sizeof(K*)*(c->m+1));
      c->a[c->m++]=b;
    }
    s->V[s->vi]=c;
  }
}
static void r020(pgs *s) { /* ee > */
  if(++s->vi==s->Vm) GROW();
  s->V[s->vi]=pnnew(s,2,0,0,2,1);
}
static void r021(pgs *s) { /* ee > e */
  (void)s;
}

static void (*F[22])(pgs *s)={r000,r001,r002,r003,r004,r005,r006,r007,r008,r009,r010,r011,r012,r013,r014,r015,r016,r017,r018,r019,r020,r021};

pgs* pgnew(void) {
  pgs *s=xcalloc(1,sizeof(pgs));
  s->Sm=s->Rm=s->Vm=1024;
  s->tm=s->vm=1024;
  s->S=xmalloc(s->Sm*sizeof(int));
  s->R=xmalloc(s->Rm*sizeof(int));
  s->V=xmalloc(s->Vm*sizeof(pn*));
  s->t=xmalloc(s->tm*sizeof(int));
  s->v=xmalloc(s->vm*sizeof(pn*));
  /* initialize global node tracking array */
  s->all_nodes = 0;
  s->node_count = 0;
  s->node_max = 0;
  return s;
}
void pgfree(pgs *s) {
  if(s->p) xfree(s->p);
  if(s->S) xfree(s->S);
  if(s->R) xfree(s->R);
  if(s->V) xfree(s->V);
  if(s->t) xfree(s->t);
  if(s->v) xfree(s->v);

  /* free all tracked parse nodes */
  if(s->all_nodes) {
    i(s->node_count, pnfree(s->all_nodes[i]))
    xfree(s->all_nodes);
  }

  xfree(s);
}
K prnew(int n) {
  K z=tn(0,n);
  n(z)=0;
  return z;
}
void prfree(K x) {
  _k(x);
}
K pgparse(char *q, int load, K locals) {
  K z,*pz,*pv,e,values,index,line,stmt=0;
  int j,r,zo;
  u64 zn,zm=256;
  if(!q) return KERR_PARSE;
  pgs *s=pgnew();
  s->locals=locals;
  z=prnew(zm);
  pz=px(z);
  s->p=q;
  s->file=pfile;
  s->valuesmax=256;
  s->ti=0;s->tc=0;s->si=-1;s->ri=-1;s->vi=-1;
  if(opencode) stmt=ksplit(q,"\r\n");
  else if(*q) {
    char q0=q[strlen(q)-1];
    q[strlen(q)-1]='}';
    stmt=ksplit(q,"\r\n");
    q[strlen(q)-1]=q0;
  }
  e=lex(s,load);
  if(e) goto cleanup;
  if(s->tc<1) { e=KERR_PARSE; goto cleanup; }
  while(1+s->ti<s->tc) {
    s->si=-1;s->ri=-1;s->vi=-1;
    zn=n(z);
    values=tn(0,s->valuesmax); n(values)=0;
    index=tn(1,s->valuesmax); n(index)=0;
    line=tn(1,s->valuesmax); n(line)=0;
    pz[zn]=tn(0,7);
    ((K*)px(pz[zn]))[0]=values;
    ((K*)px(pz[zn]))[1]=index;
    ((K*)px(pz[zn]))[2]=k_(stmt);
    ((K*)px(pz[zn]))[3]=line;
    ((K*)px(pz[zn]))[4]=tnv(3,strlen(pfile),xmemdup(pfile,1+strlen(pfile)));
    ((K*)px(pz[zn]))[5]=t(1,(u32)gline);
    ((K*)px(pz[zn]))[6]=t(1,(u32)fileline);
    n(z)++;
    s->values=values;
    s->index=index;
    s->line=line;
    s->S[++s->si]=T000; /* $a */
    for(;;) {
      if(s->S[s->si]==s->t[s->ti]) {
        if(++s->vi==s->Vm) GROW();
        s->V[s->vi]=s->v[s->ti++];
        --s->si;
      }
      else {
        if(s->S[s->si]>=LI) { e=KERR_PARSE; goto cleanup; }
        if(s->t[s->ti]>=LJ) { e=KERR_PARSE; goto cleanup; }
        r=LL[s->S[s->si--]][s->t[s->ti]];
        if(r==-1) { e=KERR_PARSE; goto cleanup; }
        s->R[++s->ri]=r;
        while(s->si+RC[r]+1>=s->Sm) GROW();
        s->S[++s->si]=-2; /* reduction marker */
        for(j=RC[r]-1;j>=0;j--) s->S[++s->si]=RT[r][j];
      }
      while(s->si>=0&&s->S[s->si]==-2) { (*F[s->R[s->ri--]])(s); --s->si; }
      if(s->si<0) { --s->vi; break; }
    }
    if(s->q) { /* quiet? ; */
      s->q=0;
      /* check if s->values needs resizing before writing */
      if(n(s->values) >= (u64)s->valuesmax) {
        s->valuesmax<<=1;
        s->values=kresize(s->values,s->valuesmax);
        s->index=kresize(s->index,s->valuesmax);
        s->line=kresize(s->line,s->valuesmax);
        /* update the main array pointer since s->values changed */
        ((K*)px(pz[n(z)-1]))[0]=s->values;
        ((K*)px(pz[n(z)-1]))[1]=s->index;
        // stmt is skipped
        ((K*)px(pz[n(z)-1]))[3]=s->line;
      }
      pv=px(s->values);
      pv[n(s->values)++]=t(6,st(0x83,0));
    }
    /* check main array resize after potentially adding quiet element */
    if(++zn==zm) {
      zo=zn;
      zm<<=1;
      z=kresize(z,zm);
      pz=px(z);
      n(z)=zo;
      s->values=((K*)px(pz[zo-1]))[0];
      s->index=((K*)px(pz[zo-1]))[1];
      // stmt is skipped
      s->line=((K*)px(pz[zo-1]))[3];
      s->valuesmax=256;
    }
  }
  _k(stmt);
  pgfree(s);
  return z;
cleanup:
  if(s->tc && stmt && n(stmt)>(u64)s->v[s->ti]->line) { /* lex error if s->tc==0 */
    K *pstmt=px(stmt);
    glinei=s->v[s->ti]->i;
    if(glinep) xfree(glinep);
    glinep=xstrdup(px(pstmt[s->v[s->ti]->line]));
  }
  _k(stmt); prfree(z); pgfree(s);
  return e;
}
