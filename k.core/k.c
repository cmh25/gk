#include "k.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "v.h"
#include "av.h"
#include "x.h"
#ifndef _WIN32
#include <sys/mman.h>
#endif

i32 precision=7;
K null=t(6,0);
K inull=t(16,0);

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

K k_(K x) {
  if(E(x)||(tx>0 && tx!=2)) return x;
  ko *k=(ko*)(b(48)&x);
  k->r+=k->r>-2?1:-1;
  return x;
}

typedef struct { K x; i32 s; } SF;
static SF *g_stack;
static i32 g_cap;

static inline void push_sf(SF **stack, i32 *sp, i32 *cap, SF *local, i32 local_cap, K x, i32 s) {
  if(*sp == *cap) {
    i32 new_cap = (*cap ? *cap : local_cap) << 1;
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

void _k(K x) {
  if(E(x)||(tx>0 && tx!=2)) return;
  // allocate a small local work stack (spills to g_stack if needed)
  SF local[64],*stack=local;
  i32 sp=0, cap=(i32)(sizeof(local)/sizeof(local[0]));
  push_sf(&stack, &sp, &cap, local, cap, x, 0);
  while(sp) {
    SF *f=&stack[--sp];
    x=f->x;
    if(E(x)||(tx>0 && tx!=2)) continue;
    ko *k=(ko*)(b(48)&x);
    if(k->r>0) { --k->r; continue; }
    else if(k->r<-2) { k->r++; continue; }
#ifndef _WIN32  // TODO: mmap on windows?
    else if(k->r==-2) {  // mapped object
      size_t len;
      if(tx == -1) len = 12 + nx*sizeof(int);
      else if(tx == -2) len = nx*sizeof(double);
      else if(tx == -3) len = nx*sizeof(char);
      else { fprintf(stderr,"unexpected mmap'd type\n"); exit(1); }
      munmap((char*)k->v - 20, len);
      xfree(k);
      continue;
    }
#endif
    else if(f->s==0) {
      push_sf(&stack, &sp, &cap, local, (i32)(sizeof local/sizeof local[0]), x, 1);
      if(tx==0) {
        K *px=(K*)k->v;
        for(i64 i=k->n-1;i>=0;--i)
          push_sf(&stack, &sp, &cap, local, (i32)(sizeof local/sizeof local[0]), px[i], 0);
      }
    }
    else {
      if(tx!=2) xfree(k->v);
      xfree(k);
    }
  }
}

K tn(i32 t, i32 n) {
  K r=0; void *v=0;
  ko *k;
  switch(t) {
  case 0: v=xcalloc(n,sizeof(K)); break;
  case 1: v=xmalloc(sizeof(i32)*n); break;
  case 2: v=xmalloc(sizeof(double)*n); break;
  case 3: v=xcalloc(n+1,1); break;
  case 4: v=xmalloc(sizeof(char*)*n); break;
  }
  k=xmalloc(sizeof(ko)); k->n=n; k->v=v; k->r=0; r=t(-t,k);
  return r;
}

K tnv(i32 t, i32 n, void *v) {
  K r=tn(t,1);
  ko* pr=(ko*)(b(48)&r);
  xfree(pr->v);
  pr->v=v;
  n(r)=n;
  return r;
}

K t2(double x) {
  ko *k=xmalloc(sizeof(ko)); k->n=0; k->f=x; k->r=0;
  return t(2,k);
}

K ki(i32 i, K a, K x, i32 ai, i32 xi) {
  K r=0,*pak,*pxk,a_=0;
  char *pac,*pxc,**pas,**pxs;
  i32 *pai,*pxi;
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
      case  1: case 3: case 4: a_=a; break;
      case  2: a_=k_(a); break;
      case -1: PAI; a_=t(1,(u32)pai[ai]); break;
      case -2: PAF; a_=t2(paf[ai]); break;
      case -3: PAC; a_=t(3,(u8)pac[ai]); break;
      case -4: PAS; a_=t(4,(K)pas[ai]); break;
      case  0: PAK; a_=k_(pak[ai]); break;
      }
      switch(tx) {
      case  1: case 2: case 3: case 4: r=k(i,a_,x); break;
      case -1: PXI; r=k(i,a_,t(1,(u32)pxi[xi])); break;
      case -2: PXF; r=k(i,a_,t2(pxf[xi])); break;
      case -3: PXC; r=k(i,a_,t(3,(u8)pxc[xi])); break;
      case -4: PXS; r=k(i,a_,t(4,(K)pxs[xi])); break;
      case  0: PXK; r=k(i,a_,k_(pxk[xi])); break;
      }
    }
    else { /* x is not indexed */
      switch(ta) {
      case  1: case 3: case 4: r=k(i,a,k_(x)); break;
      case  2: r=k(i,k_(a),k_(x)); break;
      case -1: PAI; r=k(i,t(1,(u32)pai[ai]),k_(x)); break;
      case -2: PAF; r=k(i,t2(paf[ai]),k_(x)); break;
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

    if(s(a)||s(x)) r=kcmprcb(a,x);
    else if(aa<ax) r=-1;
    else if(aa>ax) r= 1;
    else if(ta<tx) r=-1;
    else if(ta>tx) r= 1;
    else if(ta==1 && ik(a)<ik(x)) r=-1;
    else if(ta==1 && ik(a)>ik(x)) r=1;
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
  double *prf,*pxf;
  typedef struct { K r,x; size_t i; } sf;
  sf *stack;
  if(!x) return 0;
  if(s(x)) return kcpcb(x);
  switch(tx) {
  case  1: r=x; break;
  case  2: r=k_(x); break;
  case  3: r=x; break;
  case  4: r=x; break;
  case  6: case 16: r=x; break;
  case -1: PRI(nx); PXI; i(nx,*pri++=*pxi++) break;
  case -2: PRF(nx); PXF; i(nx,*prf++=*pxf++) break;
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
  double *prf;
  char t,*prc,**prs;
  if(x<20) return x; /* error */
  if(s(x)) return x;
  if(!tx&&nx) {
    PXK;
    t=T(pxk[0]);
    i(nx,if(s(pxk[i])||t!=T(pxk[i])) return r)
    switch(t) {
    case 1: PRI(nx); i(nx,pri[i]=ik(pxk[i])); _k(x); break;
    case 2: PRF(nx); i(nx,prf[i]=fk(pxk[i])) ; _k(x);break;
    case 3: PRC(nx); i(nx,prc[i]=ck(pxk[i])); _k(x); break;
    case 4: PRS(nx); i(nx,prs[i]=sk(pxk[i])) ; _k(x);break;
    }
  }
  return r;
}

K kmix(K x) {
  K r=3,*prk;
  i32 *pxi;
  double *pxf;
  char *pxc,**pxs;
  if(!tx||tx==2) return kcp(x);
  PRK(nx);
  switch(tx) {
  case -1: PXI; i(nx,prk[i]=t(1,(u32)pxi[i])) break;
  case -2: PXF; i(nx,prk[i]=t2(pxf[i])) break;
  case -3: PXC; i(nx,prk[i]=t(3,(u8)pxc[i])) break;
  case -4: PXS; i(nx,prk[i]=t(4,pxs[i])) break;
  //default: printf("unhandled case in kmix [%d]\n", tx); break;
  }
  return r;
}

K kresize(K x, i32 n) {
  ko *k=(ko*)(b(48)&x);
  switch(tx) {
  case -1: k->v=xrealloc(k->v,n*sizeof(i32)); break;
  case -2: k->v=xrealloc(k->v,n*sizeof(double)); break;
  case -3: k->v=xrealloc(k->v,n); break;
  case -4: k->v=xrealloc(k->v,n*sizeof(char*)); break;
  case  0: k->v=xrealloc(k->v,n*sizeof(K)); break;
  }
  k->n=n;
  return x;
}

extern u64 khashcb(K x);
u64 khash(K x) {
  u64 r=2654435761;
  K *pxk;
  i32 *pxi;
  double *pxf;
  char *pxc,**pxs;
  if(s(x)) return khashcb(x);
  switch(tx) {
  case  1: r=r+(u64)ik(x)*2654435761; break;
  case  2: r=r+(u64)fk(x)*2654435761; break;
  case  3: r=r+(u64)ck(x)*2654435761; break;
  case  4: r=r+xfnv1a(sk(x),strlen(sk(x))); break;
  case  6: case 16: break;
  case  0: {
    if(s(x)) return khashcb(x);
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
        char t=T(xi);
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
  case -1: PXI; i(nx,r^=r+(u64)pxi[i]*2654435761) break;
  case -2: PXF; i(nx,r^=r+(u64)pxf[i]*2654435761) break;
  case -3: PXC; i(nx,r^=r+(u64)pxc[i]*2654435761) break;
  case -4: PXS; i(nx,r^=r+xfnv1a(pxs[i],strlen(pxs[i]))) break;
  default:
    fprintf(stderr,"error: unsupported type in khash()\n");
    exit(1);
  }
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
