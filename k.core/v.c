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
K (*FD[FDSIZE])(K,K)={0,plus,minus,times,divide,minand,maxor,less,more,equal,
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

#define PMT(F,O,I) \
K F(K a, K x) { \
  K r=0; \
  i32 *pri,*pai,*pxi; \
  double f,*prf,*paf,*pxf; \
  if(s(x)||s(a)) return KERR_TYPE; \
  if(!(aa||ax)&&na!=nx) return KERR_LENGTH; \
  switch(ta) { \
  case 1: \
    switch(tx) { \
    case  1: r=t(1,(u32)(ik(a) O ik(x))); break; \
    case  2: f=fi(ik(a)) O fk(x); r=t2(f); break; \
    case -1: PRI(nx); PXI; i(nx,*pri++=ik(a) O *pxi++) break; \
    case -2: PRF(nx); PXF; i(nx,*prf++=fi(ik(a)) O *pxf++) break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case 2: \
    switch(tx) { \
    case  1: f=fk(a) O fi(ik(x)); r=t2(f); break; \
    case  2: f=fk(a) O fk(x); r=t2(f); break; \
    case -1: PRF(nx); PXI; f=fk(a); i(nx,*prf++=f O fi(*pxi++)) break; \
    case -2: PRF(nx); PXF; f=fk(a); i(nx,*prf++=f O *pxf++) break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case -1: \
    switch(tx) { \
    case  1: PRI(na); PAI; i(na,*pri++=*pai++ O ik(x)) break; \
    case  2: PRF(na); PAI; f=fk(x); i(na,*prf++=fi(*pai++) O f) break; \
    case -1: PRI(nx); PAI; PXI; i(nx,*pri++=*pai++ O *pxi++) break; \
    case -2: PRF(nx); PAI; PXF; i(nx,*prf++=fi(*pai++) O *pxf++) break; \
    case  0: r=each(I,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case -2: \
    switch(tx) { \
    case  1: PRF(na); PAF; f=fi(ik(x)); i(na,*prf++=*paf++ O f) break; \
    case  2: PRF(na); PAF; f=fk(x); i(na,*prf++=*paf++ O f) break; \
    case -1: PRF(nx); PAF; PXI; i(nx,*prf++=*paf++ O fi(*pxi++)) break; \
    case -2: PRF(nx); PAF; PXF; i(nx,*prf++=*paf++ O *pxf++) break; \
    case  0: r=each(I,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case  0: \
    switch(tx) { \
    case  1: r=irecur2(F,a,x); break; \
    case  2: r=irecur2(F,a,x); break; \
    case -1: r=each(I,a,x); break; \
    case -2: r=each(I,a,x); break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  default: return KERR_TYPE; \
  } \
  return r; \
}
PMT(plus,+,1)
PMT(minus,-,2)
PMT(times,*,3)

K divide(K a, K x) {
  K r=0;
  i32 *pai,*pxi;
  double f,*prf,*paf,*pxf;
  if(s(x)||s(a)) return KERR_TYPE;
  if(!(aa||ax)&&na!=nx) return KERR_LENGTH;
  switch(ta) {
  case 1:
    switch(tx) {
    case  1: f=fi(ik(a)) / fi(ik(x)); r=t2(f); break;
    case  2: f=fi(ik(a)) / fk(x); r=t2(f); break;
    case -1: PRF(nx); PXI; f=fi(ik(a)); i(nx,*prf++=f / fi(*pxi++)) break;
    case -2: PRF(nx); PXF; f=fi(ik(a)); i(nx,*prf++=f / *pxf++) break;
    case  0: r=irecur2(divide,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 2:
    switch(tx) {
    case  1: f=fk(a) / fi(ik(x)); r=t2(f); break;
    case  2: f=fk(a) / fk(x); r=t2(f); break;
    case -1: PRF(nx); PXI; f=fk(a); i(nx,*prf++=f / fi(*pxi++)) break;
    case -2: PRF(nx); PXF; f=fk(a); i(nx,*prf++=f / *pxf++) break;
    case  0: r=irecur2(divide,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -1:
    switch(tx) {
    case  1: PRF(na); PAI; f=fi(ik(x)); i(na,*prf++=fi(*pai++) / f) break;
    case  2: PRF(na); PAI; f=fk(x); i(na,*prf++=fi(*pai++) / f) break;
    case -1: PRF(nx); PAI; PXI; i(nx,*prf++=fi(*pai++) / fi(*pxi++)) break;
    case -2: PRF(nx); PAI; PXF; i(nx,*prf++=fi(*pai++) / *pxf++) break;
    case  0: r=each(4,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -2:
    switch(tx) {
    case  1: PRF(na); PAF; f=fi(ik(x)); i(na,*prf++=*paf++ / f) break;
    case  2: PRF(na); PAF; f=fk(x); i(na,*prf++=*paf++ / f) break;
    case -1: PRF(nx); PAF; PXI; i(nx,*prf++=*paf++ / fi(*pxi++)) break;
    case -2: PRF(nx); PAF; PXF; i(nx,*prf++=*paf++ / *pxf++) break;
    case  0: r=each(4,a,x); break;
    default: return KERR_TYPE;
    } break;
  case  0:
    switch(tx) {
    case  1: r=irecur2(divide,a,x); break;
    case  2: r=irecur2(divide,a,x); break;
    case -1: r=each(4,a,x); break;
    case -2: r=each(4,a,x); break;
    case  0: r=irecur2(divide,a,x); break;
    default: return KERR_TYPE;
    } break; \
  default: return KERR_TYPE;
  }
  return r;
}

#define MAMO(F,O,R,I) \
K F(K a, K x) { \
  K r=0; \
  i32 *pri,*pai,*pxi; \
  double f,*prf,*paf,*pxf; \
  char *prc,*pac,*pxc,**prs,**pas,**pxs; \
  if(s(x)||s(a)) return KERR_TYPE; \
  if(!(aa||ax)&&na!=nx) return KERR_LENGTH; \
  switch(ta) { \
  case 1: \
    switch(tx) { \
    case  1: r=ik(a) O ik(x)?a:x; break; \
    case  2: f=fi(ik(a)); r=t2(cmpff(f,fk(x))==R?f:fk(x)); break; \
    case -1: PRI(nx); PXI; i(nx,*pri++=ik(a) O *pxi?ik(a):*pxi;++pxi) break; \
    case -2: PRF(nx); PXF; f=fi(ik(a)); i(nx,*prf++=cmpff(f,*pxf)==R?f:*pxf;++pxf) break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case 2: \
    switch(tx) { \
    case  1: f=fi(ik(x)); r=t2(cmpff(fk(a),f)==R?fk(a):f);; break; \
    case  2: r=cmpff(fk(a),fk(x))==R?k_(a):k_(x); break; \
    case -1: PRF(nx); PXI; f=fk(a); i(nx,*prf++=cmpff(f,fi(*pxi))==R?f:fi(*pxi);++pxi) break; \
    case -2: PRF(nx); PXF; f=fk(a); i(nx,*prf++=cmpff(f,*pxf)==R?f:*pxf;++pxf) break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case 3: \
    switch(tx) { \
    case  3: r=ck(a) O ck(x)?a:x; break; \
    case -3: PRC(nx); PXC; i(nx,*prc++=ck(a) O *pxc?ck(a):*pxc; ++pxc) break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case 4: \
    switch(tx) { \
    case  4: r=strcmp(sk(a),sk(x)) O 0?a:x; break; \
    case -4: PRS(nx); PXS; i(nx,*prs++=strcmp(sk(a),*pxs) O 0?sk(a):*pxs;++pxs) break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case -1: \
    switch(tx) { \
    case  1: PRI(na); PAI; i(na,*pri++=*pai O ik(x)?*pai:ik(x);++pai) break; \
    case  2: PRF(na); PAI; f=fk(x); i(na,*prf++=cmpff(fi(*pai),f)==R?fi(*pai):f;++pai) break; \
    case -1: PRI(nx); PAI; PXI; i(nx,*pri++=*pai O *pxi?*pai:*pxi;++pai;++pxi) break; \
    case -2: PRF(nx); PAI; PXF; i(nx,*prf++=cmpff(fi(*pai),*pxf)==R?fi(*pai):*pxf;++pai;++pxf) break; \
    case  0: r=each(I,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case -2: \
    switch(tx) { \
    case  1: PRF(na); PAF; f=fi(ik(x)); i(na,*prf++=cmpff(*paf,f)==R?*paf:f;++paf) break; \
    case  2: PRF(na); PAF; f=fk(x); i(na,*prf++=cmpff(*paf,f)==R?*paf:f;++paf) break; \
    case -1: PRF(nx); PAF; PXI; i(nx,*prf++=cmpff(*paf,fi(*pxi))==R?*paf:fi(*pxi);++paf;++pxi) break; \
    case -2: PRF(nx); PAF; PXF; i(nx,*prf++=cmpff(*paf,*pxf)==R?*paf:*pxf;++paf;++pxf) break; \
    case  0: r=each(I,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case -3: \
    switch(tx) { \
    case  3: PRC(na); PAC; i(na,*prc++=*pac O ck(x)?*pac:ck(x);++pac) break; \
    case -3: PRC(nx); PAC; PXC; i(nx,*prc++=*pac O *pxc?*pac:*pxc; ++pac; ++pxc) break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case -4: \
    switch(tx) { \
    case  4: PRS(na); PAS; i(na,*prs++=strcmp(*pas,sk(x)) O 0?*pas:sk(x);++pas) break; \
    case -4: PRS(nx); PAS; PXS; i(nx,*prs++=strcmp(*pas,*pxs) O 0?*pas:*pxs;++pas;++pxs) break; \
    case  0: r=each(I,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case  0: \
    switch(tx) { \
    case  1: r=irecur2(F,a,x); break; \
    case  2: r=irecur2(F,a,x); break; \
    case  3: r=irecur2(F,a,x); break; \
    case  4: r=irecur2(F,a,x); break; \
    case -1: r=each(I,a,x); break; \
    case -2: r=each(I,a,x); break; \
    case -3: r=each(I,a,x); break; \
    case -4: r=each(I,a,x); break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  default: return KERR_TYPE; \
  } \
  return r; \
}
MAMO(minand,<,-1,5)
MAMO(maxor,>,1,6)

#define LME(F,O,R,I) \
K F(K a, K x) { \
  K r=0; \
  i32 *pri,*pai,*pxi; \
  double f,*paf,*pxf; \
  char *pac,*pxc,**pas,**pxs; \
  if(s(x)||s(a)) return KERR_TYPE; \
  if(!(aa||ax)&&na!=nx) return KERR_LENGTH; \
  switch(ta) { \
  case 1: \
    switch(tx) { \
    case  1: r=t(1,(u32)(ik(a) O ik(x))); break; \
    case  2: r=t(1,cmpfft(fi(ik(a)),fk(x))==R); break; \
    case -1: PRI(nx); PXI; i(nx,*pri++=ik(a) O *pxi++) break; \
    case -2: PRI(nx); PXF; f=fi(ik(a)); i(nx,*pri++=cmpfft(f,*pxf++)==R) break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case 2: \
    switch(tx) { \
    case  1: r=t(1,cmpfft(fk(a),fi(ik(x)))==R); break; \
    case  2: r=t(1,cmpfft(fk(a),fk(x))==R); break; \
    case -1: PRI(nx); PXI; f=fk(a); i(nx,*pri++=cmpfft(f,fi(*pxi++))==R) break; \
    case -2: PRI(nx); PXF; f=fk(a); i(nx,*pri++=cmpfft(f,*pxf++)==R) break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case 3: \
    switch(tx) { \
    case  3: r=t(1,(u32)(ck(a) O ck(x))); break; \
    case -3: PRI(nx); PXC; i(nx,*pri++=t(1,(u32)(ck(a) O *pxc++))) break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case 4: \
    switch(tx) { \
    case  4: r=t(1,(u32)(strcmp(sk(a),sk(x)) O 0)); break; \
    case -4: PRI(nx); PXS; i(nx,*pri++=strcmp(sk(a),*pxs++) O 0) break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case -3: \
    switch(tx) { \
    case  3: PRI(na); PAC; i(na,*pri++=t(1,(u32)(*pac++ O ck(x)))) break; \
    case -3: PRI(nx); PAC; PXC; i(nx,*pri++=t(1,(u32)(*pac++ O *pxc++))) break; \
    case  0: r=each(I,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case -1: \
    switch(tx) { \
    case  1: PRI(na); PAI; i(na,*pri++=*pai++ O ik(x)) break; \
    case  2: PRI(na); PAI; f=fk(x); i(na,*pri++=cmpfft(fi(*pai++),f)==R) break; \
    case -1: PRI(nx); PAI; PXI; i(nx,*pri++=*pai++ O *pxi++) break; \
    case -2: PRI(nx); PAI; PXF; i(nx,*pri++=cmpfft(fi(*pai++),*pxf++)==R) break; \
    case  0: r=each(I,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case -2: \
    switch(tx) { \
    case  1: PRI(na); PAF; f=fi(ik(x)); i(na,*pri++=cmpfft(*paf++,f)==R) break; \
    case  2: PRI(na); PAF; f=fk(x); i(na,*pri++=cmpfft(*paf++,f)==R) break; \
    case -1: PRI(nx); PAF; PXI; i(nx,*pri++=cmpfft(*paf++,fi(*pxi++))==R) break; \
    case -2: PRI(nx); PAF; PXF; i(nx,*pri++=cmpfft(*paf++,*pxf++)==R) break; \
    case  0: r=each(I,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case -4: \
    switch(tx) { \
    case  4: PRI(na); PAS; i(na,*pri++=strcmp(*pas++,sk(x)) O 0) break; \
    case -4: PRI(nx); PAS; PXS; i(nx,*pri++=strcmp(*pas++,*pxs++) O 0) break; \
    case  0: r=each(I,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  case  0: \
    switch(tx) { \
    case  1: r=irecur2(F,a,x); break; \
    case  2: r=irecur2(F,a,x); break; \
    case  3: r=irecur2(F,a,x); break; \
    case  4: r=irecur2(F,a,x); break; \
    case -1: r=each(I,a,x); break; \
    case -2: r=each(I,a,x); break; \
    case -3: r=each(I,a,x); break; \
    case -4: r=each(I,a,x); break; \
    case  0: r=irecur2(F,a,x); break; \
    default: return KERR_TYPE; \
    } break; \
  default: return KERR_TYPE; \
  } \
  return r; \
}
LME(less,<,-1,7)
LME(more,>,1,8)
LME(equal,==,0,9)

K match(K a, K x) {
  if(ta==2&&tx==2) return t(1,(u32)(0==cmpfft(fk(a),fk(x))));
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
    if(s(p)||s(t)) { _k(t); return 3; } /* type */
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
  if(a==null&&x==null) return tn(0,0);
  if(!ax&&!s(x)&&nx==0) return k_(a);
  if(s(a)||s(x)) return dotcb(a,x);
  if(ta==4||ta==3||ta==-3) { r=dotcb(a,x); if(r) return r; }
  switch(ta) {
  case -1: case -2: case -3: case -4:
    switch(tx) {
    case 1: case 6: r=at(a,x); break;
    case 0:
      if(nx!=1) r=KERR_RANK;
      else { K *px=px(x); r=at(a,px[0]); }
      break;
    case -1:
      if(nx!=1) r=KERR_RANK;
      else { i32 *px=px(x); r=at(a,t(1,(u32)px[0])); }
      break;
    default: r=KERR_RANK;
    } break;
  case  0:
    switch(tx) {
    case 1: case 4: r=at(a,x); break;
    case 0: case -1: case -4: q=kmix(x); EC(q); r=dot0(a,q); _k(q); break;
    case 6: r=at(a,x); break;
    default: r=KERR_RANK;
    } break;
  case  6:
    if(tx<=0&&!s(x)&&nx==1) {
      K *px=px(x);
      if(px[0]==null) r=tn(0,0);
      else r=k_(x);
    }
    else r=k_(x);
    break;
  default: r=KERR_RANK;
  }
  return knorm(r);
cleanup:
  if(r) _k(r);
  return e;
}

K modrot(K a, K x) {
  K r=0,*prk,*pxk;
  char *prc,*pxc,**prs,**pxs;
  i32 *pri,*pai,*pxi;
  i64 ri;
  double rf,f,*prf,*paf,*pxf;
  if(s(x)||s(a)) return KERR_TYPE;
  switch(ta) {
  case 1:
    switch(tx) {
    case  1:
      if(!ik(x)) r=t(1,(u32)INT32_MIN);
      else if(ik(x)==-1 && ik(a)==INT32_MIN) r=t(1,0);
      else { ri=ik(a) % ik(x); r=t(1,(u32)(ri<0?ri+ik(x):ri)); }
      break;
    case  2: rf=fmod(ik(a),fk(x)); f=rf<0?rf+fk(x):rf; r=t2(f); break;
    case -1: PRI(nx); PXI; i(nx,ri=((i64)i+ik(a))%(i64)nx; if(ri<0)ri+=nx; *pri++=pxi[ri];) break;
    case -2: PRF(nx); PXF; i(nx,ri=((i64)i+ik(a))%(i64)nx; if(ri<0)ri+=nx; *prf++=pxf[ri];) break;
    case -3: PRC(nx); PXC; i(nx,ri=((i64)i+ik(a))%(i64)nx; if(ri<0)ri+=nx; *prc++=pxc[ri]) break;
    case -4: PRS(nx); PXS; i(nx,ri=((i64)i+ik(a))%(i64)nx; if(ri<0)ri+=nx; *prs++=pxs[ri];) break;
    case  0: PRK(nx); PXK; i(nx,ri=((i64)i+ik(a))%(i64)nx; if(ri<0)ri+=nx; *prk++=k_(pxk[ri]);) break;
    default: return KERR_TYPE;
    } break;
  case 2:
    switch(tx) {
    case 1: rf=fmod(fk(a),ik(x)); f=rf<0?rf+ik(x):rf; r=t2(f); break;
    case 2: rf=fmod(fk(a),fk(x)); f=rf<0?rf+fk(x):rf; r=t2(f); break;
    default: return KERR_INT;
    } break;
  case -1:
    switch(tx) {
    case 1:
      PRI(na); PAI;
      if(!ik(x)) i(na,*pri++=INT32_MIN)
      else if(ik(x)==-1) { i(na,*pri++=pai[i]==INT32_MIN?0:-pai[i]) }
      else i(na,ri=pai[i]%ik(x); pri[i]=ri<0?ri+ik(x):ri) break;
    case 2: PRF(na); PAI; i(na,rf=fmod(pai[i],fk(x)); prf[i]=rf<0?rf+fk(x):rf) break;
    default: return KERR_INT;
    } break;
  case -2:
    switch(tx) {
    case 1: PRF(na); PAF; i(na,rf=fmod(paf[i],ik(x)); if(rf==-0)rf=0; prf[i]=rf<0?rf+ik(x):rf) break;
    case 2: PRF(na); PAF; i(na,rf=fmod(paf[i],fk(x)); if(rf==-0)rf=0; prf[i]=rf<0?rf+fk(x):rf) break;
    default: return KERR_INT;
    } break;
  default: return KERR_INT;
  }
  return knorm(r);
}

K at(K a, K x) {
  K r=0,*prk,*pak;
  char *prc,*pac,**prs,**pas;
  i32 *pri,*pai,*pxi;
  double f=0,*prf,*paf;
  if(s(a)||s(x)||4==ta) return atcb(a,x);;
  if(a==null&&x==null) return tn(0,0);
  if(a==null) return k_(x);
  if(x==null||x==inull) return k_(a);
  if(!tx&&!nx) return tn(0,0);
  if(aa) return KERR_TYPE;
  if((tx==1)&&((ik(x)<0)||(u64)(ik(x))>=na)) return KERR_INDEX;
  if(tx==-1) { PXI; i(nx,if(pxi[i]<0||(u64)(pxi[i])>=na)return KERR_INDEX;) }
  if(!tx) return eachright(13,a,x);
  if(tx!=1&&tx!=-1) return KERR_TYPE;
  switch(ta) {
  case -1:
    switch(tx) {
    case  1: PAI; r=t(1,(u32)pai[ik(x)]); break;
    case -1: PRI(nx); PAI; PXI; i(nx,*pri++=pai[*pxi++]) break;
    } break;
  case -2:
    switch(tx) {
    case  1: PAF; f=paf[ik(x)]; r=t2(f); break;
    case -1: PRF(nx); PAF; PXI; i(nx,*prf++=paf[*pxi++]) break;
    } break;
  case -3:
    switch(tx) {
    case  1: PAC; r=t(3,(u8)pac[ik(x)]); break;
    case -1: PRC(nx); PAC; PXI; i(nx,*prc++=pac[*pxi++]) break;
    } break;
  case -4:
    switch(tx) {
    case  1: PAS; r=t(4,pas[ik(x)]); break;
    case -1: PRS(nx); PAS; PXI; i(nx,*prs++=pas[*pxi++]) break;
    } break;
  case  0:
    switch(tx) {
    case  1: PAK; r=k_(pak[ik(x)]); break;
    case -1: PRK(nx); PAK; PXI; i(nx,*prk++=k_(pak[*pxi++])) break;
    } break;
  default: return KERR_TYPE;
  }
  return knorm(r);
}

static K draw(K a, K x) {
  K r=0;
  if(s(x)||s(a)) return KERR_TYPE;
  if(ta!=0 && ta!=1 && ta!=-1) return KERR_TYPE;
  if(ta==1 && tx==1 && ik(x)<0 && ik(a)>abs(ik(x))) return KERR_LENGTH;
  switch(ta) {
  case 1:
    if(ik(a)<0) return 6; /* domain */
    VSIZE(ik(a));
    switch(tx) {
    case 1:
      if(ik(x)>0) { r=tn(1,ik(a)); drawi((i32*)px(r),ik(a),ik(x)); }
      else if(ik(x)<0) { r=tn(1,ik(a)); deal((i32*)px(r),ik(a),abs(ik(x))); }
      else { r=tn(2,ik(a)); drawf((double*)px(r),ik(a),1.0); }
      break;
    case 2: r=tn(2,ik(a)); drawf((double*)px(r),ik(a),fk(x)); break;
    default: return KERR_TYPE;
    } break;
  default: return KERR_TYPE;
  }
  return r; 
}
K find(K a, K x) {
  K r=0,*pak;
  char *pac,**pas;
  i32 *pai;
  double *paf;
  if(ta==1) return draw(a,x);
  if(aa) return KERR_DOMAIN;
  if(s(a)) return KERR_TYPE;
  r=t(1,na);
  switch(ta) {
  case -1: if(tx!=1) break; PAI; i(na,if(pai[i]==ik(x)){r=t(1,i); break;}) break;
  case -2: if(tx!=2) break; PAF; i(na,if(paf[i]==fk(x)){r=t(1,i); break;}) break;
  case -3: if(tx!=3) break; PAC; i(na,if(pac[i]==ck(x)){r=t(1,i); break;}) break;
  case -4: if(tx!=4) break; PAS; i(na,if(!strcmp(pas[i],sk(x))){r=t(1,i); break;}) break;
  case  0: PAK; i(na,if(!kcmpr(pak[i],x)){r=t(1,i); break;}) break;
  }
  return r;
}

static i32 TJ;
static K take_(K a, K x) {
  K r=0,e,*prk,*pxk,a0=0,a_=0,leaf;
  char *prc,*pxc,**prs,**pxs,Ta,Tx;
  i32 c,d,*pri,*pxi;
  double *prf,*pxf;
  Ta=ta; if(s(a)) { if(!vstcb(a)) return KERR_TYPE; Ta=15; }
  Tx=tx; if(s(x)) { if(!vstcb(x)) return KERR_TYPE; Tx=15; }

  if(Ta<=0 && !na) {
    switch(Tx) {
    case  1: case  2: case  3: case  4: case 6: case 15: r=k_(x); break;
    case  0: if(nx) r=k_(((K*)px(x))[0]); else r=null; break;
    case -1: if(nx) r=t(1,(u32)(((i32*)px(x))[0])); else r=null; break;
    case -2: if(nx) r=t2(((double*)px(x))[0]); else r=null; break;
    case -3: if(nx) r=t(3,(u8)(((char*)px(x))[0])); else r=t(3,' '); break;
    case -4: if(nx) r=t(4,((char**)px(x))[0]); else r=t(4,sp("")); break;
    }
    return r;
  }

  switch(Ta) {
  case 1:
    if(ik(a)==INT32_MIN) return k_(x); /* 0N # x -> return x */
    c=ik(a)<0?-a:a;
    VSIZE(c);
    switch(Tx) {
    case  1: PRI(c); i(c,*pri++=ik(x)) break;
    case  2: PRF(c); i(c,*prf++=fk(x)) break;
    case  3: PRC(c); i(c,*prc++=ck(x)) break;
    case  4: PRS(c); i(c,*prs++=sk(x)) break;
    case  6: PRK(c); i(c,*prk++=null) break;
    case 15: PRK(c); i(c,*prk=kcp(x); EC(*prk); ++prk) break;
    case -1: PRI(c); PXI;
      if(nx&&ik(a)<0) { d=c%nx; i(c,*pri++=pxi[(nx-d+i+TJ)%nx]) }
      else if(nx) i(c,*pri++=pxi[(i+TJ)%nx])
      else i(c,*pri++=0)
      break;
    case -2: PRF(c); PXF;
      if(nx&&ik(a)<0) { d=c%nx; i(c,*prf++=pxf[(nx-d+i+TJ)%nx]) }
      else if(nx) i(c,*prf++=pxf[(i+TJ)%nx])
      else i(c,*prf++=0.0)
      break;
    case -3: PRC(c); PXC;
      if(nx&&ik(a)<0) { d=c%nx; i(c,*prc++=pxc[(nx-d+i+TJ)%nx]) }
      else if(nx) i(c,*prc++=pxc[(i+TJ)%nx])
      else i(c,*prc++=' ')
      break;
    case -4: PRS(c); PXS;
      if(nx&&ik(a)<0) { d=c%nx; i(c,*prs++=pxs[(nx-d+i+TJ)%nx]) }
      else if(nx) i(c,*prs++=pxs[(i+TJ)%nx])
      else i(c,*prs++="") /* sp()? */
      break;
    case  0: PRK(c); PXK;
      if(nx&&ik(a)<0) { d=c%nx; i(c,*prk++=k_(pxk[(nx-d+i+TJ)%nx])) }
      else if(nx) i(c,*prk++=k_(pxk[(i+TJ)%nx]))
      else i(c,*prk++=null)
      break;
    } break;
  case -1: {
    i32 *pa=px(a);
    i32 n=1;
    if(!na) {
      switch(tx) {
      case -1: case 1: return tn(1,0);
      case -2: case 2: return tn(2,0);
      case -3: case 3: return tn(3,0);
      case -4: case 4: return tn(4,0);
      default: return tn(0,0);
      }
    }
    /* 0N # x -> return x as-is */
    if(na==1 && pa[0]==INT32_MIN) return k_(x);
    /* 0N k # x -> chunk into k-tuples with remainder */
    if(na==2 && pa[0]==INT32_MIN) {
      i32 kk=pa[1],ln; i64 st,rows;
      if(kk<=0 || kk==INT32_MIN || kk==INT32_MAX) return KERR_DOMAIN;
      if(ax || s(x)) return k_(x);
      if(!nx) return tn(0,0);
      rows=((i64)nx+kk-1)/kk;
      if(rows>INT32_MAX) return KERR_WSFULL;
      PRK((i32)rows);
      switch(tx) {
      case -1: PXI; i(rows, st=i*kk; ln=(i==(u64)rows-1)?(i32)(nx-st):kk; prk[i]=tn(1,ln); pri=px(prk[i]); j(ln,pri[j]=pxi[st+j])) break;
      case -2: PXF; i(rows, st=i*kk; ln=(i==(u64)rows-1)?(i32)(nx-st):kk; prk[i]=tn(2,ln); prf=px(prk[i]); j(ln,prf[j]=pxf[st+j])) break;
      case -3: PXC; i(rows, st=i*kk; ln=(i==(u64)rows-1)?(i32)(nx-st):kk; prk[i]=tn(3,ln); prc=px(prk[i]); j(ln,prc[j]=pxc[st+j])) break;
      case -4: PXS; i(rows, st=i*kk; ln=(i==(u64)rows-1)?(i32)(nx-st):kk; prk[i]=tn(4,ln); prs=px(prk[i]); j(ln,prs[j]=pxs[st+j])) break;
      case  0: PXK; i(rows, st=i*kk; ln=(i==(u64)rows-1)?(i32)(nx-st):kk; prk[i]=tn(0,ln); K*pc=px(prk[i]); j(ln,pc[j]=k_(pxk[st+j]))) break;
      }
      return knorm(r);
    }
    VSIZE(pa[0]);
    for(u64 i=0;i<na;i++) {
      if(pa[i]<0 || pa[i]==INT32_MAX) { e=KERR_WSFULL; goto cleanup; }
      i64 tmp=(i64)n*pa[i];
      if(tmp>INT32_MAX) { e=KERR_WSFULL; goto cleanup; }
      n=(i32)tmp;
    }
    K flat=take_(t(1,n),x); EC(flat);
    if(na==1) { r=flat; break; }
    typedef struct { K r; u64 d; i32 i; } SF;
    i32 sm=32,sp=0;
    SF *stack=xmalloc(sizeof(SF)*sm);
    r=tn(0,pa[0]);
    stack[sp++]=(SF){r,0,0};
    i64 j=0;
    while(sp) {
      SF *f=&stack[sp - 1];
      if(f->d==na-1) {
        switch(T(flat)) {
          case -1:
            leaf=tn(1,pa[f->d]);
            pri=px(leaf);
            pxi=px(flat);
            i(pa[f->d],pri[i]=pxi[j++])
            ((K*)px(f->r))[f->i++]=leaf;
            break;
          case -2:
            leaf=tn(2,pa[f->d]);
            prf=px(leaf);
            pxf=px(flat);
            i(pa[f->d],prf[i]=pxf[j++])
            ((K*)px(f->r))[f->i++]=leaf;
            break;
          case -3:
            leaf=tn(3,pa[f->d]);
            prc=px(leaf);
            pxc=px(flat);
            i(pa[f->d],prc[i]=pxc[j++])
            ((K*)px(f->r))[f->i++]=leaf;
            break;
          case -4:
            leaf=tn(4,pa[f->d]);
            prs=px(leaf);
            pxs=px(flat);
            i(pa[f->d],prs[i]=pxs[j++])
            ((K*)px(f->r))[f->i++]=leaf;
            break;
          default:
            leaf=tn(0,pa[f->d]);
            pxk=px(flat);
            for(i32 i=0;i<pa[f->d];++i) {
              K *pl=px(leaf);
              pl[i]=s(pxk[j])?kcp(pxk[j]):k_(pxk[j]);
              if(E(pl[i])) {
                e=pl[i];
                _k(leaf);
                _k(flat);
                xfree(stack);
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
      if(f->i<pa[f->d]) {
        if(f->d+1==na-1) {
          // next depth is the leaf: push leaf frame directly
          if(sp==sm) { stack=xrealloc(stack,sizeof(SF)*(sm*=2)); f=&stack[sp-1]; }
          stack[sp++]=(SF){f->r,f->d+1,f->i};
          f->i++;
        }
        else {
          // next depth is intermediate: push sub container
          K sub=tn(0,pa[f->d+1]);
          ((K*)px(f->r))[f->i++]=sub;
          if(sp==sm) { stack=xrealloc(stack,sizeof(SF)*(sm*=2)); f=&stack[sp-1]; }
          stack[sp++]=(SF){sub,f->d+1,0};
        }
      }
      else --sp;
    }
    _k(flat);
    xfree(stack);
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
K take(K a, K x) { TJ=0; return take_(a,x); }

K drop(K a, K x) {
  K r=0,e,*prk,*pxk,r2,*pr2k;
  char *prc,*pxc,**prs,**pxs,*s,*pr2c,b[2],**pr2s;
  i32 *pri,*pai,*pxi,i,j,*pr2i,m,p,q;
  double *prf,*pxf,f,*pr2f;
  if(s(a)) return KERR_TYPE;
  switch(ta) {
  case -1:
    PAI;
    switch(tx) {
    case -1: PXI;
      i(na,if(pai[i]<0)return KERR_DOMAIN; if((u64)pai[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(pai[i]<j)return KERR_DOMAIN;j=pai[i])
      PRK(na);
      i(na,p=pai[i]; j=i==na-1?(i32)nx:pai[i+1]; prk[i]=r2=tn(1,j-p); pr2i=px(r2); for(q=0,m=p;m<j;m++) pr2i[q++]=pxi[m])
      break;
    case -2: PXF;
      i(na,if(pai[i]<0)return KERR_DOMAIN; if((u64)pai[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(pai[i]<j)return KERR_DOMAIN;j=pai[i])
      PRK(na);
      i(na,p=pai[i]; j=i==na-1?(i32)nx:pai[i+1]; prk[i]=r2=tn(2,j-p); pr2f=px(r2); for(q=0,m=p;m<j;m++) pr2f[q++]=pxf[m])
      break;
    case -3: PXC;
      i(na,if(pai[i]<0)return KERR_DOMAIN; if((u64)pai[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(pai[i]<j)return KERR_DOMAIN;j=pai[i])
      PRK(na);
      i(na,p=pai[i]; j=i==na-1?(i32)nx:pai[i+1]; prk[i]=r2=tn(3,j-p); pr2c=px(r2); for(q=0,m=p;m<j;m++) pr2c[q++]=pxc[m];)
      break;
    case -4: PXS;
      i(na,if(pai[i]<0)return KERR_DOMAIN; if((u64)pai[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(pai[i]<j)return KERR_DOMAIN;j=pai[i])
      PRK(na);
      i(na,p=pai[i]; j=i==na-1?(i32)nx:pai[i+1]; prk[i]=r2=tn(4,j-p); pr2s=px(r2); for(q=0,m=p;m<j;m++) pr2s[q++]=pxs[m])
      break;
    case  0: PXK;
      i(na,if(pai[i]<0)return KERR_DOMAIN; if((u64)pai[i]>nx)return KERR_LENGTH)
      j=-1;
      i(na,if(pai[i]<j)return KERR_DOMAIN;j=pai[i])
      PRK(na);
      i(na,p=pai[i]; j=i==na-1?(i32)nx:pai[i+1]; prk[i]=r2=tn(0,j-p); pr2k=px(r2); for(q=0,m=p;m<j;m++) pr2k[q++]=k_(pxk[m]); r2=knorm(r2); prk[i]=r2)
      break;
    default: return KERR_TYPE;
    } break;
  case 1:
    if(ax||s(x)) { r=k_(x); break; }
    switch(tx) {
    case -1:
      if((u64)abs(ik(a))>=nx) r=tn(1,0);
      else if(ik(a)>0) { PRI(nx-ik(a)); PXI; i(nx-ik(a),*pri++=pxi[i+ik(a)]) }
      else if(ik(a)<0) { PRI(nx+ik(a)); PXI; i(nx+ik(a),*pri++=*pxi++) }
      else r=k_(x);
      break;
    case -2:
      if((u64)abs(ik(a))>=nx) r=tn(2,0);
      else if(ik(a)>0) { PRF(nx-ik(a)); PXF; i(nx-ik(a),*prf++=pxf[i+ik(a)]) }
      else if(ik(a)<0) { PRF(nx+ik(a)); PXF; i(nx+ik(a),*prf++=*pxf++) }
      else r=k_(x);
      break;
    case -3:
      if((u64)abs(ik(a))>=nx) r=tn(3,0);
      else if(ik(a)>0) { PRC(nx-ik(a)); PXC; i(nx-ik(a),*prc++=pxc[i+ik(a)]) }
      else if(ik(a)<0) { PRC(nx+ik(a)); PXC; i(nx+ik(a),*prc++=*pxc++) }
      else r=k_(x);
      break;
    case -4:
      if((u64)abs(ik(a))>=nx) r=tn(4,0);
      else if(ik(a)>0) { PRS(nx-ik(a)); PXS; i(nx-ik(a),*prs++=pxs[i+ik(a)]) }
      else if(ik(a)<0) { PRS(nx+ik(a)); PXS; i(nx+ik(a),*prs++=*pxs++) }
      else r=k_(x);
      break;
    case  0:
      if((u64)abs(ik(a))>=nx) r=tn(0,0);
      else if(ik(a)>0) { PRK(nx-ik(a)); PXK; i(nx-ik(a),*prk++=k_(pxk[i+ik(a)])) }
      else if(ik(a)<0) { PRK(nx+ik(a)); PXK; i(nx+ik(a),*prk++=k_(*pxk++)) }
      else r=k_(x);
      break;
    } break;
  case 2:
    f=fk(a);
    switch(tx) {
    case 1:
      if(f==0.0) r=t2(fi(ik(x)));
      else if(isnan(f)||isinf(f)) r=t2(NAN);
      else if(f<0.00) { r=kcp(a); EC(r); for(i=1;;i++) if(cmpff(-f*i,fi(ik(x)))>=0) { _k(r); r=t2(-f*i); break; } }
      else { r=kcp(a); EC(r); for(i=1;;i++) if(cmpff(f*i,fi(ik(x)))>0) { _k(r); r=t2(f*(i-1)); break; } }
      break;
    case  2:
      if(isnan(f)||isinf(f)) r=t2(NAN);
      else r=t2(rint(fk(x)));
      break;
    case -1:
      PRF(nx); PXI;
      if(f==0.0) i(nx,prf[i]=fi(pxi[i]))
      else if(isnan(f)||isinf(f)) i(nx,prf[i]=NAN)
      else if(f<0.00) { i(nx,for(j=1;;j++) if(cmpff(-f*j,fi(pxi[i]))>=0) { prf[i]=-f*j; break; }) }
      else { i(nx,for(j=1;;j++) if(cmpff(f*j,fi(pxi[i]))>0) { prf[i]=f*(j-1); break; }) }
      break;
    case -2:
      PRF(nx); PXF;
      if(f==0.0) i(nx,prf[i]=pxf[i])
      else if(isnan(f)||isinf(f)) i(nx,prf[i]=NAN)
      else if(f<0.00) { i(nx,for(j=1;;j++) if(cmpff(-f*j,pxf[i])>=0) { prf[i]=-f*j; break; }) }
      else { i(nx,for(j=1;;j++) if(cmpff(f*j,pxf[i])>0) { prf[i]=f*(j-1); break; }) }
      break;
    default: return KERR_TYPE;
    } break;
  case 3:
    switch(tx) {
    case -3: s=xstrndup((char*)px(x),nx); b[0]=ck(a); b[1]=0; r=ksplit(s,b); xfree(s); break;
    case  4: s=xstrdup(sk(x)); b[0]=ck(a); b[1]=0; r=ksplit(s,b); xfree(s); break;
    default: return KERR_TYPE;
    } break;
  default: return KERR_TYPE;
  }
  return knorm(r);
cleanup:
  _k(r);
  return e;
}

K power(K a, K x) {
  K r=0;
  i32 *pai,*pxi;
  double f,*prf,*paf,*pxf;
  if(s(x)||s(a)) return KERR_TYPE;
  if(ta <= 0 && tx <= 0 && na != nx) return KERR_LENGTH;
  switch(ta) {
  case 1:
    switch(tx) {
    case  1: f=pow(fi(ik(a)),fi(ik(x))); r=t2(f); break;
    case  2: f=pow(fi(ik(a)),fk(x)); r=t2(f); break;
    case -1: PRF(nx); PXI; i(nx,prf[i]=pow(fi(ik(a)),fi(pxi[i]))) break;
    case -2: PRF(nx); PXF; i(nx,prf[i]=pow(fi(ik(a)),pxf[i])) break;
    case  0: r=irecur2(power,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 2:
    switch(tx) {
    case  1: f=pow(fk(a),fi(ik(x))); r=t2(f); break;
    case  2: f=pow(fk(a),fk(x)); r=t2(f); break;
    case -1: PRF(nx); PXI; i(nx, prf[i]=pow(fk(a),fi(pxi[i]))) break;
    case -2: PRF(nx); PXF; i(nx, prf[i]=pow(fk(a),pxf[i])) break;
    case  0: r=irecur2(power,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -1:
    switch(tx) {
    case  1: PRF(na); PAI; i(na, prf[i]=pow(fi(pai[i]),fi(ik(x)))) break;
    case  2: PRF(na); PAI; i(na, prf[i]=pow(fi(pai[i]),fk(x))) break;
    case -1: PRF(na); PAI; PXI; i(na, prf[i]=pow(fi(pai[i]),fi(pxi[i]))) break;
    case -2: PRF(na); PAI; PXF; i(na, prf[i]=pow(fi(pai[i]),pxf[i])) break;
    case  0: r=each(17,a,x); break;
    default: return KERR_TYPE;
    } break;
  case -2:
    switch(tx) {
    case  1: PRF(na); PAF; i(na, prf[i]=pow(paf[i],fi(ik(x)))) break;
    case  2: PRF(na); PAF; i(na, prf[i]=pow(paf[i],fk(x))) break;
    case -1: PRF(na); PAF; PXI; i(na, prf[i]=pow(paf[i],fi(pxi[i]))) break;
    case -2: PRF(na); PAF; PXF; i(na, prf[i]=pow(paf[i],pxf[i])) break;
    case  0: r=each(17,a,x); break;
    default: return KERR_TYPE;
    } break;
  case  0:
    switch(tx) {
    case  1: r=irecur2(power,a,x); break;
    case  2: r=irecur2(power,a,x); break;
    case -1: r=each(17,a,x); break;
    case -2: r=each(17,a,x); break;
    case  0: r=irecur2(power,a,x); break;
    default: return KERR_TYPE;
    } break; \
  default: return KERR_TYPE;
  }
  return knorm(r);
}

K join(K a, K x) {
  K r=0,e,*prk,*pak,*pxk;
  char *prc,*pac,*pxc,**prs,**pas,**pxs,Ta,Tx;
  i32 *pri,*pai,*pxi;
  double *prf,*paf,*pxf;
  Ta=ta; if(s(a)) { if(!vstcb(a)) return KERR_TYPE; Ta=15; }
  Tx=tx; if(s(x)) { if(!vstcb(x)) return KERR_TYPE; Tx=15; }
  if(Ta<=0&&Tx<=0&&!na&&!nx&&Ta!=Tx) return tn(0,0);
  switch(Ta) {
  case 1:
    switch(Tx) {
    case  1: PRI(2); pri[0]=ik(a); pri[1]=ik(x); break;
    case  2: PRK(2); prk[0]=a; prk[1]=k_(x); break;
    case  3: PRK(2); prk[0]=a; prk[1]=x; break;
    case  4: PRK(2); prk[0]=a; prk[1]=x; break;
    case  6: PRK(2); prk[0]=a; prk[1]=x; break;
    case 15: PRK(2); prk[0]=a; prk[1]=kcp(x); EC(prk[1]); break;
    case -3: PRK(1+nx); PXC; *prk++=a; i(nx,*prk++=t(3,(u8)*pxc++)) break;
    case -1: PRI(1+nx); PXI; *pri++=ik(a); i(nx,*pri++=*pxi++) break;
    case -2: PRK(1+nx); PXF; *prk++=a; i(nx,*prk++=t2(*pxf++)) break;
    case -4: PRK(1+nx); PXS; *prk++=a; i(nx,*prk++=t(4,*pxs++)) break;
    case  0: PRK(1+nx); PXK; *prk++=a; i(nx,*prk++=k_(*pxk++)) break;
    default: return KERR_TYPE;
    } break;
  case 2:
    switch(Tx) {
    case  1: PRK(2); prk[0]=k_(a); prk[1]=x; break;
    case  2: PRF(2); prf[0]=fk(a); prf[1]=fk(x); break;
    case  3: PRK(2); prk[0]=k_(a); prk[1]=x; break;
    case  4: PRK(2); prk[0]=k_(a); prk[1]=x; break;
    case  6: PRK(2); prk[0]=k_(a); prk[1]=x; break;
    case 15: PRK(2); prk[0]=k_(a); prk[1]=kcp(x); EC(prk[1]); break;
    case -1: PRK(1+nx); PXI; *prk++=k_(a); i(nx,*prk++=t(1,(u32)*pxi++)) break;
    case -2: PRF(1+nx); PXF; *prf++=fk(a); i(nx,*prf++=*pxf++) break;
    case -3: PRK(1+nx); PXC; *prk++=k_(a); i(nx,*prk++=t(3,(u8)*pxc++)) break;
    case -4: PRK(1+nx); PXS; *prk++=k_(a); i(nx,*prk++=t(4,*pxs++)) break;
    case  0: PRK(1+nx); PXK; *prk++=k_(a); i(nx,*prk++=k_(*pxk++)) break;
    default: return KERR_TYPE;
    } break;
  case 3:
    switch(Tx) {
    case  1: PRK(2); prk[0]=a; prk[1]=x; break;
    case  2: PRK(2); prk[0]=a; prk[1]=k_(x); break;
    case  3: PRC(2); prc[0]=ck(a); prc[1]=ck(x); break;
    case  4: PRK(2); prk[0]=a; prk[1]=x; break;
    case  6: PRK(2); prk[0]=a; prk[1]=x; break;
    case 15: PRK(2); prk[0]=a; prk[1]=kcp(x); EC(prk[1]); break;
    case -1: PRK(1+nx); PXI; *prk++=a; i(nx,*prk++=t(1,(u32)*pxi++)) break;
    case -2: PRK(1+nx); PXF; *prk++=a; i(nx,*prk++=t2(*pxf++)) break;
    case -3: PRC(1+nx); PXC; *prc++=ck(a); i(nx,*prc++=*pxc++) break;
    case -4: PRK(1+nx); PXS; *prk++=a; i(nx,*prk++=t(4,*pxs++)) break;
    case  0: PRK(1+nx); PXK; *prk++=a; i(nx,*prk++=k_(*pxk++)) break;
    default: return KERR_TYPE;
    } break;
  case 4:
    switch(Tx) {
    case  1: PRK(2); prk[0]=a; prk[1]=x; break;
    case  2: PRK(2); prk[0]=a; prk[1]=k_(x); break;
    case  3: PRK(2); prk[0]=a; prk[1]=x; break;
    case  4: PRS(2); prs[0]=sk(a); prs[1]=sk(x); break;
    case  6: PRK(2); prk[0]=a; prk[1]=x; break;
    case 15: PRK(2); prk[0]=a; prk[1]=kcp(x); EC(prk[1]); break;
    case -1: PRK(1+nx); PXI; *prk++=a; i(nx,*prk++=t(1,(u32)*pxi++)) break;
    case -2: PRK(1+nx); PXF; *prk++=a; i(nx,*prk++=t2(*pxf++)) break;
    case -3: PRK(1+nx); PXC; *prk++=a; i(nx,*prk++=t(3,(u8)*pxc++)) break;
    case -4: PRS(1+nx); PXS; *prs++=sk(a); i(nx,*prs++=*pxs++) break;
    case  0: PRK(1+nx); PXK; *prk++=a; i(nx,*prk++=k_(*pxk++)) break;
    default: return KERR_TYPE;
    } break;
  case 6:
    switch(Tx) {
    case  1: PRK(2); prk[0]=a; prk[1]=x; break;
    case  2: PRK(2); prk[0]=a; prk[1]=k_(x); break;
    case  3: PRK(2); prk[0]=a; prk[1]=x; break;
    case  4: PRK(2); prk[0]=a; prk[1]=x; break;
    case  6: PRK(2); prk[0]=a; prk[1]=x; break;
    case 15: PRK(2); prk[0]=a; prk[1]=kcp(x); EC(prk[1]); break;
    case -1: PRK(1+nx); PXI; *prk++=a; i(nx,*prk++=t(1,(u32)*pxi++)) break;
    case -2: PRK(1+nx); PXF; *prk++=a; i(nx,*prk++=t2(*pxf++)) break;
    case -3: PRK(1+nx); PXC; *prk++=a; i(nx,*prk++=t(3,(u8)*pxc++)) break;
    case -4: PRK(1+nx); PXS; *prk++=a; i(nx,*prk++=t(4,*pxs++)) break;
    case  0: PRK(1+nx); PXK; *prk++=a; i(nx,*prk++=k_(*pxk++)) break;
    default: return KERR_TYPE;
    } break;
  case 15:
    switch(Tx) {
    case 1: case 2: case 3: case 4: case 6:
      PRK(2); prk[0]=kcp(a); EC(prk[0]); prk[1]=k_(x); break;
    case 15: PRK(2); prk[0]=kcp(a); EC(prk[0]); prk[1]=kcp(x); EC(prk[1]); break;
    case -1: PRK(1+nx); PXI; prk[0]=kcp(a); EC(prk[0]); ++prk; i(nx,*prk++=t(1,(u32)*pxi++)) break;
    case -2: PRK(1+nx); PXF; prk[0]=kcp(a); EC(prk[0]); ++prk; i(nx,*prk++=t2(*pxf++)) break;
    case -3: PRK(1+nx); PXC; prk[0]=kcp(a); EC(prk[0]); ++prk; i(nx,*prk++=t(3,(u8)*pxc++)) break;
    case -4: PRK(1+nx); PXS; prk[0]=kcp(a); EC(prk[0]); ++prk; i(nx,*prk++=t(4,*pxs++)) break;
    case  0: PRK(1+nx); PXK; prk[0]=kcp(a); EC(prk[0]); ++prk; i(nx,*prk++=k_(*pxk++)) break;
    } break;
  case -3:
    switch(Tx) {
    case  1: PRK(na+1); PAC; i(na,*prk++=t(3,(u8)*pac++)) *prk=x; break;
    case  2: PRK(na+1); PAC; i(na,*prk++=t(3,(u8)*pac++)) *prk=k_(x); break;
    case  3: PRC(na+1); PAC; i(na,*prc++=*pac++) *prc=ck(x); break;
    case  4: PRK(na+1); PAC; i(na,*prk++=t(3,(u8)*pac++)) *prk=x; break;
    case  6: PRK(na+1); PAC; i(na,*prk++=t(3,(u8)*pac++)) *prk=x; break;
    case 15: PRK(na+1); PAC; i(na,*prk++=t(3,(u8)*pac++)) *prk=kcp(x); EC(*prk); break;
    case -1: PRK(na+nx); PAC; PXI; i(na,*prk++=t(3,(u8)*pac++)) i(nx,*prk++=t(1,(u32)*pxi++)) break;
    case -2: PRK(na+nx); PAC; PXF; i(na,*prk++=t(3,(u8)*pac++)) i(nx,*prk++=t2(*pxf++)) break;
    case -3: PRC(na+nx); PAC; PXC; i(na,*prc++=*pac++) i(nx,*prc++=*pxc++) break;
    case -4: PRK(na+nx); PAC; PXS; i(na,*prk++=t(3,(u8)*pac++)) i(nx,*prk++=t(4,*pxs++)) break;
    case  0: PRK(na+nx); PAC; PXK; i(na,*prk++=t(3,(u8)*pac++)) i(nx,*prk++=k_(*pxk++)) break;
    default: return KERR_TYPE;
    } break;
  case -1:
    switch(Tx) {
    case  1: PRI(na+1); PAI; i(na,*pri++=*pai++) *pri=ik(x); break;
    case  2: PRK(na+1); PAI; i(na,*prk++=t(1,(u32)*pai++)) *prk=k_(x); break;
    case  3: PRK(na+1); PAI; i(na,*prk++=t(1,(u32)*pai++)) *prk=x; break;
    case  4: PRK(na+1); PAI; i(na,*prk++=t(1,(u32)*pai++)) *prk=x; break;
    case  6: PRK(na+1); PAI; i(na,*prk++=t(1,(u32)*pai++)) *prk=x; break;
    case 15: PRK(na+1); PAI; i(na,*prk++=t(1,(u32)*pai++)) *prk=kcp(x); EC(*prk); break;
    case -1: PRI(na+nx); PAI; PXI; i(na,*pri++=*pai++) i(nx,*pri++=*pxi++) break;
    case -2: PRK(na+nx); PAI; PXF; i(na,*prk++=t(1,(u32)*pai++)) i(nx,*prk++=t2(*pxf++)) break;
    case -3: PRK(na+nx); PAI; PXC; i(na,*prk++=t(1,(u32)*pai++)) i(nx,*prk++=t(3,(u8)*pxc++)) break;
    case -4: PRK(na+nx); PAI; PXS; i(na,*prk++=t(1,(u32)*pai++)) i(nx,*prk++=t(4,*pxs++)) break;
    case  0: PRK(na+nx); PAI; PXK; i(na,*prk++=t(1,(u32)*pai++)) i(nx,*prk++=k_(*pxk++)) break;
    default: return KERR_TYPE;
    } break;
  case -2:
    switch(Tx) {
    case  1: PRK(na+1); PAF; i(na,*prk++=t2(*paf++)) *prk=x; break;
    case  2: PRF(na+1); PAF; i(na,*prf++=*paf++) *prf=fk(x); break;
    case  3: PRK(na+1); PAF; i(na,*prk++=t2(*paf++)) *prk=x; break;
    case  4: PRK(na+1); PAF; i(na,*prk++=t2(*paf++)) *prk=x; break;
    case  6: PRK(na+1); PAF; i(na,*prk++=t2(*paf++)) *prk=x; break;
    case 15: PRK(na+1); PAF; i(na,*prk++=t2(*paf++)) *prk=kcp(x); EC(*prk); break;
    case -1: PRK(na+nx); PAF; PXI; i(na,*prk++=t2(*paf++)) i(nx,*prk++=t(1,(u32)*pxi++)) break;
    case -2: PRF(na+nx); PAF; PXF; i(na,*prf++=*paf++) i(nx,*prf++=*pxf++) break;
    case -3: PRK(na+nx); PAF; PXC; i(na,*prk++=t2(*paf++)) i(nx,*prk++=t(3,(u8)*pxc++)) break;
    case -4: PRK(na+nx); PAF; PXS; i(na,*prk++=t2(*paf++)) i(nx,*prk++=t(4,*pxs++)) break;
    case  0: PRK(na+nx); PAF; PXK; i(na,*prk++=t2(*paf++)) i(nx,*prk++=k_(*pxk++)) break;
    default: return KERR_TYPE;
    } break;
  case -4:
    switch(Tx) {
    case  1: PRK(na+1); PAS; i(na,*prk++=t(4,*pas++)) *prk=x; break;
    case  2: PRK(na+1); PAS; i(na,*prk++=t(4,*pas++)) *prk=k_(x); break;
    case  3: PRK(na+1); PAS; i(na,*prk++=t(4,*pas++)) *prk=x; break;
    case  4: PRS(na+1); PAS; i(na,*prs++=*pas++) *prs=sk(x); break;
    case  6: PRK(na+1); PAS; i(na,*prk++=t(4,*pas++)) *prk=x; break;
    case 15: PRK(na+1); PAS; i(na,*prk++=t(4,*pas++)) *prk=kcp(x); EC(*prk); break;
    case -1: PRK(na+nx); PAS; PXI; i(na,*prk++=t(4,*pas++)) i(nx,*prk++=t(1,(u32)*pxi++)) break;
    case -2: PRK(na+nx); PAS; PXF; i(na,*prk++=t(4,*pas++)) i(nx,*prk++=t2(*pxf++)) break;
    case -3: PRK(na+nx); PAS; PXC; i(na,*prk++=t(4,*pas++)) i(nx,*prk++=t(3,(u8)*pxc++)) break;
    case -4: PRS(na+nx); PAS; PXS; i(na,*prs++=*pas++) i(nx,*prs++=*pxs++) break;
    case  0: PRK(na+nx); PAS; PXK; i(na,*prk++=t(4,*pas++)) i(nx,*prk++=k_(*pxk++)) break;
    default: return KERR_TYPE;
    } break;
  case 0:
    switch(Tx) {
    case  1: PRK(na+1); PAK; i(na,*prk++=k_(*pak++)) *prk=x; break;
    case  2: PRK(na+1); PAK; i(na,*prk++=k_(*pak++)) *prk=k_(x); break;
    case  3: PRK(na+1); PAK; i(na,*prk++=k_(*pak++)) *prk=x; break;
    case  4: PRK(na+1); PAK; i(na,*prk++=k_(*pak++)) *prk=x; break;
    case  6: PRK(na+1); PAK; i(na,*prk++=k_(*pak++)) *prk=x; break;
    case 15: PRK(na+1); PAK; i(na,*prk++=k_(*pak++)) *prk=kcp(x); EC(*prk); break;
    case -1: PRK(na+nx); PAK; PXI; i(na,*prk++=k_(*pak++)) i(nx,*prk++=t(1,(u32)*pxi++)) break;
    case -2: PRK(na+nx); PAK; PXF; i(na,*prk++=k_(*pak++)) i(nx,*prk++=t2(*pxf++)) break;
    case -3: PRK(na+nx); PAK; PXC; i(na,*prk++=k_(*pak++)) i(nx,*prk++=t(3,(u8)*pxc++)) break;
    case -4: PRK(na+nx); PAK; PXS; i(na,*prk++=k_(*pak++)) i(nx,*prk++=t(4,*pxs++)) break;
    case  0: PRK(na+nx); PAK; PXK; i(na,*prk++=k_(*pak++)) i(nx,*prk++=k_(*pxk++)) break;
    default: return KERR_TYPE;
    } break;
  default: return KERR_TYPE;
  }
  return knorm(r);
cleanup:
  _k(r);
  return e;
}

static K form2w(char *t, i32 w, i32 z) {
  K r=0;
  i32 i,n,l,m;
  char *prc;
  l=strlen(t); m=abs(w); n=abs(l-m);
  if(m>l) {
    PRC(m);
    if(w<0) { for(i=0;i<l;i++) prc[i]=*t++; for(;i<m;i++) prc[i]=' '; }
    else if(w>0) { for(i=0;i<n;i++) prc[i]=' '; for(;i<m;i++) prc[i]=*t++; }
  }
  else if(m<l) {
    PRC(m);
    if(z) { i(m,*prc++='*') *prc=0; }
    else { if(w<0) { i(m,prc[i]=t[i]) } else { i(m,prc[i]=*(t+l-m+i)) } }
  }
  else { PRC(l); i(m,prc[i]=t[i]) }
  return r;
}
K form(K a, K x) {
  K r=0,e,*prk;
  i32 l,m,xx,y,*pai;
  char t[2048],*s=t,*prc,*pxc,*p;
  double f;
  if(s(a)||s(x)) return formcb(a,x);
  if(ta<=0 && tx<=0 && na!=nx && ta!=-3 && tx!=-3) return KERR_LENGTH;
  switch(ta) {
  case 1:
    if(ik(a)==INT32_MAX||ik(a)==INT32_MIN||ik(a)==INT32_MIN+1) return KERR_DOMAIN;
    switch(tx) {
    case  1: sprintf(t,"%d",ik(x)); r=form2w(t,ik(a),1); break;
    case  2: sprintf(t,"%0.*g",7,fk(x)); r=form2w(t,ik(a),1); break;
    case  3:
      if(!ik(a)) { if(ik(x)<48||ik(x)>57) return KERR_DOMAIN; r=t(1,(u32)ik(x)-48); break; }
      sprintf(t,"%c",ik(x));
      r=form2w(t,ik(a),0);
      break;
    case  4:
      p=xstrdup(sk(x));
      if(strlen(p)>(K)abs(ik(a))) { PRC(abs(ik(a))); i(n(r),prc[i]='*'); }
      else r=form2w(p,ik(a),0);
      xfree(p);
      break;
    case -1:
    case -2: r=each(19,a,x); break;
    case  0: r=irecur2(form,a,x); break;
    case -3:
      PXC; pxc[nx]=0;
      if(!ik(a)) {
        i(nx,if(pxc[i]<48||pxc[i]>57) return KERR_DOMAIN;)
        r=t(1,(u32)xatoi(pxc));
        break;
      }
      r=form2w(pxc,ik(a),0);
      break;
    case -4: r=each(19,a,x); break;
    default: return KERR_TYPE;
    } break;
  case 2:
    switch(tx) {
    case  1:
    case  2:
      if(isnan(fk(a))) return KERR_DOMAIN;
      f=tx==1?fi(ik(x)):fk(x);
      sprintf(t,"%g",round(fk(a)*10)/10); s=strchr(t,'.');
      y=s?s[1]-48:0;
      xx=fk(a);
      VSIZE(abs(xx));
      m=abs(xx);
      if(xx<0||*t=='-') sprintf(t,"%s%0.*e",f<0?"":" ",y,f);
      else if(isinf(f)) *t=0;
      else if(isnan(f)) *t=0;
      else sprintf(t,"%0.*f",y,f);
      l=strlen(t);
      if(l>INT32_MAX-m-1) return KERR_WSFULL;
      s=calloc(1,l+m+1);
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
    case -2: r=each(19,a,x); break;
    case -3: r=t2(xstrtod((char*)px(x))); break;
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
    if(strlen(sk(a))) return KERR_NONCE;
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
    case  1: case  2: case  3: case -3: case 4:
      PAI; i(na,if(pai[i]==INT32_MAX||pai[i]==INT32_MIN||pai[i]==INT32_MIN+1) return KERR_DOMAIN);
      PRK(na);
      i(na,prk[i]=ki(19,a,x,i,-1); EC(prk[i]));
      break;
    case -1: case -2: case -4:
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
  i32 m=-1;
  u32 i;
  ko *pk;
  if(ax||s(x)) return k_(x);
  switch(tx) {
  case 0:
    PXK;
    i(nx, a=pxk[i]; if(ta<=0&&!s(a)) { if(m==-1)m=na; else if((i32)na!=m)return KERR_LENGTH; } )
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
  case -1: case -2: case -3: case -4: r=k_(x); break;
  default: return KERR_TYPE;
  }
  return knorm(r);
}

K negate(K x) {
  K r=0;
  i32 *pri,*pxi;
  double f,*prf,*pxf;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case  1: r=t(1,(u32)(-ik(x))); break;
  case  2: f=-fk(x); r=t2(f); break;
  case -1: PRI(nx); PXI; i(nx,*pri++=-*pxi++) break;
  case -2: PRF(nx); PXF; i(nx,*prf++=-*pxf++) break;
  case  0: r=irecur1(negate,x); break;
  default: return KERR_TYPE;
  }
  return r;
}

K first(K x) {
  K r=0,*pxk;
  char *pxc,**pxs;
  i32 *pxi;
  double *pxf;
  if(ax||s(x)) return k_(x);
  switch(tx) {
  case -1: PXI; r=nx?t(1,(u32)*pxi):t(1,0); break;
  case -2: PXF; r=nx?t2(*pxf):t2(0); break;
  case -3: PXC; r=nx?t(3,(u8)*pxc):t(3,' '); break;
  case -4: PXS; r=nx?t(4,*pxs):t(4,""); break;
  case  0: PXK; r=nx?k_(*pxk):null; break;
  }
  return r;
}

K recip(K x) {
  K r=0;
  double f;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case  1: f=1.0/fi(ik(x)); r=t2(f); break;
  case  2: f=1.0/fk(x); r=t2(f); break;
  case -1:
  case -2: r=each(4,0,x); break;
  case  0: r=irecur1(recip,x); break;
  default: return KERR_TYPE;
  }
  return knorm(r);
}

K where(K x) {
  K r=0;
  i32 j=0,kk,*pri,*pxi,c;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case  1:
    c=ik(x);
    if(c<0 || c==INT32_MAX || c==INT32_MIN || c==INT32_MIN+1) return KERR_DOMAIN;
    PRI(c); i(c,pri[i]=0)
    break;
  case  0: if(!nx) r=tn(1,0); else return KERR_TYPE; break;
  case -1:
    PXI; i(nx,if(pxi[i]<0 || pxi[i]==INT32_MAX || pxi[i]==INT32_MIN || pxi[i]==INT32_MIN+1) return KERR_DOMAIN)
    i(nx,if((pxi[i]>0&&j>INT32_MAX-pxi[i])||(pxi[i]<0&&j<INT32_MIN-pxi[i])) return KERR_DOMAIN; else j+=pxi[i]);
    PRI(j);
    j=0; i(nx,kk=pxi[i]; while(kk-->0)pri[j++]=i) break;
  default: return KERR_TYPE;
  }
  return knorm(r);
}

K reverse(K x) {
  K r=0,*prk,*pxk;
  char *prc,*pxc,**prs,**pxs;
  i32 *pri,*pxi;
  double *prf,*pxf;
  if(ax||s(x)) return k_(x);
  switch(tx) {
  case -1: PRI(nx); PXI; i(nx,*pri++=pxi[nx-i-1]) break;
  case -2: PRF(nx); PXF; i(nx,*prf++=pxf[nx-i-1]) break;
  case -3: PRC(nx); PXC; i(nx,*prc++=pxc[nx-i-1]) break;
  case -4: PRS(nx); PXS; i(nx,*prs++=pxs[nx-i-1]) break;
  case  0: PRK(nx); PXK; i(nx,*prk++=k_(pxk[nx-i-1])) break;
  }
  return r;
}

K upgrade(K x) {
  K r=0,*pxk;
  char *pxc,**pxs;
  i32 *pri,*pxi;
  double *pxf;
  if(ax||s(x)) return KERR_RANK;
  if(nx==0) return tn(1,0);
  switch(tx) {
  case -1: r=enumerate(t(1,nx)); pri=(i32*)px(r); PXI; rcsortg(pri,pxi,nx,0); break;
  case -2: r=enumerate(t(1,nx)); pri=(i32*)px(r); PXF; msortg2(pri,pxf,0,nx-1,0); break;
  case -3: r=enumerate(t(1,nx)); pri=(i32*)px(r); PXC; msortg3(pri,pxc,0,nx-1,0); break;
  case -4: r=enumerate(t(1,nx)); pri=(i32*)px(r); PXS; msortg4(pri,pxs,0,nx-1,0); break;
  case  0: r=enumerate(t(1,nx)); pri=(i32*)px(r); PXK; msortg0(pri,pxk,0,nx-1,0); break;
  default: return KERR_TYPE;
  }
  return r;
}

K downgrade(K x) {
  K r=0,*pxk;
  char *pxc,**pxs;
  i32 *pri,*pxi;
  double *pxf;
  if(ax||s(x)) return KERR_RANK;
  if(nx==0) return tn(1,0);
  switch(tx) {
  case -1: r=enumerate(t(1,nx)); pri=(i32*)px(r); PXI; rcsortg(pri,pxi,nx,1); break;
  case -2: r=enumerate(t(1,nx)); pri=(i32*)px(r); PXF; msortg2(pri,pxf,0,nx-1,1); break;
  case -3: r=enumerate(t(1,nx)); pri=(i32*)px(r); PXC; msortg3(pri,pxc,0,nx-1,1); break;
  case -4: r=enumerate(t(1,nx)); pri=(i32*)px(r); PXS; msortg4(pri,pxs,0,nx-1,1); break;
  case  0: r=enumerate(t(1,nx)); pri=(i32*)px(r); PXK; msortg0(pri,pxk,0,nx-1,1); break;
  default: return KERR_TYPE;
  }
  return r;
}

K group(K x) {
  K r=0,p=0,*ht,*pk,*hk,*prk,*pxk;
  i32 *n,min=INT32_MAX,max=INT32_MIN,*hi,bni=0,*pp,*pxi;
  u32 i;
  u64 m,w,h,rs=256,ri=0,*hm,q;
  char **s,**hs,*pxc,**pxs;
  u8 *c;
  double *f,*hf,*pxf;
  if(s(x)) return KERR_RANK;
  if(tx<=0&&nx==0) return tn(0,0);
  switch(tx) {
  case  0:
    PRK(rs); nr=0; PXK;
    m=nx;
    w=1; while(w<=m) w<<=1; q=w-1;
    ht=calloc(w,sizeof(K));
    hm=calloc(w,sizeof(u64));
    hk=calloc(w,sizeof(K));
    pk=pxk-1;
    for(i=0;i<nx;i++) {
      pk++;
      h=khash(*pk)&q;
      if(*pk) while(!h || (hk[h] && kcmprz(hk[h],*pk,0))) h=(h+1)&q; /* h=0 iff *s=0 */
      hk[h]=*pk;
      hm[h]++;
    }
    pk=pxk-1;
    for(i=0;i<nx;i++) {
      pk++;
      h=khash(*pk)&q;
      if(*pk) while(!h || (hk[h] && kcmprz(hk[h],*pk,0))) h=(h+1)&q; /* h=0 iff *s=0 */
      p=ht[h];
      if(!p) {
        p=tn(1,hm[h]);
        ht[h]=p;
        np=0;
        if(rs==ri++){rs<<=1;vr=prk=realloc(prk,sizeof(K)*rs);}
        prk[nr++]=p;
      }
      pp=(i32*)px(p);
      pp[np++]=i;
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
    if(min>=0 && !bni && max<1000000000) { /* optimize for all positive n */
      ht=calloc((u32)(max+1),sizeof(K));
      hm=calloc((u32)(max+1),sizeof(u64));
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
           np=1;
           if(rs==ri++){rs<<=1;vr=prk=realloc(prk,sizeof(K)*rs);}
           prk[nr++]=p;
         }
         else { pp=(i32*)px(p); pp[np++]=i; }
      }
      xfree(hm);
      xfree(ht);
    }
    else {
      m=max-min+1;
      if(!m) m=2; /* handle ?0I 0N */
      if(nx<m) m=nx;
      w=1; while(w<=m) w<<=1; q=w-1;
      ht=calloc(w,sizeof(K));       /* groups */
      hm=calloc(w,sizeof(u64)); /* max's */
      hi=calloc(w,sizeof(i32));
      n=pxi;n--;
      for(i=0;i<nx;i++) {
         n++;
         h=(*n*2654435761)&q;
         if(*n) while(!h || (hi[h] && hi[h]!=*n)) h=(h+1)&q; /* h=0 iff *n=0 */
         hi[h]=*n;
         hm[h]++;
      }
      n=pxi;n--;
      for(i=0;i<nx;i++) {
         n++;
         h=(*n*2654435761)&q;
         if(*n) while(!h || (hi[h] && hi[h]!=*n)) h=(h+1)&q;
         p=ht[h];
         if(!p){
           p=tn(1,hm[h]);
           ht[h]=p;
           np=0;
           if(rs==ri++){rs<<=1;vr=prk=realloc(prk,sizeof(K)*rs);}
           prk[nr++]=p;
         }
         pp=(i32*)px(p);
         pp[np++]=i;
      }
      xfree(ht); xfree(hm); xfree(hi);
    }
    break;
  case -2:
    PRK(rs); nr=0; PXF;
    m=nx;
    w=1; while(w<=m) w<<=1; q=w-1;
    ht=calloc(w,sizeof(K));       /* groups */
    hm=calloc(w,sizeof(u64)); /* max's */
    hf=calloc(w,sizeof(double));
    f=pxf;f--;
    for(i=0;i<nx;i++) {
       f++;
       h=((u64)*f*2654435761)&q;
       if(*f) while(!h || (hf[h] && cmpff(hf[h],*f))) h=(h+1)&q; /* h=0 iff *f=0 */
       hf[h]=*f;
       hm[h]++;
    }
    f=pxf;f--;
    for(i=0;i<nx;i++) {
       f++;
       h=((u64)*f*2654435761)&q;
       if(*f) while(!h || (hf[h] && cmpff(hf[h],*f))) h=(h+1)&q; /* h=0 iff *f=0 */
       p=ht[h];
       if(!p){
         p=tn(1,hm[h]);
         ht[h]=p;
         np=0;
         if(rs==ri++){rs<<=1;vr=prk=realloc(prk,sizeof(K)*rs);}
         prk[nr++]=p;
       }
       pp=(i32*)px(p);
       pp[np++]=i;
    }
    xfree(ht); xfree(hm); xfree(hf);
    break;
  case -3:
    PRK(256); nr=0; PXC;
    ht=calloc(256,sizeof(K));
    hm=calloc(256,sizeof(u64));
    c=(u8*)pxc-1;
    i(nx, c++; hm[*c]++)
    c=(u8*)pxc-1;
    for(i=0;i<nx;i++) {
       c++;
       p=ht[*c];
       if(!p){ p=tn(1,hm[*c]); ht[*c]=p; pp=(i32*)px(p); pp[0]=i; np=1; prk[nr++]=p; }
       else { pp=(i32*)px(p); pp[np++]=i; }
    }
    xfree(hm); xfree(ht);
    break;
  case -4:
    PRK(rs); nr=0; PXS;
    m=nx;
    w=1; while(w<=m) w<<=1; q=w-1;
    ht=calloc(w,sizeof(K));
    hm=calloc(w,sizeof(u64));
    hs=calloc(w,sizeof(char*));
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
        np=0;
        if(rs==ri++){rs<<=1;vr=prk=realloc(prk,sizeof(K)*rs);}
        prk[nr++]=p;
      }
      pp=(i32*)px(p);
      pp[np++]=i;
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
  double *pxf;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case  1: r=t(1,ik(x)==0); break;
  case  2: r=t(1,fk(x)==0.0); break;
  case -1: PRI(nx); PXI; i(nx,*pri++=t(1,*pxi++==0)) break;
  case -2: PRI(nx); PXF; i(nx,*pri++=t(1,*pxf++==0)) break;
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
  if(h==INVALID_HANDLE_VALUE) return 4;
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
  if(!f) return 4;
  q=tn(0,32); pqk=px(q);
  while((e=readdir(f))) {
    if(!strcmp(e->d_name,".")||!strcmp(e->d_name,"..")) continue;
    ++n;
    if(n==n(q)) { n(q)<<=1; q=kresize(q,n(q)); pqk=px(q); }
    pqk[n-1]=tn(3,1+strlen(e->d_name));
    strcpy(px(pqk[n-1]),e->d_name);
    n(pqk[n-1])--;
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
  if(s(x)) return enumeratecb(x);
  switch(tx) {
  case  1: if(ik(x)<0||ik(x)==INT32_MAX) return 6; PRI(ik(x)); i(ik(x),pri[i]=i); break;
  case  2: return 9; /* int */
  case  3: p[0]=ck(x); p[1]=0; r=lsdir(p); break;
  case  4: return enumeratecb(x);
  case  6: r=tn(4,0); break;
  case -3: p=xmalloc(1+nx); memcpy(p,px(x),nx); p[nx]=0; r=lsdir(p); xfree(p); break;
  default: return 3;
  }
  _k(q);
  return knorm(r);
}

K atom(K x) {
  return t(1,tx>0||s(x));
}

K unique(K x) {
  K r=0,*hk,*prk,*pxk;
  i32 *hi,*pri,*pxi;
  char hc[256]={0},**hs,*prc,*pxc,**prs,**pxs;
  double *hf,*prf,*pxf;
  u64 m,w,h,q;
  u32 i;
  i32 j=0,z=0,t=0,min=INT32_MAX,max=INT32_MIN,bni=0;
  if(ax||s(x)) return KERR_RANK;
  switch(tx) {
  case  0:
    if(!nx) return k_(x);
    PRK(nx); PXK;
    w=1; while(w<=nx) w<<=1; q=w-1;
    hk=calloc(w,sizeof(K));
    pxk--;
    for(i=0;i<nx;i++) {
      pxk++;
      h=khash(*pxk)&q;
      while(hk[h] && kcmprz(hk[h],*pxk,0)) h=(h+1)&q;
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
    if(min>=0 && !bni) { /* optimize for all positive n */
      hi=calloc((u32)(max+1),sizeof(i32));
      pxi--;
      i(nx, pxi++; if(!hi[*pxi]) { hi[*pxi]=1; pri[j++]=*pxi; if(++t==max+1) break; })
      nr=j;
      xfree(hi);
    }
    else {
      m=max-min+1;
      m+=3; /* handle 0I 0N -0I */
      if(nx<m) m=nx;
      w=1; while(w<=m) w<<=1; q=w-1;
      hi=calloc(w,sizeof(i32));
      pxi--;
      for(i=0,j=0;i<nx;i++) {
        pxi++;
        if(!*pxi) { if(!z) { pri[j++]=0; z=1; } continue; }
        h=(*pxi*2654435761)&q;
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
    hf=calloc(w,sizeof(double));
    pxf--;
    for(i=0;i<nx;i++) {
      pxf++;
      if(!*pxf) { if(!z) { prf[j++]=0;z=1; } continue; }
      h=((u32)*pxf*2654435761)&q;
      while(hf[h]!=0 && cmpff(hf[h],*pxf)) h=(h+1)&q;
      if(cmpff(hf[h],*pxf)) hf[h]=prf[j++]=*pxf;
    }
    nr=j;
    xfree(hf);
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
    hs=calloc(w,sizeof(char*));
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
  K r=ax||s(x)?t(1,1):t(1,nx);
  return r;
}

K floor__(K x) {
  K r=0;
  i32 *pri;
  double *pxf;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case  1: r=x; break;
  case  2: r=t(1,(u32)((i32)floor(fk(x)))); break;
  case -1: r=k_(x); break;
  case -2: PRI(nx); PXF; i(nx,*pri++=floor(*pxf++)) break;
  case  0: r=irecur1(floor__,x); break;
  default: return KERR_TYPE;
  }
  return r;
}

K shape(K x) {
  K r=0,a=0,*q=0,*t=0,*pak,*prk;
  u32 i,cm=32,*c,ci,qc,tc;
  i32 *pri;
  if(ax||s(x)) return tn(1,0);
  c=xmalloc(sizeof(u32)*cm);
  switch(tx) {
  case -1: case -2: case -3: case -4: PRI(1); pri[0]=nx; break;
  case  0:
    q=xmalloc(sizeof(K));
    q[0]=x;
    qc=1;
    ci=0;
    while(q) { /* breadth first traversal */
      if(ci==cm) { cm<<=1; c=realloc(c,sizeof(u32)*cm); }
      c[ci]=tc=0; t=0;
      for(i=0;i<qc;i++) {
        a=q[i];
        if(ta>0||s(a)) { ci--; break; }
        if(!c[ci]&&na) c[ci]=na;
        else if(c[ci]!=na) { if(t){xfree(t);t=0;}; ci--; break; }
        if(!ta&&!s(a)&&na) { /* enqueue next round */
          if(t) t=realloc(t,sizeof(K)*(na+tc));
          else t=xmalloc(sizeof(K)*na);
          PAK;
          j(na,t[tc++]=pak[j])
        }
      }
      xfree(q);
      q=t; qc=tc; /* next queue */
      ci++;
    }
    PRK(ci);
    i(ci,prk[i]=t(1,(u32)c[i]))
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
  case  3: PRC(1); *prc=ck(x); break;
  case  4: PRS(1); *prs=sk(x); break;
  case  6: PRK(1); *prk=x; break;
  case -1: case -2: case -3: case -4: case 0: PRK(1); *prk=k_(x); break;
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
      else sprintf(ds,"%d",ik(x));
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
    case  3: PRC(1); prc[0]=ck(x); break;
    case  4: s=sk(x); PRC(strlen(s)); memcpy(prc,s,1+strlen(s)); break;
    case  6: PRC(0); break;
    case  0: r=irecur1(format,x); break;
    case -1: case -2: case -4: r=knorm(each(19,0,x)); break;
    case -3: r=k_(x); break;
    default: return KERR_TYPE;
  }
  return knorm(r);
}
