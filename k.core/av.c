#include "av.h"
#include <string.h>

extern i32 vstcb(K x);

// P=":+-*%&|<>=~.!@?#_^,$'/\\"
K each(i32 f, K a, K x) {
  K r=0,*prk,e;
  char Ta,Tx;
  if(f<0||f>22) return KERR_TYPE;
  Ta=ta; if(s(a)) { if(!vstcb(a)) return KERR_TYPE; Ta=10; }
  Tx=tx; if(s(x)) { if(!vstcb(x)) return KERR_TYPE; Tx=10; }
  if(a) {
    if(Ta<=0&&Tx<=0&&na!=nx) return KERR_LENGTH;
    switch(Ta) {
    case  1: case 2: case 3: case 4: case 6: case 10:
      if(Tx>0) r=k(f,k_(a),k_(x));
      else { PRK(nx); i(nx,prk[i]=ki(f,a,x,-1,i);EC(prk[i])) }
      break;
    case -1: case -2: case -3: case -4: case 0:
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
  char Tx;
  if(f<0||f>22) return KERR_TYPE;
  Tx=tx; if(!Tx&&s(x)) { if(!vstcb(x)) return KERR_TYPE; Tx=10; }
  if(Tx>0) r=k(f,k_(a),k_(x));
  else { PRK(nx); i(nx,prk[i]=ki(f,a,x,-1,i);EC(prk[i])) }
  return knorm(r);
cleanup:
  _k(r);
  return e;
}

K eachleft(i32 f, K a, K x) {
  K r=0,*prk,e;
  char Ta;
  if(f<0||f>22) return KERR_TYPE;
  Ta=ta; if(s(a)) { if(!vstcb(a)) return KERR_TYPE; Ta=10; }
  if(Ta>0) r=k(f,k_(a),k_(x));
  else { PRK(na); i(na,prk[i]=ki(f,a,x,i,-1);EC(prk[i])) }
  return knorm(r);
cleanup:
  _k(r);
  return e;
}

K overd(i32 f, K x) {
  K r=0,*pxk,e,p;
  char *pxc,**pxs,*P=":+-*%&|<>=~.!@?#_^,$'/\\",ff,Tx;
  i32 si,*pxi;
  double sf,*pxf;
  if(f<0||f>22) return KERR_TYPE;
  Tx=tx; if(!Tx&&s(x)) { if(!vstcb(x)) return KERR_TYPE; Tx=10; }
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
      case -2:
        if(ff=='+') { sf=0.0; return t2(sf); }
        if(ff=='*') { sf=1.0; return t2(sf); }
        if(ff=='&') { sf=INFINITY; return t2(sf); }
        if(ff=='|') { sf=-INFINITY; return t2(sf); }
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
    case  1: r=k_(x); break;
    case  2: r=k_(x); break;
    case -1: PXI; si=0; i(nx,si+=*pxi++); r=t(1,(u32)si); break;
    case -2: PXF; sf=0; i(nx,sf+=*pxf++); r=t2(sf); break;
    case  0: PXK; r=k_(*pxk++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    default: return KERR_TYPE;
    } break;
  case 3:
    switch(Tx) {
    case  1: r=k_(x); break;
    case  2: r=k_(x); break;
    case -1: PXI; si=1; i(nx,si*=*pxi++); r=t(1,(u32)si); break;
    case -2: PXF; sf=1; i(nx,sf*=*pxf++); r=t2(sf); break;
    case  0: PXK; r=k_(*pxk++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    default: return KERR_TYPE;
    } break;
  default:
    switch(Tx) {
    case  1: case 2: case 3: case 4: case 10: r=k_(x); break;
    case -1: PXI; r=t(1,(u32)*pxi++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
    case -2: PXF; r=t2(*pxf++); i1(nx,p=ki(f,r,x,-1,i);_k(r);r=0;EC(p);r=p) break;
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
  char *pxc,**pxs,*P=":+-*%&|<>=~.!@?#_^,$'/\\",ff,Tx;
  i32 si,*pri,*pxi;
  double sf,*prf,*pxf;
  if(f<0||f>22) return KERR_TYPE;
  Tx=tx; if(!Tx&&s(x)) { if(!vstcb(x)) return KERR_TYPE; Tx=10; }
  ff=P[f];
  if(Tx<=0&&!nx&&strchr("+*&|",ff)) {
    switch(Tx) {
    case -1: r=tn(1,0); break;
    case -2: r=tn(2,0); break;
    case -3: r=tn(3,0); break;
    case -4: r=tn(4,0); break;
    case  0: r=tn(0,0); break;
    }
    return r;
  }
  if(Tx==-1&&!nx&&ff=='=') return tn(1,0);
  if(Tx==3||Tx==1||Tx==2||Tx==4||Tx==10) return k_(x);
  if(Tx<=0&&!nx) return tn(0,0);
  switch(f) {
  case 1:
    switch(Tx) {
    case  1: case 2: r=k_(x); break;
    case -1: PRI(nx); PXI; si=0; i(nx,si+=*pxi++; *pri++=si); break;
    case -2: PRF(nx); PXF; sf=0; i(nx,sf+=*pxf++; *prf++=sf); break;
    case  0: PRK(nx); PXK; *prk++=p=k_(*pxk++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    default: return KERR_TYPE;
    } break;
  case 3:
    switch(Tx) {
    case  1: case 2: r=k_(x); break;
    case -1: PRI(nx); PXI; si=1; i(nx,si*=*pxi++; *pri++=si); break;
    case -2: PRF(nx); PXF; sf=1.0; i(nx,sf*=*pxf++; *prf++=sf); break;
    case  0: PRK(nx); PXK; *prk++=p=k_(*pxk++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    default: return KERR_TYPE;
    } break;
  default:
    switch(Tx) {
    case -1: PRK(nx); PXI; *prk++=p=t(1,(u32)*pxi++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
    case -2: PRK(nx); PXF; *prk++=p=t2(*pxf++); i1(nx,p=ki(f,p,x,-1,i);EC(p);*prk++=p) break;
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
