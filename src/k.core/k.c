#include "k.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "v.h"
#include "av.h"
#include "x.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#endif

i32 precision=7;
K null=t(6,0);
K inull=t(10,0);

// P=":+-*%&|<>=~.!@?#_^,$'/\\"
K k(i32 i, K a, K x) {
  K r=0;
  if(a) { /* dyad */
    if(!x) { _k(a); return KERR_TYPE; }
    /* inline int-atom arithmetic/compare: FD[1..10] except % on two plain
       int atoms mirror the primitives' ta==1/tx==1 cases exactly -- PMT's
       u32 wrap (+ - *), mamo's pick-a-or-x (& |), lme's signed icmp
       (< > =), and match's value equality (~; two like-typed immediates
       are equal iff bit-equal).  Immediate atoms need no _k, so skip the
       dispatch chain and the primitives' prologues.  % stays on the
       dispatch path (float result, div-by-zero semantics).  Any other
       type/subtype falls through unchanged.  Revert = delete. */
    if((u32)(i-1)<3u && (a>>48)==0x100 && (x>>48)==0x100) {
      u32 va=(u32)a, vx=(u32)x;
      return t(1, 1==i ? va+vx : 2==i ? va-vx : va*vx);
    }
    if((u32)(i-5)<8u && 11!=i && (a>>48)==0x100 && (x>>48)==0x100) {
      i32 sa=(i32)(u32)a, sx_=(i32)(u32)x;
      switch(i) {
      case  5: return sa<sx_?a:x;         /* & min */
      case  6: return sa>sx_?a:x;         /* | max */
      case  7: return t(1,(u32)(sa<sx_)); /* < */
      case  8: return t(1,(u32)(sa>sx_)); /* > */
      case  9: return t(1,(u32)(sa==sx_));/* = */
      case 12: return t(1,(u32)modi(sa,sx_)); /* ! mod: modrot's own modi (v.h) */
      default: return t(1,(u32)(a==x));   /* ~ match */
      }
    }
    if(i>31&&i<64) r=each(i-32,a,x); /* a+'x */
    else if(i>63&&i<96) r=eachright(i-64,a,x); /* a+/x */
    else if(i>95&&i<128) r=eachleft(i-96,a,x); /* a+\x */
    else {
      switch(i) {
      case 20: if(a>20) { _k(a); _k(x); return KERR_TYPE; } r=each(a,0,x); break;
      case 21: if(a>20) { _k(a); _k(x); return KERR_TYPE; } r=overd(a,x); break;
      case 22: if(a>20) { _k(a); _k(x); return KERR_TYPE; } r=scand(a,x); break;
      default: if(i<0||i>=FDSIZE||!FD[i]) { _k(a); _k(x); return KERR_PARSE; } r=FD[i](a,x); break;
      }
    }
    if(i) { if(a>200) _k(a); _k(x); }
  }
  else { /* monad */
    if(!x) return KERR_TYPE;
    if(i<0||i>=FMSIZE||!FM[i]) r=KERR_PARSE;
    else r=FM[i](x);
    _k(x);
  }
  return r;
}

typedef struct { K x; i8 s; } SF;
static SF *g_stack;
static i64 g_cap;

static inline void push_sf(SF **stack, i64 *sp, i64 *cap, SF *local, i64 local_cap, K x, i32 s) {
  if(*sp == *cap) {
    i64 new_cap = (*cap ? *cap : local_cap) << 1;   /* i64: a >2^30-element flat list spills the whole breadth onto this stack */
    if(g_cap < new_cap) {
      g_stack = (SF*)xrealloc(g_stack, sizeof(SF)*new_cap);
      g_cap   = new_cap;
    }
    // first time we spill, copy the local frames to the global buffer
    if(*cap == local_cap) {
      memcpy(g_stack, local, sizeof(SF)*(*sp));
    }
    *stack = g_stack;
    *cap   = g_cap;
  }
  (*stack)[(*sp)++] = (SF){ x, s };
}

static void __k(K x) {
  // allocate a small local work stack (spills to g_stack if needed)
  ko *k=(ko*)(b(48)&x);
  if(tx) {
    if(k->m) {  // mapped object (from onecolon1: base = k->v-24, span 24+data)
      char *base=(char*)k->v - 24;
#ifdef _WIN32
      UnmapViewOfFile(base);  // unmaps the whole view from base; length implicit
#else
      size_t len;
      if(tx == -1) len = 24 + nx*sizeof(int);
      else if(tx == -2) len = 24 + nx*sizeof(double);
      else if(tx == -3) len = 24 + nx*sizeof(char);
      else { fprintf(stderr,"unexpected mmap'd type\n"); exit(1); }
      munmap(base, len);
#endif
      xfree(k);
    }
    else
    { if(tx!=2&&tx!=8) xfree(k->v); xfree(k); }
    return;
  }
  SF local[64],*stack=local;
  const i64 cap0=(i64)(sizeof(local)/sizeof(local[0]));
  i64 sp=0, cap=cap0;
  push_sf(&stack, &sp, &cap, local, cap, x, 0);
  while(sp) {
    SF *f=&stack[--sp];
    x=f->x;
    if(!kh(x)) continue;
    ko *k=(ko*)(b(48)&x);
    if(k->r>0) --k->r;
    else if(f->s==0) {
      stack[sp++].s=1;
      if(tx==0) {
        K *px=(K*)k->v;
        for(i64 i=k->n-1;i>=0;--i)
          push_sf(&stack, &sp, &cap, local, cap0, px[i], 0);
      }
    }
    else if(k->m) {  // mapped object (from onecolon1: base = k->v-24, span 24+data)
      char *base=(char*)k->v - 24;
#ifdef _WIN32
      UnmapViewOfFile(base);  // unmaps the whole view from base; length implicit
#else
      size_t len;
      if(tx == -1) len = 24 + nx*sizeof(int);
      else if(tx == -2) len = 24 + nx*sizeof(double);
      else if(tx == -3) len = 24 + nx*sizeof(char);
      else { fprintf(stderr,"unexpected mmap'd type\n"); exit(1); }
      munmap(base, len);
#endif
      xfree(k);
    }
    else { if(tx!=2&&tx!=8) xfree(k->v); xfree(k); }
  }
}
void _k(K x) {
  if(!kh(x)) return;
  ko *k=(ko*)(b(48)&x);
  if(k->r>0) { --k->r; return; }
  __k(x);
}

K tn(i32 t, i64 n) {
  K r=0; void *v=0;
  ko *k;
  switch(t) {
  case 0: v=xcalloc(n,sizeof(K)); break;
  case 1: v=xmalloc(sizeof(i32)*n); break;
  case 2: v=xmalloc(sizeof(double)*n); break;
  case 3: v=xcalloc(n+1,1); break;
  case 4: v=xmalloc(sizeof(char*)*n); break;
  case 8: v=xmalloc(sizeof(i64)*n); break;
  case 9: v=xmalloc(sizeof(float)*n); break;
  }
  k=xmalloc(sizeof(ko)); k->n=n; k->v=v; k->r=0; k->m=0; r=t(-t,k);
  return r;
}

K tnv(i32 t, i64 n, void *v) {
  K r=tn(t,1);
  ko* pr=(ko*)(b(48)&r);
  xfree(pr->v);
  pr->v=v;
  n(r)=n;
  return r;
}

K t2(double x) {
  ko *k=xmalloc(sizeof(ko)); k->n=0; k->f=x; k->r=0; k->m=0;
  return t(2,k);
}

K tj(i64 x) {
  ko *k=xmalloc(sizeof(ko)); k->n=0; k->j=x; k->r=0; k->m=0;
  return t(8,k);
}

extern i32 maxr;            /* eval depth cap (kinit lowers it under ASAN/wasm) */
extern int stack_lowcb(void); /* RSP stack guard, same as the eval sites */
#ifdef FUZZING
extern long gk_budget;      /* per-eval loop budget (p.c); reset in repl.c */
#endif
K ki(i32 i, K a, K x, i64 ai, i64 xi) {
  K r=0,*pak,*pxk,a_=0;
  char *pac,*pxc,**pas,**pxs;
  i32 *pai,*pxi;
  i64 *paj,*pxj;
  float *pae,*pxe;
  double *paf,*pxf;
  static i32 d=0;
  if(++d>maxr || (!(d&7)&&stack_lowcb())) { --d; return KERR_STACK; }
  if(a&&ai!=-1) { /* a is indexed */
    if(x&&xi!=-1) { /* x is indexed */
      switch(ta) {
      case  1: case 3: case 4: case 9: a_=a; break;
      case  2: case 8: a_=k_(a); break;
      case -1: PAI; a_=t(1,(u32)pai[ai]); break;
      case -2: PAF; a_=t2(paf[ai]); break;
      case -8: PAJ; a_=tj(paj[ai]); break;
      case -9: PAE; a_=te(pae[ai]); break;
      case -3: PAC; a_=t(3,(u8)pac[ai]); break;
      case -4: PAS; a_=t(4,(K)pas[ai]); break;
      case  0: PAK; a_=k_(pak[ai]); break;
      }
      switch(tx) {
      case  1: case 2: case 8: case 9: case 3: case 4: r=k(i,a_,x); break;
      case -1: PXI; r=k(i,a_,t(1,(u32)pxi[xi])); break;
      case -2: PXF; r=k(i,a_,t2(pxf[xi])); break;
      case -8: PXJ; r=k(i,a_,tj(pxj[xi])); break;
      case -9: PXE; r=k(i,a_,te(pxe[xi])); break;
      case -3: PXC; r=k(i,a_,t(3,(u8)pxc[xi])); break;
      case -4: PXS; r=k(i,a_,t(4,(K)pxs[xi])); break;
      case  0: PXK; r=k(i,a_,k_(pxk[xi])); break;
      }
    }
    else { /* x is not indexed */
      switch(ta) {
      case  1: case 3: case 4: case 9: r=k(i,a,k_(x)); break;
      case  2: case 8: r=k(i,k_(a),k_(x)); break;
      case -1: PAI; r=k(i,t(1,(u32)pai[ai]),k_(x)); break;
      case -2: PAF; r=k(i,t2(paf[ai]),k_(x)); break;
      case -8: PAJ; r=k(i,tj(paj[ai]),k_(x)); break;
      case -9: PAE; r=k(i,te(pae[ai]),k_(x)); break;
      case -3: PAC; r=k(i,t(3,(u8)pac[ai]),k_(x)); break;
      case -4: PAS; r=k(i,t(4,(K)pas[ai]),k_(x)); break;
      case  0: PAK; r=k(i,k_(pak[ai]),k_(x)); break;
      }
    }
  }
  else { /* a is not indexed */
    switch(tx) {
    case -1: PXI; r=k(i,k_(a),t(1,(u32)pxi[xi])); break;
    case -2: PXF; r=k(i,k_(a),t2(pxf[xi])); break;
    case -8: PXJ; r=k(i,k_(a),tj(pxj[xi])); break;
    case -9: PXE; r=k(i,k_(a),te(pxe[xi])); break;
    case -3: PXC; r=k(i,k_(a),t(3,(u8)pxc[xi])); break;
    case -4: PXS; r=k(i,k_(a),t(4,(K)pxs[xi])); break;
    case  0: PXK; r=k(i,k_(a),k_(pxk[xi])); break;
    }
  }
  --d;
  return r;
}

extern i32 kcmprcb(K a, K x);
i32 kcmpr(K a, K x) {
  i32 r=0,*pai,*pxi;
  double *paf,*pxf;
  char *pac,*pxc,**pas,**pxs;
  K *pak,*pxk;

  typedef struct { K a,x; size_t i; } sf;
  i32 sm=32,sp=0;
  sf *stack=xmalloc(sizeof(sf)*sm);
  stack[sp++]=(sf){a,x,0};
  while(sp) {
    sf *f=&stack[sp-1];
    a=f->a;
    x=f->x;

    if(a==x) { --sp; continue; } /* identical tagged word: same heap object or same inline atom -> equal (r stays 0); skips full element walk, e.g. converge fixed-point match(v,v) */
    if(!a) a=t(6,0);  /* empty keeper slot: the null atom, as kcp reads it.
       The literal (==null, k.c:16) rather than the global: it lets the static
       analyzer compute T()==6 and prove the word never reaches the ta==0
       deref arm, which the unknown-valued global does not. */
    if(!x) x=t(6,0);
    if(s(a)||s(x)) r=kcmprcb(a,x);
    else if(aa<ax) r=-1;
    else if(aa>ax) r= 1;
    else if(ta<tx) r=-1;
    else if(ta>tx) r= 1;
    else if(ta==1 && ik(a)<ik(x)) r=-1;
    else if(ta==1 && ik(a)>ik(x)) r=1;
    else if(ta==8 && jk(a)<jk(x)) r=-1;
    else if(ta==8 && jk(a)>jk(x)) r=1;
    else if(ta==9) r=cmpfft(ek(a),ek(x));
    else if(ta==2) r=cmpfft(fk(a),fk(x));
    else if(ta==3 && ck(a)<ck(x)) r=-1;
    else if(ta==3 && ck(a)>ck(x)) r=1;
    else if(ta==4) r=strcmp(sk(a),sk(x));
    else if(ta==-1) {
      PAI; PXI;
      for(size_t i=0;;++i) {
        if(i==na && i<nx) { r=-1; break; }
        if(i==nx && i<na) { r= 1; break; }
        if(i==na) break;
        if(pai[i]<pxi[i]) { r=-1; break; }
        if(pai[i]>pxi[i]) { r= 1; break; }
      }
    }
    else if(ta==-8) {
      i64 *paj,*pxj; PAJ; PXJ;
      for(size_t i=0;;++i) {
        if(i==na && i<nx) { r=-1; break; }
        if(i==nx && i<na) { r= 1; break; }
        if(i==na) break;
        if(paj[i]<pxj[i]) { r=-1; break; }
        if(paj[i]>pxj[i]) { r= 1; break; }
      }
    }
    else if(ta==-9) {
      float *pae,*pxe; PAE; PXE;
      for(size_t i=0;;++i) {
        if(i==na && i<nx) { r=-1; break; }
        if(i==nx && i<na) { r= 1; break; }
        if(i==na) break;
        r=cmpfft(pae[i],pxe[i]);
        if(r) break;
      }
    }
    else if(ta==-2) {
      PAF; PXF;
      for(size_t i=0;;++i) {
        if(i==na && i<nx) { r=-1; break; }
        if(i==nx && i<na) { r= 1; break; }
        if(i==na) break;
        r=cmpfft(paf[i],pxf[i]);
        if(r) break;
      }
    }
    else if(ta==-3) {
      PAC; PXC;
      for(size_t i=0;;++i) {
        if(i==na && i<nx) { r=-1; break; }
        if(i==nx && i<na) { r= 1; break; }
        if(i==na) break;
        if((u8)pac[i]<(u8)pxc[i]) { r=-1; break; }
        if((u8)pac[i]>(u8)pxc[i]) { r= 1; break; }
      }
    }
    else if(ta==-4) {
      PAS; PXS;
      for(size_t i=0;;++i) {
        if(i==na && i<nx) { r=-1; break; }
        if(i==nx && i<na) { r= 1; break; }
        if(i==na) break;
        i32 z=strcmp(pas[i], pxs[i]);
        if(z<0) { r=-1; break; }
        if(z>0) { r= 1; break; }
      }
    }
    else if(ta==0) {
      PAK; PXK;
      /* element-first, length only as the tiebreak -- exactly as every
         flat-vector arm above.  The old `if(na!=nx)` upfront short-circuit
         ordered RAGGED nested lists by length, so `<((1 2;9);(1 2;3;4))`
         came out length-first (0 1) instead of element-first (1 0). */
      u64 mn = na<nx?na:nx;
      if(f->i==mn) {
        if(na<nx) r=-1;
        else if(na>nx) r=1;
        else { --sp; continue; }   /* common prefix exhausted, equal length: equal */
        break;
      }
      K ai=pak[f->i],xi=pxk[f->i];
      ++f->i;
      if(!ai) ai=t(6,0);  /* empty keeper slot (see top of loop) */
      if(!xi) xi=t(6,0);
      if(s(ai)||s(xi)) {
        r=kcmprcb(ai,xi);
        if(r) break;
      }
      else {
        if(sp==sm) stack=xrealloc(stack,sizeof(sf)*(sm*=2));
        stack[sp++]=(sf){ai,xi,0};
      }
      continue;
    }
    if(r) break;
    --sp;
  }
  xfree(stack);
  return r;
}

extern K kcpcb(K x);
K kcp(K x) {
  K r=0,e=0,*prk,*pxk;
  char *prc,*pxc,**prs,**pxs;
  i32 *pri,*pxi,sm=32,sp=0;
  i64 *prj,*pxj;
  float *pre,*pxe;
  double *prf,*pxf;
  typedef struct { K r,x; size_t i; } sf;
  sf *stack;
  if(!x) return 0;
  if(s(x)) return kcpcb(x);
  switch(tx) {
  case  1: r=x; break;
  case  2: r=k_(x); break;
  case  8: r=k_(x); break;
  case  9: r=x; break;
  case  3: r=x; break;
  case  4: r=x; break;
  case  6: case 10: r=x; break;
  case -1: PRI(nx); PXI; i(nx,*pri++=*pxi++) break;
  case -2: PRF(nx); PXF; i(nx,*prf++=*pxf++) break;
  case -8: PRJ(nx); PXJ; i(nx,*prj++=*pxj++) break;
  case -9: PRE(nx); PXE; i(nx,*pre++=*pxe++) break;
  case -3: PRC(nx); PXC; i(nx,*prc++=*pxc++) break;
  case -4: PRS(nx); PXS; i(nx,*prs++=*pxs++) break;
  case  0:
    stack=xmalloc(sizeof(sf)*sm);
    stack[sp++]=(sf){tn(0,nx),x,0};
    while(sp) {
      sf *f=&stack[sp-1];
      K r_=f->r;
      K x_=f->x;
      size_t i=f->i;
      size_t n=n(x_);
      pxk=px(x_);
      prk=px(r_);
      while(i<n) {
        K xi=pxk[i];
        if(!xi) xi=null; /* TODO: is this really right? */
        if(s(xi)) {
          prk[i]=kcpcb(xi);
          if(E(prk[i])) { e=prk[i]; prk[i]=0; goto cleanup; }
        }
        else if(T(xi)) {
          prk[i]=kcp(xi);
          if(E(prk[i])) { e=prk[i]; prk[i]=0; goto cleanup; }
        }
        else {
          if(sp==sm) stack=xrealloc(stack,sizeof(sf)*(sm*=2));
          stack[sp-1].i=i;
          stack[sp++]=(sf){tn(0,n(xi)),xi,0};
          goto continue_outer;
        }
        ++i;
      }
      if(--sp==0) { r=r_; break; }
      ((K*)px(stack[sp-1].r))[stack[sp-1].i]=r_;
      stack[sp-1].i++;
continue_outer:;
    }
    xfree(stack);
    r|=(K)s(x)<<48;
    break;
  default: r=KERR_TYPE;
  }
  return r;
cleanup:
  if(r) _k(r);
  while(sp--) _k(stack[sp].r);
  xfree(stack);
  return e;
}

K knorm(K x) {
  K r=x,*pxk;
  i32 *pri;
  i64 *prj;
  float *pre;
  double *prf;
  char *prc,**prs; i8 t;
  if(x<20) return x; /* error */
  if(s(x)) return x;
  if(!tx&&nx) {
    PXK;
    t=T(pxk[0]);
    if(t<=0||s(pxk[0])) return r;
    i(nx,if(s(pxk[i])||t!=T(pxk[i])) return r)
    switch(t) {
    case 1: PRI(nx); i(nx,pri[i]=ik(pxk[i])); _k(x); break;
    case 2: PRF(nx); i(nx,prf[i]=fk(pxk[i])); _k(x); break;
    case 8: PRJ(nx); i(nx,prj[i]=jk(pxk[i])); _k(x); break;
    case 9: PRE(nx); i(nx,pre[i]=ek(pxk[i])); _k(x); break;
    case 3: PRC(nx); i(nx,prc[i]=ck(pxk[i])); _k(x); break;
    case 4: PRS(nx); i(nx,prs[i]=sk(pxk[i])); _k(x); break;
    }
  }
  return r;
}

K kmix(K x) {
  K r=3,*prk;
  i32 *pxi;
  i64 *pxj;
  float *pxe;
  double *pxf;
  char *pxc,**pxs;
  if(s(x)||!tx||tx==2) return kcp(x);
  PRK(nx);
  switch(tx) {
  case -1: PXI; i(nx,prk[i]=t(1,(u32)pxi[i])) break;
  case -2: PXF; i(nx,prk[i]=t2(pxf[i])) break;
  case -8: PXJ; i(nx,prk[i]=tj(pxj[i])) break;
  case -9: PXE; i(nx,prk[i]=te(pxe[i])) break;
  case -3: PXC; i(nx,prk[i]=t(3,(u8)pxc[i])) break;
  case -4: PXS; i(nx,prk[i]=t(4,pxs[i])) break;
  //default: printf("unhandled case in kmix [%d]\n", tx); break;
  }
  return r;
}

K kresize(K x, i64 n) {
  ko *k=(ko*)(b(48)&x);
  switch(tx) {
  case -1: k->v=xrealloc(k->v,n*sizeof(i32)); break;
  case -2: k->v=xrealloc(k->v,n*sizeof(double)); break;
  case -8: k->v=xrealloc(k->v,n*sizeof(i64)); break;
  case -9: k->v=xrealloc(k->v,n*sizeof(float)); break;
  case -3: k->v=xrealloc(k->v,n+1); ((char*)k->v)[n]=0; break;
  case -4: k->v=xrealloc(k->v,n*sizeof(char*)); break;
  case  0: k->v=xrealloc(k->v,n*sizeof(K)); break;
  }
  k->n=n;
  return x;
}

extern u64 khashcb(K x);
u64 khash(K x) {
  u64 r=2654435761,bits;
  K *pxk;
  i32 *pxi;
  i64 *pxj;
  float *pxe,ef;
  u32 fb;
  double *pxf,f;
  char *pxc,**pxs;
  static i32 d=0;
  /* Empty slots are real: parser keeper nodes (0xd0 and friends) carry NULL in
     unfilled channels, and khashcb strips such a node to its payload and hands
     it back here as a plain general list.  NULL reads as t0/no-subtype, so the
     case-0 walk used to push it and then deref it via n(x_).  Read it as null,
     the same substitution kcp makes, so a keeper still hashes and compares
     equal to its copy. */
  if(!x) x=null;
  if(++d>maxr || (!(d&7)&&stack_lowcb())) { --d; return KERR_STACK; }
#ifdef FUZZING
  /* Per-top-level-call node budget (reset on the outermost entry, d==1).  It
     bounds a single khash over a shared/cyclic DAG -- exponential-but-finite to
     walk (a converge builds one cheaply, e.g. (,?,)''/ ) -- WITHOUT the
     cross-call non-determinism a global budget caused: a global one, spent
     across the many khash calls of one group/in/unique, made an equal value
     hash to its real slot before exhaustion and to KERR_STACK after, so the
     count and probe passes disagreed (a group() heap overflow).  Resetting per
     call makes khash a pure function of its argument again -- the "tolerate any
     u64, resolve by kcmpr" contract holds because equal values now always hash
     alike within an operation. */
  static long khash_budget;
  if(d==1) khash_budget=1000000L;
#endif
  /* hold d ACROSS the callback: khashcb re-enters khash for the subtype's
     payload, so releasing the depth first made the mutual recursion run at a
     constant d -- neither the depth guard nor the budget (reset on d==1) could
     ever fire, and a cyclic subtyped value blew the stack. */
  if(s(x)) { r=khashcb(x); --d; return r; }
  switch(tx) {
  case  1: r=r+hmul((u32)ik(x)); break;
  case  2: f=fk(x); if(f==0) f=0.0; /* -0.0 = 0.0 but bits differ: hash=eq */
           memcpy(&bits,&f,8); r=r+hmul(bits); break;
  case  8: r=r+hmul((u64)jk(x)); break;
  case  9: ef=ek(x); if(ef==0) ef=0.0f;
           memcpy(&fb,&ef,4); r=r+hmul((u64)fb); break;
  case  3: r=r+hmul((u64)ck(x)); break;
  case  4: r=r+xfnv1a(sk(x),strlen(sk(x))); break;
  case  6: case 10: break;
  case  0: {
    if(s(x)) { r=khashcb(x); --d; return r; }
    typedef struct { K x; u64 h; size_t i; } sf;
    i32 sm=32,sp=0;
    sf *stack=xmalloc(sizeof(sf)*sm);
    stack[sp++]=(sf){x,2654435761,0};
    while(sp) {
      sf *f=&stack[sp-1];
      K x_=f->x;
      size_t i=f->i;
      size_t n=n(x_);
      pxk=px(x_);
      while(i<n) {
        K xi=pxk[i];
        i8 t=T(xi);
#ifdef FUZZING
        /* Charge the per-call node budget (see entry); bail like the depth
           guard (KERR_STACK as the hash -- unique/group tolerate any u64 and
           compare on collision, and the budget resets per call so equal values
           bail identically). */
        if(--khash_budget<0) { xfree(stack); --d; return KERR_STACK; }
#endif
        if(!xi) { xi=null; t=T(xi); }  /* empty keeper slot (see entry) */
        if(t==0 && !s(xi)) {
          if(sp==sm) stack=xrealloc(stack,sizeof(sf)*(sm*=2));
          stack[sp-1].i=i;
          stack[sp++]=(sf){xi,2654435761,0};
          goto continue_outer;
        }
        else stack[sp-1].h ^= stack[sp-1].h+khash(xi);
        ++i;
      }
      if(--sp==0) { r=stack[0].h; break; }
      stack[sp-1].h ^= stack[sp-1].h+stack[sp].h;
      stack[sp-1].i++;
  continue_outer:;
    }
    xfree(stack);
    break;
  }
  case -1: PXI; i(nx,r^=r+hmul((u32)pxi[i])) break;
  case -2: PXF; i(nx,f=pxf[i]; if(f==0) f=0.0; memcpy(&bits,&f,8); r^=r+hmul(bits)) break;
  case -8: PXJ; i(nx,r^=r+hmul((u64)pxj[i])) break;
  case -9: PXE; i(nx,ef=pxe[i]; if(ef==0) ef=0.0f; memcpy(&fb,&ef,4); r^=r+hmul((u64)fb)) break;
  case -3: PXC; r=r+xfnv1a(pxc,nx); break; /* per-char r^=r+hmul(c) mixed
    too weakly: grouping 100k digit strings probed quadratically */
  case -4: PXS; i(nx,r^=r+xfnv1a(pxs[i],strlen(pxs[i]))) break;
  default:
    fprintf(stderr,"error: unsupported type in khash()\n");
    exit(1);
  }
  --d;
  return r;
}

K ksplit(char *b, char *c) {
  K r=0,*prk;
  size_t n=0,m=32;
  char *cur=b;
  PRK(m);
  for(;;) {
    char *sep=strpbrk(cur,c);
    size_t len=sep?(size_t)(sep-cur):strlen(cur);
    if(n==m) { m<<=1; r=kresize(r,m); prk=px(r); }
    char *copy=xmalloc(len+1);
    if(len) memcpy(copy,cur,len);
    copy[len]=0;
    prk[n++]=tnv(3,len,copy);
    if(!sep) break;
    /* merge CRLF or LFCR pairs as a single separator */
    if((sep[0]=='\r' && sep[1]=='\n') ||
       (sep[0]=='\n' && sep[1]=='\r')) sep++;

    cur=sep+1;
  }
  n(r)=n;
  return r;
}

void kexit(void) {
  xfree(g_stack);
}
