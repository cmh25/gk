#include "fn.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "p.h"
#include "av.h"
#include "repl.h"
#include "fe.h"

K fnestack[EVALDEPTH];   /* per-lambda-call stack; depth-indexed, must match A0/params (k.h) */
int fnestacki=-1;
static char* spf;

void fninit(void) {
  K f;
  f=fnnew("{x in\\y}"); dset(C,sp("lin"),f); fnfree(f);
  f=fnnew("{x dvl,y}"); dset(C,sp("dv"),f); fnfree(f);
  f=fnnew("{$[@x;. di[. x;&|/(!x)=/$[@y;,y;y]];x@&@[(#x)#1;y;:;0]]}"); dset(C,sp("di"),f); fnfree(f);
  f=fnnew("{(dj _ x%86400;100 sv 24 60 60 vs x!86400)}"); dset(C,sp("gtime"),f); fnfree(f);
  f=fnnew("{gtime lt x}"); dset(C,sp("ltime"),f); fnfree(f);
  f=fnnew("{t+x-lt t:x+x-lt x}"); dset(C,sp("tl"),f); fnfree(f);
  f=fnnew("{((2##*x)#1,&#*x) lsq x}"); dset(C,sp("inv"),f); fnfree(f);
  f=fnnew("{rint exp lgamma[1+x]-lgamma[1+y]+lgamma[1+x-y]}"); dset(C,sp("choose"),f); fnfree(f);
  f=fnnew("{x*rint y%x}"); dset(C,sp("round"),f); fnfree(f);
  f=fnnew("{if[nul~x;:nul];i:1+2*!_.5*#x:(0,,/(0,+/~+\\ep[<;0,\"[\"=y]-ep[<;(\"]\"=y$:)],0)+/x ss $[3=4:y;$y;y])_ x;,/$[7=4:z;@[x;i;z];4:z$:;@[x;i;:;(#i)#,z];@[x;i;:;z]]}"); dset(C,sp("ssr"),f); fnfree(f);
  f=fnnew("{_[-1;x;y]}"); dset(C,sp("ep"),f); fnfree(f);
  spf=sp("f");
}

K fnnew(char *s) {
  K f,*pf;
  char *pd;
  if(!s||*s!='{') return null;
  f=tn(0,4);
  pf=px(f);
  pf[0]=tn(3,strlen(s)); /* definition */
  pf[1]=null;            /* parse result */
  pf[2]=null;            /* scope */
  pf[3]=FN_VF(0,0);      /* valence + force-monad FM[] index */
  pd=px(pf[0]);
  i(strlen(s),pd[i]=s[i])
  K p=fnd(f);
  if(p) { _k(f); return p; }
  return st(0xc3,f);
}

void fnfree(K f) {
  _k(f);
}

static K fncp_(K x) {
  K p;
  K f=tn(0,4);
  K *pf=px(f);
  K *px=px(x);
  pf[0]=kcp(px[0]); /* definition */
  if(E(pf[0])) { K e=pf[0]; _k(f); return e; }
  pf[1]=null;       /* parse result */
  pf[2]=null;       /* scope */
  pf[3]=FN_VF(0,0); /* valence + force-monad FM[] index */
  if((p=fnd(f))) { fnfree(f); return p; }
  return st(0xc3,f);
}
/* fncpproj_ retired in Pass 4 -- 0xc4 replaced by 0xd9. */
K fncp(K x) {
  if(0xc3==s(x)) return fncp_(x);
  return KERR_TYPE;
}

#define SB(b,m,l,c) do { \
  if((l)==(m)) { (m)<<=1; (b)=xrealloc((b),(m)*sizeof(*(b))); } \
  (b)[(l)]=(c); \
} while (0)

K fnd(K f) {
  K p,r=0,*pf;
  char *ff=0,*ff0=0,*b,**v,*g,*h;
  int j,s,n,q,vx,vy,vz,ffq=0,first=1,params=1;
  int bm=32,vm=32;
  pf=px(f);
  ff=px(pf[0]); ff0=ff;
  s=j=n=q=vx=vy=vz=0;
  if(!*ff) return r;
  if(*ff!='{') return r;
  b=xcalloc(bm,1); v=xcalloc(vm*sizeof(char*),1);
  for(;*ff;++ff) {

    /* skip comments */
    if(s!=2 && first && *ff=='/') { ++ff; while(*ff&&*ff!='\n') ++ff; --ff; continue; }
    if(s!=2 && *ff== ' ' && ff[1] && ff[1]=='/') { ff+=2; while(*ff&&*ff!='\n') ++ff; --ff; continue; }
    if(s!=2 && ff>ff0 && (ff[-1]=='('||ff[-1]=='['||ff[-1]=='{') && *ff=='/') { ++ff; while(*ff&&*ff!='\n') ++ff; --ff; continue; }
    first=0; if(*ff=='\n') first=1;

    if(*ff=='"') { params=0; while(*ff&&*ff=='"') ff=xeqs(ff); }
    if(!*ff) { r=KERR_PARSE; goto cleanup; }
    switch(s) {
    case 0:
      if(*ff=='{') s=1;
      else { r=KERR_PARSE; goto cleanup; }
      break;
    case 1:
      while(*ff&&isblank((unsigned char)*ff))++ff;
      if(*ff=='"') while(*ff&&*ff=='"') ff=xeqs(ff);
      if(!*ff) { r=KERR_PARSE; goto cleanup; }
      if(*ff=='}') s=2;
      else if(*ff=='{') { n++; s=9; }
      else if(*ff=='['&&params) s=3;
      else if(*ff=='.') s=11;
      else if(*ff=='`') s=101;
      else if(isalpha((unsigned char)*ff)) { s=7; SB(b,bm,j,*ff); j++; }
      else if((i8)*ff<0) { r=KERR_PARSE; goto cleanup; }
      else s=6;
      params=0;
      break;
    case 101:
      if(isalnum((unsigned char)*ff)) s=101;
      else s=1;
      break;
    case 2:
      r=KERR_PARSE; goto cleanup;
    case 3:
      if(isalpha((unsigned char)*ff)) { s=4; SB(b,bm,j,*ff); j++; ffq=1; }
      else if(*ff==']') s=5;
      else { r=KERR_PARSE; goto cleanup; }
      break;
    case 4:
      if(isalnum((unsigned char)*ff)) { s=4; SB(b,bm,j,*ff); j++; }
      else if(*ff==']') {
        s=5; SB(b,bm,j,0); j=0; SB(v,vm,q,sp(b)); q++;
      }
      else if(*ff==';') {
        s=3; SB(b,bm,j,0); j=0; SB(v,vm,q,sp(b)); q++;
      }
      break;
    case 5:
      if(*ff=='}') s=2;
      else if(*ff=='{') { n++; s=8; }
      else if(*ff=='.') s=15;
      else if(*ff=='`') s=105;
      else if(isalpha((unsigned char)*ff)) { s=7; SB(b,bm,j,*ff); j++; }
      else s=6;
      break;
    case 105:
      if(isalnum((unsigned char)*ff)) s=105;
      else s=5;
      break;
    case 6:
      if(*ff=='}') s=2;
      else if(*ff=='.') s=16;
      else if(*ff=='`') s=106;
      else if(isalpha((unsigned char)*ff)) { s=7; SB(b,bm,j,*ff); j++; }
      else if(*ff=='{') { n++; s=8; }
      else if((i8)*ff<0) { r=KERR_PARSE; goto cleanup; }
      else s=6;
      break;
    case 106:
      if(isalnum((unsigned char)*ff)) s=106;
      else if(*ff=='`') s=106;
      else if(*ff=='}') s=2;
      else s=6;
      break;
    case 7:
      if(*ff=='}') {
        s=2; SB(b,bm,j,0); j=0;
        if(!ffq) { /* no implicit xyz if there are formal parameters */
          if(!vx && !strcmp(b,"x")) { vx=1; SB(v,vm,q,sp(b)); q++; }
          if(!vy && !strcmp(b,"y")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } vy=1; SB(v,vm,q,sp("y")); q++; }
          if(!vz && !strcmp(b,"z")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } if(!vy) { vy=1; SB(v,vm,q,sp("y")); q++; } vz=1; SB(v,vm,q,sp("z")); q++; }
        }
      }
      else if(isalnum((unsigned char)*ff)) { SB(b,bm,j,*ff); j++; }
      else if(*ff=='{') {
        s=10; SB(b,bm,j,0); j=0;
        if(!ffq) { /* no implicit xyz if there are formal parameters */
          if(!vx && !strcmp(b,"x")) { vx=1; SB(v,vm,q,sp(b)); q++; }
          if(!vy && !strcmp(b,"y")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } vy=1; SB(v,vm,q,sp("y")); q++; }
          if(!vz && !strcmp(b,"z")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } if(!vy) { vy=1; SB(v,vm,q,sp("y")); q++; } vz=1; SB(v,vm,q,sp("z")); q++; }
        }
      }
      else { /* no implicit xyz if there are formal parameters */
        s=6; SB(b,bm,j,0); j=0;
        if(!ffq) { /* no implicit xyz if there are formal parameters */
          if(!vx && !strcmp(b,"x")) { vx=1; SB(v,vm,q,sp(b)); q++; }
          if(!vy && !strcmp(b,"y")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } vy=1; SB(v,vm,q,sp("y")); q++; }
          if(!vz && !strcmp(b,"z")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } if(!vy) { vy=1; SB(v,vm,q,sp("y")); q++; } vz=1; SB(v,vm,q,sp("z")); q++; }
        }
      }
      break;
    case 8:
      if(*ff=='}') { n--; if(!n) s=6; }
      else if(*ff=='{') n++; /* nested function */
      break;
    case 9:
      if(*ff=='}') { n--; if(!n) s=5; }
      else if(*ff=='{') n++; /* nested function */
      break;
    case 10:
      if(*ff=='}') { n--; if(!n) s=7; }
      else if(*ff=='{') n++; /* nested function */
      break;
    case 11: /* .z.s */
      if(isalnum((unsigned char)*ff)) s=11;
      else if(*ff=='.') s=11;
      else if(*ff=='}') s=2;
      else s=1;
      break;
    case 15: /* .z.s */
      if(isalnum((unsigned char)*ff)) s=15;
      else if(*ff=='.') s=15;
      else if(*ff=='}') s=2;
      else s=5;
      break;
    case 16: /* .z.s */
      if(isalnum((unsigned char)*ff)) s=16;
      else if(*ff=='.') s=16;
      else if(*ff=='}') s=2;
      else s=6;
      break;
    default: { r=KERR_PARSE; goto cleanup; }
    }
  }

  pf[2]=scope_new(cs);
  if(vz&&q<3) { SB(v,vm,q,sp("y")); q++; }
  if(vz&&q<3) { SB(v,vm,q,sp("x")); q++; }
  if(vy&&q<2) { SB(v,vm,q,sp("x")); q++; }
  pf[3]=FN_VF(q,0);
  i(q,p=scope_set(pf[2],t(4,sp(v[i])),null);if(E(p)) { r=p; goto cleanup; })
  if(q!=(int)n(((K*)px(((K*)((K*)px(pf[2])))[1]))[0])) { r=KERR_PARSE; goto cleanup; }  // {[a;b;b]a,b}
  K locals=tn(4,q); K *plocals=px(locals);
  i(q,plocals[i]=(K)sp(v[i]))

  /* parse */
  h=px(pf[0]);
  ++h;
  while(*h&&isblank((unsigned char)*h))++h;
  if(*h&&*h=='[') while(*h&&*h++!=']');
  g=xcalloc(1,5+strlen(h));
  memcpy(g,h,1+strlen(h));
  if(*g) g[strlen(g)-1]='\n';
  int opencode0=opencode;
  opencode=0;
  K cs0=cs; cs=k_(pf[2]);
  pf[1]=pgparse(g,1,locals);
  _k(cs); cs=cs0;
  opencode=opencode0;
  _k(locals);
  if(E(pf[1])) { r=pf[1]; pf[1]=null; }

  /* Force-monad recognition: a valence-1 body spelled exactly {Vx}, where
     V is a monadic primitive (one of P below, whose index is the FM[] slot),
     collapses at apply time to k(idx,0,arg) -- skipping scope setup + the
     pgreduce() interpreter loop.  We cache idx in pf[3]'s high bits (0 = not a force
     monad); the lambda otherwise stays a normal 0xc3 (prints verbatim,
     dispatches normally everywhere), so only fne_ short-circuits and there
     is no print/round-trip or fne()-handler surface to get wrong.  Requiring
     the canonical 4-char spelling means `{ ,x}`/`{,y}` stay ordinary
     lambdas. */
  pf[3]=FN_VF(q,0);
  if(!r && q==1 && n(pf[0])==4) {
    char *d0=px(pf[0]);
    static const char P[]=":+-*%&|<>=~.!@?#_^,$"; /* index == FM[] slot */
    if(d0[0]=='{' && d0[2]=='x' && d0[3]=='}') {
      const char *vp=strchr(P, d0[1]);
      if(vp && vp!=P) pf[3]=FN_VF(q,(int)(vp-P)); /* exclude ':' (idx 0, no monad) */
    }
  }
cleanup:
  xfree(b); xfree(v);
  return r;
}

// make a copy of r and parent scope
// closure only if parent scope == s0
// can't create a closure like this:
// f:{i+x}
// g:{i:0;f}
// closure() moved to fn.h as a static inline so the hot lambda-return
// path can inline it.  The 0xc4 branch was retired in Pass 4; the
// projection-of-projection closure case is left for a future pass.

K fne_(K f, K x, char *av) {
  K r=0,*pd,*ps,*pf,cs0,*px,t;
  static int d=0;
  u64 n,nn;

  if(++d>maxr || (!(d&7)&&stack_low())) { r=KERR_STACK; --d; goto cleanup; }

  pf=px(f);
  px=px(x);
  n=(u64)FN_VALENCE(pf[3]);
  int fmidx=FN_FMIDX(pf[3]); /* !=0: force monad {Vx}, FM[]/P index of V */

  u64 inc=0; i(nx,if(px[i]==inull)++inc)
  nn=nx-inc; //  if there are inulls, number of inulls must less than or equal to nx
  /* Project condition.  Without av, project on any short-fill.
     With av, project only when av is exactly "'" (each) -- "/"/"\\"
     and friends consume args (over/scan with optional seed take 1 or
     2 args regardless of inner valence) and let avdo handle valence
     itself.  The each case is what enables stacked partial-apply of
     an adverbed lambda, e.g. {x+y*z}'[2]'[3]'[4]. */
  /* Project (lift av to an outer 0xda) for each `'`, and for over/scan
     `/`/`\\` when the inner valence is >=3 -- those have no fold/seeded
     overload at that valence, so fewer args (or a gap) is under-application
     and must yield a projection, matching `'` and plain lambdas. (At
     valence 1/2, `/`/`\\` consume 1-2 args as fold/seed; leave to avdo.) */
  int proj_av = av && *av && av[1]==0 &&
                (av[0]=='\'' || ((av[0]=='/'||av[0]=='\\') && n>=3));
  if((nx<n||nn<n) && (!av||!*av||proj_av)) { /* project */
    /* Issue #2 Pass 3b-5: produce 0xd9(lambda, pargs) projection
       wrapper instead of legacy 0xc4 3-tuple.  Pass 4 extension:
       when av is "'" we lift it to an outer 0xda wrapper, giving
         0xd9(0xda(lambda, "'"), pargs)
       so subsequent applies merge args naturally. */
    K args=kcp(x);
    if(E(args)) { _k(f); _k(x); --d; return args; }
    _k(x);
    if(proj_av) {
      K w=tn(0,2); K *pw=px(w);
      pw[0]=f;  /* ownership transfers into 0xda */
      pw[1]=tnv(3,strlen(av),xmemdup(av,1+strlen(av)));
      K wrapped=st(0xda,w);
      --d;
      return wrap_proj(wrapped, args);
    }
    t=fncp(f);
    if(E(t)) { _k(f); _k(args); --d; return t; }
    _k(f);
    --d;
    return wrap_proj(t, args);
  }
  if(nx!=n && nx!=1 && n!=0 && (!av||!*av)) { r=KERR_VALENCE; goto cleanup; }
  if(av&&*av) {
    if(inc) { r=KERR_VALENCE; goto cleanup; } /* TODO: {y}'[!5;] */
    if(n==0) r=avdo(k_(f),0,k_(x),av);
    else if(n==1) {
      if(nx==1) r=avdo(k_(f),0,k_(px[0]),av);
      else if(0x81==s(x)) r=avdo(k_(f),0,k_(x),av);
      else if(!strcmp(av,"'")) r=avdo(k_(f),0,k_(x),av);
      else { r=KERR_VALENCE; goto cleanup; }
    }
    else if(n==2) {
      /* Issue #2 Pass 9: dyadic avdo on `'` zips two args (eachfe
         dyadic).  For chained-each av (all `'` chars, length>1)
         that's wrong -- we need Cartesian iteration, which only
         happens in the monadic multi-char avdo path on a 0x81
         plist.  Route accordingly.  Other multi-char avs (e.g.
         `'/`, `\\`) keep their dyadic dispatch. */
      int chained_each=0;
      if(av && av[0]) {
        chained_each=1;
        for(char *p=av; *p; ++p) if(*p!='\'') { chained_each=0; break; }
        if(av[0] && !av[1]) chained_each=0; /* single ' is dyadic-eachfe-OK */
      }
      if(nx==2 && !chained_each) r=avdo(k_(f),k_(px[0]),k_(px[1]),av);
      else if(nx==2) r=avdo(k_(f),0,k_(x),av);
      else if(nx==1) r=avdo(k_(f),0,k_(px[0]),av);
      else if(0x81==s(x)) r=avdo(k_(f),0,k_(x),av);
      else { r=KERR_VALENCE; goto cleanup; }
    }
    else if(0x81==s(x)) r=avdo(k_(f),0,k_(x),av);
    else { r=KERR_VALENCE; --d; goto cleanup; }
  }
  else {
    if(6==T(pf[2])) { K p=fnd(f); if(p) { _k(f); _k(x); return p; } fmidx=FN_FMIDX(pf[3]); }
    /* force-monad fast path: {Vx} collapses to the monadic primitive,
       skipping scope setup + pgreduce.  fmidx!=0 holds the FM[] index;
       valence is 1 so the lone arg is px[0]. */
    if(fmidx && nx==1) { r=k(fmidx,0,k_(px[0])); --d; goto cleanup; }
    ps=px(pf[2]);     // scope
    pd=px(ps[1]);     // scope dict
    cs0=cs; cs=k_(pf[2]);
    K pd1save=pd[1];  // save scope dict values
    u64 NX=nx;
    pd[1]=x;
    K ps5save=ps[5];
    ps[5]=t(1,1);
    n(pd[0])=n; n(pd[1])=n;
    fnestack[++fnestacki]=f;
    int opencode0=opencode;
    opencode=0;
    RETURN=0;  /* clear early return flag from previous calls */
    r=pgreduce(pf[1],0);
    RETURN=0;  /* a call consumes its own return; RETURN must not leak into the
                  caller's expression.  pgreduce clears it in file mode (!REPL)
                  but left it set in the REPL, so `1+g 5` with g:{:x*2} wrongly
                  yielded 10 not 11.  Only a direct top-level/subconsole `:`
                  (which never runs through fne_) should survive to the REPL. */
    opencode=opencode0;
    --fnestacki;
    /* closure detection: a returned lambda or dict-of-lambdas captures this
       scope's locals.  fne_ reuses f's persistent scope (pf[2]) as the
       activation record, so EVERY exit path must run the epilogue below to
       restore pd[1]/ps[5]/nx -- otherwise a re-entrant activation of the same
       f (e.g. recursion, or a projection/closure result re-applied) sees a
       corrupted scope and frees the still-borrowed merged-args plist (UAF).
       When scope_cp fails (KERR_STACK on a circular/deep scope) we therefore
       drop the closure and goto the epilogue with the error as the result,
       rather than restoring partially and skipping it.  Mirrors fne_fast. */
    if(0xc3==s(r)) {
      K closurescope=scope_cp(cs);
      if(E(closurescope)) { _k(r); r=closurescope; goto lam_cleanup; }
      K *pc=px(closurescope); pc[3]=t(1,1);
      r=closure(r,cs,closurescope);
      _k(closurescope);
    }
    else if(0x80==s(r)) {
      K *pr=px(r);
      K v=pr[1]; /* values */
      K *pvv=px(v);
      u32 c=0;
      i(n(v),if(0xc3==s(pvv[i]))++c)
      if(c&&c==n(v)) {
        K closurescope=scope_cp(cs);
        if(E(closurescope)) { _k(r); r=closurescope; goto lam_cleanup; }
        K *pc=px(closurescope); pc[3]=t(1,1);
        i(n(v),pvv[i]=closure(pvv[i],cs,closurescope))
        _k(closurescope);
      }
    }

lam_cleanup:
    pd=px(ps[1]);   /* ps[1] may have been refreshed during the body (a var read
                       via scope_get_ swaps the path-bound scope dict from the
                       ktree -- _k(ps[1]);ps[1]=q), freeing the dict pd was cached
                       from at entry.  Re-fetch before touching it, as fne_fast's
                       cleanup does, or the writes below are a use-after-free. */
    if(ik(ps[5])==0) _k(pd[1]);
    pd[1]=pd1save;
    ps[5]=ps5save;
    nx=NX;
    n(pd[0])=n(pd[1]); // locals could have been added, so sync key/val counts
    _k(cs); cs=cs0;
  }
  --d;
cleanup:
  _k(f);
  _k(x);
  return r;
}
/* Fast lambda apply: skips fne/fne_ function-call boundaries and the
   pgreduce() wrapper. Only valid when:
     - f is 0xc3 (not 0xc4 projection)
     - no adverbs on f
     - x is 0x81 plist with size matching valence and no inulls
     - f's scope dict isn't closure-pending (T==6)
   Caller must verify these. Consumes f and x. */
K fne_fast(K f, K x) {
  K *pf=px(f);
  i32 vf=ik(pf[3]);
  u64 nval=(u64)(vf&0xff);
  /* force-monad fast path: {Vx}[a] collapses to the monadic primitive.
     Caller guarantees x is a valence-matched plist with no inulls, so for
     a valence-1 force monad the lone arg is px(x)[0]. */
  if(vf>0xff) { int fmidx=(vf>>8)&0xff;
    if(fmidx) { K r=k(fmidx,0,k_(((K*)px(x))[0])); _k(f); _k(x); return r; } }
  K *ps=px(pf[2]);
  K *pd=px(ps[1]);

  K cs0=cs; cs=k_(pf[2]);
  K pd1save=pd[1];
  u64 NX=nx;
  pd[1]=x;
  K ps5save=ps[5];
  ps[5]=t(1,1);
  n(pd[0])=nval; n(pd[1])=nval;
  fnestack[++fnestacki]=f;
  int opencode0=opencode;
  opencode=0;
  RETURN=0;

  /* drive body statements directly; lambda bodies don't have \t/\l/etc.
     mirror pgreduce()'s RETURN handling: clear RETURN on early return
     in non-REPL mode so do_/while_/etc. don't see it leak from the
     lambda. */
  K body=pf[1];
  K *pbody=px(body);
  u64 body_n=n(body);
  K r=null;
  for(u64 i=0;i<body_n;++i) {
    int q;
    K p=pgreduce_(pbody[i],&q);
    if(E(p)) { _k(r); r=p; break; }
    if(EXIT) { _k(p); _k(r); r=null; break; }
    _k(r); r=p;
    if(RETURN) { RETURN=0; break; }  /* a call consumes its own return (see fne_) */
  }

  opencode=opencode0;
  --fnestacki;

  /* closure detection: a returned lambda or dict-of-lambdas captures
     this scope's locals.  NOTE: fne_fast reuses f's persistent scope
     (pf[2]) as the activation record, so every exit path MUST run the
     epilogue below to restore pd[1]/ps[5]/nx -- otherwise a re-entrant
     activation of the same f sees the corrupted scope.  When scope_cp
     fails (e.g. KERR_STACK on a circular/deep scope) we therefore drop
     the closure and goto cleanup with the error as the result, rather
     than returning early and skipping the restore. */
  if(0xc3==s(r)) {
    K closurescope=scope_cp(cs);
    if(E(closurescope)) { _k(r); r=closurescope; goto cleanup; }
    K *pc=px(closurescope); pc[3]=t(1,1);
    r=closure(r,cs,closurescope);
    _k(closurescope);
  }
  else if(0x80==s(r)) {
    K *pr=px(r);
    K v=pr[1];
    K *pvv=px(v);
    u32 c=0;
    i(n(v),if(0xc3==s(pvv[i]))++c)
    if(c&&c==n(v)) {
      K closurescope=scope_cp(cs);
      if(E(closurescope)) { _k(r); r=closurescope; goto cleanup; }
      K *pc=px(closurescope); pc[3]=t(1,1);
      i(n(v),pvv[i]=closure(pvv[i],cs,closurescope))
      _k(closurescope);
    }
  }

cleanup:
  pd=px(ps[1]);          /* ps[1] may have been refreshed during the body */
  if(ik(ps[5])==0) _k(pd[1]);
  pd[1]=pd1save;
  ps[5]=ps5save;
  nx=NX;
  n(pd[0])=n(pd[1]);
  _k(cs); cs=cs0;
  _k(f); _k(x);
  return r;
}

K fne(K f, K x, char *av) {
  if(0xd9==s(f)) return fapply(f,x,av);
  if(0xda==s(f)) return fapply(f,x,av);
  if(0xc3!=s(f)) return fapply(f,x,av);
  return fne_(f, x, (av && *av) ? av : "");
}

static K fapply_impl(K f, K x, char *av_outer);
K fapply(K f, K x, char *av_outer) {
  static int d=0;
  if(++d>maxr || (!(d&7)&&stack_low())) { --d; _k(f); _k(x); return KERR_STACK; }
  K r=fapply_impl(f,x,av_outer);
  --d;
  return r;
}
static K fapply_impl(K f, K x, char *av_outer) {
  if(0xda==s(f)) {
    K *pw=px(f);
    K wf=k_(pw[0]);
    K wav=pw[1];
    char av2[256];
    av2[0]=0;
    if(T(wav)==-3 && n(wav)>0) {
      char *p=px(wav);
      snprintf(av2+strlen(av2),sizeof(av2)-strlen(av2),"%s",p);
    }
    if(av_outer && *av_outer) {
      snprintf(av2+strlen(av2),sizeof(av2)-strlen(av2),"%s",av_outer);
    }
    _k(f);
    return fapply(wf,x,av2);
  }
  if(0xd9==s(f)) {
    /* Issue #2 Pass 3a / 3b-5: (f;args) projection wrapper.
       Two cases:
         1. lx >= val(f): we have enough args to either complete
            this projection or partially apply the inner -- merge
            inline and recurse fapply on the inner.
         2. lx < val(f): not enough new args -> nest as
            0xd9(f, new_args). This matches legacy 0xc4 nested
            projection semantics (fne_'s nx<val branch) and the
            way printers like k3 render f[;;2][1] as `f[;;2][1]`
            and f[1][1][1] with one bracket per partial apply,
            rather than flattening. */
    K vf2=val(f);
    if(E(vf2)) { _k(f); _k(x); return vf2; }
    i32 vn2=ik(vf2); _k(vf2);
    u64 lx=(0x81==s(x))?n(x):1;
    if(vn2>0 && lx<(u64)vn2) {
      /* Nest: build 0xd9(f, new_args).  Issue #2 Pass 5 step 3:
         if av_outer is non-empty (e.g. a chained `'` from
         eachfe/avdo dispatch), preserve it by lifting onto an
         outer 0xda wrapper around the nested projection.
         Without this, `f'[2]'[3]` would lose the outer each
         between partial-apply steps.  Issue #2 Pass 9 fix:
         x may be a borrowed params[] pool plist (c3_apply's
         `xx=params[paramsi++]` pattern -- the K is reused
         across statements with slot[0] overwritten on next
         borrow).  Copy the elements into a fresh 0x81 list so
         the long-lived projection owns its own args storage. */
      K args_new;
      if(0x81==s(x)) {
        K *pxk=px(x);
        u64 ln=n(x);
        K t=tn(0,ln); K *pt=px(t);
        i(ln, pt[i]=k_(pxk[i]))
        _k(x);
        args_new=st(0x81,t);
      }
      else { K t=tn(0,1); ((K*)px(t))[0]=x; args_new=st(0x81,t); }
      K nested=wrap_proj(f, args_new);
      if(av_outer && *av_outer) {
        K w=tn(0,2); K *pw=px(w);
        pw[0]=nested;
        pw[1]=tnv(3,strlen(av_outer),xmemdup(av_outer,1+strlen(av_outer)));
        return st(0xda,w);
      }
      return nested;
    }
    K *pw=px(f);
    K wf=pw[0];
    K wargs=pw[1];
    K merged=merge_args(wargs,x);
    if(E(merged)) { _k(f); return merged; }
    K wf_owned=k_(wf);
    _k(f);
    return fapply(wf_owned,merged,av_outer);
  }
  /* For 0xc3 lambdas: fne() is the canonical entry.
     0xc4 retired in Pass 4 -- replaced by 0xd9 above. */
  if(0xc3==s(f)) return fne(f,x,av_outer);
  /* For everything else (primitive verbs, builtins, etc.) route
     through fe() which already knows how to unpack a 0x81 plist. */
  if(0x81==s(x)) {
    if(n(x)==1) {
      K *pxk=px(x); K x0=k_(pxk[0]); _k(x);
      return fe(f,0,x0,av_outer);
    }
    if(n(x)==2) {
      K *pxk=px(x); K a=k_(pxk[0]); K x1=k_(pxk[1]); _k(x);
      return fe(f,a,x1,av_outer);
    }
    /* 3+ args: leave the plist intact for fe()'s primitive
       3/4-arg dispatch (kamendi3/4 etc.). */
    return fe(f,0,x,av_outer);
  }
  return fe(f,0,x,av_outer);
}

/* Issue #2 Pass 3a.
   Merge new args x into a projection's bound args plist p:
   - If p contains any inull slots, fill them left-to-right from x.
     Excess x elements (beyond the count of inulls in p) are an
     error (KERR_VALENCE).
   - If p has no inull slots, append x's elements (legacy
     projection-collapse semantics: f[1][2] -> f[1;2]).
   x may be either an 0x81 plist or a single K (treated as a
   1-element plist). The result is always an 0x81 plist.
   Takes ownership of x. p remains untouched (caller owns it). */
K merge_args(K p, K x) {
  K xa;
  if(0x81==s(x)) xa=x;
  else { K t=tn(0,1); ((K*)px(t))[0]=x; xa=st(0x81,t); }
  K *pp=px(p); K *pxa=px(xa);
  u64 lp=n(p), lx=n(xa);
  u64 ni=0, nn=0;
  for(u64 i=0;i<lp;++i) if(pp[i]==inull) ++ni; else ++nn;
  /* Legacy projection-collapse semantics (matches fne_ at the
     0xc4 inner-apply path): fill any inulls in p from xa
     left-to-right, then append remaining xa elements past the
     end of p. If lx < ni, return KERR_VALENCE (not enough new
     args to fill all open slots -- legacy 0xc4 behavior).
     A partial-fill projection should be built upstream as a
     fresh wrap_proj instead of trying to merge here. */
  if(lx<ni) { _k(xa); return KERR_VALENCE; }
  u64 lr=nn+lx;
  K r=tn(0,lr); K *pr=px(r);
  u64 xi=0, j=0;
  for(u64 i=0;i<lp;++i) {
    if(pp[i]==inull) pr[j++]=k_(pxa[xi++]);
    else pr[j++]=k_(pp[i]);
  }
  while(xi<lx) pr[j++]=k_(pxa[xi++]);
  _k(xa);
  return st(0x81,r);
}

/* Issue #2 Pass 3a.
   Build an 0xd9(f, args) projection wrapper. f and args are
   incorporated by ownership transfer (caller transfers refs in).
   args must be an 0x81 plist (may contain inulls).

   If f is itself an 0xd9 wrapper, this is a partial-of-partial
   application -- the canonical shape is to flatten by merging
   the new args into f's existing args. We do NOT flatten here;
   that's fapply's job. Producers that need flattening should call
   fapply(f, args, 0) instead. */
K wrap_proj(K f, K args) {
  K w=tn(0,2); K *pw=px(w);
  pw[0]=f;
  pw[1]=args;
  return st(0xd9,w);
}
