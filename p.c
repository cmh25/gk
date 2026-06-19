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
#include "ipc.h"

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
#ifdef FUZZING
long gk_budget=GK_BUDGET;
#endif
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

/* Issue #2 Pass 6: name+adverb lookup is gone -- the lexer no longer
   emits names with embedded `'`/`/`/`\` and the parser composes adverbs
   onto values via the postfix-restructure path.  vlookup is now a thin
   scope_get wrapper.  vlookupav and wrap_lambda_av retired. */
static K vlookup(K x) {
  /* Fast path: a simple global name at top level (cs==gs) resolves straight
     from the global cache, skipping the scope_get -> scope_get_ call chain
     that otherwise only checks the same cache at its bottom. Same guard
     scope_get_ uses (s==gs, non-dotted). Misses fall through unchanged --
     scope_get_ still populates the cache on first reference. */
  if(cs==gs && 4==T(x)) {
    char *n=sk(x);
    if(*n!='.') { K r=gcache_get(n); if(r) return r; }
  }
  K r=scope_get(cs,x);
  return r?r:KERR_VALUE;
}
/* Capture a value operand into a derived-verb wrapper (0xda adverb-modified
   verb, 0xd7 over/scan-with-seed, etc).  An adverb/seed only meaningfully
   pairs with a verb, but the grammar tolerates a noun operand,
   so we still build the wrapper -- we just capture the
   operand BY VALUE when it isn't a function.  A borrowed reference to a live
   dict could otherwise be stored back into its own subtree (e.g. `o.a.a:o.a'`
   or `o.a.i:o.a\`/`o.a`\`), forming a reference cycle that the refcount GC can
   never reclaim.  ISF passes primitive verbs (<256) and the 0x40-tagged
   function family -- a shared verb ref can't close a cycle -- so only
   nouns/dicts copy; scalar seeds copy for free (kcp of an inline atom is a
   no-op).  Consumes a; returns the operand to store (caller owns it), or an
   error K if the copy failed. */
static K vcap(K a) { if(ISF(a)) return a; K c=kcp(a); _k(a); return c; }

static K vlookuprs(K x, K *rs) {
  K r=0,sc=cs,*ps;
  if(0x40!=s(x)) return KERR_VALUE;
  /* fast path: indexed locals (type 1) go directly to scope_get */
  if(4!=T(x)) {
    r=scope_get(cs,x);
    if(r) { *rs=cs; return r; }
    return KERR_VALUE;
  }
  char *n=sk(x);
  char *dot=strchr(n,'.');
  char base[256];

  /* for dotted names, extract base and look up path */
  if(dot && dot!=n && (size_t)(dot-n)<sizeof(base)-1) {
    memcpy(base,n,dot-n); base[dot-n]=0;
    while(sc!=null&&sc!=ks) {
      ps=px(sc);
      if(0x80!=s(ps[1])) break;
      r=dget(ps[1],sp(base));  /* look for base "d" */
      if(r) {
        /* found base, now traverse path */
        *rs=sc;
        char *p=dot;
        while(*++p) {
          char *p2=strchr(p,'.');
          size_t len=p2?(size_t)(p2-p):strlen(p);
          if(len>=sizeof(base)) { _k(r); return KERR_LENGTH; }
          memcpy(base,p,len); base[len]=0;
          if(0x80!=s(r)) { _k(r); return KERR_VALUE; }  /* not a dict - treat as not found */
          K t=dget(r,sp(base));
          _k(r);
          if(!t) return KERR_VALUE;
          r=t;
          if(!p2) break;
          p=p2;
        }
        return r;
      }
      if(null==ps[2]) sc=ps[0];
      else break;
    }
  } else {
    /* simple name */
    while(sc!=null&&sc!=ks) {
      ps=px(sc);
      if(0x80!=s(ps[1])) break;
      r=dget(ps[1],n);
      if(r) { *rs=sc; break; }
      if(null==ps[2]) sc=ps[0];
      else break;
    }
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

static inline K reduce(K x);
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
    if(!r&&t==0) r=null;
    else if(!r&&t==0x81) r=inull;
    if(r && E(r)&&!strcmp(sk(r),"value")) {
      if(EFLAG&&!SIGNAL&&strcmp(sk(r),"abort")) {
        printerror(r,px[0],0);
        ++ecount;
        r=repl();
      }
    } // note: when running like "./gk < file", repl() can return anything
    if(E(r)||EXIT) return r;
    if(s(r)&&T(r)==0&&((ko*)(b(48)&r))->r) {
      p=kcp(r); _k(r); r=p;
    }
    if(r<EMAX) r=kerror(E[r]);
    //if(r && E(r)) {
    //  _k(x); _k(r);
    //  return a;
    //}
    RETURN=0;
    return r;
  }
  else {
    r=tn(0,nx);
    if(nx) {
      pr=px(r);
      for(i=nx-1;i>=0;--i) {
        K stmt=px[i];
        K *pstmt=px(stmt);
        K vals=pstmt[0];
        /* fast path: single-token sub-statement avoids pgreduce_ recursion.
           handles the common f[a;b;c] / f[1;2;3] / f[c] cases inline. */
        if(n(vals)==1) {
          K v=((K*)px(vals))[0];
          if(0==s(v)) { a=k_(v); q=0; }
          else if(0x40==s(v)) {
            a=vlookup(v);
            if(a && s(a)) a=reduce(a);
            if(a && a<EMAX) a=kerror(E[a]);
            if(a && E(a) && EFLAG && !SIGNAL && strcmp(sk(a),"abort")) {
              printerror(a,stmt,0);
              ++ecount;
              a=repl();
            }
            q=0;
          }
          else {
            a=pgreduce_(stmt,&q);
          }
        }
        else {
          a=pgreduce_(stmt,&q);
        }
        if(!a&&t==0) a=null;
        else if(!a&&t==0x81) a=inull;
        if(a && E(a)&&!strcmp(sk(a),"value")) {
          if(EFLAG&&!SIGNAL&&strcmp(sk(a),"abort")) {
            printerror(a,stmt,0);
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
  if(d==KERR_VALUE) { /* d[`a]:1 - d not found, create in global */
    if(T(i)==4 || T(i)==-4) { rs=gs; d=dnew(); }
    else goto cleanup; /* value */
  }
  else if(rs && rs!=gs && rs!=cs) {
    /* d[`a]:1 resolved in parent - apply closure check */
    K *prs=px(rs);
    if(!ik(prs[3])) rs=cs;  /* parent non-closure → local copy */
  }
  if(E(d)) goto cleanup;
  if(s(d)!=0x80 && T(d)>0) { _k(d); d=KERR_TYPE; goto cleanup; }
  if(0x40==s(i)) { i=vlookup(i); if(E(i)) { _k(d); d=i; goto cleanup; } }
  if(0x40==s(y)) { y=vlookup(y); if(E(y)) { _k(d); d=y; goto cleanup; } }
  if(s(y)&&s(y)!=0x80&&s(y)!=0xc3) { _k(d); d=KERR_TYPE; goto cleanup; } /* 0xc4 retired */
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
    if(E(p)) { _k(y); return p; }  /* r already freed by scope_set */
    _k(p);  /* free the stored array ref - we return the assigned value */
    return y;  /* return the assigned value, not the array */
  }

cleanup:
  _k(i); _k(y);
  return d;
}

/* resolve predefined.
   Issue #2 Pass 6: post-pass-6 names never carry adverb suffixes, so
   the av-split path is gone -- look up the name verbatim, copy the
   lambda, and stamp valence for monad-promoted entries.  The legacy
   pf[3] av slot has been removed; pf[3] now holds valence. */
static K rpd(K x) {
  K f,*pf;
  if(!(f=dget(C,sk(x)))) return KERR_VALUE;
  K f2=kcp(f); _k(f); if(E(f2)) return f2;
  f=f2;
  pf=px(f);
  if(0xc9==s(x)) pf[3]=FN_VF(1,0);
  return f;
}

static inline K r41(K x);
static K rc5(K x);
static K r44(K x) {
  K r,t;
  char avb[256],*av=avb;

  K *px=px(x);
  K f0=px[0];
  K x1_=px[1];

  /* fast path: f[a;b;...] where f is a simple variable resolving to a
     0xc3 lambda, 0xd9 projection, or 0xda wrapper, and the param list
     is 0x41 or 0x81.  Bypasses avb/strlen/strchr/snprintf and the
     fe() dispatch.  0xc4 retired in Pass 4. */
  if(0x40==s(f0) && 4==T(f0) && (0x41==s(x1_) || 0x81==s(x1_))) {
    char *skf=sk(f0);
    if(skf[0]) {
      K f=scope_get(cs,f0);
      if(f && !E(f) && (0xc3==s(f) || 0xd9==s(f) || 0xda==s(f))) {
        K x2;
        if(0x41==s(x1_)) {
          x2=r41(k_(x1_));
          if(E(x2)||EXIT) { _k(f); _k(x); return x2; }
        }
        else x2=k_(x1_);
        /* fast lambda apply: skip fne/fne_/pgreduce wrappers when:
           f is 0xc3, no adverbs, valence matches, no inulls,
           and scope dict isn't closure-pending. fne_fast gate
           is intentionally 0xc3-only -- 0xda (post-Pass-2b
           producer flip) and 0xd9 (post-Pass-3b-5) take the
           fapply slow path. */
        K *pf=px(f);
        u64 nx2=n(x2);
        if(0xc3==s(f) && 0x81==s(x2)) {
          u64 nval=(u64)FN_VALENCE(pf[3]);
          if(nx2==nval && 6!=T(pf[2])) {
            K *pxk=px(x2);
            int has_inull=0;
            for(u64 i=0;i<nx2;++i) if(pxk[i]==inull) { has_inull=1; break; }
            if(!has_inull) {
              r=fne_fast(f,x2);
              _k(x);
              return r;
            }
          }
        }
        r=fapply(f,x2,0);
        _k(x);
        return r;
      }
      if(f && !E(f)) _k(f);
    }
  }

  *av=0;
  K f=k_(f0);
  K x1=k_(x1_);
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

  /* Issue #2 Pass 6: names never carry adverbs.  Just resolve. */
  if(0x40==s(f)) {
    f=vlookup(f);
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
  else if(0xd1==s(px[1]) || 0xd2==s(px[1]) || 0xd3==s(px[1]) || 0xd8==s(px[1])) {
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

/* File-verb postfix-adverb application (0xcc monad `5:x`, 0xcd dyad
   `a 0:x`), kept out of pgreduce_ so its cases stay tiny.  A postfix adverb
   arrives as a 0x45(av,args) node (de-glued); strip the adverb, run the same
   arg conversion the inline non-adverb path uses, then dispatch via avdo
   (empty av -> builtin).  v is the (type-4) file verb, borrowed; a/b consumed. */
static K fileverb_mon(K v, K a) {
  char av[40]; av[0]=0;
  if(0x45==s(a)) {
    K *pav=px(a); K aav=pav[1];
    if(T(aav)==-3 && (u64)n(aav)<sizeof(av)) { memcpy(av,px(aav),n(aav)); av[n(aav)]=0; }
    K inner=k_(pav[2]); _k(a); a=inner;
  }
  if(s(a)) { a=reduce(a); if(E(a)||EXIT) return a; }
  if(s(a)==0x41) {
    a=r41(a); if(E(a)||EXIT) return a;
    if(n(a)==1) { K *pa=px(a); K q=pa[0]; pa[0]=0; _k(a); a=q; }
    else if(n(a)==0) { _k(a); a=null; }
  }
  else if(0x44==s(a)) { a=r44(a); if(E(a)||EXIT) return a; }
  else if(!VST(a)) { _k(a); return KERR_TYPE; }
  return *av ? avdo(k_(v),0,a,av) : builtin(k_(v),0,a);
}
static K fileverb_dyad(K v, K a, K b) {
  char av[40]; av[0]=0;
  if(0x45==s(b)) {
    K *pav=px(b); K bav=pav[1];
    if(T(bav)==-3 && (u64)n(bav)<sizeof(av)) { memcpy(av,px(bav),n(bav)); av[n(bav)]=0; }
    K inner=k_(pav[2]); _k(b); b=inner;
  }
  if(!a||!b) { _k(a); _k(b); return KERR_TYPE; }
  if(0x41==s(b)) { _k(a); _k(b); return KERR_TYPE; }
  if(s(b)) { b=reduce(b); if(E(b)||EXIT) { _k(a); return b; } }
  if(s(a)) { a=reduce(a); if(E(a)||EXIT) { _k(b); return a; } }
  if(!VST(b)) { _k(a); _k(b); return KERR_TYPE; }
  return *av ? avdo(k_(v),a,b,av) : builtin(k_(v),a,b);
}

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
  /* Value-stack depth is bounded by the token count: every branch below does
     at most one net push (*pA++) per token, so depth <= nx.  Allocate nx + a
     small pad (was nx*10, a 10x over-provision).  Statements with <= ~240
     tokens stay in the static A0[d] buffer (256 slots); only larger ones
     malloc -- so this also cuts the malloc rate for mid-size statements. */
  if(nx+16>256) {
    ma=nx+16;
    pA=A2=xmalloc(sizeof(K)*ma);
    A=A2;
  }
  for(i=0;i<nx;i++) {
    *quiet=0; c=0; q=0;
    u64 si=s(px[i]);                 /* compute subtype once per token */
    if(0xc0==si) { v=px[i]; c=ck(px[i]); q=c>>5; }
    switch(q) {
    case 0:
      *pA++=v=px[i];
      switch(si) {                   /* == s(v); v==px[i] in case 0 */
      case 0xc3: case 0: case 0x40: case 0x41: case 0x81: k_(v); continue;
      case 0xc7: /* builtin dyad */
        if(pA<=A+1) { k_(v); continue; }
        /* Issue #2 Pass 6: lexer no longer slurps adverbs onto names.
           If a postfix adverb token follows, defer to case 7 so the
           juxt step wraps draw + 0x85 into 0xda(draw, av). */
        if(i<nx-1 && 0x85==s(px[i+1])) { k_(v); break; }
        --pA;
        b=*--pA;
        if(0x40==s(b)&&i+1<nx&&64==ik(px[i+1])&&pA==A) { /* f:draw */
          ++i;
          p=scope_set(cs,b,k_(v));  /* add ref since v is borrowed from parse tree */
          if(E(p)) { *pA++=p; }
          else { *pA++=p; *quiet=1; }
          break;
        }
        /* Issue #2 Pass 6: builtin with postfix adverb produces 0x45
           as `b` (e.g. `5 draw/ 10 10 10` -> stack=[5,0x45('/, vec)]).
           `v` is the bare verb (lexer no longer glues adverbs onto
           names), so call avdo(v, a, x, av) directly with the postfix
           adverb string.  If the adverb string is empty, fall back to
           plain builtin() (avdo must not be called with empty av). */
        if(0x45==s(b)) {
          K *pb=px(b);
          K bav=pb[1]; K bv=pb[2];
          char *bavp=(T(bav)==-3)?(char*)px(bav):"";
          if(s(bv)) { bv=reduce(k_(bv)); if(E(bv)||EXIT) { _k(b); *pA++=bv; break; } }
          else bv=k_(bv);
          /* Issue #2 Pass 6: don't consume a left value from the stack
             if the next token is a dyadic operator AND the stack only
             has one value -- that value belongs to the next op, not
             to us (e.g. `A - mul/B`: stack=[A] when mul runs, A is
             the left arg of `-`, not mul).  When two or more values
             are on the stack, pop the top one (it's our left arg)
             and leave the rest for outer ops (e.g. `1 + 2 sv' +|tt`:
             stack=[1,2] when sv runs, 2 is sv's left arg, 1 is +'s).
             Always pop for JUXT, ASSIGN, monadic, or end-of-stream. */
          int next_is_dyad_op = (i<nx-1 && 0xc0==s(px[i+1])
                                 && ik(px[i+1])>=64 && ik(px[i+1])<96
                                 && ik(px[i+1])!=0xff
                                 && ik(px[i+1])!=64
                                 && (pA-A)<=1);
          if(0x81==s(bv) && n(bv)==2) {
            K *pbv=px(bv);
            *pA++ = *bavp ? avdo(k_(v),k_(pbv[0]),k_(pbv[1]),bavp)
                          : builtin(k_(v),k_(pbv[0]),k_(pbv[1]));
            _k(bv); _k(b);
          }
          else if(0x81==s(bv)) { _k(bv); _k(b); *pA++=KERR_VALENCE; }
          else if(pA>A && !next_is_dyad_op) {
            a=*--pA;
            if(s(a)) { a=reduce(a); if(E(a)||EXIT) { _k(bv); _k(b); *pA++=a; break; } }
            if(!VST(a)||!VST(bv)) { _k(a); _k(bv); _k(b); *pA++=KERR_TYPE; break; }
            *pA++ = *bavp ? avdo(k_(v),a,bv,bavp) : builtin(k_(v),a,bv);
            _k(b);
          }
          else {
            if(!VST(bv)) { _k(bv); _k(b); *pA++=KERR_TYPE; break; }
            *pA++ = *bavp ? avdo(k_(v),0,bv,bavp) : builtin(k_(v),0,bv);
            _k(b);
          }
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
            else {
              p=scope_set(cs,a,t);
              if(E(p)) { *pA++=p; }  /* t already freed by scope_set */
              else { *pA++=p; *quiet=1; }
            }
            break;
          }
          if(s(a)) { a=reduce(a); if(E(a)||EXIT) { _k(v); _k(b); *pA++=a; break; } }
          if(!VST(a)||!VST(b)) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
          else *pA++=builtin(v,a,b);
        }
        else {
          if(!VST(b)) { _k(b); *pA++=KERR_TYPE; break; }
          t=builtin(v,0,b);
          *pA++=t;
        }
        break;
      case 0xc6: /* builtin monad */
        if(pA<=A+1) { k_(v); break; } // no argument available
        if(pA<=A+2&&i<nx-1&&ik(px[i+1])==0xff) { k_(v); break; } // {x} abs 2
        /* (the old `strpbrk(sk(v),"/\\")` guard for glued `5 sin/2` is gone:
           builtins are de-glued and now int-index, so the name -- and any
           glued adverb -- no longer exists to test) */
        /* Issue #2 Pass 6: lexer no longer slurps adverbs onto names,
           so a postfix adverb token may follow.  Defer sin to case 7
           (juxtaposition) where sin + 0x85 becomes 0xda(sin,av). */
        if(i<nx-1 && 0x85==s(px[i+1])) { k_(v); break; }
        --pA;
        a=*--pA;
        if(0x40==s(a)&&i+1<nx&&64==ik(px[i+1])&&pA==A) { /* f:+ */
          ++i;
          p=scope_set(cs,a,k_(v));  /* add ref since v is borrowed from parse tree */
          if(E(p)) { _k(a); *pA++=p; }
          else { *pA++=p; *quiet=1; }
          break;
        }
        /* Issue #2 Pass 6: monadic builtin with postfix adverb produces
           0x45 as `a` (e.g. `abs'' (-2 -3;-4 -5.0)` -> stack=[0x45('',
           plist)]).  `v` is the bare verb (lexer no longer glues
           adverbs onto names), so call avdo(v, 0, x, av) directly with
           the postfix adverb string.  If the adverb string is empty,
           fall back to plain builtin() (avdo must not be called with
           empty av). */
        if(0x45==s(a)) {
          K *pav=px(a);
          K aav=pav[1]; K av_v=pav[2];
          char *aavp=(T(aav)==-3)?(char*)px(aav):"";
          if(s(av_v)) { av_v=reduce(k_(av_v)); if(E(av_v)||EXIT) { _k(a); *pA++=av_v; break; } }
          else av_v=k_(av_v);
          /* Issue #2 Pass 6: dyadic-juxt detection -- `5 sin/ 2` means
             "do sin 5 times starting from 2" (overmonadn) when next
             token is JUXT and a left seed is on stack.  Mirrors the
             dispatch in case 0xda (around line 1770-1786). */
          if(pA>A && i<nx-1 && 0xc0==s(px[i+1]) && ik(px[i+1])==0xff
             && (!strcmp(aavp,"/")||!strcmp(aavp,"\\"))) {
            ++i;
            K t0=*--pA;
            if(s(t0)) { t0=reduce(t0); if(E(t0)||EXIT) { _k(a); _k(av_v); *pA++=t0; break; } }
            if(!VST(t0)) { _k(a); _k(av_v); _k(t0); *pA++=KERR_TYPE; break; }
            K f=k_(v); /* bare sin (no av) for over/scan */
            if(s(t0)==0 && (T(t0)==1||T(t0)==8) && !strcmp(aavp,"/")) { *pA++=overmonadn(f,t0,av_v,""); *quiet=0; }
            else if(ISF(t0) && ik(val(t0))==1 && !strcmp(aavp,"/")) { *pA++=overmonadb(f,t0,av_v,""); *quiet=0; }
            else if(s(t0)==0 && (T(t0)==1||T(t0)==8) && !strcmp(aavp,"\\")) { *pA++=scanmonadn(f,t0,av_v,""); *quiet=0; }
            else if(ISF(t0) && ik(val(t0))==1 && !strcmp(aavp,"\\")) { *pA++=scanmonadb(f,t0,av_v,""); *quiet=0; }
            else { _k(f); _k(t0); _k(av_v); *pA++=KERR_TYPE; }
            _k(a);
            break;
          }
          if(0x81==s(av_v) && n(av_v)==1) {
            K *pavv=px(av_v);
            *pA++ = *aavp ? avdo(k_(v),0,k_(pavv[0]),aavp) : builtin(k_(v),0,k_(pavv[0]));
            _k(av_v); _k(a);
          }
          else if(0x81==s(av_v)) {
            /* multi-arg list to monadic builtin -- treat as the single
               argument (e.g. abs'' on (-2 -3;-4 -5.0) gets the whole
               packed list). */
            *pA++ = *aavp ? avdo(k_(v),0,av_v,aavp) : builtin(k_(v),0,av_v);
            _k(a);
          }
          else {
            if(!VST(av_v)) { _k(av_v); _k(a); *pA++=KERR_TYPE; break; }
            *pA++ = *aavp ? avdo(k_(v),0,av_v,aavp) : builtin(k_(v),0,av_v);
            _k(a);
          }
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
        if(0x40==s(a)&&i+1<nx&&64==ik(px[i+1])&&pA==A) { /* f:0: */
          ++i;
          p=scope_set(cs,a,k_(v));  /* add ref since v is borrowed from parse tree */
          if(E(p)) { _k(a); *pA++=p; }
          else { *pA++=p; *quiet=1; }
          break;
        }
        if(0x45==s(a)) { *pA++=fileverb_mon(v,a); break; } /* de-glued postfix adverb */
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
        if(0x45==s(b)) { *pA++=fileverb_dyad(v,a,b); break; } /* de-glued postfix adverb */
        if(!a||!b) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
        if(0x41==s(b)) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
        if(s(b)) { b=reduce(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
        if(s(a)) { a=reduce(a); if(E(a)||EXIT) { _k(b); *pA++=a; break; } }
        if(!VST(b)) { _k(a); _k(b); *pA++=KERR_TYPE; }
        else *pA++=builtin(v,a,b);
        break;
      case 0x82: /* a::1 or d[`a]::1 - resolve then modify */
        if(pA<=A+2) { k_(v); break; }
        --pA;
        b=*--pA;
        a=*--pA;
        if(!a||!b) { _k(a); _k(b); *pA++=KERR_TYPE; break; } /* backstop: never store/amend a NULL operand (mirrors the 0x81 guard above). Known producers are fixed at the push site, but the operand-stack-never-NULL invariant has several entry points. */
        if(s(b)) { b=reduce(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
        if(0x40==s(a)) { /* a::1 */
          if(!VST(b)) { _k(b); *pA++=KERR_PARSE; break; }
          K rs; K a_=vlookuprs(a,&rs);
          if(KERR_VALUE==a_) rs=gs;  /* not found → global */
          else if(E(a_)) { _k(b); *pA++=a_; break; }
          else {
            _k(a_);
            if(rs!=gs && rs!=cs) {
              K *prs=px(rs);
              if(!ik(prs[3])) rs=cs;  /* parent non-closure → local copy */
            }
          }
          p=scope_set(rs,a,b);
          if(E(p)) { _k(a); *pA++=p; }
          else { *pA++=p; *quiet=1; }
        }
        else if(0x44==s(a)) { /* d[`a]::1 */
          if(!VST(b)) { _k(a); _k(b); *pA++=KERR_PARSE; break; }
          pa=px(a);
          K a_=k_(pa[0]); K i_=k_(pa[1]); _k(a);
          /* resolve index */
          if(0x41==s(i_)) {
            if(n(i_)) {
              i_=r41(i_); if(E(i_)||EXIT) { _k(a_); _k(b); *pA++=i_; break; }
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
          /* resolve base, apply closure check */
          K rs; if(KERR_VALUE==(a_=vlookuprs(a_,&rs))) { a_=null; rs=gs; }
          if(E(a_)) { _k(b); _k(i_); *pA++=a_; break; }
          if(rs!=gs && rs!=cs) {
            K *prs=px(rs);
            if(!ik(prs[3])) rs=cs;  /* parent non-closure → local copy */
          }
          /* amend dict and save */
          K r=kamend4(a_,i_,0,k_(b));
          if(E(r)) { _k(b); *pA++=r; }
          else {
            p=scope_set(rs,pa[0],r);
            if(E(p)) { _k(b); *pA++=p; }
            else { _k(p); *pA++=b; *quiet=1; }
          }
        }
        else { _k(a); _k(b); *pA++=KERR_VALUE; break; }
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
        p=scope_set(cs,a,t);
        if(E(p)) { *pA++=p; }  /* t already freed by scope_set */
        else { *pA++=p; *quiet=1; }
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
          // resolve, then apply closure check
          K rs; if(KERR_VALUE==(a_=vlookuprs(a,&rs))) { a_=null; rs=gs; }
          if(E(a_)) { _k(b); *pA++=a_; break; }
          if(rs!=gs && rs!=cs) {
            K *prs=px(rs);
            if(!ik(prs[3])) rs=cs;  /* parent non-closure → local copy */
          }
          t=k(strchr(P,ik(v))-P,a_,b);
          if(E(t)||EXIT) { *pA++=t; break; };
          p=scope_set(rs,a,t);
          if(E(p)) { *pA++=p; }  /* t already freed by scope_set */
          else { *pA++=p; *quiet=1; }
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
          // resolve, then apply closure check
          K rs; if(KERR_VALUE==(a_=vlookuprs(a_,&rs))) { a_=null; rs=gs; }
          if(E(a_)) { _k(b); _k(i_); *pA++=a_; break; }
          if(rs!=gs && rs!=cs) {
            K *prs=px(rs);
            if(!ik(prs[3])) rs=cs;  /* parent non-closure → local copy */
          }
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
          else {
            p=scope_set(rs,pa[0],r);
            if(E(p)) { _k(t); *pA++=p; }  /* r already freed by scope_set */
            else { _k(p); *pA++=t; *quiet=1; }  /* return the computed value t, not the dict */
          }
        }
        else { _k(a); _k(b); *pA++=KERR_TYPE; break; }
        break;
      case 0xda: { /* (f;av) modified-verb wrapper. Inner f's encoding
                     records monad (code in [32,63]) vs dyad ([64,95])
                     intent, replacing the old 0xc1 / 0xc2 cases.
                     0xc7 inner = builtin-dyad-as-modified-monad
                     (e.g. draw/), replacing the old 0xdb case. */
        K *pw=px(v); K wf=pw[0];
        /* Pass 2b-step-4 / 3b-5 / Pass 4: 0xc3/0xd9 inner -- treat
           like a bare push: keep on stack and wait for more tokens
           (the 0xff dyad-juxt marker or args). Eager fe() here
           would skip dyadic-juxt detection in case 7 / line 1812.
           0xc4 retired in Pass 4. */
        if(0xc3==s(wf) || 0xd9==s(wf)) { k_(v); continue; }
        int dyad=0;
        if(0xc0==s(wf)) { u32 c=ck(wf); dyad=(c>=64); }
        /* 0xc7 inner is always monadic in Pass 1b (bc only wraps when
           a->a[0]==NULL); leave dyad=0. */
        if(dyad) {
          if(pA<=A+2) { k_(v); *pA++=KERR_PARSE; break; }
          --pA;
          b=*--pA;
          a=*--pA;
          if(!a||!b) { _k(a); _k(b); *pA++=KERR_PARSE; break; }
          if(s(b)) { b=reduce(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
          if(s(a)) { a=reduce(a); if(E(a)||EXIT) { _k(b); *pA++=a; break; } }
          *pA++=fe(k_(v),a,b,"");
        }
        else {
          if(pA<=A+1) { k_(v); break; }
          --pA;
          b=*--pA;
          if(0x42==s(b)) { b=r42(b); if(E(b)||EXIT) { *pA++=b; break; } }
          /* The b:+' assignment lookahead from the old 0xc1 case --
             if next token is the dyad-juxt marker (0xc0 ik 64) and
             stack has only this name on it, we're assigning. */
          if(0x40==s(b)&&i+1<nx&&64==ik(px[i+1])) {
            if(pA<=A) {
              ++i;
              p=scope_set(cs,b,k_(v));
              if(E(p)) *pA++=p;
              else { *pA++=p; *quiet=1; }
            }
            else {
              if(s(b)) { b=reduce(b); if(E(b)||EXIT) { *pA++=b; break; } }
              t=fe(k_(v),0,b,"");
              *pA++=t;
            }
          }
          else {
            if(s(b)) { b=reduce(b); if(E(b)||EXIT) { *pA++=b; break; } }
            *pA++=fe(k_(v),0,b,"");
          }
        }
        break;
      }
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
            p=scope_set(cs,b,f);
            if(E(p)) { *pA++=p; }  /* f already freed by scope_set */
            else { *pA++=p; *quiet=1; }
            break;
          }
          /* Issue #2 Pass 6: predefined function with postfix adverb
             produces 0x45 as `b` (e.g. `q dv\ 0` -> stack=[q,0x45('\\,
             0)]).  Extract av and value, then call fne with av.  fne
             handles av propagation into pf[3]/avdo. */
          if(0x45==s(b)) {
            K *pb45=px(b);
            K bav=pb45[1]; K bv=pb45[2];
            char *bavp=(T(bav)==-3)?(char*)px(bav):"";
            if(s(bv)) { bv=reduce(k_(bv)); if(E(bv)||EXIT) { _k(f); _k(b); *pA++=bv; break; } }
            else bv=k_(bv);
            if(0x81==s(bv) && n(bv)==2) {
              *pA++=fne(f,bv,bavp); _k(b);
            }
            else if(0x81==s(bv)) {
              valence=val(f);
              if((u32)valence==(u32)n(bv)) { *pA++=fne(f,bv,bavp); _k(b); }
              else { _k(bv); _k(b); _k(f); *pA++=KERR_VALENCE; }
            }
            else if(pA>A) {
              a=*--pA;
              if(s(a)) { a=reduce(a); if(E(a)||EXIT) { _k(f); _k(bv); _k(b); *pA++=a; break; } }
              if(!VST(a)||!VST(bv)) { _k(a); _k(bv); _k(b); _k(f); *pA++=KERR_TYPE; break; }
              xx=params[paramsi++]; pxk=px(xx); pxk[0]=a; pxk[1]=bv; n(xx)=2;
              *pA++=fne(f,k_(xx),bavp);
              _k(a); _k(bv); _k(b); --paramsi;
            }
            else {
              if(!VST(bv)) { _k(bv); _k(b); _k(f); *pA++=KERR_TYPE; break; }
              xx=params[paramsi++]; pxk=px(xx); pxk[0]=bv; n(xx)=1;
              *pA++=fne(f,k_(xx),bavp);
              _k(bv); _k(b); --paramsi;
            }
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
            if(0xc3==s(p)) *pA++=avdo(p,0,k_(b2),(char*)px(b1)); /* 0xc4 retired */
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
            if(0x85==s(b)) { _k(f); _k(b); *pA++=KERR_TYPE; break; }
            xx=params[paramsi++]; pxk=px(xx); pxk[0]=b; n(xx)=1;
            *pA++=fne(f,k_(xx),0);
            _k(b); --paramsi;
          }
          else if(valence==2) {
            if(0==pA-A) {
              if(0x85==s(b)) { _k(f); _k(b); *pA++=KERR_TYPE; break; }
              xx=params[paramsi++]; pxk=px(xx); pxk[0]=b; n(xx)=1;
              *pA++=fne(f,k_(xx),0);
              _k(b); --paramsi;
            }
            else {
              a=*--pA;
              if(0x40==s(a)&&i+1<nx&&64==ik(px[i+1])&&pA==A) { /* f:lin 1 2 3 */
                ++i;
                if(0x85==s(b)) { _k(f); _k(b); *pA++=KERR_TYPE; break; }
                xx=params[paramsi++]; pxk=px(xx); pxk[0]=b; n(xx)=1;
                t=fne(f,k_(xx),0);
                _k(b); --paramsi;
                if(E(t)||EXIT) *pA++=t;
                else {
                  p=scope_set(cs,a,t);
                  if(E(p)) { *pA++=p; }  /* t already freed by scope_set */
                  else { *pA++=p; *quiet=1; }
                }
                break;
              }
              if(s(a)) { a=reduce(a); if(E(a)||EXIT) { _k(f); _k(b); *pA++=a; break; } }
              if(!VST(b)) { _k(f); _k(a); _k(b); *pA++=KERR_TYPE; break; }
              if(!VST(a)) { _k(f); _k(a); _k(b); *pA++=KERR_TYPE; break; }
              if(0x85==s(a)||0x85==s(b)) { _k(f); _k(a); _k(b); *pA++=KERR_TYPE; break; }
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
      /* 0xdb retired in Pass 1b -- replaced by 0xda(0xc7,"...") */
      case 0xb0: { /* APPLY_N: f + N args are on the eval stack (from inline
                      RPN). The big win vs the old 0x44 path is that args
                      were evaluated in this same pgreduce_ frame -- no
                      recursive pgreduce_ entry per param, no
                      rl()/r41() unreduced-plist pass. To avoid making the
                      common-case slower than the old path, we use the
                      pre-allocated params[] pool (so no per-call tuple
                      allocation) and inline the f=0xc3-lambda fast path
                      (so no r44/fe/fne_/pgreduce wrapper overhead). */
        int N=ik(v);
        --pA; /* drop the just-pushed APPLY token (transient) */
        if(pA-A < N+1) { *pA++=KERR_PARSE; break; }
        pA -= N+1;
        K f_raw=pA[0];

        /* Borrow a pre-allocated 0x81 plist to hold reduced args.
           IMPORTANT: bump paramsi BEFORE entering the reduce loop --
           reduce(a) below can recursively trigger another APPLY_N
           that would re-borrow the same params[paramsi] slot and
           clobber our pxk entries (fuzz-found by afl).  Bumping
           paramsi first reserves our slot for the duration of the
           arg-reduce loop. */
        if(paramsi >= PARAMSMAX || N > 32) goto apply_n_fallback;
        K xx=params[paramsi]; K *pxk=px(xx);
        ++paramsi;
        K err=0;
        for(int j=0;j<N;j++) {
          K a=pA[1+j];
          if(s(a)) {
            a=reduce(a);
            if(a && a<EMAX) a=kerror(E[a]);
          }
          /* Args may already be bare error K's pushed by an earlier
             token in this same frame (e.g. KERR_TYPE=3 from `2+\``).
             Errors have s==0, so the s(a) block above misses them.
             Detect unconditionally so the call propagates the error
             rather than feeding the lambda a malformed arg list. */
          if(a && E(a)) {
            err=a;
            for(int k=j+1;k<N;k++) _k(pA[1+k]);
            for(int k=0;k<j;k++) _k(pxk[k]);
            break;
          }
          pxk[j]=a;
        }
        if(err) { --paramsi; _k(f_raw); *pA++=err; break; }
        n(xx)=N;

        /* Resolve f. If f is a 0x40 var that resolves to a 0xc3 lambda
           with matching valence and no special-case markers, dispatch
           via fne_fast directly. Else release params[] borrow and fall
           through to a regular r44 with a fresh 0x81. */
        K f=0;
        if(0x40==s(f_raw) && 4==T(f_raw)) {
          char *skf=sk(f_raw);
          if(skf[0]) {
            f=scope_get(cs,f_raw);
            if(f && E(f)) { for(int j=0;j<N;j++) _k(pxk[j]); n(xx)=0; --paramsi; *pA++=f; _k(f_raw); break; } /* free reduced args before bailing on an undefined head */
          }
        }
        else if(0xc3==s(f_raw) || 0xd9==s(f_raw) || 0xda==s(f_raw)) {
          /* 0xc4 retired in Pass 4 */
          f=k_(f_raw);
        }
        if(f && (0xc3==s(f) || 0xd9==s(f) || 0xda==s(f))) {
          if(0xc3==s(f)) {
            K *pf=px(f);
            if(6!=T(pf[2]) && (u64)FN_VALENCE(pf[3])==(u64)N) {
              int has_inull=0;
              for(int j=0;j<N;j++) if(pxk[j]==inull) { has_inull=1; break; }
              if(!has_inull) {
                K args_borrow=k_(xx);
                K r=fne_fast(f,args_borrow);
                _k(f_raw);
                /* args_borrow refcount goes from 2 to 1 inside fne_fast
                   (it _k's x but skips _k(pd[1]) since ps[5]=1). xx
                   stays alive in the pool, but its entries still hold
                   our +1 refs from reduce() -- release them now or
                   they leak. Inline the heap-K branch of _k to skip
                   the call overhead for atom args (T>0,T!=2). */
                for(int j=0;j<N;j++) {
                  K a=pxk[j];
                  if(!E(a) && (T(a)<=0 || T(a)==2)) {
                    ko *kk=(ko*)(b(48)&a);
                    if(kk->r>0) --kk->r;
                    else _k(a);
                  }
                }
                n(xx)=0;          /* return params[] entry to pool */
                --paramsi;
                *pA++=r;
                break;
              }
            }
          }
          /* projection / 0xda wrapped / non-fne_fast lambda: build a
             fresh 0x81 from the borrowed buffer (so r44/fne can take
             ownership) and dispatch through fapply (handles 0xda peel). */
          K xs=tn(0,N); K *pxs=px(xs);
          for(int j=0;j<N;j++) pxs[j]=k_(pxk[j]);
          K args=st(0x81,xs);
          for(int j=0;j<N;j++) _k(pxk[j]);
          n(xx)=0;
          --paramsi;
          K r=fapply(f,args,0);
          _k(f_raw);
          *pA++=r;
          break;
        }

        /* Generic fallback: build a 0x44 and let r44 dispatch. */
        if(f) _k(f);
        K xs=tn(0,N); K *pxs=px(xs);
        for(int j=0;j<N;j++) pxs[j]=k_(pxk[j]);
        K args_plist=st(0x81,xs);
        for(int j=0;j<N;j++) _k(pxk[j]);
        n(xx)=0;
        --paramsi;
        {
          K w=tn(0,2); K *pw=px(w);
          pw[0]=f_raw;
          pw[1]=args_plist;
          *pA++=r44(st(0x44,w));
        }
        break;

apply_n_fallback: {
        /* Old path: build the 0x44 from heap buffers. */
        K xs=tn(0,N); K *pxs=px(xs);
        K err2=0;
        for(int j=0;j<N;j++) {
          K a=pA[1+j];
          if(s(a)) a=reduce(a);
          if(a && E(a)) {  /* errors have s==0; check unconditionally */
            err2=a;
            for(int k=j+1;k<N;k++) _k(pA[1+k]);
            n(xs)=j;
            _k(xs);
            _k(f_raw);
            break;
          }
          pxs[j]=a;
        }
        if(err2) { *pA++=err2; break; }
        K args_plist=st(0x81,xs);
        K w=tn(0,2); K *pw=px(w);
        pw[0]=f_raw;
        pw[1]=args_plist;
        *pA++=r44(st(0x44,w));
        break;
        }
      } break;
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
        p=scope_set(cs,a,k_(v));  /* add ref since v is borrowed from parse tree */
        if(E(p)) { _k(a); *pA++=p; }
        else { *pA++=p; *quiet=1; }
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
        if(0x40==s(a)) { /* a:1 - always local */
          if(s(b)) { b=reduce(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
          if(!VST(b)) { _k(b); *pA++=KERR_PARSE; break; }
          p=scope_set(cs,a,b);
          if(E(p)) { _k(a); *pA++=p; }  /* b already freed by scope_set */
          else { *pA++=p; *quiet=1; }
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
          if((w==11||w==13)&&0x04==T(a)&&!s(a)) { /* `a . 0; `a @ 0 */
            a=vlookup(a);
            if(KERR_VALUE==a) a=null;
            else if(E(a)||EXIT) { _k(b); *pA++=a; break; }
          }
          if(w==11 && (0xcc==s(a)||0xcd==s(a))) {
            if(!s(b) && T(b)==0) b=st(0x81,b);
            *pA++=fe(a,0,b,0);
            break;
          }
          if(!VST(a)||!VST(b)) { _k(b); _k(a); *pA++=KERR_TYPE; break; }
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

      /* Issue #2 Pass 6: postfix adverb wrap. The parser's postfix-
         restructure left-folds adverb chains so a naked 0x85 always
         lands as `b` immediately after its target value. Reduce a if
         needed (0x44/0xd0/name), then wrap into 0xda(a, av) -- or
         extend an existing 0xda's av string -- so subsequent
         juxtaposition steps see a proper adverbed K.  Skip 0xc5
         verb-composition; it has its own internal av slot which the
         case-0xc5 switch branch below extends directly. */
      if(0x85==s(b) && a && 0x85!=s(a) && 0x41!=s(a) && 0x81!=s(a)
         && 0x43!=s(a) && 0x45!=s(a) && 0xc5!=s(a)) {
        char *bav=(T(b)==-3)?(char*)px(b):"";
        if(0x40==s(a)) {
          /* resolve name -- post-pass-6 names never carry adverbs */
          a=vlookup(a);
          if(E(a)) { _k(b); *pA++=a; break; }
        }
        if(0x44==s(a)) { a=r44(a); if(E(a)||EXIT) { _k(b); *pA++=a; break; } }
        if(0xc5==s(a)) { a=rc5(a); if(E(a)||EXIT) { _k(b); *pA++=a; break; } }
        if(0xd0==s(a)) { a=rd0(a); if(E(a)||EXIT) { _k(b); *pA++=a; break; } }
        if(!VST(a)) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
        if(0xda==s(a)) {
          /* extend the existing 0xda's av string in-place */
          K *pa=px(a);
          K oldav=pa[1];
          char *pav=(T(oldav)==-3)?(char*)px(oldav):"";
          int la=strlen(pav), lb=strlen(bav);
          if(la+lb>=256) { _k(a); _k(b); *pA++=KERR_LENGTH; break; }
          char av2[256];
          memcpy(av2,pav,la);
          memcpy(av2+la,bav,lb+1);
          K newav=tnv(3,la+lb,xmemdup(av2,la+lb+1));
          _k(oldav);
          pa[1]=newav;
          _k(b);
          *pA++=a;
        }
        else {
          a=vcap(a); if(E(a)) { _k(b); *pA++=a; break; } /* copy noun operand to avoid a self-referential 0xda cycle */
          int lb=strlen(bav);
          K newav=tnv(3,lb,xmemdup(bav,lb+1));
          K w=tn(0,2); K *pw=px(w);
          pw[0]=a;
          pw[1]=newav;
          _k(b);
          *pA++=st(0xda,w);
        }
        break;
      }

      if(0x41==s(b)||0x81==s(b)) {
        if(0x85==s(a)) { t=tn(0,3); pt=px(t); pt[1]=a; pt[2]=b; *pA++=st(0x45,t); }
        else { K jn=tn(0,2); K *pjn=px(jn); pjn[0]=a; pjn[1]=b; *pA++=st(0x44,jn); }
        break;
      }

      if(s(b)) { b=reduce(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
      /* Issue #2 Pass 6: names never carry adverbs.  Just resolve. */
      if(0x40==s(a)) {
        a=vlookup(a);
        if(E(a)) { _k(b); *pA++=a; break; }
      }
      if(!*av) av=0;
      if(s(a)) {
        if(0x44==s(a)) { a=r44(a); if(E(a)||EXIT) { _k(b); *pA++=a; break; } }
        if(0xd0==s(a)) { a=rd0(a); if(E(a)||EXIT) { _k(b); *pA++=a; break; } }
        if(0xc5==s(a)) { a=rc5(a); if(E(a)||EXIT) { _k(b); *pA++=a; break; } }
        if(!VST(a)) { _k(b); _k(a); *pA++=KERR_TYPE; break; }
      }

      /* Issue #2 Pass 6: 0xda(0xc6,av) -- post-pass-6 the lexer no longer
         slurps adverbs onto sin/abs/etc., so the (sin,"/") pair lands
         here as 0xda after case 7's postfix-wrap (replaces the old
         0xc6==s(b) + stripav path that pulled adverbs out of the name).
         At this point a is the seed (e.g. 5 in `5 sin/2` or `f:5 sin/`)
         and the assign target `f` is one slot below on the stack. */
      if(0xda==s(b)) {
        K *pwb=px(b); K wfb=pwb[0]; K wavb=pwb[1];
        char *pavb_str=(T(wavb)==-3 && n(wavb)>0)?(char*)px(wavb):"";
        if(0xc6==s(wfb) && a
           && (!strcmp(pavb_str,"/")||!strcmp(pavb_str,"\\"))
           && i+1<nx && 0xc0==s(px[i+1]) && ik(px[i+1])==64
           && pA>A) {
          /* f:5 sin/ -- assign 0xd0(seed, 0xda(sin,av)) to name f.
             The 0xd0 second slot holds the adverbed verb; rd0/fe
             understand 0xda(0xc6,av) via the case 0xda blocks. */
          K target=*--pA;
          if(0x40!=s(target)) { _k(target); _k(a); _k(b); *pA++=KERR_TYPE; break; }
          ++i;
          K d0=tn(0,2); K *pd0=px(d0);
          pd0[0]=a; pd0[1]=b;  /* b is the 0xda(sin,av) wrapper; transfer ownership */
          p=scope_set(cs,target,st(0xd0,d0));
          if(E(p)) { *pA++=p; }
          else { *pA++=p; *quiet=1; }
          break;
        }
      }

      /* Issue #2 Pass 3b-5: 0xd9 with av set must follow c3_apply
         dispatch (for n f/x primitive do, etc.). Bare 0xd9 (no av)
         keeps simple fe path below. */
      if(0xd9==s(a) && av && *av) goto c3_apply;
      switch(s(a)) {
      case 0xc3: /* 0xc4 retired in Pass 4 */
c3_apply:
        /* 0xd9 also lands here via `goto c3_apply` from the 0xda
           wrapper case below when an adverb is involved (val-1/2
           projection scan/over with optional left seed). Bare
           0xd9 juxtaposition (no adverb) goes through the simple
           fe-based case below. */
        if(0x81==s(b)) { *pA++=fne(a,b,av); } /* f[x] */
        else if(ISF(b)&&0xc3!=s(b)&&0xd9!=s(b)&&0xda!=s(b)&&(!av||!*av)) {  /* {x} val */
          /* Pass 2b-step-4 / Pass 4: exclude 0xda(c3,...) and 0xd9
             projection from compose -- they belong in the lambda-
             RHS branch below (0xd7 wrap).  0xc4 retired. */
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
          /* Issue #2 Pass 6: combine peeled outer av (from a 0xda peel)
             with the 0x45's own av so f={,x}' followed by ' yields
             "''" (each-of-each), not just "'". */
          char comb[256];
          {
            int la=(av&&*av)?strlen(av):0;
            int lb=strlen(pb1);
            if(la+lb>=256) { _k(b1); _k(a); _k(b); *pA++=KERR_LENGTH; break; }
            if(la) memcpy(comb,av,la);
            memcpy(comb+la,pb1,lb+1);
          }
          char *cav=comb;
          if(0x44==s(b2)) { b2=r44(b2); if(E(b2)||EXIT) { _k(b1); _k(a); _k(b); *pA++=b2; break; } }
          if(0x41==s(b2)) { b2=r41(b2); if(E(b2)||EXIT) { _k(b1); _k(a); _k(b); *pA++=b2; break; } }
          if(0x81==s(b2)) *pA++=fne(a,b2,cav);
          else {
            if(!VST(b2)) { _k(b1); _k(b2); _k(a); _k(b); *pA++=KERR_TYPE; break; }
            /* Issue #2 Pass 6: dyadic-juxt detection.  When the next
               token is JUXT (0xff) and there's a left-seed on the
               stack, treat as dyadic each: 1 g'!5 -> 1 2 3 4 5.
               Mirrors the analogous detection in case 0xda. */
            if(strlen(cav) && pA>A && i<nx-1 && 0xc0==s(px[i+1]) && ik(px[i+1])==0xff) {
              ++i;
              t=*--pA;
              if(s(t)) { t=reduce(t); if(E(t)||EXIT) { _k(a); _k(b); _k(b1); _k(b2); *pA++=t; break; } }
              if(!VST(t)) { _k(a); _k(b); _k(b1); _k(b2); _k(t); *pA++=KERR_TYPE; break; }
              /* Issue #2 Pass 6: primitive do/while.  When the lambda
                 is monadic (val=1) and av is "/" or "\\", `n f/ x`
                 means do-n-times (atom int) or do-while (monadic
                 function).  Mirrors the dispatch in case 0xda below. */
              if(0xc3==s(a) && ik(val(a))==1
                 && (!strcmp(cav,"/")||!strcmp(cav,"\\"))) {
                K f=kcp(a);
                if(E(f)) { _k(a); _k(b); _k(b1); _k(b2); _k(t); *pA++=f; break; }
                if(s(t)==0 && (T(t)==1||T(t)==8) && !strcmp(cav,"/")) { *pA++=overmonadn(f,t,b2,""); *quiet=0; }
                else if(ISF(t) && ik(val(t))==1 && !strcmp(cav,"/")) { *pA++=overmonadb(f,t,b2,""); *quiet=0; }
                else if(s(t)==0 && (T(t)==1||T(t)==8) && !strcmp(cav,"\\")) { *pA++=scanmonadn(f,t,b2,""); *quiet=0; }
                else if(ISF(t) && ik(val(t))==1 && !strcmp(cav,"\\")) { *pA++=scanmonadb(f,t,b2,""); *quiet=0; }
                else { _k(f); _k(t); _k(b2); *pA++=KERR_TYPE; }
                _k(a); _k(b); _k(b1);
                break;
              }
              *pA++=avdo(a,t,b2,cav);
            }
            else if(strlen(cav)) *pA++=avdo(a,0,b2,cav);
            else {
              xx=params[paramsi++]; pxk=px(xx); pxk[0]=b2; n(xx)=1;
              *pA++=fne(a,k_(xx),0);
              _k(b2); --paramsi;
            }
          }
          _k(b1); _k(b);
        }
        else if(0xc3==s(b)||0xda==s(b)) {
          /* Pass 2b-step-4 / Pass 4: 0xda(c3,av) RHS -- read av
             from wrapper instead of inner pf[3]. Used by 0xd7
             (over/scan-with-seed) and similar constructions.
             0xc4 retired. */
          K *pb=px(b);
          K fav=0;
          if(0xc3==s(b)) fav=0;
          else /* 0xda */ fav=pb[1];
          char *favp=-3==T(fav)?(char*)px(fav):"";
          if((!strcmp(favp,"/") || !strcmp(favp,"\\"))) {
            a=vcap(a); if(E(a)) { _k(b); *pA++=a; break; } /* copy noun seed to avoid a self-referential 0xd7 cycle */
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
        else if(0xcc==s(a)||0xcd==s(a)) {
          if(0x44==s(b)) b=r44(b);
          if(E(b)||EXIT) { _k(a); *pA++=b; break; }
          if(0x45==s(b)) {
            pb=px(b);
            mv=px(pb[1]);
            if(!VST(pb[2])) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
            *pA++=fe(a,0,k_(pb[2]),mv);
            _k(b);
          }
          else if(pA>A&&i<nx-1&&0xc0==s(px[i+1])&&ik(px[i+1])==0xff) {  /* dyadic juxtaposition */
            ++i;
            t=*--pA;
            if(s(t)) { t=reduce(t); if(E(t)||EXIT) { _k(a); _k(b); *pA++=t; break; } }
            if(!VST(t)) { _k(a); _k(b); _k(t); *pA++=KERR_TYPE; break; }
            *pA++=fe(a,t,b,av);
          }
          else *pA++=fe(a,0,b,av);
        }
        else {
          if(0x44==s(b)) b=r44(b);
          if(E(b)||EXIT) { _k(a); *pA++=b; break; }
          if(0x43==s(b)) { _k(a); _k(b); *pA++=KERR_PARSE; break; }  /* TODO */
          valence=val(a);
          if(valence==0||valence==1) { /* f x */
            K fav=0;
            /* Issue #2 Pass 6: bare 0xc3 has no av slot; 0xd9 wraps
               leave av (if any) on the outer 0xda wrapper. */
            if(0xc3==s(a) || 0xd9==s(a)) fav=0;
            else { _k(a); _k(b); *pA++=KERR_PARSE; break; } /* 0xc4 retired */
            char *favp=-3==T(fav)?(char*)px(fav):"";
            if(av && *av) favp=av;
            if((!strcmp(favp,"/") || !strcmp(favp,"\\"))
               &&pA>A&&i<nx-1&&0xc0==s(px[i+1])&&ik(px[i+1])==0xff) {  /* dyadic juxtaposition */
              /* primitive do/while */
              ++i;
              t=*--pA;
              if(0x43==s(b)) { *pA++=KERR_PARSE; _k(a); _k(b); break; } /* f[]'[] not handled here; push a real error, never a NULL operand (cf. sibling at ~1837) */
              if(s(t)) { t=reduce(t); if(E(t)||EXIT) { _k(a); _k(b); *pA++=t; break; } }
              if(!VST(t)) { _k(a); _k(b); _k(t); *pA++=KERR_TYPE; break; }
              K f=kcp(a); _k(a);
              if(E(f)) { _k(b); _k(t); *pA++=f; break; }
              if(s(t)==0 && (T(t)==1||T(t)==8) && !strcmp(favp,"/")) { *pA++=overmonadn(f,t,b,""); *quiet=0; }
              else if(ISF(t) && ik(val(t))==1 && !strcmp(favp,"/")) { *pA++=overmonadb(f,t,b,""); *quiet=0; }
              else if(s(t)==0 && (T(t)==1||T(t)==8) && !strcmp(favp,"\\")) { *pA++=scanmonadn(f,t,b,""); *quiet=0; }
              else if(ISF(t) && ik(val(t))==1 && !strcmp(favp,"\\")) { *pA++=scanmonadb(f,t,b,""); *quiet=0; }
              else { _k(f); _k(b); _k(t); *pA++=KERR_TYPE; break; }
            }
            else if(!strcmp(favp,"'")&&0==ik(val(a))) {
              K f=kcp(a);
              if(E(f)) { _k(a); _k(b); *pA++=f; break; }
              char av2[256];
              if(strlen(favp)+strlen(av?av:"")>255) { _k(f); _k(a); _k(b); *pA++=KERR_LENGTH; break; }
              memcpy(av2,favp,strlen(favp));
              memcpy(av2+strlen(favp),av?av:"",strlen(av?av:"")+1);
              *pA++=avdo(f,0,b,av2);
              _k(a);
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
              if(0x43==s(b)) { *pA++=KERR_PARSE; _k(a); _k(b); break; } /* f[]'[] not handled here; push a real error, never a NULL operand (cf. sibling at ~1837) */
              if(s(t)) { t=reduce(t); if(E(t)||EXIT) { _k(a); _k(b); *pA++=t; break; } }
              if(!VST(t)) { _k(a); _k(b); _k(t); *pA++=KERR_TYPE; break; }
              xx=params[paramsi++]; pxk=px(xx); pxk[0]=t; pxk[1]=b; n(xx)=2;
              *pA++=fne(a,k_(xx),av);
              _k(t); _k(b); --paramsi;
            }
            else if(0x43==s(b)) { *pA++=KERR_PARSE; _k(a); _k(b); break; } /* f[]'[] not handled here; push a real error, never a NULL operand (cf. sibling at ~1837) */
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
          else if(0xc3==s(t) || 0xd9==s(t)) { /* 0xc4 retired */
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
        else if(0xc3==s(b)||0xda==s(b)) {
          /* Pass 2b-step-4 / Pass 4: 0xda(c3,av) RHS -- read av
             from wrapper.  0xc4 retired. */
          K *pb=px(b);
          K fav=0;
          if(0xc3==s(b)) fav=0;
          else /* 0xda */ fav=pb[1];
          char *favp=-3==T(fav)?(char*)px(fav):"";
          if((!strcmp(favp,"/") || !strcmp(favp,"\\"))) {
            a=vcap(a); if(E(a)) { _k(b); *pA++=a; break; } /* copy noun seed to avoid a self-referential 0xd7 cycle */
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
      case 0xda: { /* (f;av) modified-verb wrapper, juxtaposition path */
        K *pw=px(a); K wf=pw[0]; K wav=pw[1];
        char *avp=(T(wav)==-3 && n(wav)>0) ? (char*)px(wav) : "";
        /* Issue #2 Pass 2b-step-3 / Pass 4: 0xc3/0xd9 inner -- peel
           and replay through case 0xc3 with combined av. Avoids
           duplicating the ~140-line case 0xc3 body. After Pass
           2b-step-4 flips producers, an adverbed lambda K is
           0xda(c3,av) and lands here instead of case 0xc3.  0xc4
           retired in Pass 4. */
        if(0xc3==s(wf) || 0xd9==s(wf)) {
          /* Pass 3b-5: 0xd9 projection inner -- treat like 0xc3
             for pgreduce_'s c3_apply: val(0xd9) is well-defined,
             and overmonadn/scanmonadn use fe() which routes 0xd9
             through fapply for projection peel. */
          if(0x40==s(b)) { b=r40(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
          int la=strlen(avp);
          int lb=(av && *av)?strlen(av):0;
          if(la+lb>=256) { _k(a); _k(b); *pA++=KERR_LENGTH; break; }
          memcpy(avb,avp,la);
          if(lb) memcpy(avb+la,av,lb);
          avb[la+lb]=0;
          av=avb;
          K wf_owned=k_(wf);
          _k(a);
          a=wf_owned;
          goto c3_apply;
        }
        int vi=-1;
        if(0xc0==s(wf)) { u32 c=ck(wf); vi=c%32; }
        /* Issue #2 Pass 6: 0xc7 builtin-dyad inner (draw/, mul/, etc.) --
           fe()'s case 0xda routes these through avdo for fold/scan
           semantics.  Without this, juxtaposition reports type error
           because the c3_apply branch only handles 0xc0/0xc3/0xd9. */
        if(0xc7==s(wf)) {
          if(0x40==s(b)) { b=r40(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
          if(pA>A && i<nx-1 && 0xc0==s(px[i+1]) && ik(px[i+1])==0xff) {  /* dyadic juxtaposition */
            ++i;
            t=*--pA;
            if(s(t)) { t=reduce(t); if(E(t)||EXIT) { _k(a); _k(b); *pA++=t; break; } }
            if(!VST(t)) { _k(a); _k(b); _k(t); *pA++=KERR_TYPE; break; }
            *pA++=fe(k_(a),t,b,avp);
          }
          else *pA++=fe(k_(a),0,b,avp);
          _k(a);
          break;
        }
        if(vi<0 || vi>20) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
        if(0x40==s(b)) { b=r40(b); if(E(b)||EXIT) { _k(a); *pA++=b; break; } }
        if(*avp) {
          if(0x45==s(b)) { /* adverbs with args, '1 2 */
            if(pA>A&&i<nx-1&&0xc0==s(px[i+1])&&ik(px[i+1])==0xff) {  /* dyadic juxtaposition */
              ++i;
              t=*--pA;
              if(s(t)) { t=reduce(t); if(E(t)||EXIT) { _k(a); _k(b); *pA++=t; break; } }
              if(!VST(t)) { _k(a); _k(b); _k(t); *pA++=KERR_TYPE; break; }
              pb=px(b);
              /* Build a compound av string by concatenating avp + pb[1] */
              int la=strlen(avp); char *pbav=(T(pb[1])==-3)?(char*)px(pb[1]):"";
              int lb=strlen(pbav);
              if(la+lb>=255) { _k(a); _k(b); _k(t); *pA++=KERR_LENGTH; break; }
              char buf[256]; memcpy(buf,avp,la); memcpy(buf+la,pbav,lb); buf[la+lb]=0;
              if(!VST(pb[2])) { _k(a); _k(b); _k(t); *pA++=KERR_TYPE; break; }
              *pA++=avdo(vi,t,k_(pb[2]),buf);
            }
            else {
              pb=px(b);
              int la=strlen(avp); char *pbav=(T(pb[1])==-3)?(char*)px(pb[1]):"";
              int lb=strlen(pbav);
              if(la+lb>=255) { _k(a); _k(b); *pA++=KERR_LENGTH; break; }
              char buf[256]; memcpy(buf,avp,la); memcpy(buf+la,pbav,lb); buf[la+lb]=0;
              if(!VST(pb[2])) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
              *pA++=avdo(vi,0,k_(pb[2]),buf);
            }
          }
          else if(pA>A&&i<nx-1&&0xc0==s(px[i+1])&&ik(px[i+1])==0xff) {  /* dyadic juxtaposition */
            ++i;
            t=*--pA;
            if(s(t)) { t=reduce(t); if(E(t)||EXIT) { _k(a); _k(b); *pA++=t; break; } }
            if(!VST(t)) { _k(a); _k(b); _k(t); *pA++=KERR_TYPE; break; }
            *pA++=avdo(vi,t,k_(b),avp);
          }
          else if(s(b)) *pA++=KERR_TYPE;
          else *pA++=avdo(vi,0,k_(b),avp);
        }
        else { *pA++=KERR_PARSE; }
        _k(a); _k(b);
        break;
      }
      case 0x85: /* adverbs alone */
        if(0x43==s(b)) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
        /* Issue #2 Pass 6: combine nested adverbs.  When b is already
           a 0x45, fold the outer adverb 'a' into b's av string instead
           of nesting 0x45-in-0x45 -- e.g. ' + 0x45('/, args) becomes
           0x45("'/", args), so subsequent juxtaposition with a head
           value sees a flat (av, args) pair. */
        if(0x45==s(b)) {
          K *pb=px(b);
          char *pav_a=(T(a)==-3)?(char*)px(a):"";
          char *pav_b=(T(pb[1])==-3)?(char*)px(pb[1]):"";
          int la=strlen(pav_a), lb=strlen(pav_b);
          if(la+lb>=256) { _k(a); _k(b); *pA++=KERR_LENGTH; break; }
          char comb[256];
          memcpy(comb,pav_a,la);
          memcpy(comb+la,pav_b,lb+1);
          K newav=tnv(3,la+lb,xmemdup(comb,la+lb+1));
          _k(pb[1]);
          pb[1]=newav;
          _k(a);
          *pA++=b;
          break;
        }
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
          memcpy(av2,pav0,strlen(pav0));
          memcpy(av2+strlen(pav0),pav1,strlen(pav1)+1);
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
      case 0xd9: /* Issue #2 Pass 3b-1: simple bare projection juxt
                    -- fe() routes 0xd9 through fapply for inull-fill /
                    nest. Adverbed-projection juxtaposition reaches
                    c3_apply via the 0xda wrapper case below.
                    0xd4 retired in Pass 4. */
        if(0x45==s(b)) {
          pb=px(b);
          mv=px(pb[1]);
          if(!VST(pb[2])) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
          /* Issue #2 Pass 6: dyadic-juxt detection -- `n f/x` and
             `n f\x` where f is a 0xd9 projection.  Routes through
             overmonadn/scanmonadn for primitive do/while; otherwise
             falls back to avdo's monadic dispatch. */
          if(pA>A && i<nx-1 && 0xc0==s(px[i+1]) && ik(px[i+1])==0xff
             && (!strcmp(mv,"/")||!strcmp(mv,"\\"))
             && ik(val(a))==1) {
            ++i;
            t=*--pA;
            if(s(t)) { t=reduce(t); if(E(t)||EXIT) { _k(a); _k(b); *pA++=t; break; } }
            if(!VST(t)) { _k(a); _k(b); _k(t); *pA++=KERR_TYPE; break; }
            if(s(t)==0 && (T(t)==1||T(t)==8) && !strcmp(mv,"/")) { *pA++=overmonadn(a,t,k_(pb[2]),""); *quiet=0; }
            else if(ISF(t) && ik(val(t))==1 && !strcmp(mv,"/")) { *pA++=overmonadb(a,t,k_(pb[2]),""); *quiet=0; }
            else if(s(t)==0 && (T(t)==1||T(t)==8) && !strcmp(mv,"\\")) { *pA++=scanmonadn(a,t,k_(pb[2]),""); *quiet=0; }
            else if(ISF(t) && ik(val(t))==1 && !strcmp(mv,"\\")) { *pA++=scanmonadb(a,t,k_(pb[2]),""); *quiet=0; }
            else { _k(a); _k(t); *pA++=KERR_TYPE; }
            _k(b);
            break;
          }
          *pA++=avdo(a,0,k_(pb[2]),mv);
          _k(b);
        }
        else *pA++=fe(a,0,b,"");
        break;
      /* 0xd5/0xd6 retired in Pass 4 -- replaced by 0xd9 (handled above). */
      case 0xd7: *pA++=fe(a,0,b,""); break;
      case 0xdc: /* 2:-linked C function followed by an adverb (each/over/scan).
                    Mirrors the 0xd9 0x45 branch: pull an optional left arg for
                    dyadic juxtaposition (`x add/y`), reduce the right args, and
                    dispatch through avdo (which treats 0xdc as callable). */
        if(0x45==s(b)) {
          pb=px(b);
          mv=px(pb[1]);
          if(!VST(pb[2])) { _k(a); _k(b); *pA++=KERR_TYPE; break; }
          t=0;
          if(pA>A && i<nx-1 && 0xc0==s(px[i+1]) && ik(px[i+1])==0xff) { /* dyadic juxtaposition */
            ++i;
            t=*--pA;
            if(s(t)) { t=reduce(t); if(E(t)||EXIT) { _k(a); _k(b); *pA++=t; break; } }
            if(!VST(t)) { _k(a); _k(b); _k(t); *pA++=KERR_TYPE; break; }
          }
          if(s(pb[2])) { p=reduce(k_(pb[2])); if(E(p)||EXIT) { _k(a); _k(b); _k(t); *pA++=p; break; } }
          else p=k_(pb[2]);
          *pA++=avdo(a,t,p,mv);
          _k(b);
        }
        else {
          t=0;
          if(pA>A && i<nx-1 && 0xc0==s(px[i+1]) && ik(px[i+1])==0xff) { /* dyadic juxtaposition */
            ++i;
            t=*--pA;
            if(s(t)) { t=reduce(t); if(E(t)||EXIT) { _k(a); _k(b); *pA++=t; break; } }
            if(!VST(t)) { _k(a); _k(b); _k(t); *pA++=KERR_TYPE; break; }
          }
          if(t) *pA++=fe(a,t,b,"");
          else *pA++=k(13,a,b); /* plain juxtaposition -> apply */
        }
        break;
      default: *pA++=k(13,a,b); /* a b */
      } break;
    }
    /* suspend console, invoke repl */
    if(pA>A&&pA[-1]) {
      if(pA[-1]<EMAX) pA[-1]=kerror(E[pA[-1]]);
      if(E(pA[-1])) {
        if(EFLAG&&!EXIT&&!SIGNAL&&strcmp(sk(pA[-1]),"abort")) {
          /* Look ahead: any unexecuted APPLY_N tokens AFTER the
             failed token? Each one represents a call-site frame
             that the OLD 0x44 path would have had its own
             pgreduce_ (and own subrepl) for. APPLY_N's inline
             arg evaluation collapsed those frames into this one;
             we emulate them below so `'` walks levels the same
             way and `:val` continues into the pending call. */
          int apn_pending=0;
          for(u64 k=i+1;k<nx;k++) {
            if(s(px[k])==0xb0) { ++apn_pending; }
          }
          /* Faux `inpl` for the arg-eval phase: the OLD path was
             inside r41()'s inpl=1 region when an arg errored, so
             printerror chose the multi-line `<line0> ... + N`
             format. Match that for APPLY_N's inline-arg eval. */
          int inpl0=inpl;
          if(apn_pending>0) inpl=1;
          printerror(pA[-1],x0,i);
          ++ecount;
          pA[-1]=repl();
          /* Walk up each collapsed call-site frame for SIGNAL (`'`). */
          while(apn_pending>0 && SIGNAL==0 && pA>A && pA[-1]
                && E(pA[-1]) && !strcmp(sk(pA[-1]),"")  /* SIGNAL */
                && EFLAG && !EXIT) {
            --apn_pending;
            /* Once we cross out of the innermost call-site (no more
               APPLY_N pending), we are at the statement's open-code
               level again -- restore inpl for the final caret. */
            if(apn_pending==0) inpl=inpl0;
            printerror(pA[-1],x0,0);
            ++ecount;
            pA[-1]=repl();
          }
          inpl=inpl0;
          /* If the user substituted a value (`:val`) AND there is a
             pending call-site (APPLY_N) ahead, continue the loop
             with `val` in place of the failed token so the call
             proceeds with the substituted arg. Without a pending
             APPLY_N the substitute means "this whole frame returns
             val" (matching the OLD path's per-frame semantics), so
             fall through to break. */
          if(apn_pending>0 && pA>A && pA[-1] && !E(pA[-1])) continue;
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
        r=null;
        continue;
      }
      else if(ck(values[1])==247) { /* \e */
        a=values[0];
        if(ta==1) EFLAG=ik(a);
        else if(ta==6) printf("%d\n",EFLAG);
        else { r=kerror("parse"); continue; }
        r=null;
        continue;
      }
      else if(ck(values[1])==246) { /* \d */
        a=k_(values[0]);
        if(!a) { r=null; continue; }
        r=dir_(a);
        continue;
      }
      else if(ck(values[1])==245) { /* \m i [PORT] */
        a=values[0];
        if(ta==1) {
          K e=ipc_listen(ik(a),IPC_MODE_ITER);
          r = e ? e : null;
        }
        else if(ta==6) { printf("%d\n",ipc_listen_port_for(IPC_MODE_ITER)); r=null; }
        else { r=kerror("parse"); continue; }
        continue;
      }
      else if(ck(values[1])==244) { /* \m f [PORT] */
        a=values[0];
        if(ta==1) {
          K e=ipc_listen(ik(a),IPC_MODE_FORK);
          r = e ? e : null;
        }
        else if(ta==6) { printf("%d\n",ipc_listen_port_for(IPC_MODE_FORK)); r=null; }
        else { r=kerror("parse"); continue; }
        continue;
      }
      else if(ck(values[1])==243) { /* \m (bare): both ports */
        printf("i %d\nf %d\n",
               ipc_listen_port_for(IPC_MODE_ITER),
               ipc_listen_port_for(IPC_MODE_FORK));
        r=null;
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
      if(cs!=gs) break;                 /* local scope: propagate up */
      /* global top-level: a return stops the statement list the same way in
         file load (opencode) and the REPL.  Previously the REPL fell through
         and kept evaluating, so `:2.0;3.0;4.0` yielded 4.0 instead of 2.0 and
         leaked the held return value when the next statement overwrote r.
         Only an embedded eval (!REPL && !opencode, e.g. value"...") still lets
         RETURN propagate to its caller. */
      if(REPL || opencode) break;
    }
  }
  return r;
}

static K listbc(pgs *s, pn *b, int t);

/* Wrap a lex-emitted modified-verb K (subtype 0xc1, payload string like
   "+/'") into the new 0xda 2-tuple representation: pf[0] = primitive verb
   K t(1, st(0xc0, code)), pf[1] = bare adverb string. The dyad/monad
   intent (was 0xc2 vs 0xc1 in the old shape) lives in the inner verb
   K's encoding: code = 32+vi for monad, 64+vi for dyad. Used at every
   parse-/bc-time site that copies an AST verb K into runtime data so
   no live 0xc1/0xc2 K's leak past the parser. Steals nothing -- caller
   still owns mv_k. Returns a fresh K. */
static K wrap_mv_to_da(K mv_k, int dyad) {
  char *mv=(char*)px(mv_k);
  if(!*mv) return KERR_PARSE;
  char *vp=strchr(P,*mv);
  if(!vp || vp-P>20) return KERR_PARSE;
  int vi=vp-P;
  K f=t(1,st(0xc0,(K)((dyad?64:32)+vi)));
  int alen=strlen(mv+1);
  K av=tn(3,alen);
  if(alen) memcpy((char*)px(av),mv+1,alen);
  K w=tn(0,2);
  K *pw=px(w);
  pw[0]=f;
  pw[1]=av;
  return st(0xda,w);
}

/* k_() if the K is anything other than the lex-emitted 0xc1 modified-verb,
   otherwise wrap into 0xda. Used at every bc()/list19() site that copies
   an AST verb K into runtime data (compositions, fixed dyads, the verb
   list of `(+/#)`, etc.). The dyad parameter records call-site intent
   (encoded into the inner 0xc0 verb K); fe() ignores it but pgreduce_'s
   outer 0xda case uses it to decide pop count. For nested storage in
   0xc5/0xd0 the inner verb is always consumed via fe(), so the encoding
   choice doesn't affect dispatch -- pass `dyad` if the surrounding
   construct is dyadic, monad otherwise. */
static K dupwrap_mv(K v, int dyad) {
  if(0xc1==s(v)) return wrap_mv_to_da(v,dyad);
  return k_(v);
}

/* Pass 1b: wrap a parser-emitted 0xc7 builtin-dyad K whose name has a
   trailing adverb suffix (e.g. "draw/", "atan2\\") into the 0xda (f;av)
   shape. This replaces the old 0xc7->0xdb subtype rewrite at parse
   time so the modified-builtin-dyad value composes the same way as
   modified primitive verbs (0xc0+adverbs in 0xda). The inner f is a
   fresh 0xc7 K whose name is the adverb-stripped base; the av string
   carries the trailing adverb chars. Caller still owns c7_k. */
static K wrap_c7_to_da(K c7_k) {
  if(4!=T(c7_k)) return KERR_PARSE; /* int-index builtins carry no name to split (dead post-de-glue) */
  char *full=sk(c7_k);
  size_t fn=strlen(full);
  size_t avstart=fn;
  while(avstart>0 && strchr("'/\\",full[avstart-1])) --avstart;
  if(avstart==0 || avstart==fn) return KERR_PARSE; /* shouldn't happen: hasav was true */
  char base[64];
  if(avstart>=sizeof(base)) return KERR_PARSE;
  memcpy(base,full,avstart); base[avstart]=0;
  K f=t(4,st(0xc7,sp(base)));
  K av=tn(3,fn-avstart);
  if(fn>avstart) memcpy((char*)px(av),full+avstart,fn-avstart);
  K w=tn(0,2); K *pw=px(w);
  pw[0]=f;
  pw[1]=av;
  return st(0xda,w);
}

static int is_headless_chain(pn *n);
K list19(pgs *s, pn *a) {
  K r=0;
  char *t;
  K v=tn(0,2); K *pv=px(v);
  /* A headless bracket chain whose hole was never filled means no head ever
     attached (e.g. `'[3][3]`, `do[" "][1;2]` -- an adverb/control verb can't
     head a bracket chain).  Reject it rather than walk into the NULL hole.
     The explicit a[0]/a[1] checks also keep the static analyzer happy about
     the unconditional derefs below (is_headless_chain already covers a[0]). */
  if(is_headless_chain(a) || !a->a[0] || !a->a[1]) { _k(v); return kerror("parse"); }
  /* A nested t19 apply chain f[i][j]... (each level a single t4 bracket)
     flattens to ONE 0x40 multi-apply so the bracket groups evaluate
     left-to-right -- matching the old t40 path and keeping f[a:1][a+1]==3.
     Plain nested 0x44 evaluates the outer arg first (right-to-left). */
  if(a->a[1] && a->a[1]->t==4 && a->a[0] && a->a[0]->t==19) {
    int depth=0; pn *n=a, *head=0;
    for(;;){
      if(!(n->a[1] && n->a[1]->t==4)) { depth=0; break; }
      depth++;
      if(n->a[0]->t!=19) { head=n->a[0]; break; }
      n=n->a[0];
    }
    if(depth>1 && head && head->t!=7) {
      K w=tn(0,depth); K *pw=px(w);
      n=a; for(int idx=depth-1; idx>=0; idx--){ pw[idx]=listbc(s,n->a[1],0x41); n=n->a[0]; }
      if(head->t==1) pv[0]=dupwrap_mv(head->v,0);
      else if(head->t==19) pv[0]=list19(s,head);
      else pv[0]=k_(head->n);
      pv[1]=st(0x40,w);
      return st(0x44,v);
    }
  }
  if(a->a[1]->t==4) {
    if(a->a[0]->t==7) {

      pn *q=a->a[0];
      K v3=tn(0,q->m); K *pv3=px(v3);
      for(int i=0;i<q->m;++i) {
        if(q->a[i]->t==11) {
          K v2=tn(0,2); K *pv2=px(v2);
          if(q->a[i]->a[0]->t==6) pv2[0]=listbc(s,q->a[i]->a[0],0x42);
          else pv2[0]=k_(q->a[i]->a[0]->n);
          if(s(q->a[i]->v)) {
            /* fixed dyad inside 0xc5 -- verb is in dyadic position */
            pv2[1]=dupwrap_mv(q->a[i]->v,1);
          }
          else {
            t=strchr(P,q->a[i]->v);
            pv2[1]=t(1,st(0xc0,t-P));
          }
          pv3[i]=st(0xd0,v2);
        }
        else if(s(q->a[i]->v)) {
          /* element of 0xc5 verb composition, e.g. +/ in (+/#) */
          pv3[i]=dupwrap_mv(q->a[i]->v,0);
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
    /* f in `f[a;b]` (4-args). Wrap a lex 0xc1 modified verb. */
    else if(a->a[0]->t==1) pv[0]=dupwrap_mv(a->a[0]->v,0);
    else pv[0]=k_(a->a[0]->n);
    pv[1]=listbc(s,a->a[1],0x41);
    r=st(0x44,v);
  }
  else { printf("fatal\n"); exit(1); }
  return r;
}

static int hasav(pn *a) {
  /* Issue #2 Pass 6 cleanup: only 0xda(c3,av) carries an adverb at
     AST-prep time now -- names and lambdas no longer embed adverb
     chars in their text, so the legacy `strchr("'/\\",name[-1])`
     and pf[3] checks are gone. */
  if(a->t==2 && 0xda==s(a->n)) {
    K *pw=px(a->n);
    if(0xc3==s(pw[0])) return 1; /* 0xc4 retired */
  }
  return 0;
}

/* Returns the inline-arg-count if this f[a;b;c]-style call can be emitted as
   inline RPN ending in an APPLY_N token, else returns -1.
   Conditions:
     - a->t == 19 (call site)
     - a->a[1]->t == 4 (plist, not 40 = [][])
     - all sub-stmts of the plist are non-null (no elided slots)
     - a->a[0]->t == 2 (named/literal callable) or a->a[0]->t == 19 (nested
       call which itself is inlinable)
     - arg count fits in the low byte of an APPLY_N token (limit 255)
     - a->lv == 0 (not an assignment LHS — those need to stay 0x44 so the
       :/:: dyad case can amend in place). */
/* A sub-statement is "stack-balanced" iff its emitted RPN tokens push
   exactly one value to the eval stack with no spillover. This is what
   APPLY_N's shared frame requires. The OLD 0x44 path runs each sub-stmt
   in its own pgreduce_ frame so it doesn't care, but we do.

   Conservative rules (errs toward the OLD 0x44 path):
   - t==2 leaf (literal / var-ref): always pushes one value. OK.
   - t==19 call: produces one value if it's itself inlineable, otherwise
     falls back to a single 0x44 token. Either way: net +1.
   - t==1 with a built-in dyad verb subtype (0xc7) AND both kids
     populated AND each kid itself balanced: emits kids' tokens then a
     dyad verb that pops 2 and pushes 1. Net +1, no interaction with
     prior stack. Covers c-2, 1+2, etc. — the L4 case in t/p005.
   - All other t==1 forms are rejected. In particular we exclude
     assignments (':', '::', '+:', etc.), monads, control verbs, and
     anything where the verb has only one operand on the inline RPN
     stream — those forms either pull state from the shared stack or
     have side-effect ordering that the OLD 0x44 path's per-sub-stmt
     pgreduce_ frame protects (see parse.k / t/t571 for the canonical
     case that broke). */
static int is_balanced_substmt(pn *p) {
  if(!p) return 0;
  if(p->t==2) return 1;
  if(p->t==19) return 1;
  if(p->t==1) {
    if(p->m!=2 || !p->a[0] || !p->a[1]) return 0;
    K v = p->v;
    K vsub = s(v);
    /* Two flavours of verb token end up emitted into the RPN stream:
         (a) explicit subtype tokens (0xc7 plain dyad, 0xc2 dyad-with-
             adverb-ready, etc.) — those have non-zero subtype.
         (b) "small char" verbs ('+', '-', '*', '%', ',', '!', '#', etc.)
             stored as the raw byte. bc() emits these as
             t(1, st(0xc0, code)) and pgreduce_'s case 0xc0 dispatches.
             They have s(v)==0 and v is in the printable-ASCII range.

       Both forms pop 1 or 2 and push 1 when fully applied. We accept
       (a)'s 0xc7 (plain builtin dyad) and (b)'s small-char verbs
       here, but exclude assignment and control tokens explicitly.
       Note: modified verbs (was 0xc2) are now wrapped in 0xda in
       bc(), so they never appear here as AST node values either. */
    if(vsub == 0) {
      /* small-char verb. Reject ':' (assignment) outright. The remaining
         small-char dyads/monads are stack-safe when fully applied. */
      if(v == 58) return 0;          /* ':' */
      if(v < 32 || v > 127) return 0;
    }
    else if(vsub != 0xc7) {
      return 0;
    }
    return is_balanced_substmt(p->a[0]) && is_balanced_substmt(p->a[1]);
  }
  return 0;
}

static int can_inline_call_uncached(pn *a) {
  if(!a || a->t!=19) return -1;
  if(a->lv) return -1;
  if(!a->a[1] || a->a[1]->t!=4) return -1;
  pn *plist=a->a[1];
  /* If every arg is a pure literal (t==2 leaf with non-var-ref n), the
     OLD 0x44 path wins: the parser pre-built a single token holding
     the full call shape, so r44 fires off the end-of-stmt branch with
     zero per-arg pgreduce_ iterations. APPLY_N would tokenize each
     arg and walk the main loop N times for the same result. Skip
     inlining in that case so L2 (do[t;f[1;2;3]]) keeps the OLD speed.
     Calls with at least one variable, sub-call, or expression arg
     still benefit from APPLY_N's shared-frame inlining. */
  int all_literal = 1;
  for(int i=0;i<plist->m;i++) {
    pn *p=plist->a[i];
    if(!p) return -1;
    if(p->t==2 && p->n==0) return -1;    /* elided slot (e.g., f[;2]) */
    if(!is_balanced_substmt(p)) return -1;
    if(all_literal) {
      if(p->t!=2) all_literal = 0;
      else if(s(p->n) == 0x40) all_literal = 0;  /* variable reference */
    }
  }
  if(all_literal && plist->m > 0) return -1;
  if(plist->m > 255) return -1;
  pn *fnode=a->a[0];
  if(!fnode) return -1;
  /* t==2 leaf: bc() emits the K stored in fnode->n directly (lambda
     literal, stored 0x40 variable ref, etc.). Always inlinable. */
  if(fnode->t==2) return plist->m;
  /* t==1 verb-position: bc() emits K stored in fnode->v. The inline RPN
     path requires the emitted token to be pushed onto the eval stack
     unchanged; built-in verbs / stdlib dyads / control tokens are
     processed inline by pgreduce_ (case 0xc6/0xc7/0xca/...) and would
     consume operands meant for our APPLY_N. So we only accept the
     stable-pushable subtypes here. */
  if(fnode->t==1) {
    K fv=fnode->v;
    K sub=s(fv);
    if(sub==0x40) return plist->m;       /* variable reference */
    return -1;                           /* builtin verb / control token */
  }
  if(fnode->t==19) {
    if(can_inline_call_uncached(fnode) < 0) return -1;
    return plist->m;
  }
  return -1;
}

/* Caches the result on the pn so bc()'s second pass can re-query after the
   children have been emitted (which clobbers their ->n). */
static int can_inline_call(pn *a) {
  if(!a || a->t!=19) return -1;
  if(a->call_n != -2) return a->call_n;
  a->call_n = can_inline_call_uncached(a);
  return a->call_n;
}

/* Mark t==19 nodes that are LHS of an assignment as lvalue, so bc()'s
   inline path leaves them as a 0x44 token for the assignment dyad. Walks
   the AST recursively. The lvalue forms we detect:
     a:1, a[i]:1               -- v is the literal ':' char (58)
     a::1, d[`a]::1            -- v carries subtype 0x82
     a+:1, a[i]+:1, etc.       -- v carries subtype 0x85 (amend dyad)
     0xff wrappers around 0x85 -- the parser sometimes nests an amend
                                  inside a t==1 with v==0xff
   In all of these, a->a[0] is the lvalue side. */
static int is_assign_verb(K v) {
  if(v==58) return 1;          /* a:1 / a[i]:1 -- raw colon char */
  if(0x82==s(v)) return 1;     /* a::1 / d[`a]::1 */
  if(0x85==s(v)) return 1;     /* amend wrapper */
  if(0x8c==s(v)) return 1;     /* alt amend (just in case the parser uses it) */
  if(0xce==s(v)) return 1;     /* a+:1 / d[`a]+:1 */
  if(0xcf==s(v)) return 1;     /* a+: */
  return 0;
}

static void mark_lvalues(pgs *s, pn *a, int d) {
  if(!a || s->overflow) return;
  if(d>maxr) { s->overflow=1; return; }  /* tree too deep: a recursive walk (this,
                               bc, can_inline_call) would overflow the C stack.  Flag
                               it -> bc bails, pgparse raises "stack".  Piggybacks on
                               the traversal bc already does, so no extra pass. */
  if(a->t==1) {
    int isassign=is_assign_verb(a->v);
    /* 0xff wraps an amend in some grammars: q=t==1 with v==0xff and the
       inner amend at q->a[0] (a t==1 with v==0x85). The inner's a[0]
       remains the lvalue. */
    if(!isassign && a->v==0xff && a->m>=1 && a->a[0] && a->a[0]->t==1
       && is_assign_verb(a->a[0]->v)) {
      pn *inner=a->a[0];
      if(inner->m>=1 && inner->a[0] && inner->a[0]->t==19)
        inner->a[0]->lv=1;
    }
    if(isassign && a->m>=1 && a->a[0] && a->a[0]->t==19) {
      a->a[0]->lv=1;
    }
  }
  for(int i=0;i<a->m;i++) mark_lvalues(s,a->a[i],d+1);
}

static void bc(pgs *s, pn *a, K values, K index, K line, int *vm) {
  char *t;
  pn **s0,**s1;
  int i0=0,i1=0,i0m=1024,i1m=1024,i,j=0;
  K *pvalues=px(values);
  int *pindex=px(index);
  int *pline=px(line);
  mark_lvalues(s,a,0);
  if(s->overflow) return;  /* parse tree too deep (mark_lvalues) -> pgparse raises "stack" */
  s0=xmalloc(sizeof(pn*)*i0m);
  s1=xmalloc(sizeof(pn*)*i1m);
  s0[++i0]=a;
  while(i0) {
    a=s0[i0--];
    if(++i1==i1m) { i1m<<=1; s1=xrealloc(s1,sizeof(pn*)*i1m); }
    s1[i1]=a;
    if(a->t==6) continue; /* klist () */
    if(a->t==4) continue; /* plist [] */
    if(a->t==19) {
      /* Inline RPN for f[args]: push f, then each arg sub-stmt. The plist
         node itself is NOT pushed; we synthesize an APPLY_N token in the
         emit pass. Falls back to single-token list19() when not inlinable.
         gk v1 evaluates parameter lists left-to-right (see doc/v1.md), so
         we want first-source-arg evaluated first. The parser stores
         plist->a in reverse-source order, so we iterate it backward here.
         Net result of bc()'s two passes: emission is
           f-tokens, first-source-tokens, ..., last-source-tokens, APPLY_N
         which matches rl()'s convention (plist[0] = first source) that
         lambda binding (x = plist[0]) expects. */
      int N=can_inline_call(a);
      if(N>=0) {
        if(++i0==i0m) { i0m<<=1; s0=xrealloc(s0,sizeof(pn*)*i0m); }
        s0[i0]=a->a[0];
        pn *plist=a->a[1];
        for(int k=plist->m-1;k>=0;--k) {
          if(++i0==i0m) { i0m<<=1; s0=xrealloc(s0,sizeof(pn*)*i0m); }
          s0[i0]=plist->a[k];
        }
      }
      continue;
    }
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
      if(0xc1==s(a->v)) {
        /* Wrap into 0xda (f;av) so pgreduce_, fe, etc. see the new
           composable shape. Dyad intent (was the 0xc1->0xc2 rewrite)
           is encoded in the inner verb K. */
        pvalues[j++]=wrap_mv_to_da(a->v,!!a->a[0]);
      }
      else if(0xc7==s(a->v)) {
        if(a->a[1]&&!a->a[0]&&hasav(a)) {
          /* draw/2 3 4 -- monadic apply of a builtin dyad with an
             adverb. Pass 1b: wrap into 0xda (f;av) instead of the
             old 0xc7->0xdb subtype rewrite. */
          pvalues[j++]=wrap_c7_to_da(a->v);
        }
        else pvalues[j++]=k_(a->v);
      }
      else if(0xca==s(a->v)) {
        if(a->a[1]&&!a->a[0]&&hasav(a)) a->v=(a->v&~((K)0xff<<48))|(K)0xc9<<48;  /* mul/x */
        pvalues[j++]=k_(a->v);
      }
      else if(0xce==s(a->v)) {
        if(a->a[0]&&!a->a[1]) a->v=(a->v&~((K)0xff<<48))|(K)0xcf<<48;
        pvalues[j++]=k_(a->v);
      }
      else if(0xc6==s(a->v)||0xc7==s(a->v)||0xc9==s(a->v)
            ||0xd1==s(a->v)||0xd2==s(a->v)||0xd3==s(a->v)||0xcc==s(a->v)
            ||0xcd==s(a->v)||0xcb==s(a->v)||0x85==s(a->v)||0x82==s(a->v)
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
    else if(a->t==19) {
      int N=can_inline_call(a);
      if(N>=0) pvalues[j++]=t(1,st(0xb0,N)); /* APPLY_N */
      else pvalues[j++]=list19(s,a); /* f[x] aka 0x44 subtype */
    }
    else if(a->t==11) {  /* 1+ */
      K v=tn(0,2); K *pv=px(v);
      if(a->a[0]->t==6) pv[0]=listbc(s,a->a[0],0x42);
      else pv[0]=k_(a->a[0]->n);
      if(s(a->v)) {
        /* 0xd0 fixed dyad: verb in dyadic position (e.g. 5+/x) */
        pv[1]=dupwrap_mv(a->v,1);
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
            /* 0xd0 fixed dyad inside t==7 composition */
            pv2[1]=dupwrap_mv(a->a[i]->v,1);
          }
          else {
            t=strchr(P,a->a[i]->v);
            pv2[1]=t(1,st(0xc0,t-P));
          }
          pv[i]=st(0xd0,v2);
        }
        else if(s(a->a[i]->v)) {
          /* element of 0xc5 verb-list, e.g. +/ in (+/-) */
          pv[i]=dupwrap_mv(a->a[i]->v,0);
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

/* parse node allocation with global tracking for memory management */
pn* pnnew(pgs *s, int t, K v, K n, int m, int f) {
  pn *r=xmalloc(sizeof(pn));
  r->t=t;
  r->v=v;
  r->n=n;
  r->m=m;
  r->f=f;
  r->i=-1;
  r->lv=0;
  r->call_n=-2;
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
/* Issue #2 Pass 6: an adverb that follows a value is a postfix
   modifier on that value -- e.g. `f'`, `f[2]'`, `(f+g)'`. The lexer
   no longer glues adverbs onto names/lambdas, so the right-leaning
   juxt tree built by the grammar can have a chain of adverbs and
   plists in its right spine. Left-fold that spine so each element
   binds postfix to the previously-built value:

       juxt(a, juxt(',  juxt([3], juxt(', [4]))))
     ->
       juxt(juxt(juxt(juxt(a, '), [3]), '), [4])

   Only triggers when the immediate spine head is a 0x85 adverb;
   value-composition chains like `f g h` (juxt with non-adverb head)
   are left untouched so they keep their right-leaning, K-style
   "f@g@h" semantics. */
static int is_adverb_pn(pn *n) {
  return n && n->t==1 && 0x85==s(n->v);
}
/* A multi-bracket chain `[i][j]...` is built (in r013, replacing the old
   t==40 bundle) as a left-nested t==19 apply chain whose deepest a[0] is the
   not-yet-known head -- left NULL as a hole.  The head-attach fills the hole;
   once filled it is an ordinary nested t==19 that list19()/bc()/can_inline()
   already flatten, so t==40 is gone.  chain_bottom() returns the deepest t==19
   (the hole owner); is_headless_chain() tests for the unfilled hole. */
static pn *chain_bottom(pn *n){ while(n->a[0] && n->a[0]->t==19) n=n->a[0]; return n; }
static int is_headless_chain(pn *n){ return n && n->t==19 && chain_bottom(n)->a[0]==0; }
/* Prepend a new innermost bracket plist p to chain c (c is a single plist or a
   headless chain).  p becomes the bracket applied first (right after the head). */
static pn *chain_prepend(pgs *s, pn *p, pn *c){
  pn *inner=pnnewi(s,19,0,0,2,1,p->i,p->line);
  inner->a[0]=0; inner->a[1]=p;
  if(c->t==19){ chain_bottom(c)->a[0]=inner; return c; }
  pn *outer=pnnewi(s,19,0,0,2,1,p->i,p->line);
  outer->a[0]=inner; outer->a[1]=c;
  return outer;
}
/* Attach head h to a bracket argument b: a lone plist becomes t19(h, plist);
   a headless chain gets its hole filled (h[i][j] = ((h[i])[j])). */
static pn *attach_args(pgs *s, pn *h, pn *b){
  if(is_headless_chain(b)){ chain_bottom(b)->a[0]=h; return b; }
  pn *q=pnnewi(s,19,0,0,2,1,h->i,h->line);
  q->a[0]=h; q->a[1]=b;
  return q;
}
static int is_args_pn(pn *n) {
  return n && (n->t==4 || is_headless_chain(n)); /* plist or [][]... chain */
}
/* A noun the postfix fold can re-apply onto a bracketed result. */
static int is_value_pn(pn *n) {
  return n && (n->t==2 || n->t==6); /* noun atom/vector, or () klist */
}
/* Foldable postfix chain: every spine element is an adverb or plist, ending
   in an adverb/plist tail (the original pure case) -- OR an adverb/plist
   chain that *contains a bracket* and ends in a trailing value (e.g.
   f/[5;0] 0).  In the latter the bracket closes the adverb application and
   the trailing value re-applies to its result; left-folding makes pgreduce_
   case-7 do exactly that.  The "contains a bracket" guard keeps f/value and
   +/1 2 3 (adverb directly over a juxtaposed argument) on the old path. */
static int is_pure_postfix_chain(pn *b) {
  int lastargs=0; /* was the most recent spine element a bracket? */
  while(b && b->t==1 && b->v==0xff && b->m>=2 && b->a[0] && b->a[1]) {
    if(is_args_pn(b->a[0])) lastargs=1;
    else if(is_adverb_pn(b->a[0])) lastargs=0;
    else return 0;
    b=b->a[1];
  }
  if(!b) return 0;
  if(is_adverb_pn(b) || is_args_pn(b)) return 1;
  /* Trailing value: fold (re-apply) only when the element right before it is
     a bracket -- i.e. the adverb already took its bracket argument, so the
     value applies to the result (f/[5;0] 0).  If the preceding element is an
     adverb (dv[1 2 3]'1 2 3), the value is that adverb's own argument: leave it. */
  return lastargs && is_value_pn(b);
}
/* Peel one head element onto a as a juxt. We deliberately build all
   peels as 0xff juxts (rather than special-casing plists into t=19
   apply nodes); pgreduce_'s case 7 handles juxt-with-plist via the
   0x41/0x81 -> 0x44 packing path, which routes through r44/fapply
   correctly for our 0xda/0xd9-headed left side. */
static pn *postfix_peel(pgs *s, pn *a, pn *head) {
  pn *q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
  q->a[0]=a;
  q->a[1]=head;
  return q;
}
/* Peel one spine element.  A headless bracket chain (`[i][j]...`) bundles
   consecutive bracket groups; peel each separately so chained application
   binds left-to-right after an adverb wrap -- e.g. f/[i][j] is (f/[i])[j].
   Without the split the whole `[i][j]` would juxtapose onto the adverbed
   verb as one args plist, dropping all but the first group.  The chain nests
   innermost-bracket deepest, so collect bottom-up and peel in that order.
   A lone plist or adverb peels as itself. */
static pn *postfix_peel_el(pgs *s, pn *a, pn *head) {
  if(head && is_headless_chain(head)) {
    pn *stk[64]; int sp=0;
    for(pn *n=head; n->t==19 && sp<64; n=n->a[0]) { stk[sp++]=n->a[1]; if(!n->a[0]) break; }
    while(sp>0) a=postfix_peel(s,a,stk[--sp]);
    return a;
  }
  return postfix_peel(s,a,head);
}
/* The chain contains at least one adverb head somewhere down the
   right-spine.  Pure plist chains (`f[1][2]`) don't need restructure
   -- their right-leaning juxt already reduces correctly via the
   r44/fapply path -- but a chain that mixes plists and adverbs
   (`f[1]'`, `f[1]'[2]'`) does, so the trailing adverb lands on the
   partial-arg projection rather than on the inner args plist.  */
static int has_adverb_in_chain(pn *b) {
  while(b && b->t==1 && b->v==0xff && b->m>=2 && b->a[0] && b->a[1]) {
    if(is_adverb_pn(b->a[0])) return 1;
    b=b->a[1];
  }
  return is_adverb_pn(b);
}
/* Shape B: the spine ends in an infix verb that wrongly swallowed a bracket
   as its left operand.  r013 builds `f/[5;]@0` as `@([5;], 0)` because by the
   time the `]` reduces, `@` already holds its right operand `0` and an empty
   left, so the bracket gets bound there (the r013 "[0]:5" else branch).  But
   the bracket is really the adverbed value's argument: `(f/[5;]) @ 0`.  This
   re-associates -- fold (base . adverb/args spine . the swallowed bracket)
   into the closed application and make it the verb's left operand. */
static pn *postfix_reassoc_infix(pgs *s, pn *a, pn *b) {
  if(!has_adverb_in_chain(b)) return 0;
  pn *tail=b;
  while(tail && tail->t==1 && tail->v==0xff && tail->m>=2 && tail->a[0] && tail->a[1])
    tail=tail->a[1];
  /* tail must be a primitive infix verb: right operand set, left a bracket */
  if(!(tail && tail->t==1 && tail->v>0 && tail->v<256 && tail->m>=2
       && tail->a[0] && (tail->a[0]->t==4 || is_headless_chain(tail->a[0])) && tail->a[1]))
    return 0;
  /* the spine before the verb must be a pure adverb/args chain */
  for(pn *w=b; w!=tail; w=w->a[1])
    if(!(w->t==1 && w->v==0xff && w->m>=2 && w->a[0] && w->a[1]
         && (is_adverb_pn(w->a[0]) || is_args_pn(w->a[0])))) return 0;
  pn *plist=tail->a[0];
  pn *folded=a;
  for(pn *w=b; w!=tail; w=w->a[1]) folded=postfix_peel_el(s,folded,w->a[0]);
  folded=postfix_peel_el(s,folded,plist);
  tail->a[0]=folded;
  return tail;
}
static pn *postfix_restructure(pgs *s, pn *a, pn *b) {
  pn *rb=postfix_reassoc_infix(s,a,b);
  if(rb) return rb;
  /* Only restructure if the entire spine is adverbs and plists.
     If a verb/value appears mid-chain (e.g. `g'!5`), the chain is
     mixed and right-leaning semantics must be preserved. */
  if(!is_pure_postfix_chain(b)) return 0;
  /* And only if there's actually an adverb in the chain -- a pure
     plist chain (`f[1][2]`) is already correct under right-lean.
     This also drops the old "leading head must be adverb" check
     so plist-first chains like `f[1]'` left-fold too. */
  if(!has_adverb_in_chain(b)) return 0;
  while(b && b->t==1 && b->v==0xff && b->m>=2 && b->a[0] && b->a[1]) {
    a=postfix_peel_el(s,a,b->a[0]);
    b=b->a[1];
  }
  return postfix_peel_el(s,a,b);
}
static void r003_(pgs *s) { /* body of e > o ez (wrapped by r003) */
  pn *q;
  pn *b=s->V[s->vi--];
  pn *a=s->V[s->vi];
  if(!b) return;
  /* Postfix-restructure (immediate case): a=value, b=juxt(adverb, ...).
     Triggered by r003 calls that combine a value with a juxt whose
     leftmost spine element is an adverb. Other paths in r003 may
     also build this shape (e.g. line 2810 inserts an apply node into
     b->a[0]) -- those are handled by the same logic re-applied at
     each store via the maybe_postfix_store() helper below. */
  if(is_adverb_pn(b) && a && (V(a) || a->t==2 || a->t==6 || a->t==19)) {
    q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
    q->a[0]=a;
    q->a[1]=b;
    s->V[s->vi]=q;
    return;
  }
  if(b->t==1 && b->v==0xff && b->m>=2 && b->a[0] && b->a[1]
     && (is_adverb_pn(b->a[0]) || is_args_pn(b->a[0]))
     && has_adverb_in_chain(b)
     && !is_adverb_pn(a)) {
    /* Skip when a is itself an adverb (`'[2]'` as a sub-expression):
       folding here would left-fold the inner chain before the real
       base value arrives, and the resulting left-folded juxt would
       break is_pure_postfix_chain at the outer fold (where `f`
       attaches).  Defer to the V(a) right-leaning juxt path below;
       postfix_restructure fires at the f-attached level instead. */
    q=postfix_restructure(s,a,b);
    if(q) { s->V[s->vi]=q; return; }
  }
  if(!a) return; /* postfix-restructure short-circuit may leave a NULL */
  if(a->t==2 && (b->t==4 || is_headless_chain(b))) {
    s->V[s->vi]=attach_args(s,a,b);
  }
  else if(a->t==2 && b->a[0] && (b->a[0]->t==4 || is_headless_chain(b->a[0]))) {
    b->a[0]=attach_args(s,a,b->a[0]);
    s->V[s->vi]=b;
  }
  else if(V(a) && (b->t==4 || is_headless_chain(b))
          && 0xd1!=s(a->v) && 0xd2!=s(a->v) && 0xd3!=s(a->v)
          && 36!=a->v && 0x85!=s(a->v)) { /* 3 2 12 36 16 = do while if $ av */
    s->V[s->vi]=attach_args(s,a,b);
  }
  else if(V(a) && V(b)) {
    if(0x85!=s(a->v) && !is_headless_chain(b) && b->a[0] && is_headless_chain(b->a[0])) {
      b->a[0]=attach_args(s,a,b->a[0]);
      s->V[s->vi]=b;
    }
    else if(0x85==s(a->v)) {
      q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
      q->a[0]=a;
      q->a[1]=b;
      s->V[s->vi]=q;
    }
    else if(0x85==s(b->v)) {
      /* Issue #2 Pass 6: verb followed by adverb -- the lexer no
         longer slurps adverbs onto verb names, so the parser sees
         them as separate V nodes here.  Build a juxt(verb, adverb)
         so postfix-restructure and pgreduce_'s case 7 wrap into
         0xda(verb, av) on application. */
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
    else if(0x85!=s(a->v)&&!is_headless_chain(b)&&b->a[0]&&(b->a[0]->t==4||is_headless_chain(b->a[0]))) { /* sqr[2]+sqr[3] */
      b->a[0]=attach_args(s,a,b->a[0]);
      s->V[s->vi]=b;
    }
    /* Issue #2 Pass 6 cleanup: `a->t==7 && 0x85==s(b->v)` retired
       (post-pass-6 no path produces this shape -- the lexer emits
       adverbs as standalone 0x85 leaves which r003 wraps via the
       earlier `0x85==s(b->v)` branch and postfix-restructure). */
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
    else if(b->v=='.' && b->a[1] && a->t==6 && a->m==1
            && a->a[0] && a->a[0]->t==1 && 0xcc==s(a->a[0]->v)) {
      b->a[0]=a->a[0];  /* (0:) . args */
      s->V[s->vi]=b;
    }
    else if(b->t==7 && b->a[0]) {   /* 1+- */
      b->a[0]->a[0]=a;
      b->a[0]->t=11;
      s->V[s->vi]=b;
    }
    else if(b->a[0]) {
      /* Issue #2 Pass 6 cleanup: `b->v==0x27|0xce|0x5c &&
         V(b->a[0])` branches retired -- these encoded "value-then-
         adverbed-verb" (e.g. `1+'`) by mutating b->v into the 256+/
         512+/1024+ space.  Post-pass-6 the adverb is a standalone
         leaf and composition happens via the postfix-restructure
         juxt path, so b->v never carries 0x27/0xce/0x5c here. */
      if(b->a[0] && (b->a[0]->t==4||is_headless_chain(b->a[0])) && b->v!=58 && b->v!=0xff) { /* d[],1   b->v != : */
        if(is_headless_chain(b->a[0])) { b->a[0]=attach_args(s,a,b->a[0]); }
        else {
          q=pnnewi(s,1,0xff,0,2,1,a->i,a->line);
          q->a[0]=a;
          q->a[1]=b->a[0];
          b->a[0]=q;
        }
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
        else if(b->v=='.' && a->t==2) {
          b->a[0]=a;
          s->V[s->vi]=b;
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
      else if(0xc3==s(a->n) || 0xda==s(a->n)) {
        /* Pass 2b-step-4: 0xda(c3,av) inline lambda also fires the
           dyad-juxt insertion path. */
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
/* Issue #2 Pass 6: r003 wrapper that runs the LR rule body and then
   applies postfix-restructure as a post-process. The result tree at
   s->V[s->vi] may be a juxt (built directly), or a juxt embedded
   inside a t==1 node (e.g. via line "b->a[0]=q; s->V[s->vi]=b"
   patterns). For each candidate sub-juxt found at the top of the
   tree whose right-spine starts with an adverb head, recursively
   left-fold it. */
static pn *postfix_restructure_top(pgs *s, pn *r) {
  if(!r) return r;
  if(r->t==1 && r->v==0xff && r->m>=2 && r->a[0] && r->a[1]) {
    pn *b=r->a[1];
    if(b->t==1 && b->v==0xff && b->m>=2 && b->a[0] && b->a[1]
       && (is_adverb_pn(b->a[0]) || is_args_pn(b->a[0]))
       && has_adverb_in_chain(b)
       && !is_adverb_pn(r->a[0])) {
      /* Same defer rule as r003_'s pre-fold gate -- if the base
         is itself an adverb (`'[2]'` as a sub-expression), leave
         the right-leaning chain alone for the outer fold. */
      pn *q=postfix_restructure(s,r->a[0],b);
      if(q) return q;
    }
  }
  return r;
}
static void r003(pgs *s) { /* e > o ez */
  int vi0=s->vi;
  r003_(s);
  if(s->vi<=vi0 && s->V[s->vi]) {
    s->V[s->vi]=postfix_restructure_top(s,s->V[s->vi]);
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
    s->V[s->vi]=chain_prepend(s,a,b);
  }
  else if(a->t==4&&is_headless_chain(b)) { /* [] [][]... */
    s->V[s->vi]=chain_prepend(s,a,b);
  }
  else if(a->t==4&&b->a[0]&&is_headless_chain(b->a[0])&&b->v==0xff) { /* [] [][]...v */
    b->a[0]=chain_prepend(s,a,b->a[0]);
    s->V[s->vi]=b;
  }
  else if(a->t==4&&b->a[0]&&b->a[0]->t==4) { /* [] []v */
    b->a[0]=chain_prepend(s,a,b->a[0]);
    s->V[s->vi]=b;
  }
  /* Issue #2 Pass 6 cleanup: t=104 `[]'` retired -- the legacy
     special node for a bracketed apply followed by a single trailing
     adverb.  Post-pass-6 the adverb folds onto the apply via the
     standard postfix-restructure path before r013 sees it. */
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
    else if(is_adverb_pn(b)) {
      /* Issue #2: `[args]'` -- the post-list pz is a solo adverb.
         The old branch stuffed the plist into b->a[0] (making the
         adverb the outer node with plist as inner), which gave the
         wrong reduce shape when an `f` arrives in r003_: the parse
         tree became f juxt adverb(plist) and bc() emitted [f, args,
         adverb, juxt] -- so case-7 saw (args, adverb) at the top
         of stack with f stranded one slot below.  Build juxt(plist,
         adverb) instead so r003_/postfix_restructure see the
         canonical adverb-in-chain shape and left-fold to
         juxt(juxt(f, plist), adverb).  Covers `f[1]'`, `f[1;2]'`,
         and chained variants like `f[1]'[2]'`. */
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
  /* The token table (pgs.tm/tc, p.h) is a 32-bit count grown by doubling, so
     it overflows past ~2^30 tokens; a source has <=1 token/char.  Reject a
     >=2^30-char source up front with a clean length error -- covers every lex
     entry point (REPL repl.c:216, load repl.c:346, lambda defs fn.c:243, the
     `.` verb v.c, watch exprs watch.c) instead of corrupting in the lexer. */
  if(strlen(q) >= ((u64)1<<30)) return KERR_LENGTH;
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
    if(s->overflow) { e=KERR_STACK; goto cleanup; }  /* parse tree too deep (mark_lvalues) */
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
