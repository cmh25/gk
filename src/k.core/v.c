#include "v.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define strtok_r strtok_s
#else
#include <dirent.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "av.h"
#include "sort.h"
#include "rand.h"
#include "sym.h"
#include "x.h"
#include <assert.h>

// P=":+-*%&|<>=~.!@?#_^,$'/\\"
K (*FD[FDSIZE])(K,K)={0,plus,minus,times_,divide,minand,maxor,less,more,equal,
                        match,dot,modrot,at,find,take,drop,power,join,form};
K (*FM[FMSIZE])(K)={0,flip,negate,first,recip,where,reverse,upgrade,downgrade,group,
                      not_,value,enumerate,atom,unique,count,floor__,shape,enlist,format};

extern K atcb(K a, K x);
extern K dotcb(K a, K x);
extern K formcb(K a, K x);
extern K valuecb(K x);
extern K enumeratecb(K x);
extern K formatcb(K x);
extern K enlistcb(K x);
extern i32 vstcb(K x);

#define IS0(x) (!s(x)&&!T(x))
extern K irecur1(K(*ff)(K), K x);
extern K irecur2(K(*ff)(K,K), K a, K x);

/* JWO(OP,A,B): wrapping i64 arithmetic (mirrors the (u32)->(i32) int wrap) */
#define JWO(OP,A,B) ((i64)((u64)(A) OP (u64)(B)))
/* promotion: any f64 -> f64; else long+f32 -> f64; else any f32 -> f32;
   else any long -> long; else int.  f32 cells compute in float (te); f32
   mixed with long/f64 compute in double (t2/prf). */
#define PMT(F,O,I) \
K F(K a, K x) { \
  K r=0; \
  i32 *pri,*pai,*pxi; \
  i64 *prj,*paj,*pxj; \
  float *pre,*pae,*pxe; \
  double f,*prf,*paf,*pxf; \
  if(s(x)||s(a)) return KERR_TYPE; \
  if(!(aa||ax)&&na!=nx) return KERR_LENGTH; \
  switch(ta) { \
  case 1: \
    switch(tx) { \
    case  1: r=t(1,(u32)ik(a) O (u32)ik(x)); break; \
    case  2: f=fi(ik(a)) O fk(x); r=t2(f); break; \
    case  8: r=tj(JWO(O,ji(ik(a)),jk(x))); break; \
    case  9: r=te(ei(ik(a)) O ek(x)); break; \
    case -1: PRI(nx); PXI; i(nx,*pri++=(i32)((u32)ik(a) O (u32)*pxi++)) break; \
    case -2: PRF(nx); PXF; i(nx,*prf++=fi(ik(a)) O *pxf++) break; \
    case -8: PRJ(nx); PXJ; { i64 A=ji(ik(a)); i(nx,prj[i]=JWO(O,A,pxj[i])) } break; \
    case -9: PRE(nx); PXE; { float A=ei(ik(a)); i(nx,pre[i]=A O pxe[i]) } break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case 2: \
    switch(tx) { \
    case  1: f=fk(a) O fi(ik(x)); r=t2(f); break; \
    case  2: f=fk(a) O fk(x); r=t2(f); break; \
    case  8: f=fk(a) O fj(jk(x)); r=t2(f); break; \
    case  9: f=fk(a) O (double)ek(x); r=t2(f); break; \
    case -1: PRF(nx); PXI; f=fk(a); i(nx,*prf++=f O fi(*pxi++)) break; \
    case -2: PRF(nx); PXF; f=fk(a); i(nx,*prf++=f O *pxf++) break; \
    case -8: PRF(nx); PXJ; f=fk(a); i(nx,prf[i]=f O fj(pxj[i])) break; \
    case -9: PRF(nx); PXE; f=fk(a); i(nx,prf[i]=f O (double)pxe[i]) break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case 8: \
    switch(tx) { \
    case  1: r=tj(JWO(O,jk(a),ji(ik(x)))); break; \
    case  2: f=fj(jk(a)) O fk(x); r=t2(f); break; \
    case  8: r=tj(JWO(O,jk(a),jk(x))); break; \
    case  9: f=fj(jk(a)) O (double)ek(x); r=t2(f); break; \
    case -1: PRJ(nx); PXI; { i64 A=jk(a); i(nx,prj[i]=JWO(O,A,ji(pxi[i]))) } break; \
    case -2: PRF(nx); PXF; { double A=fj(jk(a)); i(nx,prf[i]=A O pxf[i]) } break; \
    case -8: PRJ(nx); PXJ; { i64 A=jk(a); i(nx,prj[i]=JWO(O,A,pxj[i])) } break; \
    case -9: PRF(nx); PXE; { double A=fj(jk(a)); i(nx,prf[i]=A O (double)pxe[i]) } break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case 9: \
    switch(tx) { \
    case  1: r=te(ek(a) O ei(ik(x))); break; \
    case  2: f=(double)ek(a) O fk(x); r=t2(f); break; \
    case  8: f=(double)ek(a) O fj(jk(x)); r=t2(f); break; \
    case  9: r=te(ek(a) O ek(x)); break; \
    case -1: PRE(nx); PXI; { float A=ek(a); i(nx,pre[i]=A O ei(pxi[i])) } break; \
    case -2: PRF(nx); PXF; { double A=(double)ek(a); i(nx,prf[i]=A O pxf[i]) } break; \
    case -8: PRF(nx); PXJ; { double A=(double)ek(a); i(nx,prf[i]=A O fj(pxj[i])) } break; \
    case -9: PRE(nx); PXE; { float A=ek(a); i(nx,pre[i]=A O pxe[i]) } break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case -1: \
    switch(tx) { \
    case  1: PRI(na); PAI; i(na,*pri++=(i32)((u32)*pai++ O (u32)ik(x))) break; \
    case  2: PRF(na); PAI; f=fk(x); i(na,*prf++=fi(*pai++) O f) break; \
    case  8: PRJ(na); PAI; { i64 X=jk(x); i(na,prj[i]=JWO(O,ji(pai[i]),X)) } break; \
    case  9: PRE(na); PAI; { float X=ek(x); i(na,pre[i]=ei(pai[i]) O X) } break; \
    case -1: PRI(nx); PAI; PXI; i(nx,*pri++=(i32)((u32)*pai++ O (u32)*pxi++)) break; \
    case -2: PRF(nx); PAI; PXF; i(nx,*prf++=fi(*pai++) O *pxf++) break; \
    case -8: PRJ(nx); PAI; PXJ; i(nx,prj[i]=JWO(O,ji(pai[i]),pxj[i])) break; \
    case -9: PRE(nx); PAI; PXE; i(nx,pre[i]=ei(pai[i]) O pxe[i]) break; \
    case  0: r=each(I,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case -2: \
    switch(tx) { \
    case  1: PRF(na); PAF; f=fi(ik(x)); i(na,*prf++=*paf++ O f) break; \
    case  2: PRF(na); PAF; f=fk(x); i(na,*prf++=*paf++ O f) break; \
    case  8: PRF(na); PAF; f=fj(jk(x)); i(na,*prf++=*paf++ O f) break; \
    case  9: PRF(na); PAF; f=(double)ek(x); i(na,*prf++=*paf++ O f) break; \
    case -1: PRF(nx); PAF; PXI; i(nx,*prf++=*paf++ O fi(*pxi++)) break; \
    case -2: PRF(nx); PAF; PXF; i(nx,*prf++=*paf++ O *pxf++) break; \
    case -8: PRF(nx); PAF; PXJ; i(nx,prf[i]=paf[i] O fj(pxj[i])) break; \
    case -9: PRF(nx); PAF; PXE; i(nx,prf[i]=paf[i] O (double)pxe[i]) break; \
    case  0: r=each(I,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case -8: \
    switch(tx) { \
    case  1: PRJ(na); PAJ; { i64 X=ji(ik(x)); i(na,prj[i]=JWO(O,paj[i],X)) } break; \
    case  2: PRF(na); PAJ; { double X=fk(x); i(na,prf[i]=fj(paj[i]) O X) } break; \
    case  8: PRJ(na); PAJ; { i64 X=jk(x); i(na,prj[i]=JWO(O,paj[i],X)) } break; \
    case  9: PRF(na); PAJ; { double X=(double)ek(x); i(na,prf[i]=fj(paj[i]) O X) } break; \
    case -1: PRJ(nx); PAJ; PXI; i(nx,prj[i]=JWO(O,paj[i],ji(pxi[i]))) break; \
    case -2: PRF(nx); PAJ; PXF; i(nx,prf[i]=fj(paj[i]) O pxf[i]) break; \
    case -8: PRJ(nx); PAJ; PXJ; i(nx,prj[i]=JWO(O,paj[i],pxj[i])) break; \
    case -9: PRF(nx); PAJ; PXE; i(nx,prf[i]=fj(paj[i]) O (double)pxe[i]) break; \
    case  0: r=each(I,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case -9: \
    switch(tx) { \
    case  1: PRE(na); PAE; { float X=ei(ik(x)); i(na,pre[i]=pae[i] O X) } break; \
    case  2: PRF(na); PAE; { double X=fk(x); i(na,prf[i]=(double)pae[i] O X) } break; \
    case  8: PRF(na); PAE; { double X=fj(jk(x)); i(na,prf[i]=(double)pae[i] O X) } break; \
    case  9: PRE(na); PAE; { float X=ek(x); i(na,pre[i]=pae[i] O X) } break; \
    case -1: PRE(nx); PAE; PXI; i(nx,pre[i]=pae[i] O ei(pxi[i])) break; \
    case -2: PRF(nx); PAE; PXF; i(nx,prf[i]=(double)pae[i] O pxf[i]) break; \
    case -8: PRF(nx); PAE; PXJ; i(nx,prf[i]=(double)pae[i] O fj(pxj[i])) break; \
    case -9: PRE(nx); PAE; PXE; i(nx,pre[i]=pae[i] O pxe[i]) break; \
    case  0: r=each(I,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case  0: \
    switch(tx) { \
    case  1: r=irecur2(F,a,x); break; \
    case  2: r=irecur2(F,a,x); break; \
    case  8: r=irecur2(F,a,x); break; \
    case  9: r=irecur2(F,a,x); break; \
    case -1: r=each(I,a,x); break; \
    case -2: r=each(I,a,x); break; \
    case -8: r=each(I,a,x); break; \
    case -9: r=each(I,a,x); break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  default: return KERR_TYPE; \
  } \
  return r; \
}
PMT(plus,+,1)
PMT(minus,-,2)
PMT(times_,*,3)

K divide(K a, K x) {
  K r=0;
  i32 *pai,*pxi;
  i64 *paj,*pxj;
  float *pae,*pxe,*pre,ef;
  double f,*prf,*paf,*pxf;
  if(s(x)||s(a)) return KERR_TYPE;
  if(!(aa||ax)&&na!=nx) return KERR_LENGTH;
  switch(ta) {
  case 1:
    switch(tx) {
    case  1: f=fi(ik(a)) / fi(ik(x)); r=t2(f); break;
    case  2: f=fi(ik(a)) / fk(x); r=t2(f); break;
    case  8: f=fi(ik(a)) / fj(jk(x)); r=t2(f); break;
    case  9: r=te(ei(ik(a)) / ek(x)); break;
    case -1: PRF(nx); PXI; f=fi(ik(a)); i(nx,*prf++=f / fi(*pxi++)) break;
    case -2: PRF(nx); PXF; f=fi(ik(a)); i(nx,*prf++=f / *pxf++) break;
    case -8: PRF(nx); PXJ; f=fi(ik(a)); i(nx,prf[i]=f / fj(pxj[i])) break;
    case -9: PRE(nx); PXE; ef=ei(ik(a)); i(nx,pre[i]=ef / pxe[i]) break;
    case  0: r=irecur2(divide,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 2:
    switch(tx) {
    case  1: f=fk(a) / fi(ik(x)); r=t2(f); break;
    case  2: f=fk(a) / fk(x); r=t2(f); break;
    case  8: f=fk(a) / fj(jk(x)); r=t2(f); break;
    case  9: f=fk(a) / (double)ek(x); r=t2(f); break;
    case -1: PRF(nx); PXI; f=fk(a); i(nx,*prf++=f / fi(*pxi++)) break;
    case -2: PRF(nx); PXF; f=fk(a); i(nx,*prf++=f / *pxf++) break;
    case -8: PRF(nx); PXJ; f=fk(a); i(nx,prf[i]=f / fj(pxj[i])) break;
    case -9: PRF(nx); PXE; f=fk(a); i(nx,prf[i]=f / (double)pxe[i]) break;
    case  0: r=irecur2(divide,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 8:
    switch(tx) {
    case  1: f=fj(jk(a)) / fi(ik(x)); r=t2(f); break;
    case  2: f=fj(jk(a)) / fk(x); r=t2(f); break;
    case  8: f=fj(jk(a)) / fj(jk(x)); r=t2(f); break;
    case  9: f=fj(jk(a)) / (double)ek(x); r=t2(f); break;
    case -1: PRF(nx); PXI; f=fj(jk(a)); i(nx,prf[i]=f / fi(pxi[i])) break;
    case -2: PRF(nx); PXF; f=fj(jk(a)); i(nx,prf[i]=f / pxf[i]) break;
    case -8: PRF(nx); PXJ; f=fj(jk(a)); i(nx,prf[i]=f / fj(pxj[i])) break;
    case -9: PRF(nx); PXE; f=fj(jk(a)); i(nx,prf[i]=f / (double)pxe[i]) break;
    case  0: r=irecur2(divide,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -1:
    switch(tx) {
    case  1: PRF(na); PAI; f=fi(ik(x)); i(na,*prf++=fi(*pai++) / f) break;
    case  2: PRF(na); PAI; f=fk(x); i(na,*prf++=fi(*pai++) / f) break;
    case  8: PRF(na); PAI; f=fj(jk(x)); i(na,prf[i]=fi(pai[i]) / f) break;
    case  9: PRE(na); PAI; ef=ek(x); i(na,pre[i]=ei(pai[i]) / ef) break;
    case -1: PRF(nx); PAI; PXI; i(nx,*prf++=fi(*pai++) / fi(*pxi++)) break;
    case -2: PRF(nx); PAI; PXF; i(nx,*prf++=fi(*pai++) / *pxf++) break;
    case -8: PRF(nx); PAI; PXJ; i(nx,prf[i]=fi(pai[i]) / fj(pxj[i])) break;
    case -9: PRE(nx); PAI; PXE; i(nx,pre[i]=ei(pai[i]) / pxe[i]) break;
    case  0: r=each(4,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -2:
    switch(tx) {
    case  1: PRF(na); PAF; f=fi(ik(x)); i(na,*prf++=*paf++ / f) break;
    case  2: PRF(na); PAF; f=fk(x); i(na,*prf++=*paf++ / f) break;
    case  8: PRF(na); PAF; f=fj(jk(x)); i(na,*prf++=*paf++ / f) break;
    case  9: PRF(na); PAF; f=(double)ek(x); i(na,*prf++=*paf++ / f) break;
    case -1: PRF(nx); PAF; PXI; i(nx,*prf++=*paf++ / fi(*pxi++)) break;
    case -2: PRF(nx); PAF; PXF; i(nx,*prf++=*paf++ / *pxf++) break;
    case -8: PRF(nx); PAF; PXJ; i(nx,prf[i]=paf[i] / fj(pxj[i])) break;
    case -9: PRF(nx); PAF; PXE; i(nx,prf[i]=paf[i] / (double)pxe[i]) break;
    case  0: r=each(4,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -8:
    switch(tx) {
    case  1: PRF(na); PAJ; f=fi(ik(x)); i(na,prf[i]=fj(paj[i]) / f) break;
    case  2: PRF(na); PAJ; f=fk(x); i(na,prf[i]=fj(paj[i]) / f) break;
    case  8: PRF(na); PAJ; f=fj(jk(x)); i(na,prf[i]=fj(paj[i]) / f) break;
    case  9: PRF(na); PAJ; f=(double)ek(x); i(na,prf[i]=fj(paj[i]) / f) break;
    case -1: PRF(nx); PAJ; PXI; i(nx,prf[i]=fj(paj[i]) / fi(pxi[i])) break;
    case -2: PRF(nx); PAJ; PXF; i(nx,prf[i]=fj(paj[i]) / pxf[i]) break;
    case -8: PRF(nx); PAJ; PXJ; i(nx,prf[i]=fj(paj[i]) / fj(pxj[i])) break;
    case -9: PRF(nx); PAJ; PXE; i(nx,prf[i]=fj(paj[i]) / (double)pxe[i]) break;
    case  0: r=each(4,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 9:
    switch(tx) {
    case  1: r=te(ek(a) / ei(ik(x))); break;
    case  2: f=(double)ek(a) / fk(x); r=t2(f); break;
    case  8: f=(double)ek(a) / fj(jk(x)); r=t2(f); break;
    case  9: r=te(ek(a) / ek(x)); break;
    case -1: PRE(nx); PXI; ef=ek(a); i(nx,pre[i]=ef / ei(pxi[i])) break;
    case -2: PRF(nx); PXF; f=(double)ek(a); i(nx,prf[i]=f / pxf[i]) break;
    case -8: PRF(nx); PXJ; f=(double)ek(a); i(nx,prf[i]=f / fj(pxj[i])) break;
    case -9: PRE(nx); PXE; ef=ek(a); i(nx,pre[i]=ef / pxe[i]) break;
    case  0: r=irecur2(divide,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -9:
    switch(tx) {
    case  1: PRE(na); PAE; ef=ei(ik(x)); i(na,pre[i]=pae[i] / ef) break;
    case  2: PRF(na); PAE; f=fk(x); i(na,prf[i]=(double)pae[i] / f) break;
    case  8: PRF(na); PAE; f=fj(jk(x)); i(na,prf[i]=(double)pae[i] / f) break;
    case  9: PRE(na); PAE; ef=ek(x); i(na,pre[i]=pae[i] / ef) break;
    case -1: PRE(nx); PAE; PXI; i(nx,pre[i]=pae[i] / ei(pxi[i])) break;
    case -2: PRF(nx); PAE; PXF; i(nx,prf[i]=(double)pae[i] / pxf[i]) break;
    case -8: PRF(nx); PAE; PXJ; i(nx,prf[i]=(double)pae[i] / fj(pxj[i])) break;
    case -9: PRE(nx); PAE; PXE; i(nx,pre[i]=pae[i] / pxe[i]) break;
    case  0: r=each(4,a,x); break;
    default: return KERR_TYPE;
    } break;
  case  0:
    switch(tx) {
    case  1: r=irecur2(divide,a,x); break;
    case  2: r=irecur2(divide,a,x); break;
    case  8: r=irecur2(divide,a,x); break;
    case  9: r=irecur2(divide,a,x); break;
    case -1: r=each(4,a,x); break;
    case -2: r=each(4,a,x); break;
    case -8: r=each(4,a,x); break;
    case -9: r=each(4,a,x); break;
    case  0: r=irecur2(divide,a,x); break;
    default: return KERR_TYPE;
    } break;
  default: return KERR_TYPE;
  }
  return r;
}

/* compact min/max: op=-1(min), 1(max) */
static inline i32 isel(i32 a,i32 b,i8 op) { return op<0?(a<b?a:b):(a>b?a:b); }
static inline i64 jsel(i64 a,i64 b,i8 op) { return op<0?(a<b?a:b):(a>b?a:b); }
static inline char csel(char a,char b,i8 op) { return op<0?(a<b?a:b):(a>b?a:b); }
static inline char* ssel(char *a,char *b,i8 op) { i32 c=strcmp(a,b); return op<0?(c<0?a:b):(c>0?a:b); }
static inline double fsel(double a,double b,i8 op) { return cmpfft(a,b)==op?a:b; }
static inline float esel(float a,float b,i8 op) { return cmpfft(a,b)==op?a:b; }
static K mamo(K a, K x, i8 op, K(*F)(K,K), i32 idx) {
  K r=0;
  i32 *pri,*pai,*pxi;
  i64 *prj,*paj,*pxj;
  float *pre,*pae,*pxe;
  double f,*prf,*paf,*pxf;
  char *prc,*pac,*pxc,**prs,**pas,**pxs;
  if(s(x)||s(a)) return KERR_TYPE;
  if(!(aa||ax)&&na!=nx) return KERR_LENGTH;
  switch(ta) {
  case 1:
    switch(tx) {
    case  1: r=isel(ik(a),ik(x),op)==ik(a)?a:x; break;
    case  2: f=fi(ik(a)); r=t2(fsel(f,fk(x),op)); break;
    case  8: r=tj(jsel(ji(ik(a)),jk(x),op)); break;
    case  9: r=te(esel(ei(ik(a)),ek(x),op)); break;
    case -1: PRI(nx); PXI; i(nx,*pri++=isel(ik(a),*pxi++,op)) break;
    case -2: PRF(nx); PXF; f=fi(ik(a)); i(nx,*prf++=fsel(f,*pxf++,op)) break;
    case -8: PRJ(nx); PXJ; { i64 A=ji(ik(a)); i(nx,prj[i]=jsel(A,pxj[i],op)) } break;
    case -9: PRE(nx); PXE; { float A=ei(ik(a)); i(nx,pre[i]=esel(A,pxe[i],op)) } break;
    case  0: r=irecur2(F,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 2:
    switch(tx) {
    case  1: f=fi(ik(x)); r=t2(fsel(fk(a),f,op)); break;
    case  2: r=cmpfft(fk(a),fk(x))==op?k_(a):k_(x); break;
    case  8: f=fj(jk(x)); r=t2(fsel(fk(a),f,op)); break;
    case  9: f=(double)ek(x); r=t2(fsel(fk(a),f,op)); break;
    case -1: PRF(nx); PXI; f=fk(a); i(nx,*prf++=fsel(f,fi(*pxi++),op)) break;
    case -2: PRF(nx); PXF; f=fk(a); i(nx,*prf++=fsel(f,*pxf++,op)) break;
    case -8: PRF(nx); PXJ; f=fk(a); i(nx,prf[i]=fsel(f,fj(pxj[i]),op)) break;
    case -9: PRF(nx); PXE; f=fk(a); i(nx,prf[i]=fsel(f,(double)pxe[i],op)) break;
    case  0: r=irecur2(F,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 8:
    switch(tx) {
    case  1: r=tj(jsel(jk(a),ji(ik(x)),op)); break;
    case  2: f=fj(jk(a)); r=t2(fsel(f,fk(x),op)); break;
    case  8: r=tj(jsel(jk(a),jk(x),op)); break;
    case  9: f=fj(jk(a)); r=t2(fsel(f,(double)ek(x),op)); break;
    case -1: PRJ(nx); PXI; { i64 A=jk(a); i(nx,prj[i]=jsel(A,ji(pxi[i]),op)) } break;
    case -2: PRF(nx); PXF; f=fj(jk(a)); i(nx,prf[i]=fsel(f,pxf[i],op)) break;
    case -8: PRJ(nx); PXJ; { i64 A=jk(a); i(nx,prj[i]=jsel(A,pxj[i],op)) } break;
    case -9: PRF(nx); PXE; f=fj(jk(a)); i(nx,prf[i]=fsel(f,(double)pxe[i],op)) break;
    case  0: r=irecur2(F,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 9:
    switch(tx) {
    case  1: r=te(esel(ek(a),ei(ik(x)),op)); break;
    case  2: f=(double)ek(a); r=t2(fsel(f,fk(x),op)); break;
    case  8: f=(double)ek(a); r=t2(fsel(f,fj(jk(x)),op)); break;
    case  9: r=te(esel(ek(a),ek(x),op)); break;
    case -1: PRE(nx); PXI; { float A=ek(a); i(nx,pre[i]=esel(A,ei(pxi[i]),op)) } break;
    case -2: PRF(nx); PXF; { double A=(double)ek(a); i(nx,prf[i]=fsel(A,pxf[i],op)) } break;
    case -8: PRF(nx); PXJ; { double A=(double)ek(a); i(nx,prf[i]=fsel(A,fj(pxj[i]),op)) } break;
    case -9: PRE(nx); PXE; { float A=ek(a); i(nx,pre[i]=esel(A,pxe[i],op)) } break;
    case  0: r=irecur2(F,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 3:
    switch(tx) {
    case  3: r=csel(ck(a),ck(x),op)==ck(a)?a:x; break;
    case -3: PRC(nx); PXC; i(nx,*prc++=csel(ck(a),*pxc++,op)) break;
    case  0: r=irecur2(F,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 4:
    switch(tx) {
    case  4: r=ssel(sk(a),sk(x),op)==sk(a)?a:x; break;
    case -4: PRS(nx); PXS; i(nx,*prs++=ssel(sk(a),*pxs++,op)) break;
    case  0: r=irecur2(F,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -1:
    switch(tx) {
    case  1: PRI(na); PAI; i(na,*pri++=isel(*pai++,ik(x),op)) break;
    case  2: PRF(na); PAI; f=fk(x); i(na,*prf++=fsel(fi(*pai++),f,op)) break;
    case  8: PRJ(na); PAI; { i64 X=jk(x); i(na,prj[i]=jsel(ji(pai[i]),X,op)) } break;
    case  9: PRE(na); PAI; { float X=ek(x); i(na,pre[i]=esel(ei(pai[i]),X,op)) } break;
    case -1: PRI(nx); PAI; PXI; i(nx,*pri++=isel(*pai++,*pxi++,op)) break;
    case -2: PRF(nx); PAI; PXF; i(nx,*prf++=fsel(fi(*pai++),*pxf++,op)) break;
    case -8: PRJ(nx); PAI; PXJ; i(nx,prj[i]=jsel(ji(pai[i]),pxj[i],op)) break;
    case -9: PRE(nx); PAI; PXE; i(nx,pre[i]=esel(ei(pai[i]),pxe[i],op)) break;
    case  0: r=each(idx,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -2:
    switch(tx) {
    case  1: PRF(na); PAF; f=fi(ik(x)); i(na,*prf++=fsel(*paf++,f,op)) break;
    case  2: PRF(na); PAF; f=fk(x); i(na,*prf++=fsel(*paf++,f,op)) break;
    case  8: PRF(na); PAF; f=fj(jk(x)); i(na,prf[i]=fsel(paf[i],f,op)) break;
    case  9: PRF(na); PAF; f=(double)ek(x); i(na,prf[i]=fsel(paf[i],f,op)) break;
    case -1: PRF(nx); PAF; PXI; i(nx,*prf++=fsel(*paf++,fi(*pxi++),op)) break;
    case -2: PRF(nx); PAF; PXF; i(nx,*prf++=fsel(*paf++,*pxf++,op)) break;
    case -8: PRF(nx); PAF; PXJ; i(nx,prf[i]=fsel(paf[i],fj(pxj[i]),op)) break;
    case -9: PRF(nx); PAF; PXE; i(nx,prf[i]=fsel(paf[i],(double)pxe[i],op)) break;
    case  0: r=each(idx,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -8:
    switch(tx) {
    case  1: PRJ(na); PAJ; { i64 X=ji(ik(x)); i(na,prj[i]=jsel(paj[i],X,op)) } break;
    case  2: PRF(na); PAJ; f=fk(x); i(na,prf[i]=fsel(fj(paj[i]),f,op)) break;
    case  8: PRJ(na); PAJ; { i64 X=jk(x); i(na,prj[i]=jsel(paj[i],X,op)) } break;
    case  9: PRF(na); PAJ; f=(double)ek(x); i(na,prf[i]=fsel(fj(paj[i]),f,op)) break;
    case -1: PRJ(nx); PAJ; PXI; i(nx,prj[i]=jsel(paj[i],ji(pxi[i]),op)) break;
    case -2: PRF(nx); PAJ; PXF; i(nx,prf[i]=fsel(fj(paj[i]),pxf[i],op)) break;
    case -8: PRJ(nx); PAJ; PXJ; i(nx,prj[i]=jsel(paj[i],pxj[i],op)) break;
    case -9: PRF(nx); PAJ; PXE; i(nx,prf[i]=fsel(fj(paj[i]),(double)pxe[i],op)) break;
    case  0: r=each(idx,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -9:
    switch(tx) {
    case  1: PRE(na); PAE; { float X=ei(ik(x)); i(na,pre[i]=esel(pae[i],X,op)) } break;
    case  2: PRF(na); PAE; f=fk(x); i(na,prf[i]=fsel((double)pae[i],f,op)) break;
    case  8: PRF(na); PAE; f=fj(jk(x)); i(na,prf[i]=fsel((double)pae[i],f,op)) break;
    case  9: PRE(na); PAE; { float X=ek(x); i(na,pre[i]=esel(pae[i],X,op)) } break;
    case -1: PRE(nx); PAE; PXI; i(nx,pre[i]=esel(pae[i],ei(pxi[i]),op)) break;
    case -2: PRF(nx); PAE; PXF; i(nx,prf[i]=fsel((double)pae[i],pxf[i],op)) break;
    case -8: PRF(nx); PAE; PXJ; i(nx,prf[i]=fsel((double)pae[i],fj(pxj[i]),op)) break;
    case -9: PRE(nx); PAE; PXE; i(nx,pre[i]=esel(pae[i],pxe[i],op)) break;
    case  0: r=each(idx,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -3:
    switch(tx) {
    case  3: PRC(na); PAC; i(na,*prc++=csel(*pac++,ck(x),op)) break;
    case -3: PRC(nx); PAC; PXC; i(nx,*prc++=csel(*pac++,*pxc++,op)) break;
    case  0: r=each(idx,a,x); break;  /* every sibling arm (sym/int/float/long/real) uses each; irecur2 rejected the char vector with a length error */
    default: return KERR_TYPE;
    } break;
  case -4:
    switch(tx) {
    case  4: PRS(na); PAS; i(na,*prs++=ssel(*pas++,sk(x),op)) break;
    case -4: PRS(nx); PAS; PXS; i(nx,*prs++=ssel(*pas++,*pxs++,op)) break;
    case  0: r=each(idx,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 0:
    switch(tx) {
    case 1: case 2: case 3: case 4: case 8: case 9: case 0: r=irecur2(F,a,x); break;
    case -1: case -2: case -3: case -4: case -8: case -9: r=each(idx,a,x); break;
    default: return KERR_TYPE;
    } break;
  default: return KERR_TYPE;
  }
  return r;
}
K minand(K a, K x) { return mamo(a,x,-1,minand,5); }
K maxor(K a, K x)  { return mamo(a,x,1,maxor,6); }

/* compact comparison: op=-1(less), 0(equal), 1(more) */
/* icmp/jcmp hoisted to v.h (shared with fuse.c and the k() inline) */
static inline i32 ccmp(char a,char b,i8 op) { return op<0?a<b:op>0?a>b:a==b; }
static inline i32 scmp(char *a,char *b,i8 op) { i32 c=strcmp(a,b); return op<0?c<0:op>0?c>0:c==0; }
static K lme(K a, K x, i8 op, K(*F)(K,K), i32 idx) {
  K r=0;
  i32 *pri,*pai,*pxi;
  i64 *paj,*pxj;
  float *pae,*pxe;
  double f,*paf,*pxf;
  char *pac,*pxc,**pas,**pxs;
  if(s(x)||s(a)) return KERR_TYPE;
  if(!(aa||ax)&&na!=nx) return KERR_LENGTH;
  switch(ta) {
  case 1:
    switch(tx) {
    case  1: r=t(1,(u32)icmp(ik(a),ik(x),op)); break;
    case  2: r=t(1,cmpfft(fi(ik(a)),fk(x))==op); break;
    case  8: r=t(1,(u32)jcmp(ji(ik(a)),jk(x),op)); break;
    case  9: r=t(1,cmpfft(fi(ik(a)),(double)ek(x))==op); break;
    case -1: PRI(nx); PXI; i(nx,*pri++=icmp(ik(a),*pxi++,op)) break;
    case -2: PRI(nx); PXF; f=fi(ik(a)); i(nx,*pri++=cmpfft(f,*pxf++)==op) break;
    case -8: PRI(nx); PXJ; { i64 A=ji(ik(a)); i(nx,pri[i]=jcmp(A,pxj[i],op)) } break;
    case -9: PRI(nx); PXE; f=fi(ik(a)); i(nx,pri[i]=cmpfft(f,(double)pxe[i])==op) break;
    case  0: r=irecur2(F,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 2:
    switch(tx) {
    case  1: r=t(1,cmpfft(fk(a),fi(ik(x)))==op); break;
    case  2: r=t(1,cmpfft(fk(a),fk(x))==op); break;
    case  8: r=t(1,cmpfft(fk(a),fj(jk(x)))==op); break;
    case  9: r=t(1,cmpfft(fk(a),(double)ek(x))==op); break;
    case -1: PRI(nx); PXI; f=fk(a); i(nx,*pri++=cmpfft(f,fi(*pxi++))==op) break;
    case -2: PRI(nx); PXF; f=fk(a); i(nx,*pri++=cmpfft(f,*pxf++)==op) break;
    case -8: PRI(nx); PXJ; f=fk(a); i(nx,pri[i]=cmpfft(f,fj(pxj[i]))==op) break;
    case -9: PRI(nx); PXE; f=fk(a); i(nx,pri[i]=cmpfft(f,(double)pxe[i])==op) break;
    case  0: r=irecur2(F,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 8:
    switch(tx) {
    case  1: r=t(1,(u32)jcmp(jk(a),ji(ik(x)),op)); break;
    case  2: r=t(1,cmpfft(fj(jk(a)),fk(x))==op); break;
    case  8: r=t(1,(u32)jcmp(jk(a),jk(x),op)); break;
    case  9: r=t(1,cmpfft(fj(jk(a)),(double)ek(x))==op); break;
    case -1: PRI(nx); PXI; { i64 A=jk(a); i(nx,pri[i]=jcmp(A,ji(pxi[i]),op)) } break;
    case -2: PRI(nx); PXF; f=fj(jk(a)); i(nx,pri[i]=cmpfft(f,pxf[i])==op) break;
    case -8: PRI(nx); PXJ; { i64 A=jk(a); i(nx,pri[i]=jcmp(A,pxj[i],op)) } break;
    case -9: PRI(nx); PXE; f=fj(jk(a)); i(nx,pri[i]=cmpfft(f,(double)pxe[i])==op) break;
    case  0: r=irecur2(F,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 9:
    switch(tx) {
    case  1: r=t(1,cmpfft((double)ek(a),fi(ik(x)))==op); break;
    case  2: r=t(1,cmpfft((double)ek(a),fk(x))==op); break;
    case  8: r=t(1,cmpfft((double)ek(a),fj(jk(x)))==op); break;
    case  9: r=t(1,cmpfft((double)ek(a),(double)ek(x))==op); break;
    case -1: PRI(nx); PXI; f=(double)ek(a); i(nx,pri[i]=cmpfft(f,fi(pxi[i]))==op) break;
    case -2: PRI(nx); PXF; f=(double)ek(a); i(nx,pri[i]=cmpfft(f,pxf[i])==op) break;
    case -8: PRI(nx); PXJ; f=(double)ek(a); i(nx,pri[i]=cmpfft(f,fj(pxj[i]))==op) break;
    case -9: PRI(nx); PXE; f=(double)ek(a); i(nx,pri[i]=cmpfft(f,(double)pxe[i])==op) break;
    case  0: r=irecur2(F,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 3:
    switch(tx) {
    case  3: r=t(1,(u32)ccmp(ck(a),ck(x),op)); break;
    case -3: PRI(nx); PXC; i(nx,*pri++=ccmp(ck(a),*pxc++,op)) break;
    case  0: r=irecur2(F,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 4:
    switch(tx) {
    case  4: r=t(1,(u32)scmp(sk(a),sk(x),op)); break;
    case -4: PRI(nx); PXS; i(nx,*pri++=scmp(sk(a),*pxs++,op)) break;
    case  0: r=irecur2(F,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -3:
    switch(tx) {
    case  3: PRI(na); PAC; i(na,*pri++=ccmp(*pac++,ck(x),op)) break;
    case -3: PRI(nx); PAC; PXC; i(nx,*pri++=ccmp(*pac++,*pxc++,op)) break;
    case  0: r=each(idx,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -1:
    switch(tx) {
    case  1: PRI(na); PAI; i(na,*pri++=icmp(*pai++,ik(x),op)) break;
    case  2: PRI(na); PAI; f=fk(x); i(na,*pri++=cmpfft(fi(*pai++),f)==op) break;
    case  8: PRI(na); PAI; { i64 X=jk(x); i(na,pri[i]=jcmp(ji(pai[i]),X,op)) } break;
    case  9: PRI(na); PAI; f=(double)ek(x); i(na,pri[i]=cmpfft(fi(pai[i]),f)==op) break;
    case -1: PRI(nx); PAI; PXI; i(nx,*pri++=icmp(*pai++,*pxi++,op)) break;
    case -2: PRI(nx); PAI; PXF; i(nx,*pri++=cmpfft(fi(*pai++),*pxf++)==op) break;
    case -8: PRI(nx); PAI; PXJ; i(nx,pri[i]=jcmp(ji(pai[i]),pxj[i],op)) break;
    case -9: PRI(nx); PAI; PXE; i(nx,pri[i]=cmpfft(fi(pai[i]),(double)pxe[i])==op) break;
    case  0: r=each(idx,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -2:
    switch(tx) {
    case  1: PRI(na); PAF; f=fi(ik(x)); i(na,*pri++=cmpfft(*paf++,f)==op) break;
    case  2: PRI(na); PAF; f=fk(x); i(na,*pri++=cmpfft(*paf++,f)==op) break;
    case  8: PRI(na); PAF; f=fj(jk(x)); i(na,pri[i]=cmpfft(paf[i],f)==op) break;
    case  9: PRI(na); PAF; f=(double)ek(x); i(na,pri[i]=cmpfft(paf[i],f)==op) break;
    case -1: PRI(nx); PAF; PXI; i(nx,*pri++=cmpfft(*paf++,fi(*pxi++))==op) break;
    case -2: PRI(nx); PAF; PXF; i(nx,*pri++=cmpfft(*paf++,*pxf++)==op) break;
    case -8: PRI(nx); PAF; PXJ; i(nx,pri[i]=cmpfft(paf[i],fj(pxj[i]))==op) break;
    case -9: PRI(nx); PAF; PXE; i(nx,pri[i]=cmpfft(paf[i],(double)pxe[i])==op) break;
    case  0: r=each(idx,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -8:
    switch(tx) {
    case  1: PRI(na); PAJ; { i64 X=ji(ik(x)); i(na,pri[i]=jcmp(paj[i],X,op)) } break;
    case  2: PRI(na); PAJ; f=fk(x); i(na,pri[i]=cmpfft(fj(paj[i]),f)==op) break;
    case  8: PRI(na); PAJ; { i64 X=jk(x); i(na,pri[i]=jcmp(paj[i],X,op)) } break;
    case  9: PRI(na); PAJ; f=(double)ek(x); i(na,pri[i]=cmpfft(fj(paj[i]),f)==op) break;
    case -1: PRI(nx); PAJ; PXI; i(nx,pri[i]=jcmp(paj[i],ji(pxi[i]),op)) break;
    case -2: PRI(nx); PAJ; PXF; i(nx,pri[i]=cmpfft(fj(paj[i]),pxf[i])==op) break;
    case -8: PRI(nx); PAJ; PXJ; i(nx,pri[i]=jcmp(paj[i],pxj[i],op)) break;
    case -9: PRI(nx); PAJ; PXE; i(nx,pri[i]=cmpfft(fj(paj[i]),(double)pxe[i])==op) break;
    case  0: r=each(idx,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -9:
    switch(tx) {
    case  1: PRI(na); PAE; f=fi(ik(x)); i(na,pri[i]=cmpfft((double)pae[i],f)==op) break;
    case  2: PRI(na); PAE; f=fk(x); i(na,pri[i]=cmpfft((double)pae[i],f)==op) break;
    case  8: PRI(na); PAE; f=fj(jk(x)); i(na,pri[i]=cmpfft((double)pae[i],f)==op) break;
    case  9: PRI(na); PAE; f=(double)ek(x); i(na,pri[i]=cmpfft((double)pae[i],f)==op) break;
    case -1: PRI(nx); PAE; PXI; i(nx,pri[i]=cmpfft((double)pae[i],fi(pxi[i]))==op) break;
    case -2: PRI(nx); PAE; PXF; i(nx,pri[i]=cmpfft((double)pae[i],pxf[i])==op) break;
    case -8: PRI(nx); PAE; PXJ; i(nx,pri[i]=cmpfft((double)pae[i],fj(pxj[i]))==op) break;
    case -9: PRI(nx); PAE; PXE; i(nx,pri[i]=cmpfft((double)pae[i],(double)pxe[i])==op) break;
    case  0: r=each(idx,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -4:
    switch(tx) {
    case  4: PRI(na); PAS; i(na,*pri++=scmp(*pas++,sk(x),op)) break;
    case -4: PRI(nx); PAS; PXS; i(nx,*pri++=scmp(*pas++,*pxs++,op)) break;
    case  0: r=each(idx,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 0:
    switch(tx) {
    case 1: case 2: case 3: case 4: case 8: case 9: case 0: r=irecur2(F,a,x); break;
    case -1: case -2: case -3: case -4: case -8: case -9: r=each(idx,a,x); break;
    default: return KERR_TYPE;
    } break;
  default: return KERR_TYPE;
  }
  return r;
}
K less(K a, K x)  { return lme(a,x,-1,less,7); }
K more(K a, K x)  { return lme(a,x,1,more,8); }
K equal(K a, K x) { return lme(a,x,0,equal,9); }

K match(K a, K x) {
  if(!s(a)&&!s(x)&&ta==2&&tx==2) return t(1,(u32)(0==cmpfft(fk(a),fk(x))));
  return t(1,(u32)!kcmpr(a,x));
}

K dot0(K a, K x) {
  K r=0,e,*prk,p,pm=0,*pmu,q,t=0,*pxk=px(x);
  if(0xc==s(a)||0x2c==s(a)) return dotcb(a,x);
  if(!ax&&nx==1) r=at(a,pxk[0]);
  else {
    p=pxk[0];
    t=nx==2?k_(pxk[1]):k(16,t(1,1),k_(x)); /* drop */
    K(*ff)(K,K)=nx==2?at:dot;
    if(s(p)||s(t)) { _k(t); return KERR_TYPE; }
    else if(T(p)<=0&&n(p)) {
      PRK(n(p));
      pm=kmix(p); if(E(pm)) { e=pm; goto cleanup; }
      pmu=px(pm);
      i(n(p),q=at(a,pmu[i]);EC(q);prk[i]=ff(q,t);_k(q);EC(prk[i]))
      _k(pm);
    }
    else if(p==null||p==inull) {
      PRK(na);
      i(na,q=at(a,t(1,i));EC(q);prk[i]=ff(q,t);_k(q);EC(prk[i]))
    }
    else { q=at(a,pxk[0]);EC(q);r=ff(q,t);_k(q); }
    _k(t);
  }
  return knorm(r);
cleanup:
  if(r) _k(r);
  if(t) _k(t);
  if(pm) _k(pm);
  return e;
}
K dot(K a, K x) {
  K r=0,e,q;
  int b=0;
  if(!s(x)&&!ax&&!nx) {
    if(!tx) {
      if(!s(a)&&(ta==1||ta==2||ta==8||ta==9)) return k_(a);
      x=null;  /* a[] = a[nul] = a . () = a@nul = a nul */
    }
    else if(s(a)&&0x80!=s(a)) {
      /* FUNCTION application: a typed empty as the argument list calls
         with that type's PROTOTYPE argument ({1,x} . 0#0 -> 1 0 -- t647
         "prototype conformance"). */
      switch(tx) {
      case -1: x=t(1,0); break;        /* f . 0#0   = f[0]   */
      case -2: x=t2(0.0); b=1; break;  /* f . 0#0.0 = f[0.0] */
      case -3: x=t(3,' '); break;      /* f . ""    = f[" "] */
      case -4: x=t(4,sp("")); break;   /* f . 0#`   = f[`]   */
      case -8: x=tj(0); b=1; break;    /* f . !0j   = f[0j]  */
      case -9: x=te(0.0f); b=1; break; /* f . 0#2.0e = f[0.0e] */
      }
    }
  }
  if(a==null&&x==null) { r=tn(0,0); if(b) _k(x); return r; }
  if(s(a)||s(x)) { r=dotcb(a,x); if(b) _k(x); return r; }
  if(!ax&&nx==0) { r=k_(a); if(b) _k(x); return r; }
  if(ta==4||ta==3||ta==-3) { r=dotcb(a,x); if(r) { if(b) _k(x); return r; } }
  switch(ta) {
  case -1: case -2: case -3: case -4:
    switch(tx) {
    case 1: case 8: case 6: r=at(a,x); break;
    case 0:
      if(nx!=1) r=KERR_RANK;
      else { K *px=px(x); r=at(a,px[0]); }
      break;
    case -1:
      if(nx!=1) r=KERR_RANK;
      else { i32 *px=px(x); r=at(a,t(1,(u32)px[0])); }
      break;
    case -8:
      if(nx!=1) r=KERR_RANK;
      else { i64 *pj=px(x); K ix=tj(pj[0]); r=at(a,ix); _k(ix); } /* at borrows; free the boxed long index */
      break;
    default: r=KERR_RANK;
    } break;
  case  0:
    switch(tx) {
    case 1: case 8: case 4: r=at(a,x); break;
    case 0: case -1: case -8: case -4: q=kmix(x); EC(q); r=dot0(a,q); _k(q); break;
    case 6: r=at(a,x); break;
    default: r=KERR_RANK;
    } break;
  case  6: {
    if(tx>0) { r=at(a,x); break; }
    if(!nx) { r=k_(x); break; }
    { K first=xi_(x,0,tx);
      if(nx==1) r=first;
      else {
        K one=t(1,1), rest=drop(one,x);
        if(E(rest)) { _k(first); r=rest; }
        else { r=dot(first,rest); _k(rest); _k(first); }
      }
    }
    break;
  }
  default: r=KERR_RANK;
  }
  if(b) _k(x);
  return knorm(r);
cleanup:
  if(b) _k(x);
  if(r) _k(r);
  return e;
}

/* modi hoisted to v.h (shared with the k() int-atom mod inline) */
static inline i64 modj(i64 a, i64 b){ i64 r; if(!b) return J_NULL; if(b==-1) return 0; r=a%b; if(r&&((r<0)!=(b<0))) r+=b; return r; }
static inline double modd(double a, double b){ double r=fmod(a,b); if(r&&((r<0)!=(b<0))) r+=b; return r; }
static inline float  mode(float a, float b){ float r=fmodf(a,b); if(r&&((r<0)!=(b<0))) r+=b; return r; }
/* rotate a vector by OFF: result[i] = x[(i+OFF) mod nx].  Hoist the modulo out
   of the loop into a start offset + wrap counter (ri), replacing a per-element
   idiv (cf. the take cyclic fix).  Guard nx==0: the modulo would divide by zero,
   and the original avoided it only because the loop body never ran. */
#define ROT(PR,PX,OFF,ASSIGN) \
  PR(nx); PX; \
  if(nx){ ri=(i64)(OFF)%(i64)nx; if(ri<0)ri+=nx; i(nx, ASSIGN; if(++ri==(i64)nx)ri=0;) } break;

K modrot(K a, K x) {
  K r=0,*prk,*pxk;
  char *prc,*pxc,**prs,**pxs;
  i32 *pri,*pai,*pxi;
  i64 *prj,*paj,*pxj;
  i64 ri;
  float *pre,*pae,*pxe;
  double *prf,*paf,*pxf;
  if(s(x)||s(a)) return KERR_TYPE;
  switch(ta) {
  case 1:
    switch(tx) {
    case  1: r=t(1,(u32)modi(ik(a),ik(x))); break;
    case  2: r=t2(modd(fi(ik(a)),fk(x))); break;
    case  8: r=tj(modj(ji(ik(a)),jk(x))); break;
    case  9: r=te(mode(ei(ik(a)),ek(x))); break;
    case -1: ROT(PRI,PXI,ik(a),*pri++=pxi[ri])
    case -2: ROT(PRF,PXF,ik(a),*prf++=pxf[ri])
    case -3: ROT(PRC,PXC,ik(a),*prc++=pxc[ri])
    case -4: ROT(PRS,PXS,ik(a),*prs++=pxs[ri])
    case -8: ROT(PRJ,PXJ,ik(a),*prj++=pxj[ri])
    case -9: ROT(PRE,PXE,ik(a),*pre++=pxe[ri])
    case  0: ROT(PRK,PXK,ik(a),*prk++=k_(pxk[ri]))
    default: return KERR_TYPE;
    } break;
  case 2:
    switch(tx) {
    case 1: r=t2(modd(fk(a),fi(ik(x)))); break;
    case 2: r=t2(modd(fk(a),fk(x))); break;
    case 8: r=t2(modd(fk(a),fj(jk(x)))); break;
    case 9: r=t2(modd(fk(a),(double)ek(x))); break;
    default: return KERR_INT;
    } break;
  case 8:
    switch(tx) {
    case 1: r=tj(modj(jk(a),ji(ik(x)))); break;
    case 2: r=t2(modd(fj(jk(a)),fk(x))); break;
    case 8: r=tj(modj(jk(a),jk(x))); break;
    case 9: r=t2(modd(fj(jk(a)),(double)ek(x))); break;
    case -1: ROT(PRI,PXI,jk(a),*pri++=pxi[ri])
    case -2: ROT(PRF,PXF,jk(a),*prf++=pxf[ri])
    case -3: ROT(PRC,PXC,jk(a),*prc++=pxc[ri])
    case -4: ROT(PRS,PXS,jk(a),*prs++=pxs[ri])
    case -8: ROT(PRJ,PXJ,jk(a),*prj++=pxj[ri])
    case -9: ROT(PRE,PXE,jk(a),*pre++=pxe[ri])
    case  0: ROT(PRK,PXK,jk(a),*prk++=k_(pxk[ri]))
    default: return KERR_TYPE;
    } break;
  case 9:
    switch(tx) {
    case 1: r=te(mode(ek(a),ei(ik(x)))); break;
    case 2: r=t2(modd((double)ek(a),fk(x))); break;
    case 8: r=t2(modd((double)ek(a),fj(jk(x)))); break;
    case 9: r=te(mode(ek(a),ek(x))); break;
    default: return KERR_INT;
    } break;
  case -1:
    switch(tx) {
    case 1: PRI(na); PAI; { i32 X=ik(x); i(na,pri[i]=modi(pai[i],X)) } break;
    case 2: PRF(na); PAI; { double X=fk(x); i(na,prf[i]=modd(fi(pai[i]),X)) } break;
    case 8: PRJ(na); PAI; { i64 X=jk(x); i(na,prj[i]=modj(ji(pai[i]),X)) } break;
    case 9: PRE(na); PAI; { float X=ek(x); i(na,pre[i]=mode(ei(pai[i]),X)) } break;
    default: return KERR_INT;
    } break;
  case -2:
    switch(tx) {
    case 1: PRF(na); PAF; { double X=fi(ik(x)); i(na,prf[i]=modd(paf[i],X)) } break;
    case 2: PRF(na); PAF; { double X=fk(x); i(na,prf[i]=modd(paf[i],X)) } break;
    case 8: PRF(na); PAF; { double X=fj(jk(x)); i(na,prf[i]=modd(paf[i],X)) } break;
    case 9: PRF(na); PAF; { double X=(double)ek(x); i(na,prf[i]=modd(paf[i],X)) } break;
    default: return KERR_INT;
    } break;
  case -8:
    switch(tx) {
    case 1: PRJ(na); PAJ; { i64 X=ji(ik(x)); i(na,prj[i]=modj(paj[i],X)) } break;
    case 2: PRF(na); PAJ; { double X=fk(x); i(na,prf[i]=modd(fj(paj[i]),X)) } break;
    case 8: PRJ(na); PAJ; { i64 X=jk(x); i(na,prj[i]=modj(paj[i],X)) } break;
    case 9: PRF(na); PAJ; { double X=(double)ek(x); i(na,prf[i]=modd(fj(paj[i]),X)) } break;
    default: return KERR_INT;
    } break;
  case -9:
    switch(tx) {
    case 1: PRE(na); PAE; { float X=ei(ik(x)); i(na,pre[i]=mode(pae[i],X)) } break;
    case 2: PRF(na); PAE; { double X=fk(x); i(na,prf[i]=modd((double)pae[i],X)) } break;
    case 8: PRF(na); PAE; { double X=fj(jk(x)); i(na,prf[i]=modd((double)pae[i],X)) } break;
    case 9: PRE(na); PAE; { float X=ek(x); i(na,pre[i]=mode(pae[i],X)) } break;
    default: return KERR_INT;
    } break;
  default: return KERR_INT;
  }
  return knorm(r);
}
#undef ROT


/* single-pass bounds-checked gather for x@intvec: folds the former standalone
   bounds pre-scan into the gather loop, so the index vector is traversed once
   instead of twice.  On an out-of-range index, free the partial result and
   return KERR_INDEX (same pattern as the tx==-8 path).  na/r are in scope. */
#define ATG(PRX,PAX,ASSIGN) \
  PRX(nx); PAX; PXI; \
  i(nx, i32 ix_=pxi[i]; if(ix_<0||(u64)ix_>=na){_k(r);return KERR_INDEX;} ASSIGN) break;

/* as ATG but for a long (i64) index vector, so indices can reach past 2^31 */
#define ATGJ(PRX,PAX,ASSIGN) \
  PRX(nx); PAX; PXJ; \
  i(nx, i64 ix_=pxj[i]; if(ix_<0||(u64)ix_>=na){_k(r);return KERR_INDEX;} ASSIGN) break;

K at(K a, K x) {
  K r=0,*prk,*pak,t,*pxk,*pr;
  char *prc,*pac,**prs,**pas;
  i32 *pri,*pai,*pxi;
  i64 *prj,*paj,*pxj;
  float *pre,*pae;
  double f=0,*prf,*paf;
  if(a==null) {
    if(x==null||x==inull) return tn(0,0);       /* nul nul -> () */
    if(s(x)) return KERR_TYPE;                  /* dict, lambda, ... */
    switch(tx) {
    case  1: case  8: case -1: case -8: return k_(x);   /* integer index -> itself */
    case  4: return null;                               /* symbol index misses */
    case -4: { K r=tn(0,nx),*pr=px(r); i(nx,pr[i]=null) return r; }
    case  0:                                            /* elementwise */
      if(!nx) return tn(0,0);
      r=tn(0,nx); pr=px(r); pxk=px(x);
      i(nx, t=at(null,pxk[i]); if(E(t)) { n(r)=i; _k(r); return t; } pr[i]=t)
      return knorm(r);
    default:
      if(!ax&&!nx) return tn(0,0);   /* empty index -> () */
      return KERR_TYPE;              /* char, float, real */
    }
  }
  if(s(a)||s(x)||4==ta) return atcb(a,x);
  /* a non-nul ATOM cannot be indexed, not even by nul or an empty index */
  if(x==null||x==inull) return aa?KERR_RANK:k_(a);
  if(!tx&&!nx) return aa?KERR_RANK:tn(0,0);
  if(aa) return KERR_TYPE;
  /* long index atom -> validate in [0,na) and gather the element directly
     (na may exceed INT32, so we must NOT truncate the index to i32) */
  if(tx==8) { i64 v=jk(x); if(v<0||(u64)v>=na) return KERR_INDEX; return xi_(a,(u64)v,ta); }
  /* long index vector -> gather with i64 indices (reaches past 2^31) */
  if(tx==-8) {
    switch(ta) {
    case -1: ATGJ(PRI,PAI,pri[i]=pai[ix_])
    case -2: ATGJ(PRF,PAF,prf[i]=paf[ix_])
    case -8: ATGJ(PRJ,PAJ,prj[i]=paj[ix_])
    case -9: ATGJ(PRE,PAE,pre[i]=pae[ix_])
    case -3: ATGJ(PRC,PAC,prc[i]=pac[ix_])
    case -4: ATGJ(PRS,PAS,prs[i]=pas[ix_])
    case  0: ATGJ(PRK,PAK,prk[i]=k_(pak[ix_]))
    default: return KERR_TYPE;
    }
    return knorm(r);
  }
  if((tx==1)&&((ik(x)<0)||(u64)(ik(x))>=na)) return KERR_INDEX;
  if(!tx) return eachright(13,a,x);
  if(tx!=1&&tx!=-1) return KERR_TYPE;
  switch(ta) {
  case -8:
    switch(tx) {
    case  1: PAJ; r=tj(paj[ik(x)]); break;
    case -1: ATG(PRJ,PAJ,prj[i]=paj[ix_])
    } break;
  case -1:
    switch(tx) {
    case  1: PAI; r=t(1,(u32)pai[ik(x)]); break;
    case -1: ATG(PRI,PAI,pri[i]=pai[ix_])
    } break;
  case -9:
    switch(tx) {
    case  1: PAE; r=te(pae[ik(x)]); break;
    case -1: ATG(PRE,PAE,pre[i]=pae[ix_])
    } break;
  case -2:
    switch(tx) {
    case  1: PAF; f=paf[ik(x)]; r=t2(f); break;
    case -1: ATG(PRF,PAF,prf[i]=paf[ix_])
    } break;
  case -3:
    switch(tx) {
    case  1: PAC; r=t(3,(u8)pac[ik(x)]); break;
    case -1: ATG(PRC,PAC,prc[i]=pac[ix_])
    } break;
  case -4:
    switch(tx) {
    case  1: PAS; r=t(4,pas[ik(x)]); break;
    case -1: ATG(PRS,PAS,prs[i]=pas[ix_])
    } break;
  case  0:
    switch(tx) {
    case  1: PAK; r=k_(pak[ik(x)]); break;
    case -1: ATG(PRK,PAK,prk[i]=k_(pak[ix_]))
    } break;
  default: return KERR_TYPE;
  }
  return knorm(r);
}
#undef ATG

static K draw(K a, K x) {
  K r=0;
  i64 cnt;                       /* sample count; long-atom count -> big draw */
  if(s(x)||s(a)) return KERR_TYPE;
  /* atoms only: ik() below reads the immediate payload, and the lone caller
     (find) forwards nothing else -- a vector here would read pointer bits */
  if(ta!=1 && ta!=8) return KERR_TYPE;
  cnt=ta==8?jk(a):ji(ik(a));
  if(cnt==J_INF) return KERR_WSFULL;
  if(cnt==J_NULL || cnt==J_NINF) return KERR_DOMAIN;
  if(tx==1 && (ik(x)==INT32_MIN || ik(x)==INT32_MIN+1))
    return KERR_WSFULL;
  if(tx==8 && (jk(x)==J_NULL || jk(x)==J_NINF))
    return KERR_WSFULL;
  if(tx==1 && ik(x)<0 && cnt>abs(ik(x))) return KERR_LENGTH;
  switch(ta) {
  case 1: case 8:
    if(cnt<0) return KERR_DOMAIN;
    VLEN(cnt);
    switch(tx) {
    case 1:
      if(ik(x)>0) { r=tn(1,cnt); drawi((i32*)px(r),cnt,ik(x)); }
      else if(ik(x)<0) { VSIZE(abs(ik(x))); r=tn(1,cnt); deal((i32*)px(r),(i32)cnt,abs(ik(x))); }
      else { r=tn(2,cnt); drawf((double*)px(r),cnt,1.0); }
      break;
    case 2: r=tn(2,cnt); drawf((double*)px(r),cnt,fk(x)); break;
    case 8: {
      i64 m=jk(x);
      if(m>0) { r=tn(8,cnt); drawj((i64*)px(r),cnt,m); }
      else if(m<0) {
        i64 am=-m;
        if(am>INT32_MAX) return KERR_WSFULL;
        if(cnt>am) return KERR_LENGTH;
        VSIZE(am); r=tn(8,cnt); dealj((i64*)px(r),(i32)cnt,(i32)am);
      }
      else { r=tn(2,cnt); drawf((double*)px(r),cnt,1.0); }
      break;
    }
    case 9: r=tn(9,cnt); drawe((float*)px(r),cnt,ek(x)); break;
    default: return KERR_TYPE;
    } break;
  default: return KERR_TYPE;
  }
  return r;
}
K find(K a, K x) {
  K *pak;
  char *pac,**pas;
  i32 *pai;
  i64 *paj;
  float *pae;
  double *paf;
  i64 idx;
  if(ta==1||ta==8) return draw(a,x);
  if(aa) return KERR_DOMAIN;
  if(s(a)) return KERR_TYPE;
  idx=na;                       /* default: not found -> na (the length) */
  if(s(x) && ta!=0) goto done;  /* a callable is not a typed atom */
  switch(ta) {
  case -1: if(tx!=1) break; PAI; i(na,if(pai[i]==ik(x)){idx=i; break;}) break;
  case -2: if(tx!=2) break; PAF; if(isnan(fk(x))) { i(na,if(isnan(paf[i])){idx=i; break;}) } else i(na,if(paf[i]==fk(x)){idx=i; break;}) break;
  case -8: if(tx!=8) break; PAJ; i(na,if(paj[i]==jk(x)){idx=i; break;}) break;
  case -9: if(tx!=9) break; PAE; if(isnan(ek(x))) { i(na,if(isnan(pae[i])){idx=i; break;}) } else i(na,if(pae[i]==ek(x)){idx=i; break;}) break;
  case -3: if(tx!=3) break; PAC; { char *p=memchr(pac,(u8)ck(x),na); if(p) idx=(i64)(p-pac); } break;
  case -4: if(tx!=4) break; PAS; i(na,if(!strcmp(pas[i],sk(x))){idx=i; break;}) break;
  case  0: PAK; i(na,if(!kcmpr(pak[i],x)){idx=i; break;}) break;
  default: return KERR_TYPE;
  }
done:
  /* index is a position in a, so it needs a long only when #a>2^31 (mirror #a) */
  return na>BIGV ? tj(idx) : t(1,(u32)idx);
}

/* Batched each-right find: x?/y with a plain-list haystack x hashes it ONCE
   (key -> first index) and probes each item of y -- O(#x+#y) where the
   generic eachright loop pays a full find scan per item.  Same semantics as
   find(): type-strict, first occurrence, not-found -> #x; NaNs are one value
   and -0.0 = 0.0 (find/kcmpr equality).  lin/in/dvl/dv all route through
   x?/y, so they inherit the speedup. */
static inline u64 fmix(u64 h) {
  h^=h>>33; h*=0xff51afd7ed558ccdULL;
  h^=h>>33; h*=0xc4ceb9fe1a85ec53ULL;
  h^=h>>33;
  return h;
}
static inline u64 fkf(double v) {   /* canonical f64 key */
  u64 b;
  if(isnan(v)) return 0x7ff8000000000000ULL;
  if(v==0.0) return 0;
  memcpy(&b,&v,8);
  return b;
}
static inline u64 fke(float v) {    /* canonical f32 key */
  u32 b;
  if(isnan(v)) return 0x7fc00000u;
  if(v==0.0f) return 0;
  memcpy(&b,&v,4);
  return (u64)b;
}
#define FRINS(key) { h=fmix(key)&q; while(ho[h]&&hk[h]!=(key)) h=(h+1)&q; if(!ho[h]) { ho[h]=1; hk[h]=(key); hv[h]=(i64)i; } }
#define FRGET(key,res) { h=fmix(key)&q; while(ho[h]&&hk[h]!=(key)) h=(h+1)&q; (res)=ho[h]?hv[h]:miss; }
K findr(K a, K x) {
  u64 w,q,h,i,m;
  i64 miss,ix;
  i32 big,*pri;
  i64 *prj;
  K r;
  if(s(a)||ta>0||s(x)||tx>0) return 0;   /* not this path; caller falls through */
  m=nx;
  miss=(i64)na;
  big=na>BIGV;
  r=big?tn(8,(i64)m):tn(1,(i64)m);
  pri=(i32*)px(r); prj=(i64*)px(r);      /* same buffer; big picks the write width */
#define FRPUT(v) { ix=(v); if(big) prj[i]=ix; else pri[i]=(i32)ix; }
  if(!na) { for(i=0;i<m;i++) FRPUT(miss) return r; }
  if(ta==-3) {                            /* char haystack: 256-entry table */
    i64 fi256[256];
    char *pc=(char*)px(a);
    for(i=0;i<256;i++) fi256[i]=miss;
    for(i=0;i<na;i++) if(fi256[(u8)pc[i]]==miss) fi256[(u8)pc[i]]=(i64)i;
    if(tx==-3) { char *p=(char*)px(x); for(i=0;i<m;i++) FRPUT(fi256[(u8)p[i]]) }
    else if(tx==0) { K *p=(K*)px(x); for(i=0;i<m;i++) FRPUT((p[i]&&!s(p[i])&&T(p[i])==3)?fi256[(u8)ck(p[i])]:miss) }
    else for(i=0;i<m;i++) FRPUT(miss)
    return r;
  }
  if(ta==0) {                             /* generic haystack: khash/kcmpr map */
    K *hk,*pak=(K*)px(a),p,b;
    i64 *hv;
    w=16; while(w<2*na) w<<=1; q=w-1;
    hk=xcalloc(w,sizeof(K));
    hv=xmalloc(w*sizeof(i64));
    for(i=0;i<na;i++) {
      p=pak[i];
      h=khash(p)&q;
      if(p) { while(!h || (hk[h] && kcmpr(hk[h],p))) h=(h+1)&q; if(!hk[h]) { hk[h]=p; hv[h]=(i64)i; } }
    }
    for(i=0;i<m;i++) {
      b=0;
      if(tx==0) b=((K*)px(x))[i];
      else switch(tx) {                   /* box the typed element to probe */
      case -1: b=t(1,(u32)((i32*)px(x))[i]); break;
      case -8: b=tj(((i64*)px(x))[i]); break;
      case -2: b=t2(((double*)px(x))[i]); break;
      case -9: b=te(((float*)px(x))[i]); break;
      case -4: b=t(4,(u64)((char**)px(x))[i]); break;
      }
      if(!b) { FRPUT(miss) continue; }
      h=khash(b)&q;
      while(!h || (hk[h] && kcmpr(hk[h],b))) h=(h+1)&q;
      FRPUT(hk[h]?hv[h]:miss)
      if(tx!=0) _k(b);
    }
    xfree(hk); xfree(hv);
    return r;
  }
  {                                       /* plain typed haystack: canonical-key map */
    u64 *hk,key; u8 *ho; i64 *hv,f;
    w=16; while(w<2*na) w<<=1; q=w-1;
    hk=xmalloc(w*sizeof(u64)); hv=xmalloc(w*sizeof(i64)); ho=xcalloc(w,1);
    switch(ta) {
    case -1: { i32 *p=(i32*)px(a); for(i=0;i<na;i++) { key=(u64)(u32)p[i]; FRINS(key) } } break;
    case -8: { i64 *p=(i64*)px(a); for(i=0;i<na;i++) { key=(u64)p[i]; FRINS(key) } } break;
    case -2: { double *p=(double*)px(a); for(i=0;i<na;i++) { key=fkf(p[i]); FRINS(key) } } break;
    case -9: { float *p=(float*)px(a); for(i=0;i<na;i++) { key=fke(p[i]); FRINS(key) } } break;
    case -4: { char **p=(char**)px(a); for(i=0;i<na;i++) { key=(u64)p[i]; FRINS(key) } } break;
    default: xfree(hk); xfree(hv); xfree(ho); _k(r); return 0;
    }
    if(tx==ta) {
      switch(tx) {
      case -1: { i32 *p=(i32*)px(x); for(i=0;i<m;i++) { key=(u64)(u32)p[i]; FRGET(key,f) FRPUT(f) } } break;
      case -8: { i64 *p=(i64*)px(x); for(i=0;i<m;i++) { key=(u64)p[i]; FRGET(key,f) FRPUT(f) } } break;
      case -2: { double *p=(double*)px(x); for(i=0;i<m;i++) { key=fkf(p[i]); FRGET(key,f) FRPUT(f) } } break;
      case -9: { float *p=(float*)px(x); for(i=0;i<m;i++) { key=fke(p[i]); FRGET(key,f) FRPUT(f) } } break;
      case -4: { char **p=(char**)px(x); for(i=0;i<m;i++) { key=(u64)p[i]; FRGET(key,f) FRPUT(f) } } break;
      }
    }
    else if(tx==0) {                      /* generic probes into typed haystack */
      K *p=(K*)px(x),b;
      for(i=0;i<m;i++) {
        b=p[i]; f=miss;
        if(b&&!s(b)) switch(ta) {
        case -1: if(T(b)==1) { key=(u64)(u32)ik(b); FRGET(key,f) } break;
        case -8: if(T(b)==8) { key=(u64)jk(b); FRGET(key,f) } break;
        case -2: if(T(b)==2) { key=fkf(fk(b)); FRGET(key,f) } break;
        case -9: if(T(b)==9) { key=fke(ek(b)); FRGET(key,f) } break;
        case -4: if(T(b)==4) { key=(u64)sk(b); FRGET(key,f) } break;
        }
        FRPUT(f)
      }
    }
    else { for(i=0;i<m;i++) FRPUT(miss) }
    xfree(hk); xfree(hv); xfree(ho);
    return r;
  }
#undef FRPUT
}

static K take_(K a, K x) {
  K r=0,e,*prk,*pxk,a0=0,a_=0,leaf;
  char *prc,*pxc,**prs,**pxs; i8 Ta,Tx;
  i64 c; i32 *pri,*pxi;
  i64 *prj,*pxj;
  float *pre,*pxe;
  double *prf,*pxf;
  Ta=ta; if(s(a)) { if(!vstcb(a)) return KERR_TYPE; Ta=15; }
  Tx=tx; if(s(x)) { if(!vstcb(x)) return KERR_TYPE; Tx=15; }

  if(Ta<=0 && !na) {
    /* empty dims: the PROTOTYPE of x -- its first element, or for an
       empty x the type's prototype atom (0 / 0.0 / 0j / 0.0e / " " / `),
       matching the reference (the numeric arms used to yield nul). */
    switch(Tx) {
    case  1: case  2: case  8: case  9: case  3: case  4: case 6: case 15: r=k_(x); break;
    case  0: if(nx) r=k_(((K*)px(x))[0]); else r=null; break;
    case -1: if(nx) r=t(1,(u32)(((i32*)px(x))[0])); else r=t(1,0); break;
    case -2: if(nx) r=t2(((double*)px(x))[0]); else r=t2(0.0); break;
    case -8: if(nx) r=tj(((i64*)px(x))[0]); else r=tj(0); break;
    case -9: if(nx) r=te(((float*)px(x))[0]); else r=te(0.0f); break;
    case -3: if(nx) r=t(3,(u8)(((char*)px(x))[0])); else r=t(3,' '); break;
    case -4: if(nx) r=t(4,((char**)px(x))[0]); else r=t(4,sp("")); break;
    default: r=KERR_TYPE;
    }
    return r;
  }

  switch(Ta) {
  case 8:
  case 1: {
    /* scalar count: int (Ta==1) or long (Ta==8). c is the i64 magnitude;
       neg flags a negative take (last |c| elements, with cycling). */
    int neg;
    if(Ta==1) {
      if(ik(a)==INT32_MIN) return k_(x); /* 0N # x -> return x */
      c=ji(ik(a));
      if(c==J_INF||c==J_NINF) return KERR_WSFULL;
    } else {
      i64 av=jk(a);
      if(av==J_NULL) return k_(x);       /* 0Nj # x -> return x */
      if(av==J_INF||av==J_NINF) return KERR_WSFULL;
      c=av;
    }
    neg=c<0; if(neg) c=-c;
    VLEN(c);
    switch(Tx) {
    case  1: PRI(c); i(c,*pri++=ik(x)) break;
    case  2: PRF(c); i(c,*prf++=fk(x)) break;
    case  8: PRJ(c); i(c,*prj++=jk(x)) break;
    case  9: PRE(c); i(c,*pre++=ek(x)) break;
    case  3: PRC(c); i(c,*prc++=ck(x)) break;
    case  4: PRS(c); i(c,*prs++=sk(x)) break;
    case  6: PRK(c); i(c,*prk++=null) break;
    case 15: PRK(c); i(c,*prk=kcp(x); EC(*prk); ++prk) break;
    case -1: PRI(c); PXI;
      if(nx&&neg) { u64 w=(nx-c%nx)%nx; i(c,*pri++=pxi[w]; if(++w==nx)w=0;) }
      else if(nx) { u64 w=0; i(c,*pri++=pxi[w]; if(++w==nx)w=0;) }
      else i(c,*pri++=0)
      break;
    case -2: PRF(c); PXF;
      if(nx&&neg) { u64 w=(nx-c%nx)%nx; i(c,*prf++=pxf[w]; if(++w==nx)w=0;) }
      else if(nx) { u64 w=0; i(c,*prf++=pxf[w]; if(++w==nx)w=0;) }
      else i(c,*prf++=0.0)
      break;
    case -8: PRJ(c); PXJ;
      if(nx&&neg) { u64 w=(nx-c%nx)%nx; i(c,*prj++=pxj[w]; if(++w==nx)w=0;) }
      else if(nx) { u64 w=0; i(c,*prj++=pxj[w]; if(++w==nx)w=0;) }
      else i(c,*prj++=0)
      break;
    case -9: PRE(c); PXE;
      if(nx&&neg) { u64 w=(nx-c%nx)%nx; i(c,*pre++=pxe[w]; if(++w==nx)w=0;) }
      else if(nx) { u64 w=0; i(c,*pre++=pxe[w]; if(++w==nx)w=0;) }
      else i(c,*pre++=0.0)
      break;
    case -3: PRC(c); PXC;
      if(nx&&neg) { u64 w=(nx-c%nx)%nx; i(c,*prc++=pxc[w]; if(++w==nx)w=0;) }
      else if(nx) { u64 w=0; i(c,*prc++=pxc[w]; if(++w==nx)w=0;) }
      else i(c,*prc++=' ')
      break;
    case -4: PRS(c); PXS;
      if(nx&&neg) { u64 w=(nx-c%nx)%nx; i(c,*prs++=pxs[w]; if(++w==nx)w=0;) }
      else if(nx) { u64 w=0; i(c,*prs++=pxs[w]; if(++w==nx)w=0;) }
      else i(c,*prs++="") /* sp()? */
      break;
    case  0: PRK(c); PXK;
      if(nx&&neg) { u64 w=(nx-c%nx)%nx; i(c,*prk++=k_(pxk[w]); if(++w==nx)w=0;) }
      else if(nx) { u64 w=0; i(c,*prk++=k_(pxk[w]); if(++w==nx)w=0;) }
      else i(c,*prk++=null)
      break;
    default: r=KERR_TYPE;
    } break;
  }
  case -1:
  case -8: {
    /* multi-dim reshape, i64 throughout: read the shape (int -1 or long -8)
       into dim[] (J_NULL = inferred 0N axis).  Total, flat intermediate, the
       nested-builder child-index, and the leaf/container loops are all i64, so
       a big vector reshapes into (few) big rows given memory; total <= VMAX. */
    i64 *dim, n=1;
    dim=xmalloc(na*sizeof(i64));
    if(ta==-8){ i64 *pp=px(a); for(u64 k=0;k<na;k++) dim[k]=pp[k]; }
    else { i32 *pp=px(a); for(u64 k=0;k<na;k++) dim[k]=ji(pp[k]); }
    /* 0N # x -> return x as-is */
    if(na==1 && dim[0]==J_NULL) { xfree(dim); return k_(x); }
    /* 2-D shape with exactly one 0N axis
         0N k # x : rows of width k, row count inferred, last row ragged
         r 0N # x : exactly r rows, split evenly */
    if(na==2 && ((dim[0]==J_NULL) != (dim[1]==J_NULL))) {
      i64 rows, st=0, *rl, ln;
      if(ax || s(x)) { xfree(dim); return k_(x); }
      if(dim[0]==J_NULL) {            /* 0N k # x */
        i64 kk=dim[1];
        if(kk<=0 || kk==J_INF) { xfree(dim); return KERR_DOMAIN; }
        if(!nx) { xfree(dim); return tn(0,0); }
        rows=((i64)nx+kk-1)/kk;
        if(rows>=VMAX) { xfree(dim); return KERR_WSFULL; }
        rl=xmalloc((size_t)rows*sizeof(i64));
        for(i64 i=0;i<rows;i++) rl[i]=(i==rows-1)?((i64)nx-i*kk):kk;
      } else {                          /* r 0N # x */
        i64 rr=dim[0];
        if(rr<0 || rr==J_INF) { xfree(dim); return KERR_DOMAIN; }
        rows=rr;
        if(!rows) { xfree(dim); return tn(0,0); }
        if(rows>=VMAX) { xfree(dim); return KERR_WSFULL; }
        rl=xmalloc((size_t)rows*sizeof(i64));
        { i64 prev=0; for(i64 i=0;i<rows;i++) { i64 b=((i+1)*(i64)nx)/rows; rl[i]=b-prev; prev=b; } }
      }
      xfree(dim);
      PRK(rows);
      switch(Tx) {
      case -1: PXI; i(rows, ln=rl[i]; prk[i]=tn(1,ln); pri=px(prk[i]); j(ln,pri[j]=pxi[st+j]); st+=ln) break;
      case -2: PXF; i(rows, ln=rl[i]; prk[i]=tn(2,ln); prf=px(prk[i]); j(ln,prf[j]=pxf[st+j]); st+=ln) break;
      case -8: PXJ; i(rows, ln=rl[i]; prk[i]=tn(8,ln); prj=px(prk[i]); j(ln,prj[j]=pxj[st+j]); st+=ln) break;
      case -9: PXE; i(rows, ln=rl[i]; prk[i]=tn(9,ln); pre=px(prk[i]); j(ln,pre[j]=pxe[st+j]); st+=ln) break;
      case -3: PXC; i(rows, ln=rl[i]; prk[i]=tn(3,ln); prc=px(prk[i]); j(ln,prc[j]=pxc[st+j]); st+=ln) break;
      case -4: PXS; i(rows, ln=rl[i]; prk[i]=tn(4,ln); prs=px(prk[i]); j(ln,prs[j]=pxs[st+j]); st+=ln) break;
      case  0: PXK; i(rows, ln=rl[i]; prk[i]=tn(0,ln); K*pc=px(prk[i]); j(ln,pc[j]=k_(pxk[st+j])); st+=ln) break;
      default: r=KERR_TYPE;
      }
      xfree(rl);
      return knorm(r);
    }
    for(u64 i=0;i<na;i++) if(dim[i]<0) { xfree(dim); e=KERR_DOMAIN; goto cleanup; } /* negative / stray 0N (J_NULL<0) */
    for(u64 i=0;i<na;i++) {
      if(dim[i]==J_INF) { xfree(dim); e=KERR_WSFULL; goto cleanup; }
      if(dim[i] && n>VMAX/dim[i]) { xfree(dim); e=KERR_WSFULL; goto cleanup; }
      n*=dim[i];
    }
    K cnt=tj(n), flat=take_(cnt,x); _k(cnt); /* tj is boxed; take_ borrows it */
    if(E(flat)) { xfree(dim); e=flat; goto cleanup; }
    if(na==1) { r=flat; xfree(dim); break; }
    typedef struct { K r; u64 d; i64 i; } SF;
    i32 sm=32,sp=0;
    SF *stack=xmalloc(sizeof(SF)*sm);
    r=tn(0,dim[0]);
    stack[sp++]=(SF){r,0,0};
    i64 j=0;
    while(sp) {
      SF *f=&stack[sp - 1];
      if(f->d==na-1) {
        switch(T(flat)) {
          case -1:
            leaf=tn(1,dim[f->d]);
            pri=px(leaf);
            pxi=px(flat);
            i(dim[f->d],pri[i]=pxi[j++])
            ((K*)px(f->r))[f->i++]=leaf;
            break;
          case -2:
            leaf=tn(2,dim[f->d]);
            prf=px(leaf);
            pxf=px(flat);
            i(dim[f->d],prf[i]=pxf[j++])
            ((K*)px(f->r))[f->i++]=leaf;
            break;
          case -8:
            leaf=tn(8,dim[f->d]);
            prj=px(leaf);
            pxj=px(flat);
            i(dim[f->d],prj[i]=pxj[j++])
            ((K*)px(f->r))[f->i++]=leaf;
            break;
          case -9:
            leaf=tn(9,dim[f->d]);
            pre=px(leaf);
            pxe=px(flat);
            i(dim[f->d],pre[i]=pxe[j++])
            ((K*)px(f->r))[f->i++]=leaf;
            break;
          case -3:
            leaf=tn(3,dim[f->d]);
            prc=px(leaf);
            pxc=px(flat);
            i(dim[f->d],prc[i]=pxc[j++])
            ((K*)px(f->r))[f->i++]=leaf;
            break;
          case -4:
            leaf=tn(4,dim[f->d]);
            prs=px(leaf);
            pxs=px(flat);
            i(dim[f->d],prs[i]=pxs[j++])
            ((K*)px(f->r))[f->i++]=leaf;
            break;
          default:
            leaf=tn(0,dim[f->d]);
            pxk=px(flat);
            for(i64 i=0;i<dim[f->d];++i) {
              K *pl=px(leaf);
              pl[i]=s(pxk[j])?kcp(pxk[j]):k_(pxk[j]);
              if(E(pl[i])) {
                e=pl[i];
                _k(leaf);
                _k(flat);
                xfree(stack);
                xfree(dim);
                goto cleanup;
              }
              ++j;
            }
            ((K*)px(f->r))[f->i++]=leaf;
            break;
        }
        sp--;
        continue;
      }

      // non-leaf: push next sub box
      if(f->i<dim[f->d]) {
        if(f->d+1==na-1) {
          // next depth is the leaf: push leaf frame directly
          if(sp==sm) { stack=xrealloc(stack,sizeof(SF)*(sm*=2)); f=&stack[sp-1]; }
          stack[sp++]=(SF){f->r,f->d+1,f->i};
          f->i++;
        }
        else {
          // next depth is intermediate: push sub container
          K sub=tn(0,dim[f->d+1]);
          ((K*)px(f->r))[f->i++]=sub;
          if(sp==sm) { stack=xrealloc(stack,sizeof(SF)*(sm*=2)); f=&stack[sp-1]; }
          stack[sp++]=(SF){sub,f->d+1,0};
        }
      }
      else --sp;
    }
    _k(flat);
    xfree(stack);
    xfree(dim);
    break;
  }
  default: return KERR_TYPE;
  }
  return knorm(r);
cleanup:
  if(r) _k(r);
  if(a0) _k(a0);
  if(a_) _k(a_);
  return e;
}
K take(K a, K x) { return take_(a,x); }

/* `f _ x` with a float f rounds x to a multiple of f: down (floor) for f>0, up (ceil) for f<0. */
static inline double dropmul(double f, double x) {
  double g,q,m;
  if(!isfinite(x)) return x;
  g = f<0.0 ? -f : f;
  q = x/g;
  if(!isfinite(q)) return x;  /* g subnormal (or 0): x/g overflows, but multiples
                                 of g are denser than the fp spacing near x, so x
                                 already rounds to itself -- return it, not inf */
  if(f<0.0) { m=ceil(q);  if(g*m<x) m+=1.0; if(g*(m-1.0)>=x) m-=1.0; }
  else      { m=floor(q); if(g*m>x) m-=1.0; if(g*(m+1.0)<=x) m+=1.0; }
  return g*m;
}

K drop(K a, K x) {
  K r=0,*prk,*pxk,r2,*pr2k;
  char *prc,*pxc,**prs,**pxs,*s,*pr2c,b[2],**pr2s;
  i32 *pri,*pai,*pxi,*pr2i;
  i64 j,m,p,q;               /* cut indices/positions: i64 for big x */
  i64 *prj,*pxj,*pr2j;
  float *pre,*pxe,*pr2e;
  double *prf,*pxf,f,*pr2f;
  if(s(a)) return KERR_TYPE;
  switch(ta) {
  case 8: { /* long drop count: route through take_ (handles i64 counts) */
    i64 av=jk(a);
    if(ax||s(x)) return k_(x);            /* drop from an atom -> unchanged */
    if(av==J_NULL||!av) return k_(x);
    { i64 mag=av<0?-av:av;
      if(mag>=(i64)nx) return tn(tx<0?-tx:0,0);          /* drop everything */
      K cnt=tj(av>0?-((i64)nx-av):(i64)nx+av);           /* >0 drop first->keep last; <0 drop last->keep first */
      K rr=take_(cnt,x); _k(cnt);                        /* take_ borrows cnt; must free it (cf. L1150) */
      return rr; }
  }
  case -1:
    if(s(x)) return KERR_TYPE;
    PAI;
    switch(tx) {
    case -1: PXI;
      i(na,if(pai[i]<0)return KERR_DOMAIN; if((u64)pai[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(pai[i]<j)return KERR_DOMAIN;j=pai[i])
      PRK(na);
      i(na,p=pai[i]; j=i==na-1?(i64)nx:pai[i+1]; prk[i]=r2=tn(1,j-p); pr2i=px(r2); for(q=0,m=p;m<j;m++) pr2i[q++]=pxi[m])
      break;
    case -2: PXF;
      i(na,if(pai[i]<0)return KERR_DOMAIN; if((u64)pai[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(pai[i]<j)return KERR_DOMAIN;j=pai[i])
      PRK(na);
      i(na,p=pai[i]; j=i==na-1?(i64)nx:pai[i+1]; prk[i]=r2=tn(2,j-p); pr2f=px(r2); for(q=0,m=p;m<j;m++) pr2f[q++]=pxf[m])
      break;
    case -8: PXJ;
      i(na,if(pai[i]<0)return KERR_DOMAIN; if((u64)pai[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(pai[i]<j)return KERR_DOMAIN;j=pai[i])
      PRK(na);
      i(na,p=pai[i]; j=i==na-1?(i64)nx:pai[i+1]; prk[i]=r2=tn(8,j-p); pr2j=px(r2); for(q=0,m=p;m<j;m++) pr2j[q++]=pxj[m])
      break;
    case -9: PXE;
      i(na,if(pai[i]<0)return KERR_DOMAIN; if((u64)pai[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(pai[i]<j)return KERR_DOMAIN;j=pai[i])
      PRK(na);
      i(na,p=pai[i]; j=i==na-1?(i64)nx:pai[i+1]; prk[i]=r2=tn(9,j-p); pr2e=px(r2); for(q=0,m=p;m<j;m++) pr2e[q++]=pxe[m])
      break;
    case -3: PXC;
      i(na,if(pai[i]<0)return KERR_DOMAIN; if((u64)pai[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(pai[i]<j)return KERR_DOMAIN;j=pai[i])
      PRK(na);
      i(na,p=pai[i]; j=i==na-1?(i64)nx:pai[i+1]; prk[i]=r2=tn(3,j-p); pr2c=px(r2); for(q=0,m=p;m<j;m++) pr2c[q++]=pxc[m];)
      break;
    case -4: PXS;
      i(na,if(pai[i]<0)return KERR_DOMAIN; if((u64)pai[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(pai[i]<j)return KERR_DOMAIN;j=pai[i])
      PRK(na);
      i(na,p=pai[i]; j=i==na-1?(i64)nx:pai[i+1]; prk[i]=r2=tn(4,j-p); pr2s=px(r2); for(q=0,m=p;m<j;m++) pr2s[q++]=pxs[m])
      break;
    case  0: PXK;
      i(na,if(pai[i]<0)return KERR_DOMAIN; if((u64)pai[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(pai[i]<j)return KERR_DOMAIN;j=pai[i])
      PRK(na);
      i(na,p=pai[i]; j=i==na-1?(i64)nx:pai[i+1]; prk[i]=r2=tn(0,j-p); pr2k=px(r2); for(q=0,m=p;m<j;m++) pr2k[q++]=k_(pxk[m]); r2=knorm(r2); prk[i]=r2)
      break;
    default: return KERR_TYPE;
    } break;
  case -8: {
    /* long cut-point vector: read positions as i64, mirroring case -1.  The
       slicing positions (p/j/m/q) are already i64, so this cuts a >2^31 vector
       at any point -- each slice is a flat sub-vector (not object-explosive). */
    i64 *paj;
    if(s(x)) return KERR_TYPE;
    paj=px(a);
    switch(tx) {
    case -1: PXI;
      i(na,if(paj[i]<0)return KERR_DOMAIN; if((u64)paj[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(paj[i]<j)return KERR_DOMAIN;j=paj[i])
      PRK(na);
      i(na,p=paj[i]; j=i==na-1?(i64)nx:paj[i+1]; prk[i]=r2=tn(1,j-p); pr2i=px(r2); for(q=0,m=p;m<j;m++) pr2i[q++]=pxi[m])
      break;
    case -2: PXF;
      i(na,if(paj[i]<0)return KERR_DOMAIN; if((u64)paj[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(paj[i]<j)return KERR_DOMAIN;j=paj[i])
      PRK(na);
      i(na,p=paj[i]; j=i==na-1?(i64)nx:paj[i+1]; prk[i]=r2=tn(2,j-p); pr2f=px(r2); for(q=0,m=p;m<j;m++) pr2f[q++]=pxf[m])
      break;
    case -8: PXJ;
      i(na,if(paj[i]<0)return KERR_DOMAIN; if((u64)paj[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(paj[i]<j)return KERR_DOMAIN;j=paj[i])
      PRK(na);
      i(na,p=paj[i]; j=i==na-1?(i64)nx:paj[i+1]; prk[i]=r2=tn(8,j-p); pr2j=px(r2); for(q=0,m=p;m<j;m++) pr2j[q++]=pxj[m])
      break;
    case -9: PXE;
      i(na,if(paj[i]<0)return KERR_DOMAIN; if((u64)paj[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(paj[i]<j)return KERR_DOMAIN;j=paj[i])
      PRK(na);
      i(na,p=paj[i]; j=i==na-1?(i64)nx:paj[i+1]; prk[i]=r2=tn(9,j-p); pr2e=px(r2); for(q=0,m=p;m<j;m++) pr2e[q++]=pxe[m])
      break;
    case -3: PXC;
      i(na,if(paj[i]<0)return KERR_DOMAIN; if((u64)paj[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(paj[i]<j)return KERR_DOMAIN;j=paj[i])
      PRK(na);
      i(na,p=paj[i]; j=i==na-1?(i64)nx:paj[i+1]; prk[i]=r2=tn(3,j-p); pr2c=px(r2); for(q=0,m=p;m<j;m++) pr2c[q++]=pxc[m];)
      break;
    case -4: PXS;
      i(na,if(paj[i]<0)return KERR_DOMAIN; if((u64)paj[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(paj[i]<j)return KERR_DOMAIN;j=paj[i])
      PRK(na);
      i(na,p=paj[i]; j=i==na-1?(i64)nx:paj[i+1]; prk[i]=r2=tn(4,j-p); pr2s=px(r2); for(q=0,m=p;m<j;m++) pr2s[q++]=pxs[m])
      break;
    case  0: PXK;
      i(na,if(paj[i]<0)return KERR_DOMAIN; if((u64)paj[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(paj[i]<j)return KERR_DOMAIN;j=paj[i])
      PRK(na);
      i(na,p=paj[i]; j=i==na-1?(i64)nx:paj[i+1]; prk[i]=r2=tn(0,j-p); pr2k=px(r2); for(q=0,m=p;m<j;m++) pr2k[q++]=k_(pxk[m]); r2=knorm(r2); prk[i]=r2)
      break;
    default: return KERR_TYPE;
    } break;
  }
  case 1:
    if(ax||s(x)) { r=k_(x); break; }
    switch(tx) {
    case -1:
      if((u64)llabs((i64)ik(a))>=nx) r=tn(1,0);
      else if(ik(a)>0) { PRI(nx-ik(a)); PXI; i(nx-ik(a),*pri++=pxi[i+ik(a)]) }
      else if(ik(a)<0) { PRI(nx+ik(a)); PXI; i(nx+ik(a),*pri++=*pxi++) }
      else r=k_(x);
      break;
    case -2:
      if((u64)llabs((i64)ik(a))>=nx) r=tn(2,0);
      else if(ik(a)>0) { PRF(nx-ik(a)); PXF; i(nx-ik(a),*prf++=pxf[i+ik(a)]) }
      else if(ik(a)<0) { PRF(nx+ik(a)); PXF; i(nx+ik(a),*prf++=*pxf++) }
      else r=k_(x);
      break;
    case -8:
      if((u64)llabs((i64)ik(a))>=nx) r=tn(8,0);
      else if(ik(a)>0) { PRJ(nx-ik(a)); PXJ; i(nx-ik(a),*prj++=pxj[i+ik(a)]) }
      else if(ik(a)<0) { PRJ(nx+ik(a)); PXJ; i(nx+ik(a),*prj++=*pxj++) }
      else r=k_(x);
      break;
    case -9:
      if((u64)llabs((i64)ik(a))>=nx) r=tn(9,0);
      else if(ik(a)>0) { PRE(nx-ik(a)); PXE; i(nx-ik(a),*pre++=pxe[i+ik(a)]) }
      else if(ik(a)<0) { PRE(nx+ik(a)); PXE; i(nx+ik(a),*pre++=*pxe++) }
      else r=k_(x);
      break;
    case -3:
      if((u64)llabs((i64)ik(a))>=nx) r=tn(3,0);
      else if(ik(a)>0) { PRC(nx-ik(a)); PXC; i(nx-ik(a),*prc++=pxc[i+ik(a)]) }
      else if(ik(a)<0) { PRC(nx+ik(a)); PXC; i(nx+ik(a),*prc++=*pxc++) }
      else r=k_(x);
      break;
    case -4:
      if((u64)llabs((i64)ik(a))>=nx) r=tn(4,0);
      else if(ik(a)>0) { PRS(nx-ik(a)); PXS; i(nx-ik(a),*prs++=pxs[i+ik(a)]) }
      else if(ik(a)<0) { PRS(nx+ik(a)); PXS; i(nx+ik(a),*prs++=*pxs++) }
      else r=k_(x);
      break;
    case  0:
      if((u64)llabs((i64)ik(a))>=nx) r=tn(0,0);
      else if(ik(a)>0) { PRK(nx-ik(a)); PXK; i(nx-ik(a),*prk++=k_(pxk[i+ik(a)])) }
      else if(ik(a)<0) { PRK(nx+ik(a)); PXK; i(nx+ik(a),*prk++=k_(*pxk++)) }
      else r=k_(x);
      break;
    } break;
  case 2:
    /* round-to-multiple: x rounded to a multiple of the FLOAT a.  An f64
       multiple dominates every x type (int/long/f32/f64), so the result is
       always f64 -- same promotion as `a + x`.  f==0 -> x itself; a NaN/inf
       multiple -> NaN. */
    if(s(x)) return KERR_TYPE;
    f=fk(a);
    switch(tx) {
    case 1:
      if(f==0.0) r=t2(fi(ik(x)));
      else if(isnan(f)||isinf(f)) r=t2(NAN);
      else r=t2(dropmul(f,fi(ik(x))));
      break;
    case  2:
      if(f==0.0) r=t2(fk(x));
      else if(isnan(f)||isinf(f)) r=t2(NAN);
      else r=t2(dropmul(f,fk(x)));
      break;
    case  8:
      if(f==0.0) r=t2(fj(jk(x)));
      else if(isnan(f)||isinf(f)) r=t2(NAN);
      else r=t2(dropmul(f,fj(jk(x))));
      break;
    case  9:
      if(f==0.0) r=t2((double)ek(x));
      else if(isnan(f)||isinf(f)) r=t2(NAN);
      else r=t2(dropmul(f,(double)ek(x)));
      break;
    case -1:
      PRF(nx); PXI;
      if(f==0.0) i(nx,prf[i]=fi(pxi[i]))
      else if(isnan(f)||isinf(f)) i(nx,prf[i]=NAN)
      else i(nx,prf[i]=dropmul(f,fi(pxi[i])))
      break;
    case -2:
      PRF(nx); PXF;
      if(f==0.0) i(nx,prf[i]=pxf[i])
      else if(isnan(f)||isinf(f)) i(nx,prf[i]=NAN)
      else i(nx,prf[i]=dropmul(f,pxf[i]))
      break;
    case -8:
      PRF(nx); PXJ;
      if(f==0.0) i(nx,prf[i]=fj(pxj[i]))
      else if(isnan(f)||isinf(f)) i(nx,prf[i]=NAN)
      else i(nx,prf[i]=dropmul(f,fj(pxj[i])))
      break;
    case -9:
      PRF(nx); PXE;
      if(f==0.0) i(nx,prf[i]=(double)pxe[i])
      else if(isnan(f)||isinf(f)) i(nx,prf[i]=NAN)
      else i(nx,prf[i]=dropmul(f,(double)pxe[i]))
      break;
    default: return KERR_TYPE;
    } break;
  case 9: {
    /* round-to-multiple with an f32 (real) multiple.  Result type follows
       `a + x`: f32 for x in {int,f32}, but f64 for x in {long,f64} (an f64 or
       a 64-bit-precision long dominates).  dropmul computes in double; the
       f32 arms narrow the result. */
    if(s(x)) return KERR_TYPE;
    double ff=(double)ek(a);
    int f0=(ff==0.0), fbad=(isnan(ff)||isinf(ff));
    switch(tx) {
    case 1:  r=te(f0?(float)fi(ik(x)):fbad?(float)NAN:(float)dropmul(ff,fi(ik(x)))); break;
    case 9:  r=te(f0?ek(x):fbad?(float)NAN:(float)dropmul(ff,(double)ek(x))); break;
    case 8:  r=t2(f0?fj(jk(x)):fbad?NAN:dropmul(ff,fj(jk(x)))); break;
    case 2:  r=t2(f0?fk(x):fbad?NAN:dropmul(ff,fk(x))); break;
    case -1:
      PRE(nx); PXI;
      if(f0) i(nx,pre[i]=(float)fi(pxi[i]))
      else if(fbad) i(nx,pre[i]=(float)NAN)
      else i(nx,pre[i]=(float)dropmul(ff,fi(pxi[i])))
      break;
    case -9:
      PRE(nx); PXE;
      if(f0) i(nx,pre[i]=pxe[i])
      else if(fbad) i(nx,pre[i]=(float)NAN)
      else i(nx,pre[i]=(float)dropmul(ff,(double)pxe[i]))
      break;
    case -8:
      PRF(nx); PXJ;
      if(f0) i(nx,prf[i]=fj(pxj[i]))
      else if(fbad) i(nx,prf[i]=NAN)
      else i(nx,prf[i]=dropmul(ff,fj(pxj[i])))
      break;
    case -2:
      PRF(nx); PXF;
      if(f0) i(nx,prf[i]=pxf[i])
      else if(fbad) i(nx,prf[i]=NAN)
      else i(nx,prf[i]=dropmul(ff,pxf[i]))
      break;
    default: return KERR_TYPE;
    } break;
  }
  case 3:
    if(s(x)) return KERR_TYPE;
    switch(tx) {
    case -3: s=xstrndup((char*)px(x),nx); b[0]=ck(a); b[1]=0; r=ksplit(s,b); xfree(s); break;
    case  4: s=xstrdup(sk(x)); b[0]=ck(a); b[1]=0; r=ksplit(s,b); xfree(s); break;
    default: return KERR_TYPE;
    } break;
  default: return KERR_TYPE;
  }
  return knorm(r);
}

/* power: K reports a domain error for a negative base raised to a
   non-integral exponent (gk has no complex numbers).  Scan operands
   (broadcasting atoms) for any such (base;exp) pair before computing. */
static double powel_(K v, u64 i) {
  switch(T(v)) {
  case  1: return fi(ik(v));
  case  2: return         fk(v);
  case  8: return fj(jk(v));
  case  9: return (double)ek(v);
  case -1: return fi(((i32*)px(v))[i]);
  case -2: return         ((double*)px(v))[i];
  case -8: return fj(((i64*)px(v))[i]);
  case -9: return (double)((float*)px(v))[i];
  } return 0;
}
static i32 powdom_(K a, K x) {
  if(ta==0 || tx==0) return 0;            /* lists handled by recursion */
  u64 n = ta<0 ? na : (tx<0 ? nx : 1);
  for(u64 i=0;i<n;i++) {
    double b=powel_(a,i), e=powel_(x,i);
    if(b<0 && isfinite(b) && isfinite(e) && e!=floor(e)) return 1;
  }
  return 0;
}
K power(K a, K x) {
  K r=0;
  i32 *pai,*pxi;
  i64 *paj,*pxj;
  float *pae,*pxe,*pre,ef;
  double f,*prf,*paf,*pxf;
  if(s(x)||s(a)) return KERR_TYPE;
  if(ta <= 0 && tx <= 0 && na != nx) return KERR_LENGTH;
  if(powdom_(a,x)) return KERR_DOMAIN;
  switch(ta) {
  case 1:
    switch(tx) {
    case  1: f=pow(fi(ik(a)),fi(ik(x))); r=t2(f); break;
    case  2: f=pow(fi(ik(a)),fk(x)); r=t2(f); break;
    case  8: f=pow(fi(ik(a)),fj(jk(x))); r=t2(f); break;
    case  9: r=te(powf(ei(ik(a)),ek(x))); break;
    case -1: PRF(nx); PXI; i(nx,prf[i]=pow(fi(ik(a)),fi(pxi[i]))) break;
    case -2: PRF(nx); PXF; i(nx,prf[i]=pow(fi(ik(a)),pxf[i])) break;
    case -8: PRF(nx); PXJ; i(nx,prf[i]=pow(fi(ik(a)),fj(pxj[i]))) break;
    case -9: PRE(nx); PXE; ef=ei(ik(a)); i(nx,pre[i]=powf(ef,pxe[i])) break;
    case  0: r=irecur2(power,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 2:
    switch(tx) {
    case  1: f=pow(fk(a),fi(ik(x))); r=t2(f); break;
    case  2: f=pow(fk(a),fk(x)); r=t2(f); break;
    case  8: f=pow(fk(a),fj(jk(x))); r=t2(f); break;
    case  9: f=pow(fk(a),(double)ek(x)); r=t2(f); break;
    case -1: PRF(nx); PXI; i(nx, prf[i]=pow(fk(a),fi(pxi[i]))) break;
    case -2: PRF(nx); PXF; i(nx, prf[i]=pow(fk(a),pxf[i])) break;
    case -8: PRF(nx); PXJ; i(nx, prf[i]=pow(fk(a),fj(pxj[i]))) break;
    case -9: PRF(nx); PXE; i(nx, prf[i]=pow(fk(a),(double)pxe[i])) break;
    case  0: r=irecur2(power,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 8:
    switch(tx) {
    case  1: f=pow(fj(jk(a)),fi(ik(x))); r=t2(f); break;
    case  2: f=pow(fj(jk(a)),fk(x)); r=t2(f); break;
    case  8: f=pow(fj(jk(a)),fj(jk(x))); r=t2(f); break;
    case  9: f=pow(fj(jk(a)),(double)ek(x)); r=t2(f); break;
    case -1: PRF(nx); PXI; { double A=fj(jk(a)); i(nx,prf[i]=pow(A,fi(pxi[i]))) } break;
    case -2: PRF(nx); PXF; { double A=fj(jk(a)); i(nx,prf[i]=pow(A,pxf[i])) } break;
    case -8: PRF(nx); PXJ; { double A=fj(jk(a)); i(nx,prf[i]=pow(A,fj(pxj[i]))) } break;
    case -9: PRF(nx); PXE; { double A=fj(jk(a)); i(nx,prf[i]=pow(A,(double)pxe[i])) } break;
    case  0: r=irecur2(power,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 9:
    switch(tx) {
    case  1: r=te(powf(ek(a),ei(ik(x)))); break;
    case  2: f=pow((double)ek(a),fk(x)); r=t2(f); break;
    case  8: f=pow((double)ek(a),fj(jk(x))); r=t2(f); break;
    case  9: r=te(powf(ek(a),ek(x))); break;
    case -1: PRE(nx); PXI; ef=ek(a); i(nx,pre[i]=powf(ef,ei(pxi[i]))) break;
    case -2: PRF(nx); PXF; { double A=(double)ek(a); i(nx,prf[i]=pow(A,pxf[i])) } break;
    case -8: PRF(nx); PXJ; { double A=(double)ek(a); i(nx,prf[i]=pow(A,fj(pxj[i]))) } break;
    case -9: PRE(nx); PXE; ef=ek(a); i(nx,pre[i]=powf(ef,pxe[i])) break;
    case  0: r=irecur2(power,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -1:
    switch(tx) {
    case  1: PRF(na); PAI; i(na, prf[i]=pow(fi(pai[i]),fi(ik(x)))) break;
    case  2: PRF(na); PAI; i(na, prf[i]=pow(fi(pai[i]),fk(x))) break;
    case  8: PRF(na); PAI; { double X=fj(jk(x)); i(na,prf[i]=pow(fi(pai[i]),X)) } break;
    case  9: PRE(na); PAI; ef=ek(x); i(na,pre[i]=powf(ei(pai[i]),ef)) break;
    case -1: PRF(na); PAI; PXI; i(na, prf[i]=pow(fi(pai[i]),fi(pxi[i]))) break;
    case -2: PRF(na); PAI; PXF; i(na, prf[i]=pow(fi(pai[i]),pxf[i])) break;
    case -8: PRF(na); PAI; PXJ; i(na, prf[i]=pow(fi(pai[i]),fj(pxj[i]))) break;
    case -9: PRE(na); PAI; PXE; i(na, pre[i]=powf(ei(pai[i]),pxe[i])) break;
    case  0: r=each(17,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -2:
    switch(tx) {
    case  1: PRF(na); PAF; i(na, prf[i]=pow(paf[i],fi(ik(x)))) break;
    case  2: PRF(na); PAF; i(na, prf[i]=pow(paf[i],fk(x))) break;
    case  8: PRF(na); PAF; { double X=fj(jk(x)); i(na,prf[i]=pow(paf[i],X)) } break;
    case  9: PRF(na); PAF; { double X=(double)ek(x); i(na,prf[i]=pow(paf[i],X)) } break;
    case -1: PRF(na); PAF; PXI; i(na, prf[i]=pow(paf[i],fi(pxi[i]))) break;
    case -2: PRF(na); PAF; PXF; i(na, prf[i]=pow(paf[i],pxf[i])) break;
    case -8: PRF(na); PAF; PXJ; i(na, prf[i]=pow(paf[i],fj(pxj[i]))) break;
    case -9: PRF(na); PAF; PXE; i(na, prf[i]=pow(paf[i],(double)pxe[i])) break;
    case  0: r=each(17,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -8:
    switch(tx) {
    case  1: PRF(na); PAJ; { double X=fi(ik(x)); i(na,prf[i]=pow(fj(paj[i]),X)) } break;
    case  2: PRF(na); PAJ; { double X=fk(x); i(na,prf[i]=pow(fj(paj[i]),X)) } break;
    case  8: PRF(na); PAJ; { double X=fj(jk(x)); i(na,prf[i]=pow(fj(paj[i]),X)) } break;
    case  9: PRF(na); PAJ; { double X=(double)ek(x); i(na,prf[i]=pow(fj(paj[i]),X)) } break;
    case -1: PRF(na); PAJ; PXI; i(na, prf[i]=pow(fj(paj[i]),fi(pxi[i]))) break;
    case -2: PRF(na); PAJ; PXF; i(na, prf[i]=pow(fj(paj[i]),pxf[i])) break;
    case -8: PRF(na); PAJ; PXJ; i(na, prf[i]=pow(fj(paj[i]),fj(pxj[i]))) break;
    case -9: PRF(na); PAJ; PXE; i(na, prf[i]=pow(fj(paj[i]),(double)pxe[i])) break;
    case  0: r=each(17,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -9:
    switch(tx) {
    case  1: PRE(na); PAE; ef=ei(ik(x)); i(na,pre[i]=powf(pae[i],ef)) break;
    case  2: PRF(na); PAE; { double X=fk(x); i(na,prf[i]=pow((double)pae[i],X)) } break;
    case  8: PRF(na); PAE; { double X=fj(jk(x)); i(na,prf[i]=pow((double)pae[i],X)) } break;
    case  9: PRE(na); PAE; ef=ek(x); i(na,pre[i]=powf(pae[i],ef)) break;
    case -1: PRE(na); PAE; PXI; i(na, pre[i]=powf(pae[i],ei(pxi[i]))) break;
    case -2: PRF(na); PAE; PXF; i(na, prf[i]=pow((double)pae[i],pxf[i])) break;
    case -8: PRF(na); PAE; PXJ; i(na, prf[i]=pow((double)pae[i],fj(pxj[i]))) break;
    case -9: PRE(na); PAE; PXE; i(na, pre[i]=powf(pae[i],pxe[i])) break;
    case  0: r=each(17,a,x); break;
    default: return KERR_TYPE;
    } break;
  case  0:
    switch(tx) {
    case  1: r=irecur2(power,a,x); break;
    case  2: r=irecur2(power,a,x); break;
    case  8: r=irecur2(power,a,x); break;
    case  9: r=irecur2(power,a,x); break;
    case -1: r=each(17,a,x); break;
    case -2: r=each(17,a,x); break;
    case -8: r=each(17,a,x); break;
    case -9: r=each(17,a,x); break;
    case  0: r=irecur2(power,a,x); break;
    default: return KERR_TYPE;
    } break;
  default: return KERR_TYPE;
  }
  return knorm(r);
}

K join(K a, K x) {
  K r=0,e,*prk;
  i32 *pri;
  i64 *prj;
  float *pre;
  double *prf;
  char *prc,**prs;
  i8 Ta,Tx;
  u64 j=0;
  Ta=ta; if(s(a)) { if(!vstcb(a)) return KERR_TYPE; Ta=15; }
  Tx=tx; if(s(x)) { if(!vstcb(x)) return KERR_TYPE; Tx=15; }
  if(Ta<=0&&Tx<=0&&!na&&!nx&&Ta!=Tx) return tn(0,0);
  if(Ta>0&&Tx>0) { /* both scalars */
    if(Ta==1&&Tx==1) { PRI(2); pri[0]=ik(a); pri[1]=ik(x); }
    else if(Ta==2&&Tx==2) { PRF(2); prf[0]=fk(a); prf[1]=fk(x); }
    else if(Ta==8&&Tx==8) { PRJ(2); prj[0]=jk(a); prj[1]=jk(x); }
    else if(Ta==9&&Tx==9) { PRE(2); pre[0]=ek(a); pre[1]=ek(x); }
    else if(Ta==3&&Tx==3) { PRC(2); prc[0]=ck(a); prc[1]=ck(x); }
    else if(Ta==4&&Tx==4) { PRS(2); prs[0]=sk(a); prs[1]=sk(x); }
    else {
      PRK(2);
      prk[0]=(Ta==2||Ta==8)?k_(a):Ta==15?kcp(a):a;
      EC(prk[0]);
      prk[1]=(Tx==2||Tx==8)?k_(x):Tx==15?kcp(x):x;
      EC(prk[1]);
    }
    return r;
  }
  if(Ta>0&&Tx==-Ta) { /* scalar + matching vector */
    switch(Tx) {
    case -1: PRI(1+nx); pri[0]=ik(a); i(nx,pri[1+i]=((i32*)px(x))[i]) break;
    case -2: PRF(1+nx); prf[0]=fk(a); i(nx,prf[1+i]=((double*)px(x))[i]) break;
    case -8: PRJ(1+nx); prj[0]=jk(a); i(nx,prj[1+i]=((i64*)px(x))[i]) break;
    case -9: PRE(1+nx); pre[0]=ek(a); i(nx,pre[1+i]=((float*)px(x))[i]) break;
    case -3: PRC(1+nx); prc[0]=ck(a); i(nx,prc[1+i]=((char*)px(x))[i]) break;
    case -4: PRS(1+nx); prs[0]=sk(a); i(nx,prs[1+i]=((char**)px(x))[i]) break;
    }
    return r;
  }
  if(Ta<0&&Tx==-Ta) { /* vector + matching scalar */
    switch(Ta) {
    case -1: PRI(na+1); i(na,pri[i]=((i32*)px(a))[i]) pri[na]=ik(x); break;
    case -2: PRF(na+1); i(na,prf[i]=((double*)px(a))[i]) prf[na]=fk(x); break;
    case -8: PRJ(na+1); i(na,prj[i]=((i64*)px(a))[i]) prj[na]=jk(x); break;
    case -9: PRE(na+1); i(na,pre[i]=((float*)px(a))[i]) pre[na]=ek(x); break;
    case -3: PRC(na+1); i(na,prc[i]=((char*)px(a))[i]) prc[na]=ck(x); break;
    case -4: PRS(na+1); i(na,prs[i]=((char**)px(a))[i]) prs[na]=sk(x); break;
    }
    return r;
  }
  if(Ta<=0&&Tx<=0&&Ta==Tx) { /* same type vectors */
    switch(Ta) {
    case -1: PRI(na+nx); i(na,pri[j++]=((i32*)px(a))[i]) i(nx,pri[j++]=((i32*)px(x))[i]) break;
    case -2: PRF(na+nx); i(na,prf[j++]=((double*)px(a))[i]) i(nx,prf[j++]=((double*)px(x))[i]) break;
    case -8: PRJ(na+nx); i(na,prj[j++]=((i64*)px(a))[i]) i(nx,prj[j++]=((i64*)px(x))[i]) break;
    case -9: PRE(na+nx); i(na,pre[j++]=((float*)px(a))[i]) i(nx,pre[j++]=((float*)px(x))[i]) break;
    case -3: PRC(na+nx); i(na,prc[j++]=((char*)px(a))[i]) i(nx,prc[j++]=((char*)px(x))[i]) break;
    case -4: PRS(na+nx); i(na,prs[j++]=((char**)px(a))[i]) i(nx,prs[j++]=((char**)px(x))[i]) break;
    case  0: PRK(na+nx); i(na,prk[j++]=k_(((K*)px(a))[i])) i(nx,prk[j++]=k_(((K*)px(x))[i])) break;
    }
    return r;
  }
  /* box to K list */
  u64 an=Ta>0?1:na,xn=Tx>0?1:nx;
  PRK(an+xn);
  if(Ta>0) { prk[j]=(Ta==2||Ta==8)?k_(a):Ta==15?kcp(a):a; EC(prk[j]); ++j; }
  else i(na,prk[j++]=xi_(a,i,Ta))
  if(Tx>0) { prk[j]=(Tx==2||Tx==8)?k_(x):Tx==15?kcp(x):x; EC(prk[j]); ++j; }
  else i(nx,prk[j++]=xi_(x,i,Tx))
  return knorm(r);
cleanup:
  if(r) _k(r);
  return e;
}

static K form2w(char *t, i32 w, i32 z) {
  K r=0;
  i64 l;                  /* l = strlen: a char vector can exceed 2^31, so
                             these MUST be 64-bit.  In an i32, `N $ x` with
                             #x >= 2^31 made l negative, so `m>l` took the
                             PAD branch: PRC(m) allocated m (e.g. 5) bytes and
                             then wrote n=abs(l-m) ~= 2^31 spaces into it -- a
                             heap overflow, and `5$x` is the ordinary
                             spelling (a negative width took the safe arm).
                             With l 64-bit the m<l TRUNCATE branch is taken,
                             which is the correct answer. */
  i32 m;
  char *prc;
  if(w==INT32_MIN) return KERR_DOMAIN; /* abs(INT32_MIN) is UB */
  l=(i64)strlen(t); m=abs(w);
  /* Every read of `t` below is written as an explicit t[0 .. l-1], where l is
     strlen(t).  The pad arm used to walk a cursor (`prc[i]=*t++`) with its
     bound expressed via n=|l-m| instead, which is the same thing but leaves
     the reads tied to n rather than to l -- `clang --analyze` could not relate
     the two and reported a false uninitialized read on the path where fmtjs
     writes the empty null spelling (l==0, so the copy is zero-iteration).
     Bounding by l directly keeps `make analyze` clean and makes the in-range
     argument local. */
  if(m>l) {                       /* pad to width m */
    i64 pad=(i64)m-l;
    PRC(m);
    if(w<0)      { memcpy(prc,t,(size_t)l); memset(prc+l,' ',(size_t)pad); }
    else if(w>0) { memset(prc,' ',(size_t)pad); memcpy(prc+pad,t,(size_t)l); }
  }
  else if(m<l) {                  /* truncate to width m */
    PRC(m);
    if(z) { i(m,*prc++='*') *prc=0; }
    else if(w<0) memcpy(prc,t,(size_t)m);          /* keep the head */
    else         memcpy(prc,t+(l-m),(size_t)m);    /* keep the tail */
  }
  else { PRC(l); memcpy(prc,t,(size_t)l); }        /* m==l: exact fit */
  return r;
}
/* decimal itoa for the $ paths: byte-identical to sprintf("%d"/"%lld")
   but without the printf machinery (which was ~42% of atom-$ Ir).  The u64
   magnitude makes INT64_MIN safe.  Writes NUL, returns length. */
static int fmtj(char *d, i64 v) {
  char b[20]; int n=0,m=0;
  u64 u = v<0 ? 0-(u64)v : (u64)v;
  if(v<0) d[m++]='-';
  do { b[n++] = (char)('0'+(int)(u%10)); u/=10; } while(u);
  while(n) d[m++]=b[--n];
  d[m]=0;
  return m;
}

/* $-parse helpers.  trimtok: strip surrounding blanks in place.
   0$" 12 " is 12, 0.0$" 1.5 " is 1.5)
   inttok: a valid int token is empty (the null),
   a 0N/0I sentinel spelling, or [-]digits -- anything else is a
   domain error (0$"-4000000000" is -0I, 0$"12x" and 0$"+5" are domain
   errors).  Range handling lives in xatoi/xatol. */
/* Sentinel-aware decimal text for the $ paths.  Monadic format() (below) has
   always spelled the sentinels 0I / -0I / "" ; DYADIC a$x went straight to
   fmtj/sprintf and printed the raw bit pattern instead, so `20$0N` gave
   "         -2147483648" and `20$0I` gave "          2147483647" */
/* The spellings are 2-3 bytes, written out directly: strcpy appears nowhere
   else in gk, and the MSVC/clang-on-Windows UCRT headers mark it deprecated,
   so using it here was the sole source of -Wdeprecated-declarations in the
   Windows build.  d is the caller's 256-byte scratch buffer. */
static void spell(char *d, const char *s) { while((*d++=*s++)); }
static void fmt1s(char *d, i32 v) {
  if(v==INT32_MAX)        spell(d,"0I");
  else if(v==INT32_MIN+1) spell(d,"-0I");
  else if(v==INT32_MIN)   d[0]=0;
  else fmtj(d,(i64)v);
}
static void fmtjs(char *d, i64 v) {
  if(v==J_INF)       spell(d,"0I");
  else if(v==J_NINF) spell(d,"-0I");
  else if(v==J_NULL) d[0]=0;
  else fmtj(d,v);
}
static void fmtfs(char *d, double f, i32 prec) {
  if(isinf(f)&&f>0.0)      spell(d,"0i");
  else if(isinf(f)&&f<0.0) spell(d,"-0i");
  else if(isnan(f))        d[0]=0;
  else sprintf(d,"%0.*g",prec,f);
}

static char* trimtok(char *s) {
  char *e=s+strlen(s);
  while(*s==' '||*s=='\t'||*s=='\n'||*s=='\r') ++s;
  while(e>s&&(e[-1]==' '||e[-1]=='\t'||e[-1]=='\n'||e[-1]=='\r')) --e;
  *e=0;
  return s;
}
static int inttok(char *s) {
  if(!*s) return 1;
  if(!strcmp(s,"0N")||!strcmp(s,"-0N")||!strcmp(s,"0I")||!strcmp(s,"-0I")) return 1;
  if('-'==*s) ++s;
  if(!*s) return 0;
  for(;*s;++s) if(*s<48||*s>57) return 0;
  return 1;
}

K form(K a, K x) {
  K r=0,e,*prk;
  i32 l,m,xx,y,*pai;
  char t[2048],*s=t,*prc,*pxc,*p;
  double f,g;
  if(s(a)||s(x)) return formcb(a,x);
  if(ta<=0 && tx<=0 && na!=nx && ta!=-3 && tx!=-3) return KERR_LENGTH;
  switch(ta) {
  case 1:
    if(ik(a)==INT32_MAX||ik(a)==INT32_MIN||ik(a)==INT32_MIN+1) return KERR_DOMAIN;
    switch(tx) {
    case  1: fmt1s(t,ik(x)); r=form2w(t,ik(a),1); break;
    case  2: fmtfs(t,fk(x),7); r=form2w(t,ik(a),1); break;
    case  8: fmtjs(t,jk(x)); r=form2w(t,ik(a),1); break;
    case  9: fmtfs(t,(double)ek(x),7); r=form2w(t,ik(a),1); break;
    case  3:
      if(!ik(a)) { if(ik(x)<48||ik(x)>57) return KERR_DOMAIN; r=t(1,(u32)ik(x)-48); break; }
      sprintf(t,"%c",ik(x));
      r=form2w(t,ik(a),0);
      break;
    case  4:
      p=xstrdup(sk(x));
      if(strlen(p)>(K)llabs((i64)ik(a))) { PRC(llabs((i64)ik(a))); i(n(r),prc[i]='*'); }
      else r=form2w(p,ik(a),0);
      xfree(p);
      break;
    case -1:
    case -2:
    case -8:
    case -9: r=each(19,a,x); break;
    case  0: r=irecur2(form,a,x); break;
    case -3: {
      /* Parse a NUL-terminated COPY, never `pxc[nx]=0` into the input: a
         1:-mmap'd char vector is exactly nx bytes at the tail of its mapping
         (no terminator slot), so the write is a 1-byte OOB -- a SIGSEGV when
         the mapping ends on a page boundary, and it also mutates the caller's
         value.  Stack buffer for the common short case, heap for the rest. */
      PXC; char sbuf[256], *cs=nx<(u64)sizeof(sbuf)?sbuf:xmalloc(nx+1);
      memcpy(cs,pxc,nx); cs[nx]=0;
      if(!ik(a)) {
        char *b=trimtok(cs);
        if(!inttok(b)) { if(cs!=sbuf) xfree(cs); return KERR_DOMAIN; }
        r=t(1,(u32)xatoi(b));
      }
      else r=form2w(cs,ik(a),0);
      if(cs!=sbuf) xfree(cs);
      break;
    }
    case -4: r=each(19,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 2:
    switch(tx) {
    case  1:
    case  2:
    case  8:
    case  9:
      if(isnan(fk(a))) return KERR_DOMAIN;
      f= tx==1?fi(ik(x)) : tx==2?fk(x) : tx==8?fj(jk(x)) : (double)ek(x);
      sprintf(t,"%g",round(fk(a)*10)/10); s=strchr(t,'.');
      y=s?s[1]-48:0;
      g=fk(a);
      /* guard the float->i64 cast in VSIZE (and float->i32 in xx=g below):
         (i64)inf / (i64)huge is UB.  isnan is already rejected above; inf and
         out-of-i64-range widths return wsfull like the post-cap check does in
         production, but without the UB. */
      if(!isfinite(g) || fabs(g) >= 9e18) return KERR_WSFULL;
      VSIZE(fabs(g));
      xx=g;
      //VSIZE(abs(xx));
      m=abs(xx);
      /* Null formats as an empty field, but infinities keep their language
         spelling.  K's float-width form right-aligns 10.2$-0i as
         "       -0i"; treating every non-finite value as empty lost the sign
         and infinity marker.  This also gives widened i32/i64 sentinels the
         same result as native float sentinels. */
      if(isnan(f)) *t=0;
      else if(isinf(f)) spell(t,f<0?"-0i":"0i");
      else if(xx<0||*t=='-') sprintf(t,"%s%0.*e",f<0?"":" ",y,f);
      else sprintf(t,"%0.*f",y,f);
      l=strlen(t);
      if(l>INT32_MAX-m-1) return KERR_WSFULL;
      s=xcalloc(1,l+m+1);
      if(!m) sprintf(s,"%s",t);
      else if(m<l) { i(m,s[i]='*') s[m]=0; }
      else if(xx<0) sprintf(s,"%s%*s",t,m-l,"");
      else sprintf(s,"%*s%s",m-l,"",t);
      PRC(strlen(s));
      i(strlen(s),prc[i]=s[i])
      xfree(s);
      break;
    case  3: if(ck(x)<48||ck(x)>57) return KERR_DOMAIN; sprintf(s,"%c",ck(x)); r=t2(xstrtod(s)); break;
    case  4: return KERR_TYPE;
    case  0: r=irecur2(form,a,x); break;
    case -1:
    case -2:
    case -8:
    case -9: r=each(19,a,x); break;
    case -3: {
      /* NUL-terminated copy, not the raw payload */
      PXC; char sbuf[256], *cs=nx<(u64)sizeof(sbuf)?sbuf:xmalloc(nx+1);
      memcpy(cs,pxc,nx); cs[nx]=0;
      { char *b=trimtok(cs); double dd=xstrtod(b);
        if(isnan(dd)&&*b&&strcmp(b,"0n")&&strcmp(b,"-0n")&&strcmp(b,"0N")&&strcmp(b,"-0N")) { if(cs!=sbuf) xfree(cs); return KERR_DOMAIN; }
        r=t2(dd); }
      if(cs!=sbuf) xfree(cs);
      break;
    }
    default: return KERR_TYPE;
    } break;
  case 8: /* Nj$x : N is the width. 0j$<int token/digit char> parses to long
             (0j$"807"->807j; [-]digits, 0N/0I spellings, surrounding blanks
             -- see inttok). Everything else -- numbers, and nonzero-width
             strings -- delegates to int $ (number->format so 0j$3.9->"";
             Nj$"s" right-justifies like int). */
    if(jk(a)>=INT32_MAX||jk(a)<=(i64)INT32_MIN+1) return KERR_DOMAIN;
    if(!jk(a) && tx==3) { if(ck(x)<48||ck(x)>57) return KERR_DOMAIN; r=tj((i64)ck(x)-48); break; }
    if(!jk(a) && tx==-3) {
      /* NUL-terminated copy, not `pxc[nx]=0` -- see case -3 above (mmap OOB) */
      PXC; char sbuf[256], *cs=nx<(u64)sizeof(sbuf)?sbuf:xmalloc(nx+1);
      memcpy(cs,pxc,nx); cs[nx]=0;
      { char *b=trimtok(cs);
        if(!inttok(b)) { if(cs!=sbuf) xfree(cs); return KERR_DOMAIN; }
        r=tj(xatol(b)); }
      if(cs!=sbuf) xfree(cs);
      break;
    }
    if(!jk(a) && tx==0) { r=irecur2(form,a,x); break; } /* list: recurse keeping the long tag (matches case 9) */
    { K ai=t(1,(u32)(i32)jk(a)); r=form(ai,x); _k(ai); }
    break;
  case 9: /* N.Me$x : N.M is width/precision. A NUMBER formats (delegates to
             float $, so 0.0e$3.2 -> "3"); a string/digit char
             parses to real (0.0e$"3.5" -> 3.5e, any width, like float $). */
    switch(tx) {
    case  3: if(ck(x)<48||ck(x)>57) return KERR_DOMAIN; { char cs[2]; cs[0]=ck(x); cs[1]=0; r=te((float)xstrtod(cs)); } break;
    case -3: {
      /* NUL-terminated copy, not the raw payload -- see int $ case -3 (mmap
         OOB).  xstrtoe takes the f32 spellings the lexer takes (0ne/0ie/-0ie,
         trailing-e through a decimal point or exponent -- never "5e").
         Parse failure is a domain error like float $; empty is the null;
         the f64 null spellings adopt, as they do in an e vector literal. */
      PXC; char sbuf[256], *cs=nx<(u64)sizeof(sbuf)?sbuf:xmalloc(nx+1);
      memcpy(cs,pxc,nx); cs[nx]=0;
      { char *b=trimtok(cs); float ge=xstrtoe(b);
        if(isnan(ge)&&*b&&strcmp(b,"0ne")&&strcmp(b,"-0ne")&&strcmp(b,"0n")&&strcmp(b,"-0n")&&strcmp(b,"0N")&&strcmp(b,"-0N")) { if(cs!=sbuf) xfree(cs); return KERR_DOMAIN; }
        r=te(ge); }
      if(cs!=sbuf) xfree(cs);
      break;
    }
    case  1: case  2: case  8: case  9:
      { K af=t2((double)ek(a)); r=form(af,x); _k(af); }
      break;
    case -1: case -2: case -8: case -9: r=each(19,a,x); break;
    case  0: r=irecur2(form,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 3:
    switch(tx) {
    case  3:
    case -3: r=k_(x); break;
    case  0: r=irecur2(form,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 4:
    if(strlen(sk(a))) return KERR_DOMAIN;
    switch(tx) {
    case  3: sprintf(s,"%c",ck(x)); r=t(4,sp(s)); break;
    case  0: r=irecur2(form,a,x); break;
    case -3: PXC; s=xmalloc(nx+1); i(nx,s[i]=pxc[i]); s[nx]=0; r=t(4,sp(s)); xfree(s); break;
    default: return KERR_TYPE;
    } break;
  case 0:
    switch(tx) {
    case  3:
    case -3: PRK(na); i(na,prk[i]=ki(19,a,x,i,-1); EC(prk[i])) break;
    case  0: r=irecur2(form,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -1:
    switch(tx) {
    case  1: case  2: case  8: case  9: case  3: case -3: case 4:
      PAI; i(na,if(pai[i]==INT32_MAX||pai[i]==INT32_MIN||pai[i]==INT32_MIN+1) return KERR_DOMAIN);
      PRK(na);
      i(na,prk[i]=ki(19,a,x,i,-1); EC(prk[i]));
      break;
    case -1: case -2: case -8: case -9: case -4:
    case  0: PRK(na); i(na,prk[i]=ki(19,a,x,i,i); EC(prk[i])); break;
    default: return KERR_TYPE;
    } break;
  case -2:
    switch(tx) {
    case  3:
    case -3: PRK(na); i(na,prk[i]=ki(19,a,x,i,-1); EC(prk[i])); break;
    case -1: case -2:
    case  0: PRK(na); i(na,prk[i]=ki(19,a,x,i,i); EC(prk[i])); break;
    default: return KERR_TYPE;
    } break;
  case -3:
    switch(tx) {
    case  3: PRC(1); prc[0]=ck(x); break;
    case  0: PRK(nx); i(nx,prk[i]=ki(19,a,x,-1,i); EC(prk[i])) break;
    case -3: r=k_(x); break;
    default: return KERR_TYPE;
    } break;
  case -4:
    switch(tx) {
    case  3:
    case -3: PRK(na); i(na,prk[i]=ki(19,a,x,i,-1); EC(prk[i])); break;
    case  0: PRK(na); i(na,prk[i]=ki(19,a,x,i,i); EC(prk[i])); break;
    default: return KERR_TYPE;
    } break;
  default: return KERR_TYPE;
  }
  return knorm(r);
cleanup:
  if(r) _k(r);
  return e;
}

K flip(K x) {
  K r=0,p=0,*prk,*pxk,a,*r2,*p2;
  i64 m=-1;   /* column count: i64 so a >2^31-col matrix doesn't truncate */
  u64 i;      /* row cursor: u64 so a >2^31-row matrix doesn't wrap/loop */
  ko *pk;
  if(ax||s(x)) return k_(x);
  switch(tx) {
  case 0:
    PXK;
    i(nx, a=pxk[i]; if(ta<=0&&!s(a)) { if(m==-1)m=na; else if((i64)na!=m)return KERR_LENGTH; } )
    if(m==-1) r=k_(x);
    else if(m==0) { PRK(0); }
    else {
      PRK(m); i(m,prk[i]=tn(0,nx))
      for(i=0;i<nx;i++) {
        a=pxk[i];
        if(ta>0||s(a)) j(m, r2=(K*)px(prk[j]); r2[i]=k_(a))
        else {
          p=kmix(a); if(E(p)) { _k(r); return p; }
          p2=(K*)px(p);
          j(m,r2=(K*)px(prk[j]);r2[i]=p2[j]);
          pk=(ko*)(b(48)&p); xfree(pk->v); xfree(pk);
        }
      }
      i(m,prk[i]=knorm(prk[i]))
    }
    break;
  case -1: case -2: case -3: case -4: case -8: case -9: r=k_(x); break;  /* flip is a noop for any plain vector; -8/-9 were the atom-type-caselist gap */
  default: return KERR_TYPE;
  }
  return knorm(r);
}

K negate(K x) {
  K r=0;
  i32 *pri,*pxi;
  i64 *prj,*pxj;
  float *pre,*pxe;
  double f,*prf,*pxf;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case  1: r=t(1,-(u32)ik(x)); break;  /* unsigned negate: INT_MIN-safe (matches long cases) */
  case  2: f=-fk(x); r=t2(f); break;
  case  8: r=tj((i64)(-(u64)jk(x))); break;
  case  9: r=te(-ek(x)); break;
  case -1: PRI(nx); PXI; i(nx,*pri++=(i32)(-(u32)*pxi++)) break;
  case -2: PRF(nx); PXF; i(nx,*prf++=-*pxf++) break;
  case -8: PRJ(nx); PXJ; i(nx,prj[i]=(i64)(-(u64)pxj[i])) break;
  case -9: PRE(nx); PXE; i(nx,*pre++=-*pxe++) break;
  case  0: r=irecur1(negate,x); break;
  default: return KERR_TYPE;
  }
  return r;
}

K first(K x) {
  K r=0,*pxk;
  char *pxc,**pxs;
  i32 *pxi;
  i64 *pxj;
  float *pxe;
  double *pxf;
  if(ax||s(x)) return k_(x);
  switch(tx) {
  case -1: PXI; r=nx?t(1,(u32)*pxi):t(1,0); break;
  case -2: PXF; r=nx?t2(*pxf):t2(0); break;
  case -8: PXJ; r=nx?tj(*pxj):tj(0); break;
  case -9: PXE; r=nx?te(*pxe):te(0); break;
  case -3: PXC; r=nx?t(3,(u8)*pxc):t(3,' '); break;
  case -4: PXS; r=nx?t(4,*pxs):t(4,""); break;
  case  0: PXK; r=nx?k_(*pxk):null; break;
  default: return KERR_TYPE;
  }
  return r;
}

K recip(K x) {
  K r=0;
  i64 *pxj;
  float *pxe,*pre;
  double f,*prf;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case  1: f=1.0/fi(ik(x)); r=t2(f); break;
  case  2: f=1.0/fk(x); r=t2(f); break;
  case  8: f=1.0/fj(jk(x)); r=t2(f); break;
  case  9: r=te(1.0f/ek(x)); break;
  case -1:
  case -2: if(!nx) { r=tn(2,0); break; } /* each() of an empty gives an
             untyped (); the result type is float regardless */
           r=each(4,0,x); break;
  case -8: PRF(nx); PXJ; i(nx,prf[i]=1.0/fj(pxj[i])) break;
  case -9: PRE(nx); PXE; i(nx,pre[i]=1.0f/pxe[i]) break;
  case  0: r=irecur1(recip,x); break;
  default: return KERR_TYPE;
  }
  return knorm(r);
}

K where(K x) {
  K r=0;
  i64 j=0,*prj,*pxj;        /* j: output cursor (may exceed 2^31) */
  i32 *pri,*pxi;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case  1: {
    i32 c=ik(x);
    if(c<0 || c==INT32_MAX || c==INT32_MIN || c==INT32_MIN+1) return KERR_DOMAIN;
    PRI(c); i(c,pri[i]=0)            /* values all 0 -> i32 */
    } break;
  case  8: {
    i64 v=jk(x);
    if(v<0 || v==J_INF || v==J_NULL || v==J_NINF) return KERR_DOMAIN;
    VLEN(v);
    PRI(v); i(v,pri[i]=0)            /* values all 0 -> i32, count may be huge */
    } break;
  case  0: if(!nx) r=tn(1,0); else return KERR_TYPE; break;
  /* vector source: result values are positions 0..nx-1, so they need i64 only
     when nx>2^31 (mirror #x).  The result COUNT (sum) is independent. */
  /* The counts are nearly always a 0/1 mask (`&x>c`), where the general
     `while(kk-->0)` expansion costs a branch mispredict per element on random
     data.  Pass 1 ors the counts into `mx`, so mx<=1 proves every count is 0 or
     1 and pass 2 can compact branchlessly.  That loop is bounded by j<tot rather
     than i<nx, which keeps the unconditional pri[j] store in bounds (j never
     passes tot) and skips trailing zeros for free. */
  case -1: {
    i64 tot=0,mx=0; PXI;
    i(nx, i64 c=pxi[i];
      if(c<0 || c==INT32_MAX || c==INT32_MIN || c==INT32_MIN+1) return KERR_DOMAIN;
      if(tot>VMAX-c) return KERR_WSFULL; mx|=c; tot+=c)
    if(nx>BIGV) { PRJ(tot);
      if(mx>1) { i(nx, i64 kk=pxi[i]; while(kk-->0)prj[j++]=(i64)i) }
      else    { u64 i=0; while(j<tot){ prj[j]=(i64)i; j+=pxi[i]; ++i; } } }
    else        { PRI(tot);
      if(mx>1) { i(nx, i64 kk=pxi[i]; while(kk-->0)pri[j++]=(i32)i) }
      else    { u64 i=0; while(j<tot){ pri[j]=(i32)i; j+=pxi[i]; ++i; } } }
    } break;
  case -8: {
    i64 tot=0,mx=0; PXJ;
    i(nx, i64 c=pxj[i];
      if(c<0 || c==J_INF || c==J_NULL || c==J_NINF) return KERR_DOMAIN;
      if(tot>VMAX-c) return KERR_WSFULL; mx|=c; tot+=c)
    if(nx>BIGV) { PRJ(tot);
      if(mx>1) { i(nx, i64 kk=pxj[i]; while(kk-->0)prj[j++]=(i64)i) }
      else    { u64 i=0; while(j<tot){ prj[j]=(i64)i; j+=pxj[i]; ++i; } } }
    else        { PRI(tot);
      if(mx>1) { i(nx, i64 kk=pxj[i]; while(kk-->0)pri[j++]=(i32)i) }
      else    { u64 i=0; while(j<tot){ pri[j]=(i32)i; j+=pxj[i]; ++i; } } }
    } break;
  default: return KERR_TYPE;
  }
  return knorm(r);
}

/* wherecmp moved to fuse.c */

K reverse(K x) {
  K r=0,*prk,*pxk;
  char *prc,*pxc,**prs,**pxs;
  i32 *pri,*pxi;
  i64 *prj,*pxj;
  float *pre,*pxe;
  double *prf,*pxf;
  if(ax||s(x)) return k_(x);
  switch(tx) {
  case -1: PRI(nx); PXI; i(nx,*pri++=pxi[nx-i-1]) break;
  case -2: PRF(nx); PXF; i(nx,*prf++=pxf[nx-i-1]) break;
  case -8: PRJ(nx); PXJ; i(nx,*prj++=pxj[nx-i-1]) break;
  case -9: PRE(nx); PXE; i(nx,*pre++=pxe[nx-i-1]) break;
  case -3: PRC(nx); PXC; i(nx,*prc++=pxc[nx-i-1]) break;
  case -4: PRS(nx); PXS; i(nx,*prs++=pxs[nx-i-1]) break;
  case  0: PRK(nx); PXK; i(nx,*prk++=k_(pxk[nx-i-1])) break;
  default: return KERR_TYPE;
  }
  return r;
}

/* grade with an i64 index permutation, for #x>2^31 (mirror #x).  Returns an
   i64 index vector.  down=0 -> up (<), down=1 -> down (>). */
static K gradej(K x, i32 down) {
  K r;
  i64 *prj,*pxj; i32 *pxi; double *pxf; float *pxe; char *pxc,**pxs; K *pxk;
  switch(tx) {
  case -1: r=tn(8,nx); prj=px(r); PXI; rcsortgj(prj,pxi,nx,down); break;
  case -8: r=tn(8,nx); prj=px(r); PXJ; rcsortg8j(prj,pxj,nx,down); break;
  case -2: r=tn(8,nx); prj=px(r); PXF; rcsortg2j(prj,pxf,nx,down); break;
  case -9: r=tn(8,nx); prj=px(r); PXE; rcsortg9j(prj,pxe,nx,down); break;
  case -3: r=tn(8,nx); prj=px(r); PXC; csortg3j(prj,pxc,nx,down); break;
  case -4: r=tn(8,nx); prj=px(r); PXS; rsortg4j(prj,pxs,nx,down); break;
  case  0: r=tn(8,nx); prj=px(r); PXK; i(nx,prj[i]=i); msortg0j(prj,pxk,0,(i64)nx-1,down); break;
  default: return KERR_TYPE;
  }
  return r;
}

K upgrade(K x) {
  K r=0,*pxk;
  char *pxc,**pxs;
  i32 *pri,*pxi;
  i64 *pxj;
  double *pxf;
  float *pxe;
  if(ax||s(x)) return KERR_RANK;
  if(nx==0) return tn(1,0);
  if(nx>BIGV) return gradej(x,0);
  /* enumerate can fail (it rejects the 0I-sentinel length); its result is a
     pointer here, so check before px().  BIGV excludes the only reachable
     failing length -- this is the belt to that suspenders. */
  r=enumerate(t(1,nx)); if(E(r)) return r;
  pri=(i32*)px(r);
  switch(tx) {
  case -1: PXI; rcsortg(pri,pxi,nx,0); break;
  case -8: PXJ; rcsortg8(pri,pxj,nx,0); break;
  case -2: PXF; rcsortg2(pri,pxf,nx,0); break;
  case -9: PXE; rcsortg9(pri,pxe,nx,0); break;
  case -3: PXC; csortg3(pri,pxc,nx,0); break;
  case -4: PXS; rsortg4(pri,pxs,nx,0); break;
  case  0: PXK; msortg0(pri,pxk,0,nx-1,0); break;
  default: _k(r); return KERR_TYPE;
  }
  return r;
}

K downgrade(K x) {
  K r=0,*pxk;
  char *pxc,**pxs;
  i32 *pri,*pxi;
  i64 *pxj;
  double *pxf;
  float *pxe;
  if(ax||s(x)) return KERR_RANK;
  if(nx==0) return tn(1,0);
  if(nx>BIGV) return gradej(x,1);
  r=enumerate(t(1,nx)); if(E(r)) return r;   /* see upgrade() */
  pri=(i32*)px(r);
  switch(tx) {
  case -1: PXI; rcsortg(pri,pxi,nx,1); break;
  case -8: PXJ; rcsortg8(pri,pxj,nx,1); break;
  case -2: PXF; rcsortg2(pri,pxf,nx,1); break;
  case -9: PXE; rcsortg9(pri,pxe,nx,1); break;
  case -3: PXC; csortg3(pri,pxc,nx,1); break;
  case -4: PXS; rsortg4(pri,pxs,nx,1); break;
  case  0: PXK; msortg0(pri,pxk,0,nx-1,1); break;
  default: _k(r); return KERR_TYPE;
  }
  return r;
}

/* group with i64 index positions, for #x>2^31 (mirror #x).  Mirrors group()
   exactly but the per-value index vectors are long (tn(8,..)) and the element
   cursor is u64.  group() (i32 path) is left untouched for the common case. */
/* float/real entry points for hmul (k.h).  hmulf sends -0.0 to slot 0
   like +0.0 (their bit patterns differ, but they always shared a group). */
static inline u64 hmulf(double v) {
  u64 b;
  if(v==0) v=0.0;
  memcpy(&b,&v,8);
  return hmul(b);
}
static inline u64 hmule(float v) {
  u32 b;
  if(v==0) v=0.0f;
  memcpy(&b,&v,4);
  return hmul((u64)b);
}

static K groupj(K x) {
  K r=0,p=0,*ht,*pk,*hk,*prk,*pxk;
  i32 *n,min=INT32_MAX,max=INT32_MIN,*hi,bni=0,*pxi;
  i64 *pj;
  u64 i,m,w,h,rs=256,ri=0,*hm,q;
  char **s,**hs,*pxc,**pxs;
  u8 *c;
  double *f,*hf,*pxf;
  i64 *pxj;
  float *pxe;
  switch(tx) {
  case  0:
    PRK(rs); nr=0; PXK;
    m=nx;
    w=1; while(w<=m) w<<=1; q=w-1;
    ht=xcalloc(w,sizeof(K));
    hm=xcalloc(w,sizeof(u64));
    hk=xcalloc(w,sizeof(K));
    pk=pxk-1;
    for(i=0;i<nx;i++) {
      pk++;
      h=khash(*pk)&q;
      if(*pk) while(!h || (hk[h] && kcmpr(hk[h],*pk))) h=(h+1)&q;
      hk[h]=*pk;
      hm[h]++;
    }
    pk=pxk-1;
    for(i=0;i<nx;i++) {
      pk++;
      h=khash(*pk)&q;
      if(*pk) while(!h || (hk[h] && kcmpr(hk[h],*pk))) h=(h+1)&q;
      p=ht[h];
      if(!p) {
        p=tn(8,hm[h]);
        ht[h]=p;
        hm[h]=0;
        if(rs==ri++){rs<<=1;vr=prk=xrealloc(prk,sizeof(K)*rs);}
        prk[nr++]=p;
      }
      pj=(i64*)px(p);
      pj[hm[h]++]=i;
    }
    xfree(ht); xfree(hm); xfree(hk);
    break;
  case -1:
    PRK(rs); nr=0; PXI;
    for(i=0;i<nx;i++) {
      if(pxi[i]==INT32_MAX) { bni=1; continue; }
      if(pxi[i]==INT32_MIN) { bni=1; continue; }
      if(pxi[i]==INT32_MIN+1) { bni=1; continue; }
      if(max<pxi[i])max=pxi[i];
      if(min>pxi[i])min=pxi[i];
    }
    if(min>=0 && !bni && (u64)max+1 <= ((u64)nx<<3)) {
      ht=xcalloc((u32)(max+1),sizeof(K));
      hm=xcalloc((u32)(max+1),sizeof(u64));
      n=pxi;n--;
      i(nx, n++; hm[*n]++)
      n=pxi;n--;
      for(i=0;i<nx;i++) {
         n++;
         p=ht[*n];
         if(!p) {
           p=tn(8,hm[*n]);
           ht[*n]=p;
           pj=(i64*)px(p);
           pj[0]=i;
           hm[*n]=1;
           if(rs==ri++){rs<<=1;vr=prk=xrealloc(prk,sizeof(K)*rs);}
           prk[nr++]=p;
         }
         else { pj=(i64*)px(p); pj[hm[*n]++]=i; }
      }
      xfree(hm);
      xfree(ht);
    }
    else {
      m=(i64)max-(i64)min+1;
      m+=3; /* sentinel headroom -- see group() */
      if(nx<m) m=nx;
      w=1; while(w<=m) w<<=1; q=w-1;
      ht=xcalloc(w,sizeof(K));
      hm=xcalloc(w,sizeof(u64));
      hi=xcalloc(w,sizeof(i32));
      n=pxi;n--;
      for(i=0;i<nx;i++) {
         n++;
         h=hmul((u32)*n)&q;
         if(*n) while(!h || (hi[h] && hi[h]!=*n)) h=(h+1)&q;
         hi[h]=*n;
         hm[h]++;
      }
      n=pxi;n--;
      for(i=0;i<nx;i++) {
         n++;
         h=hmul((u32)*n)&q;
         if(*n) while(!h || (hi[h] && hi[h]!=*n)) h=(h+1)&q;
         p=ht[h];
         if(!p){
           p=tn(8,hm[h]);
           ht[h]=p;
           hm[h]=0;
           if(rs==ri++){rs<<=1;vr=prk=xrealloc(prk,sizeof(K)*rs);}
           prk[nr++]=p;
         }
         pj=(i64*)px(p);
         pj[hm[h]++]=i;
      }
      xfree(ht); xfree(hm); xfree(hi);
    }
    break;
  case -2:
    PRK(rs); nr=0; PXF;
    m=nx;
    w=1; while(w<=m) w<<=1; q=w-1;
    ht=xcalloc(w,sizeof(K));
    hm=xcalloc(w,sizeof(u64));
    hf=xcalloc(w,sizeof(double));
    f=pxf;f--;
    for(i=0;i<nx;i++) {
       f++;
       h=hmulf(*f)&q;
       if(*f) while(!h || (hf[h] && cmpfft(hf[h],*f))) h=(h+1)&q;
       hf[h]=*f;
       hm[h]++;
    }
    f=pxf;f--;
    for(i=0;i<nx;i++) {
       f++;
       h=hmulf(*f)&q;
       if(*f) while(!h || (hf[h] && cmpfft(hf[h],*f))) h=(h+1)&q;
       p=ht[h];
       if(!p){
         p=tn(8,hm[h]);
         ht[h]=p;
         hm[h]=0;
         if(rs==ri++){rs<<=1;vr=prk=xrealloc(prk,sizeof(K)*rs);}
         prk[nr++]=p;
       }
       pj=(i64*)px(p);
       pj[hm[h]++]=i;
    }
    xfree(ht); xfree(hm); xfree(hf);
    break;
  case -8:
    PRK(rs); nr=0; PXJ;
    m=nx;
    w=1; while(w<=m) w<<=1; q=w-1;
    ht=xcalloc(w,sizeof(K));
    hm=xcalloc(w,sizeof(u64));
    { i64 *hj=xcalloc(w,sizeof(i64));
      for(i=0;i<nx;i++) {
        i64 v=pxj[i]; h=hmul((u64)v)&q;
        if(v) while(!h || (hj[h] && hj[h]!=v)) h=(h+1)&q;
        hj[h]=v; hm[h]++;
      }
      for(i=0;i<nx;i++) {
        i64 v=pxj[i]; h=hmul((u64)v)&q;
        if(v) while(!h || (hj[h] && hj[h]!=v)) h=(h+1)&q;
        p=ht[h];
        if(!p){ p=tn(8,hm[h]); ht[h]=p; hm[h]=0; if(rs==ri++){rs<<=1;vr=prk=xrealloc(prk,sizeof(K)*rs);} prk[nr++]=p; }
        pj=(i64*)px(p); pj[hm[h]++]=i;
      }
      xfree(ht); xfree(hm); xfree(hj);
    }
    break;
  case -9:
    PRK(rs); nr=0; PXE;
    m=nx;
    w=1; while(w<=m) w<<=1; q=w-1;
    ht=xcalloc(w,sizeof(K));
    hm=xcalloc(w,sizeof(u64));
    { float *he=xcalloc(w,sizeof(float));
      for(i=0;i<nx;i++) {
        float v=pxe[i];
        h=hmule(v)&q;
        if(v) while(!h || (he[h] && cmpfft(he[h],v))) h=(h+1)&q;
        he[h]=v; hm[h]++;
      }
      for(i=0;i<nx;i++) {
        float v=pxe[i];
        h=hmule(v)&q;
        if(v) while(!h || (he[h] && cmpfft(he[h],v))) h=(h+1)&q;
        p=ht[h];
        if(!p){ p=tn(8,hm[h]); ht[h]=p; hm[h]=0; if(rs==ri++){rs<<=1;vr=prk=xrealloc(prk,sizeof(K)*rs);} prk[nr++]=p; }
        pj=(i64*)px(p); pj[hm[h]++]=i;
      }
      xfree(ht); xfree(hm); xfree(he);
    }
    break;
  case -3:
    PRK(256); nr=0; PXC;
    ht=xcalloc(256,sizeof(K));
    hm=xcalloc(256,sizeof(u64));
    c=(u8*)pxc-1;
    i(nx, c++; hm[*c]++)
    c=(u8*)pxc-1;
    for(i=0;i<nx;i++) {
       c++;
       p=ht[*c];
       if(!p){ p=tn(8,hm[*c]); ht[*c]=p; pj=(i64*)px(p); pj[0]=i; hm[*c]=1; prk[nr++]=p; }
       else { pj=(i64*)px(p); pj[hm[*c]++]=i; }
    }
    xfree(hm); xfree(ht);
    break;
  case -4:
    PRK(rs); nr=0; PXS;
    m=nx;
    w=1; while(w<=m) w<<=1; q=w-1;
    ht=xcalloc(w,sizeof(K));
    hm=xcalloc(w,sizeof(u64));
    hs=xcalloc(w,sizeof(char*));
    s=pxs-1;
    for(i=0;i<nx;i++) {
      s++;
      h=xfnv1a((char*)*s, strlen(*s))&q;
      if(*s) while(!h || (hs[h] && strcmp(hs[h],*s))) h=(h+1)&q;
      hs[h]=*s;
      hm[h]++;
    }
    s=pxs-1;
    for(i=0;i<nx;i++) {
      s++;
      h=xfnv1a((char*)*s, strlen(*s))&q;
      if(*s) while(!h || (hs[h] && strcmp(hs[h],*s))) h=(h+1)&q;
      p=ht[h];
      if(!p) {
        p=tn(8,hm[h]);
        ht[h]=p;
        hm[h]=0;
        if(rs==ri++){rs<<=1;vr=prk=xrealloc(prk,sizeof(K)*rs);}
        prk[nr++]=p;
      }
      pj=(i64*)px(p);
      pj[hm[h]++]=i;
    }
    xfree(ht); xfree(hm); xfree(hs);
    break;
  default: return KERR_RANK;
  }
  return knorm(r);
}

K group(K x) {
  K r=0,p=0,*ht,*pk,*hk,*prk,*pxk;
  i32 *n,min=INT32_MAX,max=INT32_MIN,*hi,bni=0,*pp,*pxi;
  u32 i;
  u64 m,w,h,rs=256,ri=0,*hm,q;
  char **s,**hs,*pxc,**pxs;
  u8 *c;
  double *f,*hf,*pxf;
  i64 *pxj;
  float *pxe;
  if(s(x)) return KERR_RANK;
  if(tx<=0&&nx==0) return tn(0,0);
  if(tx<=0&&nx>BIGV) return groupj(x);  /* nx only valid for tx<=0 */
  switch(tx) {
  case  0:
    PRK(rs); nr=0; PXK;
    m=nx;
    w=1; while(w<=m) w<<=1; q=w-1;
    ht=xcalloc(w,sizeof(K));
    hm=xcalloc(w,sizeof(u64));
    hk=xcalloc(w,sizeof(K));
    pk=pxk-1;
    for(i=0;i<nx;i++) {
      pk++;
      h=khash(*pk)&q;
      if(*pk) while(!h || (hk[h] && kcmpr(hk[h],*pk))) h=(h+1)&q; /* h=0 iff *s=0 */
      hk[h]=*pk;
      hm[h]++;
    }
    pk=pxk-1;
    for(i=0;i<nx;i++) {
      pk++;
      h=khash(*pk)&q;
      if(*pk) while(!h || (hk[h] && kcmpr(hk[h],*pk))) h=(h+1)&q; /* h=0 iff *s=0 */
      p=ht[h];
      if(!p) {
        p=tn(1,hm[h]);
        ht[h]=p;
        hm[h]=0;
        if(rs==ri++){rs<<=1;vr=prk=xrealloc(prk,sizeof(K)*rs);}
        prk[nr++]=p;
      }
      pp=(i32*)px(p);
      pp[hm[h]++]=i;
    }
    xfree(ht); xfree(hm); xfree(hk);
    break;
  case -1:
    PRK(rs); nr=0; PXI;
    for(i=0;i<nx;i++) {
      if(pxi[i]==INT32_MAX) { bni=1; continue; }
      if(pxi[i]==INT32_MIN) { bni=1; continue; }
      if(pxi[i]==INT32_MIN+1) { bni=1; continue; }
      if(max<pxi[i])max=pxi[i];
      if(min>pxi[i])min=pxi[i];
    }
    /* dense non-negative range only: this path allocates two max-sized arrays
       (ht + hm), so guard on magnitude relative to nx (~8x).  A short vector with
       a huge value (e.g. =0 999999999) would otherwise calloc ~16*max bytes.  The
       hash branch below handles arbitrary positive ints, capped at nx. */
    if(min>=0 && !bni && (u64)max+1 <= ((u64)nx<<3)) {
      ht=xcalloc((u32)(max+1),sizeof(K));
      hm=xcalloc((u32)(max+1),sizeof(u64));
      n=pxi;n--;
      i(nx, n++; hm[*n]++)
      n=pxi;n--;
      for(i=0;i<nx;i++) {
         n++;
         p=ht[*n];
         if(!p) {
           p=tn(1,hm[*n]);
           ht[*n]=p;
           pp=(i32*)px(p);
           pp[0]=i;
           hm[*n]=1;
           if(rs==ri++){rs<<=1;vr=prk=xrealloc(prk,sizeof(K)*rs);}
           prk[nr++]=p;
         }
         else { pp=(i32*)px(p); pp[hm[*n]++]=i; }
      }
      xfree(hm);
      xfree(ht);
    }
    else {
      m=(i64)max-(i64)min+1;
      m+=3; /* 0I 0N -0I were skipped by the min/max scan above but still go
               through this table: without headroom for them, =0I,1 has more
               distinct values than nonzero slots and the probe loop below
               never finds a free one (spins forever).  Mirrors distinct(). */
      if(nx<m) m=nx;
      w=1; while(w<=m) w<<=1; q=w-1;
      ht=xcalloc(w,sizeof(K));       /* groups */
      hm=xcalloc(w,sizeof(u64)); /* max's */
      hi=xcalloc(w,sizeof(i32));
      n=pxi;n--;
      for(i=0;i<nx;i++) {
         n++;
         h=hmul((u32)*n)&q;
         if(*n) while(!h || (hi[h] && hi[h]!=*n)) h=(h+1)&q; /* h=0 iff *n=0 */
         hi[h]=*n;
         hm[h]++;
      }
      n=pxi;n--;
      for(i=0;i<nx;i++) {
         n++;
         h=hmul((u32)*n)&q;
         if(*n) while(!h || (hi[h] && hi[h]!=*n)) h=(h+1)&q;
         p=ht[h];
         if(!p){
           p=tn(1,hm[h]);
           ht[h]=p;
           hm[h]=0;
           if(rs==ri++){rs<<=1;vr=prk=xrealloc(prk,sizeof(K)*rs);}
           prk[nr++]=p;
         }
         pp=(i32*)px(p);
         pp[hm[h]++]=i;
      }
      xfree(ht); xfree(hm); xfree(hi);
    }
    break;
  case -2:
    PRK(rs); nr=0; PXF;
    m=nx;
    w=1; while(w<=m) w<<=1; q=w-1;
    ht=xcalloc(w,sizeof(K));       /* groups */
    hm=xcalloc(w,sizeof(u64)); /* max's */
    hf=xcalloc(w,sizeof(double));
    f=pxf;f--;
    for(i=0;i<nx;i++) {
       f++;
       h=hmulf(*f)&q;
       if(*f) while(!h || (hf[h] && cmpfft(hf[h],*f))) h=(h+1)&q; /* h=0 iff *f=0 */
       hf[h]=*f;
       hm[h]++;
    }
    f=pxf;f--;
    for(i=0;i<nx;i++) {
       f++;
       h=hmulf(*f)&q;
       if(*f) while(!h || (hf[h] && cmpfft(hf[h],*f))) h=(h+1)&q; /* h=0 iff *f=0 */
       p=ht[h];
       if(!p){
         p=tn(1,hm[h]);
         ht[h]=p;
         hm[h]=0;
         if(rs==ri++){rs<<=1;vr=prk=xrealloc(prk,sizeof(K)*rs);}
         prk[nr++]=p;
       }
       pp=(i32*)px(p);
       pp[hm[h]++]=i;
    }
    xfree(ht); xfree(hm); xfree(hf);
    break;
  case -8:
    PRK(rs); nr=0; PXJ;
    m=nx;
    w=1; while(w<=m) w<<=1; q=w-1;
    ht=xcalloc(w,sizeof(K));
    hm=xcalloc(w,sizeof(u64));
    { i64 *hj=xcalloc(w,sizeof(i64));
      for(i=0;i<nx;i++) {
        i64 v=pxj[i]; h=hmul((u64)v)&q;
        if(v) while(!h || (hj[h] && hj[h]!=v)) h=(h+1)&q; /* h=0 iff v=0 */
        hj[h]=v; hm[h]++;
      }
      for(i=0;i<nx;i++) {
        i64 v=pxj[i]; h=hmul((u64)v)&q;
        if(v) while(!h || (hj[h] && hj[h]!=v)) h=(h+1)&q;
        p=ht[h];
        if(!p){ p=tn(1,hm[h]); ht[h]=p; hm[h]=0; if(rs==ri++){rs<<=1;vr=prk=xrealloc(prk,sizeof(K)*rs);} prk[nr++]=p; }
        pp=(i32*)px(p); pp[hm[h]++]=i;
      }
      xfree(ht); xfree(hm); xfree(hj);
    }
    break;
  case -9:
    PRK(rs); nr=0; PXE;
    m=nx;
    w=1; while(w<=m) w<<=1; q=w-1;
    ht=xcalloc(w,sizeof(K));
    hm=xcalloc(w,sizeof(u64));
    { float *he=xcalloc(w,sizeof(float));
      for(i=0;i<nx;i++) {
        float v=pxe[i];
        h=hmule(v)&q;
        if(v) while(!h || (he[h] && cmpfft(he[h],v))) h=(h+1)&q; /* h=0 iff v=0 */
        he[h]=v; hm[h]++;
      }
      for(i=0;i<nx;i++) {
        float v=pxe[i];
        h=hmule(v)&q;
        if(v) while(!h || (he[h] && cmpfft(he[h],v))) h=(h+1)&q;
        p=ht[h];
        if(!p){ p=tn(1,hm[h]); ht[h]=p; hm[h]=0; if(rs==ri++){rs<<=1;vr=prk=xrealloc(prk,sizeof(K)*rs);} prk[nr++]=p; }
        pp=(i32*)px(p); pp[hm[h]++]=i;
      }
      xfree(ht); xfree(hm); xfree(he);
    }
    break;
  case -3:
    PRK(256); nr=0; PXC;
    ht=xcalloc(256,sizeof(K));
    hm=xcalloc(256,sizeof(u64));
    c=(u8*)pxc-1;
    i(nx, c++; hm[*c]++)
    c=(u8*)pxc-1;
    for(i=0;i<nx;i++) {
       c++;
       p=ht[*c];
       if(!p){ p=tn(1,hm[*c]); ht[*c]=p; pp=(i32*)px(p); pp[0]=i; hm[*c]=1; prk[nr++]=p; }
       else { pp=(i32*)px(p); pp[hm[*c]++]=i; }
    }
    xfree(hm); xfree(ht);
    break;
  case -4:
    PRK(rs); nr=0; PXS;
    m=nx;
    w=1; while(w<=m) w<<=1; q=w-1;
    ht=xcalloc(w,sizeof(K));
    hm=xcalloc(w,sizeof(u64));
    hs=xcalloc(w,sizeof(char*));
    s=pxs-1;
    for(i=0;i<nx;i++) {
      s++;
      h=xfnv1a((char*)*s, strlen(*s))&q;
      if(*s) while(!h || (hs[h] && strcmp(hs[h],*s))) h=(h+1)&q; /* h=0 iff *s=0 */
      hs[h]=*s;
      hm[h]++;
    }
    s=pxs-1;
    for(i=0;i<nx;i++) {
      s++;
      h=xfnv1a((char*)*s, strlen(*s))&q;
      if(*s) while(!h || (hs[h] && strcmp(hs[h],*s))) h=(h+1)&q; /* h=0 iff *s=0 */
      p=ht[h];
      if(!p) {
        p=tn(1,hm[h]);
        ht[h]=p;
        hm[h]=0;
        if(rs==ri++){rs<<=1;vr=prk=xrealloc(prk,sizeof(K)*rs);}
        prk[nr++]=p;
      }
      pp=(i32*)px(p);
      pp[hm[h]++]=i;
    }
    xfree(ht); xfree(hm); xfree(hs);
    break;
  default: return KERR_RANK;
  }
  return knorm(r);
}

K not_(K x) {
  K r=0;
  i32 *pri,*pxi;
  i64 *pxj;
  float *pxe;
  double *pxf;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case  1: r=t(1,ik(x)==0); break;
  case  2: r=t(1,fk(x)==0.0); break;
  case  8: r=t(1,jk(x)==0); break;
  case  9: r=t(1,ek(x)==0.0f); break;
  case -1: PRI(nx); PXI; i(nx,*pri++=t(1,*pxi++==0)) break;
  case -2: PRI(nx); PXF; i(nx,*pri++=t(1,*pxf++==0)) break;
  case -8: PRI(nx); PXJ; i(nx,*pri++=t(1,*pxj++==0)) break;
  case -9: PRI(nx); PXE; i(nx,*pri++=t(1,*pxe++==0.0f)) break;
  case  0: r=irecur1(not_,x); break;
  default: return KERR_TYPE;
  }
  return knorm(r);
}

K value(K x) {
  return valuecb(x);
}

#ifdef _WIN32
static K lsdir(char *p) {
  K r=0,q=0,s=0,*pqk;
  u64 n=0;
  char *t=xmalloc(5+strlen(p));
  snprintf(t,5+strlen(p),"%s%s",p,"\\*.*");
  WIN32_FIND_DATA ffd;
  HANDLE h;
  h=FindFirstFile(t,&ffd);
  if(h==INVALID_HANDLE_VALUE) return KERR_VALUE;
  q=tn(0,32); pqk=px(q);
  do {
    if(!strcmp(ffd.cFileName,".")||!strcmp(ffd.cFileName,"..")) continue;
    ++n;
    if(n==n(q)) { n(q)<<=1; kresize(q,n(q)); pqk=px(q); }
    pqk[n-1]=tn(3,1+strlen(ffd.cFileName));
    memcpy(px(pqk[n-1]),ffd.cFileName,strlen(ffd.cFileName));
    n(pqk[n-1])--;
  } while(FindNextFile(h,&ffd));
  n(q)=n;
  FindClose(h);
  s=upgrade(q);
  r=at(q,s);
  _k(q);
  _k(s);
  return r;
}
#else
static K lsdir(char *p) {
  K r=0,q=0,s=0,*pqk;
  u64 n=0;
  DIR *f=opendir(p);
  struct dirent *e;
  if(!f) return KERR_VALUE;
  q=tn(0,32); pqk=px(q);
  while((e=readdir(f))) {
    if(!strcmp(e->d_name,".")||!strcmp(e->d_name,"..")) continue;
    ++n;
    if(n==n(q)) { n(q)<<=1; q=kresize(q,n(q)); pqk=px(q); }
    { size_t dl=strlen(e->d_name);
      pqk[n-1]=tn(3,1+dl);
      memcpy(px(pqk[n-1]),e->d_name,dl+1);  /* name + nul */
      n(pqk[n-1])--; }
  }
  n(q)=n;
  closedir(f);
  s=upgrade(q);
  r=at(q,s);
  _k(q);
  _k(s);
  return r;
}
#endif

K enumerate(K x) {
  K r=0,q=null;
  char s[2],*p=s;
  i32 *pri;
  i64 *prj;
  if(s(x)) return enumeratecb(x);
  switch(tx) {
  case  1: if(ik(x)<0||ik(x)==INT32_MAX) return KERR_DOMAIN; PRI(ik(x)); i(ik(x),pri[i]=i); break;
  case  8: { i64 v=jk(x); if(v<0||v==J_INF) return KERR_DOMAIN; VLEN(v); PRJ(v); i(v,prj[i]=i); } break;
  case  2: return KERR_INT;
  case  3: p[0]=ck(x); p[1]=0; r=lsdir(p); break;
  case  4: return enumeratecb(x);
  case  6: r=tn(4,0); break;
  case -3: p=xmalloc(1+nx); memcpy(p,px(x),nx); p[nx]=0; r=lsdir(p); xfree(p); break;
  default: return KERR_TYPE;
  }
  _k(q);
  return r;
}

K atom(K x) {
  return t(1,tx>0||s(x));
}

K unique(K x) {
  K r=0,*hk,*prk,*pxk;
  i32 *hi,*pri,*pxi;
  i64 *prj,*pxj;
  float *pre,*pxe;
  char hc[256]={0},**hs,*prc,*pxc,**prs,**pxs;
  double *hf,*prf,*pxf;
  u64 m,w,h,q;
  u64 i;                /* element cursor: u64 for >2^31 */
  i64 j=0;              /* distinct count: can exceed 2^31 */
  i32 z=0,t=0,min=INT32_MAX,max=INT32_MIN,bni=0;
  if(ax||s(x)) return KERR_RANK;
  switch(tx) {
  case  0:
    if(!nx) return k_(x);
    PRK(nx); PXK;
    w=1; while(w<=nx) w<<=1; q=w-1;
    hk=xcalloc(w,sizeof(K));
    pxk--;
    for(i=0;i<nx;i++) {
      pxk++;
      h=khash(*pxk)&q;
      while(hk[h] && kcmpr(hk[h],*pxk)) h=(h+1)&q;
      if(!hk[h]) hk[h]=prk[j++]=k_(*pxk);
    }
    nr=j;
    xfree(hk);
    break;
  case -1:
    if(!nx) return k_(x);
    PRI(nx); PXI;
    for(i=0;i<nx;i++) {
      if(pxi[i]==INT32_MAX) { bni=1; continue; }
      if(pxi[i]==INT32_MIN) { bni=1; continue; }
      if(pxi[i]==INT32_MIN+1) { bni=1; continue; }
      if(max<pxi[i])max=pxi[i];
      if(min>pxi[i])min=pxi[i];
    }
    /* dense non-negative range: a value-indexed bitmap is O(nx) and beats the
       hash.  Guard on magnitude: only when max+1 is within ~8x nx, else a short
       vector with a huge value (e.g. ?5 2000000000) would calloc ~4*max bytes.
       The hash branch below handles arbitrary positive ints, capped at nx. */
    if(min>=0 && !bni && (u64)max+1 <= ((u64)nx<<3)) {
      hi=xcalloc((u32)(max+1),sizeof(i32));
      pxi--;
      i(nx, pxi++; if(!hi[*pxi]) { hi[*pxi]=1; pri[j++]=*pxi; if(++t==max+1) break; })
      nr=j;
      xfree(hi);
    }
    else {
      m=(i64)max-(i64)min+1;
      m+=3; /* handle 0I 0N -0I */
      if(nx<m) m=nx;
      w=1; while(w<=m) w<<=1; q=w-1;
      hi=xcalloc(w,sizeof(i32));
      pxi--;
      for(i=0,j=0;i<nx;i++) {
        pxi++;
        if(!*pxi) { if(!z) { pri[j++]=0; z=1; } continue; }
        h=hmul((u32)*pxi)&q;
        while(hi[h] && hi[h]!=*pxi) h=(h+1)&q;
        if(hi[h]!=*pxi) hi[h]=pri[j++]=*pxi;
        //if(++t==m) break;
      }
      nr=j;
      xfree(hi);
    }
    break;
  case -2:
    if(!nx) return k_(x);
    PRF(nx); PXF;
    w=1; while(w<=nx) w<<=1; q=w-1;
    hf=xcalloc(w,sizeof(double));
    pxf--;
    for(i=0;i<nx;i++) {
      pxf++;
      /* 0.0 is the table's empty marker, so zeros bypass the hash.  Emit the
         value, not a literal 0: -0.0 is `!*pxf` too, and ? keeps first-seen. */
      if(!*pxf) { if(!z) { prf[j++]=*pxf;z=1; } continue; }
      h=hmulf(*pxf)&q;
      while(hf[h]!=0 && cmpfft(hf[h],*pxf)) h=(h+1)&q;
      if(cmpfft(hf[h],*pxf)) hf[h]=prf[j++]=*pxf;
    }
    nr=j;
    xfree(hf);
    break;
  case -8:
    if(!nx) return k_(x);
    PRJ(nx); PXJ;
    w=1; while(w<=nx) w<<=1; q=w-1;
    { i64 *hj=xcalloc(w,sizeof(i64));
      pxj--;
      for(i=0;i<nx;i++) {
        pxj++;
        if(!*pxj) { if(!z) { prj[j++]=0;z=1; } continue; }
        h=hmul((u64)*pxj)&q;
        while(hj[h]!=0 && hj[h]!=*pxj) h=(h+1)&q;
        if(hj[h]!=*pxj) hj[h]=prj[j++]=*pxj;
      }
      nr=j;
      xfree(hj);
    }
    break;
  case -9:
    if(!nx) return k_(x);
    PRE(nx); PXE;
    w=1; while(w<=nx) w<<=1; q=w-1;
    { float *he=xcalloc(w,sizeof(float));
      pxe--;
      for(i=0;i<nx;i++) {
        pxe++;
        /* as case -2: emit the value so a leading -0.0e keeps its sign */
        if(!*pxe) { if(!z) { pre[j++]=*pxe;z=1; } continue; }
        h=hmule(*pxe)&q;
        while(he[h]!=0 && cmpfft(he[h],*pxe)) h=(h+1)&q;
        if(cmpfft(he[h],*pxe)) he[h]=pre[j++]=*pxe;
      }
      nr=j;
      xfree(he);
    }
    break;
  case -3:
    if(!nx) return k_(x);
    PRC(nx); PXC;
    pxc--;
    i(nx, pxc++; if(!hc[(u8)*pxc]) { hc[(u8)*pxc]=1; *prc++=*pxc; j++; })
    nr=j;
    break;
  case -4:
    if(!nx) return k_(x);
    PRS(nx); PXS;
    w=1; while(w<=nx) w<<=1; q=w-1;
    hs=xcalloc(w,sizeof(char*));
    pxs--;
    for(i=0;i<nx;i++) {
      pxs++;
      h=xfnv1a((char*)*pxs, strlen(*pxs))&q;
      while(hs[h]!=0 && strcmp(hs[h],*pxs)) h=(h+1)&q;
      if(!hs[h]) hs[h]=prs[j++]=*pxs;
    }
    nr=j;
    xfree(hs);
    break;
  default: return KERR_TYPE;
  }
  return knorm(r);
}

K count(K x) {
  u64 c=(ax||s(x))?1:nx;
  /* promote to a long atom when the count exceeds the int32 range */
  return c>BIGV?tj((i64)c):t(1,(u32)c);
}

/* `_` (floor verb) float->long support.  promotej: a floored double promotes to
   a long iff it is finite AND lands outside int32 but inside int64 range, i.e.
   it is losslessly representable as a long.  +-inf, NaN, and beyond-int64
   values do NOT promote -- they clamp to the int sentinels: +inf -> 0I,
   -inf -> -0I, NaN -> 0N (-inf used to land on 0N, INT32_MIN, one below -0I).
   ftoj: convert for the long path once a vector has promoted, clamping the
   unrepresentable elements (NaN/inf/beyond-int64) to the long sentinels. */
static int promotej(double f) {
  /* The two ends must be SYMMETRIC about the i32 sentinels.  `f>=INT32_MAX`
     is right: INT32_MAX is 0I, so a floor landing there has to promote.  But
     `f<INT32_MIN` missed BOTH negative sentinels -- INT32_MIN is 0N and
     INT32_MIN+1 is -0I -- so a value that merely LOOKED like one came back as
     one:  _ -2147483647.0 -> -0I  and  _ -2147483648.0 -> 0N, while
     _ 2147483647.0 -> 2147483647j and _ -2147483649.0 -> -2147483649j were
     already correct.  Promote from INT32_MIN+1 down. */
  return f==f && (f>=INT32_MAX||f<=(double)(INT32_MIN+1)) && f<(double)INT64_MAX && f>(double)INT64_MIN;
}
static i64 ftoj(double f) {
  if(f!=f) return J_NULL;                  /* NaN */
  if(f>=(double)INT64_MAX) return J_INF;   /* +inf / >= 2^63 */
  /* `<=`, not `<`: at exactly -2^63 the (i64) cast yields INT64_MIN, which IS
     J_NULL -- so `_ -9223372036854775808.0 30000000000.0` produced 0Nj rather
     than the -0Ij clamp every other out-of-range value gets.  Same shape as
     the i32 case above, one width up. */
  if(f<=(double)INT64_MIN) return J_NINF;  /* -inf / <= -2^63 */
  return (i64)f;
}
/* int clamp for the non-promoting arms, same sentinel mapping */
static i32 ftoi_(double f) {
  if(f!=f) return INT32_MIN;               /* NaN -> 0N */
  if(f>=INT32_MAX) return INT32_MAX;       /* +inf -> 0I */
  if(f<INT32_MIN) return INT32_MIN+1;      /* -inf -> -0I */
  return (i32)f;
}

K floor__(K x) {
  K r=0;
  i32 *pri;
  i64 *prj;
  float *pxe;
  double f,*pxf;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case  1: r=x; break;
  case  8: r=k_(x); break;
  case  2:
    f=floor(fk(x));
    if(promotej(f)) r=tj((i64)f);                            /* finite, fits long not int */
    else r=t(1,(u32)ftoi_(f));
    break;
  case  9:
    f=floor((double)ek(x));
    if(promotej(f)) r=tj((i64)f);
    else r=t(1,(u32)ftoi_(f));
    break;
  case -1: r=k_(x); break;
  case -8: r=k_(x); break;
  case -2: { int needj=0; PXF;
    i(nx,if(promotej(floor(pxf[i]))){needj=1;break;})
    if(needj) { r=tn(8,nx); prj=(i64*)px(r); i(nx,prj[i]=ftoj(floor(pxf[i]))) }
    else { PRI(nx); i(nx,pri[i]=ftoi_(floor(pxf[i]))) }
    } break;
  case -9: { int needj=0; PXE;
    i(nx,if(promotej(floor((double)pxe[i]))){needj=1;break;})
    if(needj) { r=tn(8,nx); prj=(i64*)px(r); i(nx,prj[i]=ftoj(floor((double)pxe[i]))) }
    else { PRI(nx); i(nx,pri[i]=ftoi_(floor((double)pxe[i]))) }
    } break;
  case  0: r=irecur1(floor__,x); break;
  default: return KERR_TYPE;
  }
  return r;
}

K shape(K x) {
  K r=0,a=0,*q=0,*t=0,*pak;
  u32 cm=32,ci;
  u64 i,qc,tc;   /* BFS level sizes: a level can hold >2^32 nodes */
  i64 *c;        /* dimension counts: u64-valued, so NOT u32 -- see the emit below */
  i32 *pri;
  i64 *prj;
  if(ax||s(x)) return tn(1,0);
  c=xmalloc(sizeof(i64)*cm);
  switch(tx) {
  case -1: case -2: case -3: case -4: case -8: case -9:
    if(nx>BIGV) { r=tn(8,1); ((i64*)px(r))[0]=(i64)nx; }
    else { PRI(1); pri[0]=(i32)nx; }
    break;
  case  0:
    q=xmalloc(sizeof(K));
    q[0]=x;
    qc=1;
    ci=0;
    while(q) { /* breadth first traversal */
      u64 enq=0; /* nodes at this level that enqueued children */
      if(ci==cm) { cm<<=1; c=xrealloc(c,sizeof(i64)*cm); }
      c[ci]=-1; tc=0; t=0; /* -1 = count unset: an EMPTY child (na 0)
        must conflict with a nonempty sibling either way round -- the old
        `!c[ci]` test let a leading empty pass unrecorded, so ^((^-18);,16)
        was 2 1, not ,2 */
      for(i=0;i<qc;i++) {
        a=q[i];
        if(ta>0||s(a)) { if(t){xfree(t);t=0;} ci--; break; } /* atom ends the
          dimension: drop any children already enqueued from earlier siblings,
          or the walk would continue into the next depth with a partial level
          and emit a phantom dimension (^((,0;,1);-17) gave 2 1, not ,2) */
        if(c[ci]==-1) c[ci]=(i64)na;
        else if((u64)c[ci]!=na) { if(t){xfree(t);t=0;}; ci--; break; }
        if(!ta&&!s(a)&&na) { /* enqueue next round */
          if(t) t=xrealloc(t,sizeof(K)*(na+tc));
          else t=xmalloc(sizeof(K)*na);
          PAK;
          j(na,t[tc++]=pak[j])
          enq++;
        }
      }
      /* a deeper level exists only if EVERY node here contributed children:
         a plain-vector sibling (its elements are atoms) next to a nested
         list otherwise dropped out silently and the walk continued on the
         list's children alone -- ^((,16;,(-19,!3))) grew a phantom 3rd
         dimension */
      if(t && enq<qc) { xfree(t); t=0; tc=0; }
      xfree(q);
      q=t; qc=tc; /* next queue */
      ci++;
    }
    /* Mirror the flat-vector arm above: a dimension >= 2^31 must ship as an
       i64.  This arm had NO BIGV test at all, so after the BIGV fix the two
       arms DISAGREED -- for a 2147483647-long vector x, `^x` gave
       ,2147483647j (correct) while `^,x` gave `1 0I`.  Worse further out:
       2^31 printed as 0N and 2^32+5 as 5, because the count was also being
       truncated through a u32.  count() promotes for every type, so this arm
       was simply the one that was missed.
       ONE width for the whole shape, chosen by the widest dimension, rather
       than per-dimension boxing: a mixed (1;2147483647j) general list would be
       a third representation for the same idea, and gk's own adoption rule
       (int + long -> long vector) says these should ride together.  So the
       result is always a plain vector -- -1 when every dimension fits, -8 when
       any does not, matching what the flat arm emits for the same data. */
    { i32 big=0;
      i(ci,if((u64)c[i]>BIGV) { big=1; break; })
      if(big) { PRJ(ci); i(ci,prj[i]=c[i]) }
      else    { PRI(ci); i(ci,pri[i]=(i32)c[i]) } }
    break;
  default: r=KERR_TYPE;
  }
  xfree(c);
  return knorm(r);
}

K enlist(K x) {
  K r=0,*prk;
  i32 *pri;
  double *prf;
  char *prc,**prs;
  if(s(x)) return enlistcb(x);
  switch(tx) {
  case  1: PRI(1); *pri=ik(x); break;
  case  2: PRF(1); *prf=fk(x); break;
  case  8: { i64 *prj; PRJ(1); *prj=jk(x); } break;
  case  9: { float *pre; PRE(1); *pre=ek(x); } break;
  case  3: PRC(1); *prc=ck(x); break;
  case  4: PRS(1); *prs=sk(x); break;
  case  6: PRK(1); *prk=x; break;
  case -1: case -2: case -8: case -9: case -3: case -4: case 0: PRK(1); *prk=k_(x); break;
  default: r=KERR_TYPE;
  }
  return r;
}

K format(K x) {
  K r=0;
  char ds[256]={0},*prc,*s;
  double f;
  if(s(x)) return formatcb(x);
  switch (tx) {
    case  1:
      if(ik(x)==INT32_MAX) sprintf(ds,"0I");
      else if(ik(x)==INT32_MIN+1) sprintf(ds,"-0I");
      else if(ik(x)==INT32_MIN) ds[0]=0;
      else fmtj(ds,(i64)ik(x));
      PRC(strlen(ds));
      i(strlen(ds),*prc++=ds[i]);
      break;
    case  2:
      f=fk(x);
      if(isinf(f) && f>0.0) sprintf(ds,"0i");
      else if(isinf(f) && f< 0.0) sprintf(ds,"-0i");
      else if(isnan(f)) ds[0]=0;
      else sprintf(ds,"%0.*g",precision,f);
      PRC(strlen(ds));
      i(strlen(ds),*prc++=ds[i]);
      break;
    case  8: {
      i64 v=jk(x);
      if(v==J_INF) sprintf(ds,"0I");
      else if(v==J_NINF) sprintf(ds,"-0I");
      else if(v==J_NULL) ds[0]=0;
      else fmtj(ds,v);
      PRC(strlen(ds));
      i(strlen(ds),*prc++=ds[i]);
      } break;
    case  9: {
      int p32=precision<9?precision:9; /* float32: 9 sig digits round-trip (FLT_DECIMAL_DIG) */
      f=(double)ek(x);
      if(isinf(f) && f>0.0) sprintf(ds,"0i");
      else if(isinf(f) && f< 0.0) sprintf(ds,"-0i");
      else if(isnan(f)) ds[0]=0;
      else sprintf(ds,"%0.*g",p32,f);
      PRC(strlen(ds));
      i(strlen(ds),*prc++=ds[i]);
      } break;
    case  3: PRC(1); prc[0]=ck(x); break;
    case  4: s=sk(x); PRC(strlen(s)); memcpy(prc,s,1+strlen(s)); break;
    case  6: PRC(0); break;
    case  0: r=irecur1(format,x); break;
    case -1: case -2: case -8: case -9: case -4: r=knorm(each(19,0,x)); break;
    case -3: r=k_(x); break;
    default: return KERR_TYPE;
  }
  return knorm(r);
}
