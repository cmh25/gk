#include "av.h"
#include <string.h>

extern i32 vstcb(K x);

static inline i32 av_isel(i32 a, i32 b, i8 op) {
  return op < 0 ? (a < b ? a : b) : (a > b ? a : b);
}
static inline i64 av_jsel(i64 a, i64 b, i8 op) {
  return op < 0 ? (a < b ? a : b) : (a > b ? a : b);
}
static inline float av_esel(float a, float b, i8 op) {
  return cmpff(a, b) == op ? a : b;
}
static inline double av_fsel(double a, double b, i8 op) {
  return cmpff(a, b) == op ? a : b;
}

// P=":+-*%&|<>=~.!@?#_^,$'/\\"
K each(i32 f, K a, K x) {
  K r=0,*prk,e;
  i8 Ta,Tx;
  if(f<0||f>22) return KERR_TYPE;
  Ta=ta; if(s(a)) { if(!vstcb(a)) return KERR_TYPE; Ta=15; }
  Tx=tx; if(s(x)) { if(!vstcb(x)) return KERR_TYPE; Tx=15; }
  if(a) {
    if(Ta<=0&&Tx<=0&&na!=nx) return KERR_LENGTH;
    switch(Ta) {
    case  1: case 2: case 3: case 4: case 6: case 8: case 9: case 15:
      if(Tx>0) r=k(f,k_(a),k_(x));
      else { PRK(nx); i(nx,prk[i]=ki(f,a,x,-1,i);EC(prk[i])) }
      break;
    case -1: case -2: case -3: case -4: case -8: case -9: case 0:
      if(Tx>0) { PRK(na); i(na,prk[i]=ki(f,a,x,i,-1);EC(prk[i])) }
      else { PRK(nx); i(nx,prk[i]=ki(f,a,x,i,i);EC(prk[i])) }
      break;
    }
  }
  else {
    if(Tx>0) r=k(f,0,k_(x));
    else { PRK(nx); i(nx,prk[i]=ki(f,0,x,-1,i);EC(prk[i])) }
  }
  return knorm(r);
cleanup:
  _k(r);
  return e;
}

K eachright(i32 f, K a, K x) {
  K r=0,*prk,e;
  i8 Tx;
  if(f<0||f>22) return KERR_TYPE;
  Tx=tx; if(!Tx&&s(x)) { if(!vstcb(x)) return KERR_TYPE; Tx=15; }
  if(Tx>0) r=k(f,k_(a),k_(x));
  else { PRK(nx); i(nx,prk[i]=ki(f,a,x,-1,i);EC(prk[i])) }
  return knorm(r);
cleanup:
  _k(r);
  return e;
}

K eachleft(i32 f, K a, K x) {
  K r=0,*prk,e;
  i8 Ta;
  if(f<0||f>22) return KERR_TYPE;
  Ta=ta; if(s(a)) { if(!vstcb(a)) return KERR_TYPE; Ta=15; }
  if(Ta>0) r=k(f,k_(a),k_(x));
  else { PRK(na); i(na,prk[i]=ki(f,a,x,i,-1);EC(prk[i])) }
  return knorm(r);
cleanup:
  _k(r);
  return e;
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
static K raze_(K x) {
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

K overd(i32 f, K x) {
  K r=0,*pxk,e,p;
  char *pxc,**pxs,*P=":+-*%&|<>=~.!@?#_^,$'/\\",ff; i8 Tx;
  i32 *pxi;
  i64 *pxj;
  float *pxe;
  double sf,*pxf;
  if(f<0||f>22) return KERR_TYPE;
  Tx=tx; if(!Tx&&s(x)) { if(!vstcb(x)) return KERR_TYPE; Tx=15; }
  if(Tx==1||Tx==2||Tx==3||Tx==4||Tx==8||Tx==9) return k_(x);
  if(Tx==15) return kcp(x);
  if(f==18 && Tx<0 && nx>=2) return k_(x);  /* ,/ of a flat vector is the vector (joining its scalars rebuilds it) */
  if(f==18 && Tx==0 && nx>=2) { r=raze_(x); if(r) return r; }  /* fast raze (,/) for uniform lists */
  if(Tx<=0&&!nx) {
    ff=P[f];
    if(strchr("+*&|",ff)) {
      switch(tx) {
      case -3:
        if(ff=='&') return t(3,0);
        if(ff=='|') return t(3,0);
        break;
      case 0:
      case -1:
        if(ff=='+') return t(1,0);
        if(ff=='*') return t(1,1);
        if(ff=='&') return t(1,1);
        if(ff=='|') return t(1,0);
        break;
      case -8:
        if(ff=='+') return tj(0);
        if(ff=='*') return tj(1);
        if(ff=='&') return tj(J_INF);
        if(ff=='|') return tj(J_NINF);
        break;
      case -2:
        if(ff=='+') { sf=0.0; return t2(sf); }
        if(ff=='*') { sf=1.0; return t2(sf); }
        if(ff=='&') { sf=INFINITY; return t2(sf); }
        if(ff=='|') { sf=-INFINITY; return t2(sf); }
        break;
      case -9:
        if(ff=='+') return te(0.0f);
        if(ff=='*') return te(1.0f);
        if(ff=='&') return te((float)INFINITY);
        if(ff=='|') return te(-(float)INFINITY);
        break;
      default: return KERR_TYPE;
      }
    }
    else if(ff==',') return k_(x);
    else return KERR_LENGTH;
  }
  switch(f) {
  case 1:
    switch(Tx) {
    case -1: PXI; { u32 su=0; i(nx,su+=(u32)*pxi++); r=t(1,su); } break;
    case -2: PXF; sf=0; i(nx,sf+=*pxf++); r=t2(sf); break;
    case -8: PXJ; { u64 su=0; i(nx,su+=(u64)*pxj++); r=tj((i64)su); } break;
    case -9: PXE; { float su=0; i(nx,su+=*pxe++); r=te(su); } break;
    case  0: PXK; r=k_(*pxk++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    default: return KERR_TYPE;
    } break;
  case 2:
    switch(Tx) {
    case -1: PXI; { i32 su=*pxi++; i(nx-1,su=(i32)((u32)su-(u32)*pxi++)); r=t(1,(u32)su); } break;
    case -2: PXF; sf=*pxf++; i(nx-1,sf-=*pxf++); r=t2(sf); break;
    case -8: PXJ; { i64 su=*pxj++; i(nx-1,su=(i64)((u64)su-(u64)*pxj++)); r=tj(su); } break;
    case -9: PXE; { float su=*pxe++; i(nx-1,su-=*pxe++); r=te(su); } break;
    case  0: PXK; r=k_(*pxk++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    default: return KERR_TYPE;
    } break;
  case 3:
    switch(Tx) {
    case -1: PXI; { u32 su=1; i(nx,su*=(u32)*pxi++); r=t(1,su); } break;
    case -2: PXF; sf=1; i(nx,sf*=*pxf++); r=t2(sf); break;
    case -8: PXJ; { u64 su=1; i(nx,su*=(u64)*pxj++); r=tj((i64)su); } break;
    case -9: PXE; { float su=1; i(nx,su*=*pxe++); r=te(su); } break;
    case  0: PXK; r=k_(*pxk++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    default: return KERR_TYPE;
    } break;
  case 4:
    switch(Tx) {
    case -1: PXI; { sf=fi(*pxi++); i(nx-1,sf/=fi(*pxi++)); r=t2(sf); } break;
    case -2: PXF; sf=*pxf++; i(nx-1,sf/=*pxf++); r=t2(sf); break;
    case -8: PXJ; { sf=fj(*pxj++); i(nx-1,sf/=fj(*pxj++)); r=t2(sf); } break;
    case -9: PXE; { sf=(double)*pxe++; i(nx-1,sf/=(double)*pxe++); r=t2(sf); } break;
    case  0: PXK; r=k_(*pxk++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    case -3: PXC; r=t(3,(u8)*pxc++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    case -4: PXS; r=t(4,(K)*pxs++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    default: return KERR_TYPE;
    } break;
  case 5:
    switch(Tx) {
    case -1: PXI; { i32 su=*pxi++; i(nx-1,su=av_isel(su,*pxi++,-1)); r=t(1,(u32)su); } break;
    case -2: PXF; sf=*pxf++; i(nx-1,sf=av_fsel(sf,*pxf++,-1)); r=t2(sf); break;
    case -8: PXJ; { i64 su=*pxj++; i(nx-1,su=av_jsel(su,*pxj++,-1)); r=tj(su); } break;
    case -9: PXE; { float su=*pxe++; i(nx-1,su=av_esel(su,*pxe++,-1)); r=te(su); } break;
    case  0: PXK; r=k_(*pxk++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    case -3: PXC; r=t(3,(u8)*pxc++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    case -4: PXS; r=t(4,(K)*pxs++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    default: return KERR_TYPE;
    } break;
  case 6:
    switch(Tx) {
    case -1: PXI; { i32 su=*pxi++; i(nx-1,su=av_isel(su,*pxi++,1)); r=t(1,(u32)su); } break;
    case -2: PXF; sf=*pxf++; i(nx-1,sf=av_fsel(sf,*pxf++,1)); r=t2(sf); break;
    case -8: PXJ; { i64 su=*pxj++; i(nx-1,su=av_jsel(su,*pxj++,1)); r=tj(su); } break;
    case -9: PXE; { float su=*pxe++; i(nx-1,su=av_esel(su,*pxe++,1)); r=te(su); } break;
    case  0: PXK; r=k_(*pxk++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    case -3: PXC; r=t(3,(u8)*pxc++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    case -4: PXS; r=t(4,(K)*pxs++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    default: return KERR_TYPE;
    } break;
  default:
    switch(Tx) {
    case -1: PXI; r=t(1,(u32)*pxi++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    case -2: PXF; r=t2(*pxf++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    case -8: PXJ; r=tj(*pxj++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    case -9: PXE; r=te(*pxe++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    case -3: PXC; r=t(3,(u8)*pxc++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    case -4: PXS; r=t(4,(K)*pxs++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    case  0: PXK; r=k_(*pxk++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    default: return KERR_TYPE;
    } break;
  }
  return knorm(r);
cleanup:
  _k(r);
  return e;
}

K scand(i32 f, K x) {
  K r=0,*prk,*pxk,e,p;
  char *pxc,**pxs,*P=":+-*%&|<>=~.!@?#_^,$'/\\",ff; i8 Tx;
  i32 *pri,*pxi;
  i64 *prj,*pxj;
  float *pre,*pxe;
  double sf,*prf,*pxf;
  if(f<0||f>22) return KERR_TYPE;
  Tx=tx; if(!Tx&&s(x)) { if(!vstcb(x)) return KERR_TYPE; Tx=15; }
  ff=P[f];
  if(Tx<=0&&!nx&&strchr("+*&|",ff)) {
    switch(Tx) {
    case -1: r=tn(1,0); break;
    case -2: r=tn(2,0); break;
    case -8: r=tn(8,0); break;
    case -9: r=tn(9,0); break;
    case -3: r=tn(3,0); break;
    case -4: r=tn(4,0); break;
    case  0: r=tn(0,0); break;
    }
    return r;
  }
  if(Tx==-1&&!nx&&ff=='=') return tn(1,0);
  if(Tx==1||Tx==2||Tx==3||Tx==4||Tx==8||Tx==9) return k_(x);
  if(Tx==15) return kcp(x);
  if(Tx<=0&&!nx) return tn(0,0);
  switch(f) {
  case 1:
    switch(Tx) {
    case -1: PRI(nx); PXI; { u32 su=0; i(nx,su+=(u32)*pxi++; *pri++=(i32)su); } break;
    case -2: PRF(nx); PXF; sf=0; i(nx,sf+=*pxf++; *prf++=sf); break;
    case -8: PRJ(nx); PXJ; { u64 su=0; i(nx,su+=(u64)*pxj++; *prj++=(i64)su); } break;
    case -9: PRE(nx); PXE; { float su=0; i(nx,su+=*pxe++; *pre++=su); } break;
    case  0: PRK(nx); PXK; *prk++=p=k_(*pxk++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    default: return KERR_TYPE;
    } break;
  case 2:
    switch(Tx) {
    case -1: PRI(nx); PXI; { i32 su=*pxi++; *pri++=su; i(nx-1,su=(i32)((u32)su-(u32)*pxi++); *pri++=su); } break;
    case -2: PRF(nx); PXF; sf=*pxf++; *prf++=sf; i(nx-1,sf-=*pxf++; *prf++=sf); break;
    case -8: PRJ(nx); PXJ; { i64 su=*pxj++; *prj++=su; i(nx-1,su=(i64)((u64)su-(u64)*pxj++); *prj++=su); } break;
    case -9: PRE(nx); PXE; { float su=*pxe++; *pre++=su; i(nx-1,su-=*pxe++; *pre++=su); } break;
    case  0: PRK(nx); PXK; *prk++=p=k_(*pxk++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    default: return KERR_TYPE;
    } break;
  case 3:
    switch(Tx) {
    case -1: PRI(nx); PXI; { u32 su=1; i(nx,su*=(u32)*pxi++; *pri++=(i32)su); } break;
    case -2: PRF(nx); PXF; sf=1.0; i(nx,sf*=*pxf++; *prf++=sf); break;
    case -8: PRJ(nx); PXJ; { u64 su=1; i(nx,su*=(u64)*pxj++; *prj++=(i64)su); } break;
    case -9: PRE(nx); PXE; { float su=1.0; i(nx,su*=*pxe++; *pre++=su); } break;
    case  0: PRK(nx); PXK; *prk++=p=k_(*pxk++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    default: return KERR_TYPE;
    } break;
  case 4:
    switch(Tx) {
    case -1: PRK(nx); PXI; *prk++=p=t(1,(u32)*pxi++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    case -2: PRF(nx); PXF; sf=*pxf++; *prf++=sf; i(nx-1,sf/=*pxf++; *prf++=sf); break;
    case -8: PRK(nx); PXJ; *prk++=p=tj(*pxj++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    case -9: PRF(nx); PXE; sf=(double)*pxe++; *prf++=sf; i(nx-1,sf/=(double)*pxe++; *prf++=sf); break;
    case  0: PRK(nx); PXK; *prk++=p=k_(*pxk++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    case -3: PRK(nx); PXC; *prk++=p=t(3,(u8)*pxc++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    case -4: PRK(nx); PXS; *prk++=p=t(4,(K)*pxs++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    default: return KERR_TYPE;
    } break;
  case 5:
    switch(Tx) {
    case -1: PRI(nx); PXI; { i32 su=*pxi++; *pri++=su; i(nx-1,su=av_isel(su,*pxi++,-1); *pri++=su); } break;
    case -2: PRF(nx); PXF; sf=*pxf++; *prf++=sf; i(nx-1,sf=av_fsel(sf,*pxf++,-1); *prf++=sf); break;
    case -8: PRJ(nx); PXJ; { i64 su=*pxj++; *prj++=su; i(nx-1,su=av_jsel(su,*pxj++,-1); *prj++=su); } break;
    case -9: PRE(nx); PXE; { float su=*pxe++; *pre++=su; i(nx-1,su=av_esel(su,*pxe++,-1); *pre++=su); } break;
    case  0: PRK(nx); PXK; *prk++=p=k_(*pxk++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    case -3: PRK(nx); PXC; *prk++=p=t(3,(u8)*pxc++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    case -4: PRK(nx); PXS; *prk++=p=t(4,(K)*pxs++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    default: return KERR_TYPE;
    } break;
  case 6:
    switch(Tx) {
    case -1: PRI(nx); PXI; { i32 su=*pxi++; *pri++=su; i(nx-1,su=av_isel(su,*pxi++,1); *pri++=su); } break;
    case -2: PRF(nx); PXF; sf=*pxf++; *prf++=sf; i(nx-1,sf=av_fsel(sf,*pxf++,1); *prf++=sf); break;
    case -8: PRJ(nx); PXJ; { i64 su=*pxj++; *prj++=su; i(nx-1,su=av_jsel(su,*pxj++,1); *prj++=su); } break;
    case -9: PRE(nx); PXE; { float su=*pxe++; *pre++=su; i(nx-1,su=av_esel(su,*pxe++,1); *pre++=su); } break;
    case  0: PRK(nx); PXK; *prk++=p=k_(*pxk++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    case -3: PRK(nx); PXC; *prk++=p=t(3,(u8)*pxc++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    case -4: PRK(nx); PXS; *prk++=p=t(4,(K)*pxs++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    default: return KERR_TYPE;
    } break;
  default:
    switch(Tx) {
    case -1: PRK(nx); PXI; *prk++=p=t(1,(u32)*pxi++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    case -2: PRK(nx); PXF; *prk++=p=t2(*pxf++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    case -8: PRK(nx); PXJ; *prk++=p=tj(*pxj++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    case -9: PRK(nx); PXE; *prk++=p=te(*pxe++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    case -3: PRK(nx); PXC; *prk++=p=t(3,(u8)*pxc++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    case -4: PRK(nx); PXS; *prk++=p=t(4,(K)*pxs++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    case  0: PRK(nx); PXK; *prk++=p=k_(*pxk++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    default: return KERR_TYPE;
    } break;
  }
  return knorm(r);
cleanup:
  _k(r);
  return e;
}
