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

K ki(i32 i, K a, K x, i64 ai, i64 xi) {
  K r=0,*pak,*pxk,a_=0;
  char *pac,*pxc,**pas,**pxs;
  i32 *pai,*pxi;
  i64 *paj,*pxj;
  float *pae,*pxe;
  double *paf,*pxf;
  static i32 d=0;
#ifdef ASAN_ENABLED
  if(++d>40) { --d; return KERR_STACK; }
#else
  if(++d>100) { --d; return KERR_STACK; }
#endif
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
/* tol: 1=use float tolerance, 0=exact comparison */
i32 kcmprz(K a, K x, i32 tol) {
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
    if(s(a)||s(x)) r=kcmprcb(a,x);
    else if(aa<ax) r=-1;
    else if(aa>ax) r= 1;
    else if(ta<tx) r=-1;
    else if(ta>tx) r= 1;
    else if(ta==1 && ik(a)<ik(x)) r=-1;
    else if(ta==1 && ik(a)>ik(x)) r=1;
    else if(ta==8 && jk(a)<jk(x)) r=-1;
    else if(ta==8 && jk(a)>jk(x)) r=1;
    else if(ta==9) r=tol?cmpffte(ek(a),ek(x)):cmpffx(ek(a),ek(x));
    else if(ta==2) r=tol?cmpfft(fk(a),fk(x)):cmpffx(fk(a),fk(x));
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
        r=tol?cmpffte(pae[i],pxe[i]):cmpffx(pae[i],pxe[i]);
        if(r) break;
      }
    }
    else if(ta==-2) {
      PAF; PXF;
      for(size_t i=0;;++i) {
        if(i==na && i<nx) { r=-1; break; }
        if(i==nx && i<na) { r= 1; break; }
        if(i==na) break;
        r=tol?cmpfft(paf[i],pxf[i]):cmpffx(paf[i],pxf[i]);
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
      if(na!=nx) { r=(na<nx)?-1:1; break; }
      PAK; PXK;
      if(f->i==na) { --sp; continue; }
      K ai=pak[f->i],xi=pxk[f->i];
      ++f->i;
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

i32 kcmpr(K a, K x) {
  return kcmprz(a,x,1); /* use tolerance */
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
  if(!tx||tx==2) return kcp(x);
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
  case -3: k->v=xrealloc(k->v,n); break;
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
#ifdef ASAN_ENABLED
  if(++d>40) { --d; return KERR_STACK; }
#else
  if(++d>100) { --d; return KERR_STACK; }
#endif
  if(s(x)) { --d; return khashcb(x); }
  switch(tx) {
  case  1: r=r+(u64)(u32)ik(x)*2654435761U; break;
  case  2: f=fk(x); memcpy(&bits,&f,8); r=r+bits*2654435761U; break;
  case  8: r=r+(u64)jk(x)*2654435761U; break;
  case  9: ef=ek(x); memcpy(&fb,&ef,4); r=r+(u64)fb*2654435761U; break;
  case  3: r=r+(u64)ck(x)*2654435761U; break;
  case  4: r=r+xfnv1a(sk(x),strlen(sk(x))); break;
  case  6: case 10: break;
  case  0: {
    if(s(x)) { --d; return khashcb(x); }
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
  case -1: PXI; i(nx,r^=r+(u64)(u32)pxi[i]*2654435761U) break;
  case -2: PXF; i(nx,memcpy(&bits,&pxf[i],8); r^=r+bits*2654435761U) break;
  case -8: PXJ; i(nx,r^=r+(u64)pxj[i]*2654435761U) break;
  case -9: PXE; i(nx,memcpy(&fb,&pxe[i],4); r^=r+(u64)fb*2654435761U) break;
  case -3: PXC; i(nx,r^=r+(u64)pxc[i]*2654435761U) break;
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
