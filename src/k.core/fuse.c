#include "fuse.h"
#include "v.h"
#include "x.h"
#include <string.h>

/* Fused &a<x / &a>x / &a=x */
#define WC_RUN(PRED)                                                      \
  do {                                                                    \
    i64 j=0;                                                              \
    if(N>BIGV) {                                                          \
      i64 tot=0;                                                          \
      for(u64 i=0;i<N;++i) tot+=(PRED);                                   \
      r=tn(8,tot); { i64 *p=(i64*)px(r);                                  \
        for(u64 i=0; j<tot; ++i){ p[j]=(i64)i; j+=(PRED); } }             \
    } else {                                                              \
      r=tn(1,N); { i32 *p=(i32*)px(r);                                    \
        for(u64 i=0;i<N;++i){ p[j]=(i32)i; j+=(PRED); } }                 \
      if(j<(i64)N) r=kresize(r,j); else n(r)=j;                           \
    }                                                                     \
  } while(0)

int wherecmp(K a, K x, i8 op, K *out) {
  K r=0; u64 N;
  if(s(a)||s(x)||ta>=0) return 0;      /* left operand must be a plain vector */
  N=na;
  if(N>VMAX) return 0;
  if     (ta==-1&&tx== 1){ i32 *pa=(i32*)px(a);    i32 c=ik(x);          WC_RUN(icmp(pa[i],c,op)); }
  else if(ta==-8&&tx== 8){ i64 *pa=(i64*)px(a);    i64 c=jk(x);          WC_RUN(jcmp(pa[i],c,op)); }
  else if(ta==-2&&tx== 2){ double *pa=(double*)px(a); double c=fk(x);    WC_RUN(cmpfft(pa[i],c)==op); }
  else if(ta==-9&&tx== 9){ float *pa=(float*)px(a);   double c=(double)ek(x); WC_RUN(cmpfft((double)pa[i],c)==op); }
  else if(ta==-1&&tx==-1&&na==nx){ i32 *pa=(i32*)px(a),*pb=(i32*)px(x);  WC_RUN(icmp(pa[i],pb[i],op)); }
  else if(ta==-8&&tx==-8&&na==nx){ i64 *pa=(i64*)px(a),*pb=(i64*)px(x);  WC_RUN(jcmp(pa[i],pb[i],op)); }
  else if(ta==-2&&tx==-2&&na==nx){ double *pa=(double*)px(a),*pb=(double*)px(x); WC_RUN(cmpfft(pa[i],pb[i])==op); }
  else if(ta==-9&&tx==-9&&na==nx){ float *pa=(float*)px(a),*pb=(float*)px(x);    WC_RUN(cmpfft((double)pa[i],(double)pb[i])==op); }
  else return 0;
  *out=knorm(r);
  return 1;
}

/* Fast `,/x` (join-over / raze) for a general list x (Tx==0, nx>=2).
   The generic fold does r=r,x[i] one item at a time -- O(n^2) copying.
   We always know the result up front and build it in a single O(total) pass:
     - all items general lists (T==0)  -> concatenate (share) into one list
     - all items the same base scalar type (vectors and/or atoms of one of
       int/float/char/sym/long/real) -> one flat vector of that type
     - otherwise -> box every leaf into a general list (gk's join never
       promotes numeric types; it boxes), then knorm -- exactly join()'s
       box path.  This replaces the O(n^2) generic fold even for the mixed
       case (e.g. `(...)`,1.0), which would otherwise re-box and re-copy a
       growing accumulator quadratically.
   Only a subtyped item (dict/function/...) returns 0 to fall through to the
   generic fold, which knows those valences.  Results match join()
   byte-for-byte (including the no-knorm pure-general-list case). */
K raze_(K x) {
  K *pxk=(K*)px(x);
  u64 nx_=n(x),total=0,j=0;
  i8 B=0;          /* common scalar base type among non-list items (0=unset) */
  int hasgl=0;     /* saw a general-list item (T==0) */
  int hasva=0;     /* saw a vector/atom item (T!=0) */
  int mismatch=0;  /* base types disagree */
  i(nx_, K xi=pxk[i];
        if(s(xi)) return 0;             /* subtyped item -> bail to generic */
        i8 t=T(xi);
        total += t<=0 ? n(xi) : 1;
        if(t==0) hasgl=1;
        else { i8 b=t<0?(i8)-t:t; if(!B) B=b; else if(B!=b) mismatch=1; hasva=1; })
  if(hasgl && !hasva) {                  /* pure general lists -> share-concat, no knorm */
    K r=tn(0,total); K *pr=(K*)px(r);
    i(nx_, K xi=pxk[i]; K *pe=px(xi); u64 ni=n(xi); u64 q=0; while(q<ni) pr[j++]=k_(pe[q++]);)
    return r;
  }
  if(!hasgl && !mismatch) switch(B) {    /* uniform scalar base -> flat vector */
  case 1: { K r=tn(1,total); i32  *pr=px(r); i(nx_, K xi=pxk[i]; if(T(xi)<0){u64 ni=n(xi); memcpy(pr+j,px(xi),ni*sizeof(i32));    j+=ni;} else pr[j++]=ik(xi);) return r; }
  case 2: { K r=tn(2,total); double*pr=px(r); i(nx_, K xi=pxk[i]; if(T(xi)<0){u64 ni=n(xi); memcpy(pr+j,px(xi),ni*sizeof(double)); j+=ni;} else pr[j++]=fk(xi);) return r; }
  case 3: { K r=tn(3,total); char  *pr=px(r); i(nx_, K xi=pxk[i]; if(T(xi)<0){u64 ni=n(xi); memcpy(pr+j,px(xi),ni);              j+=ni;} else pr[j++]=ck(xi);) return r; }
  case 4: { K r=tn(4,total); char **pr=px(r); i(nx_, K xi=pxk[i]; if(T(xi)<0){u64 ni=n(xi); memcpy(pr+j,px(xi),ni*sizeof(char*));  j+=ni;} else pr[j++]=sk(xi);) return r; }
  case 8: { K r=tn(8,total); i64   *pr=px(r); i(nx_, K xi=pxk[i]; if(T(xi)<0){u64 ni=n(xi); memcpy(pr+j,px(xi),ni*sizeof(i64));    j+=ni;} else pr[j++]=jk(xi);) return r; }
  case 9: { K r=tn(9,total); float *pr=px(r); i(nx_, K xi=pxk[i]; if(T(xi)<0){u64 ni=n(xi); memcpy(pr+j,px(xi),ni*sizeof(float));  j+=ni;} else pr[j++]=ek(xi);) return r; }
  }
  /* mixed types -> box each leaf into a general list (matches join's box path) */
  { K r=tn(0,total); K *pr=(K*)px(r);
    i(nx_, K xi=pxk[i]; i8 t=T(xi);
          if(t<=0){ u64 ni=n(xi); u64 q=0; while(q<ni) pr[j++]=xi_(xi,q++,t); }
          else pr[j++]=xi_(xi,0,t);)
    return knorm(r); }
}
