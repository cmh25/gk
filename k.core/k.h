#ifndef K_H
#define K_H

#include <limits.h>
#include <math.h>
#include "types.h"
#include "x.h"

/*
# K value k-core
     8      8    48 bits
    tx     st    px
63..56 55..48 47..0
*/
typedef u64 K;
typedef struct {
  u64 n;
  union { void *v; double f; i64 j; float e; short h; };
  i32 r;
} ko;


#define b(i) (((K)1<<(i))-1)
#define n(x) ((ko*)(b(48)&x))->n
#define T(x) ((char)((x)>>56))
#define tx T(x)
#define ax (tx>0)
#define nx n(x)
#define rx ((ko*)(b(48)&x))->r
#define ta T(a)
#define aa (ta>0)
#define na n(a)
#define nr n(r) 
#define np n(p) 
#define vr ((ko*)(b(48)&r))->v
#define i(b,z) {u64 n_=b;u64 i=0;while(i<n_){z;++i;}}
#define j(b,z) {u64 n_=b;u64 j=0;while(j<n_){z;++j;}}
#define i1(b,z) {u64 n_=b;u64 i=1;while(i<n_){z;++i;}}
#define j1(b,z) {u64 n_=b;u64 j=1;while(j<n_){z;++j;}}
#define t(t,z) ((K)(t)<<56|((K)(z)&b(56)))
#define ik(x) ((i32)(b(32)&x))
#define fk(x) (((ko*)(b(48)&x))->f)
#define ck(x) ((u8)(b(8)&(x)))
#define sk(x) ((char*)(b(48)&x))
#define s(x) (b(8)&x>>48)

#define px(x) (void*)(((ko*)(b(48)&(x)))->v)
#define PAI pai=(i32*)px(a)
#define PAF paf=(double*)px(a)
#define PAC pac=(char*)px(a)
#define PAS pas=(char**)px(a)
#define PAK pak=(K*)px(a)
#define PXI pxi=(i32*)px(x)
#define PXF pxf=(double*)px(x)
#define PXC pxc=(char*)px(x)
#define PXS pxs=(char**)px(x)
#define PXK pxk=(K*)px(x)
#define PRI(x) r=tn(1,(x));pri=(i32*)px(r)
#define PRF(x) r=tn(2,(x));prf=(double*)px(r)
#define PRC(x) r=tn(3,(x));prc=(char*)px(r)
#define PRS(x) r=tn(4,(x));prs=(char**)px(r)
#define PRK(x) r=tn(0,(x));prk=(K*)px(r)

#define KERR_NYI 0
#define KERR_RANK 1
#define KERR_LENGTH 2
#define KERR_TYPE 3
#define KERR_VALUE 4
#define KERR_RANGE 5
#define KERR_DOMAIN 6
#define KERR_VALENCE 7
#define KERR_INDEX 8
#define KERR_INT 9
#define KERR_PARSE 10
#define KERR_NONCE 11
#define KERR_STACK 12
#define KERR_RESERVED 13
#define KERR_WSFULL 14

#define E(x) ((x)<256||0x84==s(x))
#define EC(x) do { K t_=(x); if(E(t_)) { e=t_; goto cleanup; } } while(0);

#ifdef ASAN_ENABLED
#define VSIZE(x) do { if((x)<0||(x)>1000000000) return KERR_WSFULL; } while(0);
#else
#define VSIZE(x) do { if((x)<0||(x)==INT32_MAX) return KERR_WSFULL; } while(0);
#endif

/* floating point comparision */
static inline i32 cmpff(double a, double x) {
  return isnan(a) && !isnan(x) ? -1
       : isnan(x) && !isnan(a) ?  1
       : isnan(a) && isnan(x)  ?  0
       : (a)<(x) ? -1
       : (a)>(x) ?  1
       : 0;
}
#define GK_EPSILON 1e-13
static inline i32 cmpffe(double a, double b) {
  double da = a - b;
  //double tol = GK_EPSILON * fabs(a + b);
  double tol = GK_EPSILON * fmax(fabs(a + b), 1.0);

  return (da >  tol) ?  1
       : (da < -tol) ? -1
       : 0;
}
static inline i32 cmpfft(double a, double b) {
  if(isnan(a) && !isnan(b)) return -1;
  if(isnan(b) && !isnan(a)) return  1;
  if(isnan(a) &&  isnan(b)) return  0;

  if(isinf(a) && isinf(b)) {
    if(signbit(a) && !signbit(b)) return -1;  /* -inf < +inf */
    if(!signbit(a) && signbit(b)) return  1;
    return 0;
  }
  if(isinf(a)) return signbit(a) ? -1 : 1;
  if(isinf(b)) return signbit(b) ? 1 : -1;

  return cmpffe(a,b);
}

/* exact float comparison without tolerance - for group/unique operations */
static inline i32 cmpffx(double a, double b) {
  if(isnan(a) && !isnan(b)) return -1;
  if(isnan(b) && !isnan(a)) return  1;
  if(isnan(a) &&  isnan(b)) return  0;

  if(isinf(a) && isinf(b)) {
    if(signbit(a) && !signbit(b)) return -1;  /* -inf < +inf */
    if(!signbit(a) && signbit(b)) return  1;
    return 0;
  }
  if(isinf(a)) return signbit(a) ? -1 : 1;
  if(isinf(b)) return signbit(b) ? 1 : -1;

  return cmpff(a,b);  /* exact comparison, no tolerance */
}

static inline double fi(i32 x) {
  if(x == INT32_MAX)    return INFINITY;
  if(x == INT32_MIN)    return NAN;
  if(x == INT32_MIN+1)  return -INFINITY;
  return (double)x;
}

extern i32 precision;
extern K null,inull;
extern char* sp(char*);
extern void sf(void);

K k(i32 i, K a, K x);
K k_(K x);
void _k(K x);
K tn(i32 t, i32 n);
K tnv(i32 t, i32 n, void *v);
K t2(double x);
K ki(i32 i, K a, K x, i32 ai, i32 xi);
i32 kcmpr(K a, K x);
i32 kcmprz(K a, K x, i32 tol); /* tol: 1=use tolerance, 0=exact */
K kcp(K x);
K knorm(K x);
K kmix(K x);
K kresize(K x, i32 n);
u64 khash(K x);
K ksplit(char *b, char *c);
void kexit(void);

#endif /* K_H */
