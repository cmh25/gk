#include "b.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#ifdef _WIN32
#include "systime.h"
#include "unistd.h"
#else
#include <sys/time.h>
#include <unistd.h>
#endif
#include "av.h"
#include "fn.h"
#include "p.h"
#include "ms.h"
#include <ctype.h>
#include "lzw.h"
#include "md5.h"
#include "sha1.h"
#include "sha2.h"
#include "aes256.h"
#include "io.h"
#include "irecur.h"
#include "la.h"
#include "repl.h"

#ifdef _WIN32
#define strtok_r strtok_s
#endif

static K vlookup(K x) {
  K r=scope_get(cs,x);
  return r?r:KERR_VALUE;
}

K builtin(K f, K a, K x) {
  K r=0,*pr,t;
  char *cf=sk(f);
  char avb[256],a2b[256],*av=avb,*a2=a2b;
  static i32 d=0;

  if(s(a)==0x40) if(4==(a=vlookup(a))) { _k(x); return KERR_VALUE; }
  if(s(x)==0x40) if(4==(x=vlookup(x))) { _k(a); return KERR_VALUE; }
  if(255<strlen(cf)) { _k(a); _k(x); return KERR_VALUE; }

  if(++d>maxr) { --d; _k(a); _k(x); return KERR_STACK; }

  *av=0;
  memcpy(a2,cf,1+strlen(cf));
  char *p=strpbrk(a2,"'/\\");
  if(p) { memcpy(av,p,1+strlen(p)); *p=0; }
  a2=sp(a2);
  if(av&&*av&&x) {
    if(a2==sp("exit")) { --d; _k(x); return KERR_PARSE; }
    r=avdo(t(4,st(s(f),a2)),k_(a),k_(x),av);
  }
  else if(0xc8==s(f)) { /* projection */
    if(a) { --d; _k(a); _k(x); return KERR_VALENCE; }
    K *pf=px(f);
    K g=k_(pf[0]);
    K q=kcp(pf[1]);
    if(E(q)) { --d; _k(a); _k(x); return q; }
    r=builtin(g,q,k_(x));
  }
  else if(0xc7==s(f)) {
    if(!x) r=KERR_TYPE;
    else if(!a) { /* project */
      r=st(0xc8,tn(0,3)); pr=px(r);
      pr[0]=t(4,st(0xc7,sp(a2)));
      t=kcp(x);
      if(E(t)) { --d; _k(r); _k(a); _k(x); return t; }
      pr[1]=t;
      pr[2]=null; /* TODO: prevent crash for now until we have adverb support here */
    }
    else if(a2==R_DRAW) r=draw(a,x);
    else if(a2==R_VS) r=vs(a,x);
    else if(a2==R_SV) r=sv(a,x);
    else if(a2==R_ATAN2) r=atan2_(a,x);
    else if(a2==R_DIV) r=div_(a,x);
    else if(a2==R_AND) r=and_(a,x);
    else if(a2==R_OR) r=or_(a,x);
    else if(a2==R_XOR) r=xor_(a,x);
    else if(a2==R_EUCLID) r=euclid_(a,x);
    else if(a2==R_ROT) r=rot_(a,x);
    else if(a2==R_SHIFT) r=shift_(a,x);
    else if(a2==R_DOT) r=dotp(a,x);
    else if(a2==R_AT) r=at_(a,x);
    else if(a2==R_SM) r=sm(a,x);
    else if(a2==R_SS) r=ss(a,x);
    else if(a2==R_LSQ) r=lsq_(a,x);
    else if(a2==R_ENCRYPT) r=encrypt_(a,x);
    else if(a2==R_DECRYPT) r=decrypt_(a,x);
    else if(a2==R_RENAME) r=rename_(a,x);
    else if(a2==R_SETENV) r=setenv_(a,x);
    else if(a2==R_IN) r=in_(a,x);
    else if(a2==R_DVL) r=dvl_(a,x);
    else r=KERR_VALUE;
  }
  else if(0xc6==s(f)) {
    if(!x) r=KERR_TYPE;
    else if(a2==R_SLEEP) r=sleep_(x);
    else if(a2==R_IC) r=ic(x);
    else if(a2==R_CI) r=ci(x);
    else if(a2==R_DJ) r=dj(x);
    else if(a2==R_JD) r=jd(x);
    else if(a2==R_LT) r=lt(x);
    else if(a2==R_LOG) r=log_(x);
    else if(a2==R_EXP) r=exp_(x);
    else if(a2==R_ABS) r=abs_(x);
    else if(a2==R_SQR) r=square(x);
    else if(a2==R_SQRT) r=sqrt_(x);
    else if(a2==R_FLOOR) r=floor_(x);
    else if(a2==R_CEIL) r=ceil_(x);
    else if(a2==R_SIN) r=sin_(x);
    else if(a2==R_COS) r=cos_(x);
    else if(a2==R_TAN) r=tan_(x);
    else if(a2==R_ASIN) r=asin_(x);
    else if(a2==R_ACOS) r=acos_(x);
    else if(a2==R_ATAN) r=atan_(x);
    else if(a2==R_SINH) r=sinh_(x);
    else if(a2==R_COSH) r=cosh_(x);
    else if(a2==R_TANH) r=tanh_(x);
    else if(a2==R_ERF) r=erf_(x);
    else if(a2==R_ERFC) r=erfc_(x);
    else if(a2==R_GAMMA) r=gamma_(x);
    else if(a2==R_LGAMMA) r=lgamma_(x);
    else if(a2==R_RINT) r=rint_(x);
    else if(a2==R_TRUNC) r=trunc_(x);
    else if(a2==R_NOT) r=not__(x);
    else if(a2==R_KV) r=kv_(x);
    else if(a2==R_VK) r=vk_(x);
    else if(a2==R_VAL) r=val_(x);
    else if(a2==R_DEL) r=del_(x);
    else if(a2==R_BD) r=bd_(x);
    else if(a2==R_DB) r=db_(x);
    else if(a2==R_HB) r=hb_(x);
    else if(a2==R_BH) r=bh_(x);
    else if(a2==R_ZB) r=zb_(x);
    else if(a2==R_BZ) r=bz_(x);
    else if(a2==R_MD5) r=md5_(x);
    else if(a2==R_SHA1) r=sha1_(x);
    else if(a2==R_SHA2) r=sha2_(x);
    else if(a2==R_SVD) r=svd_(x);
    else if(a2==R_LU) r=lu_(x);
    else if(a2==R_QR) r=qr_(x);
    else if(a2==R_GETENV) r=getenv_(x);
    else if(a2==R_EXIT) r=exit_(x);
    else r=KERR_VALUE;
  }
  else if(0xcc==s(f)) {
    if(!x) r=KERR_TYPE;
    else if(a2==sp("0:")) r=zerocolon(a,x);
    else if(a2==sp("1:")) r=onecolon(a,x);
    else if(a2==sp("2:")) r=twocolon(a,x);
    else if(a2==sp("3:")) r=threecolon(a,x);
    else if(a2==sp("4:")) r=fourcolon(a,x);
    else if(a2==sp("5:")) r=fivecolon(a,x);
    else if(a2==sp("6:")) r=sixcolon(a,x);
    else if(a2==sp("8:")) r=eightcolon(a,x);
    else r=KERR_VALUE;
  }
  else if(0xcd==s(f)) {
    if(!x) r=KERR_TYPE;
    else if(!a) { /* project */
      r=st(0xc8,tn(0,3)); pr=px(r);
      pr[0]=t(4,st(0xcd,sp(a2)));
      pr[1]=x;
      pr[2]=null; /* TODO: prevent crash for now until we have adverb support here */
    }
    else if(a2==sp("0:")) r=zerocolon(a,x);
    else if(a2==sp("1:")) r=onecolon(a,x);
    else if(a2==sp("2:")) r=twocolon(a,x);
    else if(a2==sp("3:")) r=threecolon(a,x);
    else if(a2==sp("4:")) r=fourcolon(a,x);
    else if(a2==sp("5:")) r=fivecolon(a,x);
    else if(a2==sp("6:")) r=sixcolon(a,x);
    else if(a2==sp("8:")) r=eightcolon(a,x);
    else r=KERR_VALUE;
  }
  else {
    if(a2==R_DO) r=do_(x);
    else if(a2==R_WHILE) r=while_(x);
    else if(a2==R_IF) r=if_(x);
    else r=KERR_VALUE;
  }
  --d;
  _k(a); _k(x);
  return r;
}

K draw(K a, K x) {
  K r=0,e,p,q;
  i32 n=1,m=ik(x),*pai;
  if(!a||!x) { e=KERR_TYPE; goto cleanup; }
  if(ta!=0 && ta!=1 && ta!=-1) { e=KERR_TYPE; goto cleanup; }
  if(tx!=1 && tx!=2 && ta!=-1) { e=KERR_TYPE; goto cleanup; }
  if(ta==1 && tx==1 && ik(x)<0 && ik(a)>abs(ik(x))) { e=KERR_LENGTH; goto cleanup; }
  switch(ta) {
  case  0: if(na) { e=KERR_TYPE; goto cleanup; } r=k(14,t(1,(u32)n),t(1,(u32)m)); /* n?m */ break;
  case  1: n=ik(a); r=k(14,t(1,(u32)n),t(1,(u32)m)); /* n?m */ break;
  case -1:
    switch(tx) {
    case 1: case 2:
      PAI; i(na,if(pai[i]<0){e=KERR_DOMAIN;goto cleanup;})
      p=k(21,3,k_(a)); EC(p); // */a
      q=k(14,p,t(1,(u32)m)); EC(q); // p?m
      r=k(15,k_(a),q); // a#q
      break;
    case -1:
      if(na!=nx) { e=KERR_LENGTH; goto cleanup; }
      r=k(14+32,k_(a),k_(x)); /* a?'x */
      break;
    default: r=KERR_TYPE; break;
    } break;
  default: r=KERR_TYPE; break;
  }
  if(r<EMAX) { e=r; r=0; goto cleanup; }
  if(ta==0) r=k(3,0,r); // *r
  return r;
cleanup:
  _k(r);
  return e;
}

K dotp(K a, K x) {
  K r=0;
  if(!a||!x) return KERR_TYPE;
  if(s(a)||s(x)) return KERR_TYPE;
  r=k(3,k_(a),k_(x));  /* a*x */
  if(E(r)) return r;
  r=k(21,1,r); /* +/r */
  return knorm(r);
}

K at_(K a, K x) {
  K r=0;
  i32 *pri,*pai,*pxi;
  double *prf,*paf;
  char *prc,*pac,**prs,**pas;

  if(s(a)||s(x)||aa) return KERR_RANK;
  if(tx==6) return k_(a);
  if(!tx&&!nx) return tn(0,0);

  switch(ta) {
  case -1:
    switch(tx) {
    case  1: PAI; r=t(1,(u32)((ik(x)<0||(u64)(ik(x))>=na)?INT32_MIN:pai[ik(x)])); break;
    case -1: PRI(nx); PAI; PXI; i(nx, pri[i]=(pxi[i]<0||pxi[i]>=(i32)na)?INT32_MIN:pai[pxi[i]]); break;
    case  0: r=irecur2(at_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case -2:
    switch(tx) {
    case  1: PAF; r=t2((ik(x)<0||(u64)(ik(x))>=na)?NAN:paf[ik(x)]); break;
    case -1: PRF(nx); PAF; PXI; i(nx, prf[i]=(pxi[i]<0||pxi[i]>=(i32)na)?NAN:paf[pxi[i]]); break;
    case  0: r=irecur2(at_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case -3:
    switch(tx) {
    case  1: PAC; r=t(3,(ik(x)<0||(u64)(ik(x))>=na)?' ':(u8)pac[ik(x)]); break;
    case -1: PRC(nx); PAC; PXI; i(nx, prc[i]=(pxi[i]<0||pxi[i]>=(i32)na)?' ':pac[pxi[i]]); break;
    case  0: r=irecur2(at_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case -4:
    switch(tx) {
    case  1: PAS; r=t(4,(ik(x)<0||(u64)(ik(x))>=na)?sp(""):pas[ik(x)]); break;
    case -1: PRS(nx); PAS; PXI; i(nx, prs[i]=(pxi[i]<0||pxi[i]>=(i32)na)?sp(""):pas[pxi[i]]); break;
    case  0: r=irecur2(at_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case  0: r=irecur2(at_,a,x); break;
  default: r=KERR_TYPE;
  }
  return knorm(r);
}

static i32 sm_(char *a, char *x, i32 an, i32 xn, i32 i, i32 j) {
  i32 m=1;
  for(;;++i,++j) {
    if(i>=an&&j>=xn) break;
    if(i>=an) { m=0; break; }
    if(j>=xn) { m=0; break; }
    if(x[j]=='?') continue;
    if(x[j]=='*') {
      if(++j==xn) { ++i; break; }
      m=0; while(++i<an) if((m=sm_(a,x,an,xn,i,j))) break;
    }
    else if(x[j]=='[' && j+1<xn && x[j+1]==']') { ++j; ++j; } /* empty escaped? */
    else if(x[j]=='[' && j+2<xn && x[j+2]==']') { /* escaped? */
      ++j;
      if(i<an&&a[i]!=x[j]) { m=0; break; }
      ++j;
    }
    else if(x[j]=='[' && strchr(x+j,']')) {
      if(x[++j]=='^') {
        m=1; for(++j;j<xn&&x[j]!=']';j++) if(a[i]==x[j]) { m=0; break; }
      }
      else {
        m=0; for(;j<xn&&x[j]!=']';j++) if(a[i]==x[j]) { m=1; break; }
      }
      while(j<xn&&x[j]!=']') ++j;
    }
    else if(i<an&&j<xn&&a[i]!=x[j]) { m=0; break; }
  }
  return m;
}

K sm(K a, K x) {
  K r=0;
  char *pac,*pxc,**pas,**pxs,s[2];
  i32 *pri,m;

  if(s(a) || s(x)) return KERR_TYPE;
  if(ta<=0 && tx<=0 && ta!=-3 && tx !=-3 && na!=nx) return KERR_LENGTH;

  switch(ta) {
  case  4:
    pac=sk(a);
    switch(tx) {
    case  3: s[0]=ck(x); s[1]=0; r=t(1,sm_(pac,s,strlen(pac),1,0,0)); break;
    case  4: pxc=sk(x); r=t(1,sm_(pac,pxc,strlen(pac),strlen(pxc),0,0)); break;
    case -3: PXC; r=t(1,sm_(pac,pxc,strlen(pac),nx,0,0)); break;
    case -4: PRI(nx); PXS; m=strlen(pac); i(nx,pri[i]=t(1,sm_(pac,pxs[i],m,strlen(pxs[i]),0,0))) break;
    case  0: r=irecur2(sm,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case -3:
    PAC;
    switch(tx) {
    case  3: s[0]=ck(x); s[1]=0; r=t(1,sm_(pac,s,strlen(pac),1,0,0)); break;
    case  4: pxc=sk(x); r=t(1,sm_(px(a),pxc,na,strlen(pxc),0,0)); break;
    case -3: r=t(1,sm_(px(a),px(x),na,nx,0,0)); break;
    case -4: PRI(nx); PAC; PXS; i(nx,pri[i]=t(1,sm_(pac,pxs[i],na,strlen(pxs[i]),0,0))) break;
    case  0: r=irecur2(sm,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case -4:
    PAS;
    switch(tx) {
    case  3: PRI(na); s[0]=ck(x); s[1]=0; i(na,pri[i]=t(1,sm_(pas[i],s,strlen(pas[i]),1,0,0))) break;
    case  4: PRI(na); pxc=sk(x); m=strlen(pxc); i(na,pri[i]=t(1,sm_(pas[i],pxc,strlen(pas[i]),m,0,0))) break;
    case -3: PRI(na); PXC; i(na,pri[i]=t(1,sm_(pas[i],pxc,strlen(pas[i]),nx,0,0))) break;
    case -4: PRI(na); PXS; i(nx,pri[i]=t(1,sm_(pas[i],pxs[i],strlen(pas[i]),strlen(pxs[i]),0,0))) break;
    case  0: r=avdo(t(4,st(0xc7,"sm")),k_(a),k_(x),"'"); break;
    default: r=KERR_TYPE;
    } break;
  case  0:
    switch(tx) {
    case  3: r=irecur2(sm,a,x); break;
    case  4: r=irecur2(sm,a,x); break;
    case -3: r=irecur2(sm,a,x); break;
    case -4: r=irecur2(sm,a,x); break;
    case  0: r=irecur2(sm,a,x); break;
    default: r=KERR_TYPE;
    } break;
  default: r=KERR_TYPE;
  }
  return knorm(r);
}

static i32 ss_(char *a, char *x, i32 an, i32 xn, i32 i) {
  i32 m=1,j,k=i;
  for(j=0;j<xn;j++) {
    if(k==an) break;
    if(x[j]=='?') { }
    else if(x[j]=='[' && j+2<xn && x[j+2]=='[') { /* escaped */
      ++j;
      if(k<an&&a[k]!=x[j]) { m=0; break; }
      ++j;
    }
    else if(x[j]=='[' && strchr(x+j,'[')) {
      if(x[++j]=='^') {
        m=1; for(++j;j<xn&&x[j]!=']';j++) if(a[k]==x[j]) { m=0; break; }
      }
      else if(j+2<xn && x[j+1]=='-') {
        char p=x[j++];
        ++j;
        char q=x[j++];
        m=0; while(p<=q) { if(a[k]==p++) { m=1; break; } }
      }
      else {
        m=0; for(;j<xn&&x[j]!=']';j++) if(a[k]==x[j]) { m=1; break; }
      }
      while(j<xn&&x[j]!=']') ++j;
    }
    else if(a[k]!=x[j]) break;
    ++k;
  }
  return m&&j==xn ? k-i : 0;
}

K ss(K a, K x) {
  K r=0;
  u32 i,n=0,m,s;
  i32 *pri;
  char *pac,*pxc,s0[2];

  if(s(a) || s(x)) return KERR_TYPE;
  if(ta!= 0 && ta!=-3) return KERR_TYPE;
  if(ta== 0 && tx==-4 && na!=nx) return KERR_LENGTH;
  if(ta== 0 && tx== 0 && na!=nx) return KERR_LENGTH;
  if(tx==-3 && !nx) return KERR_LENGTH;
  if(tx== 4 && !strlen(sk(x))) return KERR_LENGTH;

  switch(ta) {
  case -3:
    PAC;
    switch(tx) {
    case  3:
      PRI(2);
      s0[0]=ck(x); s0[1]=0;
      s=1;
      for(i=0;i<na;i++) {
        if((m=ss_(pac,s0,na,s,i))) {
          if(n==nr) { r=kresize(r,n<<1); pri=px(r); }
          pri[n++]=i;
          i+=m-1;
        }
      }
      nr=n;
      break;
    case  4:
      PRI(2);
      pxc=sk(x);
      s=strlen(pxc);
      for(i=0;i<na;i++) {
        if((m=ss_(pac,pxc,na,s,i))) {
          if((i==0||!isalpha(pac[i-1])) && ((i+m<na&&!isalpha(pac[i+m]))||i+m==na)) {
            if(n==nr) { r=kresize(r,n<<1); pri=px(r); }
            pri[n++]=i;
            i+=m-1;
          }
        }
      }
      nr=n;
      break;
    case -3:
      PRI(2);
      PXC;
      for(i=0;i<na;i++) {
        if((m=ss_(pac,pxc,na,nx,i))) {
          if(n==nr) { r=kresize(r,n<<1); pri=px(r); }
          pri[n++]=i;
          i+=m-1;
        }
      }
      nr=n;
      break;
    case -4: r=avdo(t(4,st(0xc7,"ss")),k_(a),k_(x),"/"); break;
    case  0: r=avdo(t(4,st(0xc7,"ss")),k_(a),k_(x),"/"); break;
    default: r=KERR_TYPE;
    } break;
  case  0:
    switch(tx) {
    case  3: r=irecur2(ss,a,x); break;
    case  4: r=irecur2(ss,a,x); break;
    case -3: r=avdo(t(4,st(0xc7,"ss")),k_(a),k_(x),"\\"); break;
    case -4: r=avdo(t(4,st(0xc7,"ss")),k_(a),k_(x),"'"); break;
    case  0: r=avdo(t(4,st(0xc7,"ss")),k_(a),k_(x),"'"); break;
    default: r=KERR_TYPE;
    } break;
  default: r=KERR_TYPE;
  }
  return knorm(r);
}

static K vs_(K a, K x, u32 w) {
  K r=0,e,q=0,p=0,*pq,*pxk;
  i32 i,m,*pri,*pai,*pxi,*pqi;
  u64 xx,y;
  switch(ta) {
  case 1:
    if(ik(a)<=1) return KERR_DOMAIN;
    switch(tx) {
    case  1:
      if(ik(x)==0) return tn(1,0);
      xx=(u32)x; y=ik(a); i=w; r=tn(1,w); pri=px(r);
      while(xx>0) { m=xx%y; xx/=y; pri[--i]=m; }
      while(--i>=0) pri[i]=0;
      break;
    case  0:
      q=tn(0,nx); pq=px(q); pxk=px(x);
      i(n(q), pq[i]=vs_(a,pxk[i],w); EC(pq[i]); if(!n(pq[i])){_k(pq[i]);pq[i]=t(1,0);})
      r=k(1,0,q); /* flip */
      break;
    case -1:
      q=tn(0,nx); pq=px(q); pxi=px(x);
      i(n(q), p=t(1,(u32)pxi[i]); pq[i]=vs_(a,p,w); EC(pq[i]); if(!n(pq[i])){_k(pq[i]);pq[i]=t(1,0);})
      r=k(1,0,q); /* flip */
      break;
    default: r=KERR_TYPE;
    } break;
  case -1:
    PAI;
    switch(tx) {
    case  1:
      q=k(6,0,k_(a)); /* reverse */
      p=k(22,3,q);    // *\q
      q=k(6,0,p);     /* reverse */
      pqi=px(q);
      i(n(q)-1,pqi[i]=pqi[i+1]) pqi[n(q)-1]=1;
      xx=(u32)x; r=tn(1,n(q)); pri=px(r);
      i(n(q),if(0==pqi[i]) { _k(q); _k(r); return KERR_DOMAIN;})
      i(n(q),y=pqi[i];m=xx/y;xx-=m*y;pri[i]=pai[i]?m%pai[i]:m)
      _k(q);
      break;
    case  0:
      q=tn(0,nx); pq=px(q); pxk=px(x);
      i(n(q), pq[i]=vs_(a,pxk[i],w); EC(pq[i]); if(!n(pq[i])){_k(pq[i]);pq[i]=t(1,0);})
      r=k(1,0,q); /* flip */
      break;
    case -1:
      q=tn(0,nx); pq=px(q); pxi=px(x);
      i(n(q), p=t(1,(u32)pxi[i]); pq[i]=vs_(a,p,w); EC(pq[i]); if(!n(pq[i])){_k(pq[i]);pq[i]=t(1,0);})
      r=k(1,0,q); /* flip */
      break;
    default: r=KERR_TYPE;
    } break;
  default: r=KERR_TYPE;
  }
  return knorm(r);
cleanup:
  if(q) _k(q);
  if(r) _k(r);
  return e;
}

static i32 maxw(K x, u64 b) {
  K *pxk;
  i32 *pxi,w=0,z;
  u64 n;
  if(tx==1) {
    if(b==1) return b;
    w=1; n=(u32)x; while(b&&n>=b) { n/=b; ++w; }
  }
  else if(tx==-1) { pxi=px(x); i(nx,z=maxw(t(1,pxi[i]),b);if(w<z)w=z) }
  else if(tx==0) { pxk=px(x); i(nx,z=maxw(pxk[i],b);if(w<z)w=z) }
  return w;
}

K vs(K a, K x) {
  i32 w,*pai,z;
  K *pxk;
  if(s(a)||s(x)) return KERR_TYPE;
  if((ta==1&&ik(a)<=1)) return KERR_DOMAIN;
  if(ta==-1) {
    if(!na) return KERR_DOMAIN;
    pai=px(a);
    i(na,if(pai[i]<0) return KERR_DOMAIN)
    i(na,if(i&&pai[i]<1) return KERR_DOMAIN)
    i(na,if(pai[i]==INT32_MAX||pai[i]==INT32_MIN||pai[i]==INT32_MIN+1) return KERR_DOMAIN)
  }
  if(tx==0) {
    pxk=px(x);
    i(nx,if(1!=T(pxk[i])&&-1!=T(pxk[i])) return KERR_DOMAIN)
  }

  if(ta==1) w=maxw(x,ik(a));
  else if(ta==-1) { w=0; pai=px(a); i(na,z=maxw(x,pai[i]);if(w<z)w=z) }
  else return KERR_TYPE;
  return vs_(a,x,w);
}

K sv(K a, K x) {
  K r=0,e,q=0,p=0,s=0,*pxk,*prk;
  i32 *pxi;
  u32 j;
  double jf,*pxf;
  if(s(a)||s(x)) return KERR_TYPE;
  if(tx==0) {
    p=k(1,0,k_(x)); EC(p); /* flip */
    EC(p);
    x=p;
  }
  switch(ta) {
  case 1:
    switch(tx) {
    case  1: r=k_(x); break;
    case  0: pxk=px(x); r=tn(0,n(x)); prk=px(r); i(nr, prk[i]=sv(a,pxk[i]); EC(prk[i])) break;
    case -1: pxi=px(x); j=nx?pxi[0]:0; i1(nx,j=j*ik(a)+pxi[i]) r=t(1,(u32)j); break;
    case -2: pxf=px(x); jf=nx?pxf[0]:0; i1(nx,jf=jf*ik(a)+pxf[i]) r=t2(jf); break;
    default: r=KERR_TYPE;
    } break;
  case -1:
    switch(tx) {
    case  1:
      q=k(6,0,k_(a)); EC(q);          // |a
      _k(p); p=k(22,3,q); q=0; EC(p); // *\q
      n(p)--;
      q=k(6,0,p); p=0; EC(q);         // |p
      r=k(3,q,k_(x)); q=0; EC(r);     // q*x
      r=k(21,1,r); EC(r);             // +/r
      r=k(1,r,k_(x)); EC(r);          // r+x
      break;
    case  0:
      pxk=px(x);
      r=tn(0,n(x)); prk=px(r);
      i(nr, prk[i]=sv(a,pxk[i]); EC(prk[i]))
      break;
    case -1:
      pxi=px(x);
      if(na!=nx) { e=KERR_LENGTH; goto cleanup; }
      q=k(6,0,k_(a)); EC(q);          // |a
      s=k(22,3,q); q=0; EC(s);        // *\q
      n(s)--;
      q=k(6,0,s); s=0; EC(q);         // |p
      n(x)--;
      r=k(3,q,k_(x)); q=0; EC(r);     // q*x
      r=k(21,1,r); EC(r);             // +/r
      r=k(1,r,t(1,pxi[nx])); EC(r);
      break;
    default: r=KERR_TYPE;
    } break;
  default: r=KERR_TYPE;
  }
  _k(p); _k(q); _k(s);
  return knorm(r);
cleanup:
  _k(r); _k(p); _k(q); _k(s);
  return e;
}

K square(K x) {
  K r=0,e;
  i32 *pxi;
  double f,*prf,*pxf;
  if(!x) { e=KERR_TYPE; goto cleanup; }
  if(s(x)) { e=KERR_TYPE; goto cleanup; }
  switch(tx) {
  case  1: f=fi(ik(x))*fi(ik(x)); r=t2(f); break;
  case  2: f=-fk(x); f*=f; r=t2(f); break;
  case -1: PRF(nx); PXI; i(nx,*prf++=*pxi*fi(*pxi);++pxi) break;
  case -2: PRF(nx); PXF; i(nx,*prf++=*pxf**pxf;++pxf) break;
  case  0: r=irecur1(square,x); break;
  default: r=KERR_TYPE;
  }
  return knorm(r);
cleanup:
  _k(r);
  return e;
}

K abs_(K x) {
  K r=0,e;
  i32 i,j,*pri,*pxi;
  double f,g,*prf,*pxf;
  if(!x) { e=KERR_TYPE; goto cleanup; }
  if(s(x)) { e=KERR_TYPE; goto cleanup; }
  switch(tx) {
  case  1: i=ik(x); j=i==INT32_MIN?INT32_MIN:i==INT32_MIN+1?INT32_MAX:i<0?-i:i; r=t(1,(u32)j); break;
  case  2: f=fk(x); g=isinf(f)&&f<0.0?INFINITY:f<0.0?-f:f; r=t2(g); break;
  case -1: PRI(nx); PXI; i(nx,j=*pxi++; *pri++=j==INT32_MIN?INT32_MIN:j==INT32_MIN+1?INT32_MAX:j<0?-j:j); break;
  case -2: PRF(nx); PXF; i(nx,f=*pxf++; *prf++=isinf(f)&&f<0.0?INFINITY:f<0.0?-f:f); break;
  case  0: r=irecur1(abs_,x); break;
  default: r=KERR_TYPE;
  }
  return knorm(r);
cleanup:
  _k(r);
  return e;
}

K sleep_(K x) {
  if(s(x)) return KERR_TYPE;
  if(tx != 1 && tx != 2) return KERR_TYPE;
  double d = (tx == 1) ? (double)ik(x) : fk(x);
  if(!isfinite(d) || d < 0) return KERR_DOMAIN;
#ifdef _WIN32
  // Sleep expects DWORD ms. round float to nearest ms.
  i64 ms = (i64)llround(d);
  while(ms > 0) {
    DWORD chunk = (ms > UINT_MAX) ? UINT_MAX : (DWORD)ms;
    Sleep(chunk);
    ms -= chunk;
  }
#else
  // nanosleep wants sec + nsec
  long double ns = d * 1.0e6L; // milliseconds -> nanoseconds
  struct timespec ts;
  ts.tv_sec  = (time_t)(ns / 1000000000L);
  ts.tv_nsec = (long)(fmodl(ns, 1000000000L));
  nanosleep(&ts, NULL);
#endif
  return null;
}

K ic(K x) {
  K r=0;
  u8 *s=0;
  i32 *pri;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case  3: r=t(1,ik(x)); break;
  case  0: r=irecur1(ic,x); break;
  case -3: s=px(x); PRI(nx); i(nx,pri[i]=s[i]) break;
  default: r=KERR_TYPE;
  }
  return r;
}

K ci(K x) {
  K r=0;
  i32 *pxi;
  char *prc;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case  1: r=t(3,(u8)ik(x)%256); break;
  case  0: r=irecur1(ci,x); break;
  case -1: PRC(nx); PXI; i(nx, prc[i]=pxi[i]%256); break;
  default: r=KERR_TYPE;
  }
  return r;
}

K do_(K x) {
  if(!x) return KERR_TYPE;
  K e,a,*px=px(x),p,r=null;
  i32 i=nx-1,q;
  i64 c;
  if(nx<2) return KERR_VALENCE;
  if(0x81==s(x)) return null; // all constants, nothing to do
  a=pgreduce_(px[i],&q);
  if(ta!=1) { _k(a); return KERR_TYPE; }
  c=ik(a);
  if(c<0) return r;
  while(c-->0)
    for(i=nx-2;i>=0;i--) {
      p=pgreduce_(px[i],&q); if(!p)p=null;
      EC(p);
      if(RETURN) return p;
      _k(p);
    }
  return r;
cleanup:
  _k(p);
  return e;
}

K while_(K x) {
  if(!x) return KERR_TYPE;
  K e,a,*px=px(x),p,r=null;
  i32 i,q;
  if(nx<2) return KERR_VALENCE;
  if(0x81==s(x)) return null; // all constants, nothing to do
  a=pgreduce_(px[nx-1],&q);
  if(ta!=1) { _k(a); return KERR_TYPE; }
  while(ik(a)) {
    for(i=nx-2;i>=0;i--) {
      p=pgreduce_(px[i],&q); if(!p)p=null;
      EC(p);
      if(RETURN) return p;
      _k(p);
    }
    a=pgreduce_(px[nx-1],&q);
    if(ta!=1) { _k(a); return KERR_TYPE; }
  }
  return r;
cleanup:
  _k(p);
  return e;
}

K if_(K x) {
  if(!x) return KERR_TYPE;
  K e,a,*px=px(x),p,r=null;
  i32 i=nx-1,q;
  if(nx<2) return KERR_VALENCE;
  if(0x81==s(x)) return null; // all constants, nothing to do
  a=pgreduce_(px[i],&q);
  if(ta!=1) { _k(a); return KERR_TYPE; }
  if(ik(a))
    for(i=nx-2;i>=0;i--) {
      p=pgreduce_(px[i],&q); if(!p)p=null;
      EC(p);
      if(RETURN) return p;
      _k(p);
    }
  return r;
cleanup:
  _k(p);
  return e;
}

static K cond81_(K x) {
  K r=null,a,*px=px(x);
  u32 i;
  for(i=0;i<nx;++i) {
    if(i<nx-1) {
      a=k_(px[i++]);;
      if(ta!=1) { _k(a); return KERR_TYPE; }
      if(ik(a)&&i<=nx-1) { _k(r); r=k_(px[i]); if(!r) r=null; break; }
    }
    else { r=k_(px[i]); if(!r) r=null; }
    if(RETURN) return r;
  }
  return r;
}
K cond_(K x) {
  K r=null,a,*px=px(x);
  i32 i,q;
  if(nx<2) return KERR_VALENCE;
  if(0x81==s(x)) return cond81_(x);
  for(i=nx-1;i>=0;--i) {
    if(i) {
      a=pgreduce_(px[i--],&q); if(!a) a=null;
      if(ta!=1) { _k(a); return KERR_TYPE; }
      if(ik(a)&&i>=0) { _k(r); r=pgreduce_(px[i],&q); if(!r) r=null; break; }
    }
    else { r=pgreduce_(px[i],&q); if(!r) r=null; }
    if(RETURN) return r;
  }
  return r;
}

K dj(K x) {
  K r=0;
  i32 year,month,day;
  i64 g,b,c,d,e,m;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case  1:
    g = (i64)ik(x)+ 32044 + 2464329; /* 20350101 */
    b = (4*g+3)/146097;
    c = g - (b*146097)/4;
    d = (4*c+3)/1461;
    e = c - (1461*d)/4;
    m = (5*e+2)/153;
    day   = (i32)(e-(153*m+2)/5+1);
    month = (i32)(m+3-12*(m/10));
    year  = (i32)(b*100+d-4800+m/10);
    // encode YMD in 64-bit, then clamp to 32-bit range
    i64 ymd64 = (i64)year * 10000 + (i64)month * 100 + day;
    if(ymd64<INT32_MIN || ymd64>INT32_MAX) r=t(1, (u32)INT32_MIN);
    else r=t(1, (u32)(i32)ymd64);
    break;
  case -1: r=avdo(t(4,st(0xc6,sp("dj"))),0,k_(x),"'"); break;
  case  0: r=irecur1(dj,x); break;
  default: return KERR_TYPE;
  }
  return knorm(r);
}

K jd(K x) {
  K r=0;
  i32 year,month,day;
  i64 g,y,m,val;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case  1:
    year = ik(x)/10000;
    month = (ik(x)/100)%100;
    day = ik(x)%100;
    g = (14-month)/12;
    y = (i64)year+4800-g;
    m = month+12*g-3;
    val = (i64)day + (153*m+2)/5 + y*365 + y/4 - y/100 + y/400 - 32045 - 2464329;
    // val is days; it already fits in 64-bit. pack to i32 domain.
    if(val<INT32_MIN || val>INT32_MAX) r=t(1, (u32)INT32_MIN);
    else r=t(1, (u32)(i32)val);
    break;
  case -1: r=avdo(t(4,st(0xc6,sp("jd"))),0,k_(x),"'"); break;
  case  0: r=irecur1(jd,x); break;
  default: return KERR_TYPE;
  }
  return knorm(r);
}

K lt(K x) {
  K r=0;
  time_t t,g,o;
  struct tm *tm;
  i64 q;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case  1:
    t = ik(x)+2051222400l; /* 2035-1970 */
#ifdef _WIN32
    struct tm result;
    tm=&result;
    localtime_s(tm,&t);
    g = _mkgmtime(tm);
#else
    tm = localtime(&t);
    g = timegm(tm);
#endif
    o = g-t;
    q=ik(x)+(i64)o;
    r = t(1,(u32)q);
    break;
  case -1: r=avdo(t(4,st(0xc6,sp("lt"))),0,k_(x),"'"); break;
  case  0: r=irecur1(lt,x); break;
  default: return KERR_TYPE;
  }
  return knorm(r);
}

/*
 *  F: function
 * F0: function to compute
*/
#define MC1D(F,F0) \
K F(K x) { \
  K r=0,e; \
  i32 *pxi; \
  double *prf,*pxf; \
  if(s(x)) return KERR_TYPE; \
  switch(tx) { \
  case  1: r=t2(F0(fi(ik(x)))); break; \
  case  2: r=t2(F0(fk(x))); break; \
  case  0: r=irecur1(F,x); break; \
  case -1: r=tn(2,nx); prf=px(r); PXI; i(nx,prf[i]=F0(fi(pxi[i]))) break; \
  case -2: r=tn(2,nx); prf=px(r); PXF; i(nx,prf[i]=F0(pxf[i])) break; \
  default: { e=KERR_TYPE; goto cleanup; } \
  } \
  return knorm(r); \
cleanup: \
  _k(r); \
  return e; \
}
MC1D(log_,log)
MC1D(exp_,exp)
MC1D(sqrt_,sqrt)
MC1D(floor_,floor)
MC1D(ceil_,ceil)
MC1D(sin_,sin)
MC1D(cos_,cos)
MC1D(tan_,tan)
MC1D(asin_,asin)
MC1D(acos_,acos)
MC1D(atan_,atan)
MC1D(sinh_,sinh)
MC1D(cosh_,cosh)
MC1D(tanh_,tanh)
MC1D(erf_,erf)
MC1D(erfc_,erfc)
MC1D(gamma_,tgamma)
MC1D(lgamma_,lgamma)
MC1D(rint_,rint)
MC1D(trunc_,trunc)

/*
 *  F: function
 * F0: function to compute
*/
#define MC2D(F,F0) \
K F(K a,K x) { \
  K r=0; \
  i32 *pai,*pxi; \
  double *prf,*paf,*pxf; \
  if(s(a)||s(x)) return KERR_TYPE; \
  if(ta<=0 && tx<=0 && na!=nx) return KERR_LENGTH; \
  switch(ta) { \
  case  1: \
    switch(tx) { \
    case  1: r=t2(F0(fi(ik(a)),fi(ik(x)))); break; \
    case  2: r=t2(F0(fi(ik(a)),fk(x))); break; \
    case -1: PRF(nx); PXI; i(nx,prf[i]=F0(fi(ik(a)),fi(pxi[i]))) break; \
    case -2: PRF(nx); PXF; i(nx,prf[i]=F0(fi(ik(a)),pxf[i])) break; \
    case  0: r=irecur2(F,a,x); break; \
    default: r=KERR_TYPE; break; \
    } break; \
  case  2: \
    switch(tx) { \
    case  1: r=t2(F0(fk(a),fi(ik(x)))); break; \
    case  2: r=t2(F0(fk(a),fk(x))); break; \
    case -1: PRF(nx); PXI; i(nx,prf[i]=F0(fk(a),fi(pxi[i]))) break; \
    case -2: PRF(nx); PXF; i(nx,prf[i]=F0(fk(a),pxf[i])) break; \
    case  0: r=irecur2(F,a,x); break; \
    default: r=KERR_TYPE; break; \
    } break; \
  case -1: \
    switch(tx) { \
    case  1: PRF(na); PAI; i(na,prf[i]=F0(fi(pai[i]),fi(ik(x)))) break; \
    case  2: PRF(na); PAI; i(na,prf[i]=F0(fi(pai[i]),fk(x))) break; \
    case -1: PRF(na); PAI; PXI; i(na,prf[i]=F0(fi(pai[i]),fi(pxi[i]))) break; \
    case -2: PRF(na); PAI; PXF; i(na,prf[i]=F0(fi(pai[i]),pxf[i])) break; \
    case  0: r=avdo(t(4,st(0xc7,#F0)),k_(a),k_(x),"'"); break; \
    default: r=KERR_TYPE; break; \
    } break; \
  case -2: \
    switch(tx) { \
    case  1: PRF(na); PAF; i(na,prf[i]=F0(paf[i],fi(ik(x)))) break; \
    case  2: PRF(na); PAF; i(na,prf[i]=F0(paf[i],fk(x))) break; \
    case -1: PRF(na); PAF; PXI; i(na,prf[i]=F0(paf[i],fi(pxi[i]))) break; \
    case -2: PRF(na); PAF; PXF; i(na,prf[i]=F0(paf[i],pxf[i])) break; \
    case  0: r=avdo(t(4,st(0xc7,#F0)),k_(a),k_(x),"'"); break; \
    default: r=KERR_TYPE; break; \
    } break; \
  case  0: \
    switch(tx) { \
    case  1: r=irecur2(F,a,x); break; \
    case  2: r=irecur2(F,a,x); break; \
    case -1: r=avdo(t(4,st(0xc7,#F0)),k_(a),k_(x),"'"); break; \
    case -2: r=avdo(t(4,st(0xc7,#F0)),k_(a),k_(x),"'"); break; \
    case  0: r=irecur2(F,a,x); break; \
    default: r=KERR_TYPE; break; \
    } break; \
  default: r=KERR_TYPE; break; \
  } \
  return knorm(r); \
}
MC2D(atan2_,atan2)

/* integer ops */
#define MC2IO(F,F0,O,N) \
K F(K a, K x) { \
  K r=0; \
  i32 *pri,*pai,*pxi; \
  if(s(a)||s(x)) return KERR_TYPE; \
  if((N)) {  /* nonce */ \
    if(tx==1 && !ik(x)) return KERR_NONCE; \
    if(tx==-1) { PXI; i(nx,if(!pxi[i]) return KERR_NONCE) } \
    if(ta==1 && INT32_MIN==ik(a)) { /* 0N / -1 */ \
      if(tx==1 && -1==ik(x)) return KERR_NONCE; \
      if(tx==-1) { PXI; i(nx,if(-1==pxi[i]) return KERR_NONCE) } \
    } \
    if(ta==-1) { \
      i32 b=0; \
      PAI; i(na,if(INT32_MIN==pai[i]) {b=1;break;}) \
      if(b && tx==1 && -1==ik(x)) return KERR_NONCE; \
      if(b && tx==-1) { PXI; i(nx,if(-1==pxi[i]) return KERR_NONCE) } \
    } \
  } \
  if(ta<=0 && tx<=0 && na!=nx) return KERR_LENGTH; \
  switch(ta) { \
  case  1: \
    switch(tx) { \
    case  1: r=t(1,(u32)(ik(a) O ik(x))); break; \
    case -1: PRI(nx); PXI; i(nx,pri[i]=ik(a) O pxi[i]) break; \
    case  0: r=irecur2(F,a,x); break; \
    default: r=KERR_TYPE; \
    } break; \
  case -1: \
    switch(tx) { \
    case  1: PRI(na); PAI; i(na,pri[i]=pai[i] O ik(x)) break; \
    case -1: PRI(na); PAI; PXI; i(na,pri[i] = pai[i] O pxi[i]) break; \
    case  0: r=avdo(t(4,st(0xc7,#F0)),k_(a),k_(x),"'"); break; \
    default: r=KERR_TYPE; \
    } break; \
  case  0: \
    switch(tx) { \
    case  1: r=irecur2(F,a,x); break; \
    case -1: r=avdo(t(4,st(0xc7,#F0)),k_(a),k_(x),"'"); break; \
    case  0: r=irecur2(F,a,x); break; \
    default: r=KERR_TYPE; \
    } break; \
  default: r=KERR_TYPE; \
  } \
  return knorm(r); \
}
MC2IO(div_,div,/,1)
MC2IO(and_,and,&,0)
MC2IO(or_,or,|,0)
MC2IO(xor_,xor,^,0)

K euclid_(K a,K x) {
  K r=0;
  double af,xf,*prf,*paf,*pxf;
  i32 *pai,*pxi;

  if(s(a)||s(x)) return KERR_TYPE;
  if(ta<0 && tx<0 && na!=nx) return KERR_LENGTH;

  switch(ta) {
  case  1:
    switch(tx) {
    case  1: af=fi(ik(a)); xf=fi(ik(x)); r=t2(sqrt(af*af+xf*xf)); break;
    case  2: af=fi(ik(a)); xf=fk(x); r=t2(sqrt(af*af+xf*xf)); break;
    case -1: PRF(nx); PXI; xf=fi(ik(a)); af=xf*xf; i(nx,xf=fi(pxi[i]);prf[i]=sqrt(af+xf*xf)) break;
    case -2: PRF(nx); PXF; xf=fi(ik(a)); af=xf*xf; i(nx,xf=pxf[i];prf[i]=sqrt(af+xf*xf)) break;
    case  0: r=irecur2(euclid_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case  2:
    switch(tx) {
    case  1: af=fk(a); xf=fi(ik(x)); r=t2(sqrt(af*af+xf*xf)); break;
    case  2: af=fk(a); xf=fk(x); r=t2(sqrt(af*af+xf*xf)); break;
    case -1: PRF(nx); PXI; af=fk(a)*fk(a); i(nx, xf=fi(pxi[i]); prf[i]=sqrt(af+xf*xf)) break;
    case -2: PRF(nx); PXF; af=fk(a)*fk(a); i(nx, xf=pxf[i]; prf[i]=sqrt(af+xf*xf)) break;
    case  0: r=irecur2(euclid_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case -1:
    switch(tx) {
    case  1: PRF(na); PAI; xf=fi(ik(x))*fi(ik(x)); i(na, af=fi(pai[i]); prf[i]=sqrt(af*af+xf)); break;
    case  2: PRF(na); PAI; xf=fk(x)*fk(x); i(na, af=fi(pai[i]); prf[i]=sqrt(af*af+xf)); break;
    case -1: PRF(na); PAI; PXI; i(na, af=fi(pai[i]); xf=fi(pxi[i]); prf[i]=sqrt(af*af+xf*xf)) break;
    case -2: PRF(na); PAI; PXF; i(na, af=fi(pai[i]); xf=pxf[i]; prf[i]=sqrt(af*af+xf*xf)) break;
    case  0: r=avdo(t(4,st(0xc7,"euclid")),k_(a),k_(x),"'"); break;
    default: r=KERR_TYPE;
    } break;
  case -2:
    switch(tx) {
    case  1: PRF(na); PAF; xf=fi(ik(x))*fi(ik(x)); i(na, af=paf[i]; prf[i]=sqrt(af*af+xf)); break;
    case  2: PRF(na); PAF; xf=fk(x)*fk(x); i(na, af=paf[i]; prf[i]=sqrt(af*af+xf)); break;
    case -1: PRF(na); PAF; PXI; i(na, af=paf[i]; xf=fi(pxi[i]); prf[i]=sqrt(af*af+xf*xf)) break;
    case -2: PRF(na); PAF; PXF; i(na, af=paf[i]; xf=pxf[i]; prf[i]=sqrt(af*af+xf*xf)) break;
    case  0: r=avdo(t(4,st(0xc7,"euclid")),k_(a),k_(x),"'"); break;
    default: r=KERR_TYPE;
    } break;
  case  0:
    switch(tx) {
    case  1: r=irecur2(euclid_,a,x); break;
    case  2: r=irecur2(euclid_,a,x); break;
    case -1: r=avdo(t(4,st(0xc7,"euclid")),k_(a),k_(x),"'"); break;
    case -2: r=avdo(t(4,st(0xc7,"euclid")),k_(a),k_(x),"'"); break;
    case  0: r=irecur2(euclid_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  default: r=KERR_TYPE;
  }
  return knorm(r);
}

K not__(K x) {
  K r=0;
  i32 *pri,*pxi;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case  1: r=t(1,(u32)~ik(x)); break;
  case  0: r=irecur1(not__,x); break;
  case -1: PRI(nx); PXI; i(nx,pri[i]=~pxi[i]) break;
  default: r=KERR_TYPE; break;
  }
  return knorm(r);
}

K kv_(K x) {
  if(s(x)) return KERR_TYPE;
  K r=tx<0?kmix(x):3;
  return r;
}

K vk_(K x) {
  if(tx||s(x)) return KERR_TYPE;
  return knorm(k_(x));
}

K val_(K x) {
  return val(x);
}

static inline i32 raw_rot32(i32 v, i32 sh) {
  u32 u = (u32)v;
  u32 s = (u32)sh & 31;  // normalize to [0..31]
  if(!s) return v;
  return (i32)((u << s) | (u >> (32 - s)));
}

static inline i32 safe_rot(i32 v, i32 sh) {
  if(sh == INT32_MIN)   return v;    /* 0N shift count -> identity */
  if(sh == INT32_MAX)   sh = 31;     /* 0I -> treat as rotate-by-31 */
  if(sh == INT32_MIN+1) sh = 1;      /* -0I -> treat as rotate-by-1 */
  return raw_rot32(v, sh);
}

K rot_(K a, K x) {
  K r=0;
  i32 *pri,*pai,*pxi;

  if(s(a)||s(x)) return KERR_TYPE;
  if(ta<=0 && tx<=0 && na!=nx) return KERR_LENGTH;

  switch(ta) {
  case 1:
    switch(tx) {
    case  1: r=t(1,(u32)safe_rot(ik(a),ik(x))); break;
    case -1: PRI(nx); PXI; i(nx,pri[i]=safe_rot(ik(a),pxi[i])); break;
    case  0: r=irecur2(rot_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case -1:
    switch(tx) {
    case  1: PRI(na); PAI; i(na,pri[i]=safe_rot(pai[i],ik(x))); break;
    case -1: PRI(na); PAI; PXI; i(na,pri[i]=safe_rot(pai[i],pxi[i])); break;
    case  0: r=avdo(t(4,st(0xc7,"rot")),k_(a),k_(x),"'"); break;
    default: r=KERR_TYPE;
    } break;
  case 0:
    switch(tx) {
    case  1: r=irecur2(rot_,a,x); break;
    case -1: r=avdo(t(4,st(0xc7,"rot")),k_(a),k_(x),"'"); break;
    case  0: r=irecur2(rot_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  default: r=KERR_TYPE;
  }
  return knorm(r);
}

static inline i32 raw_shift32(i32 v, i32 sh) {
  u64 u = (u32)v;
  if(sh >= 0) u <<= (sh & 31);
  else u >>= ((-sh) & 31);
  return (i32)(u32)u;
}

static inline i32 safe_shift(i32 v, i32 sh) {
  if(sh == INT32_MIN)   return v;   /* 0N shift count -> identity */
  if(sh == INT32_MAX)   return (v & 1) ? INT32_MIN : 0;  /* 0I shift count */
  if(sh == INT32_MIN+1) return 0;   /* -0I shift count -> always 0 */
  return raw_shift32(v, sh);
}

K shift_(K a, K x) {
  K r=0;
  i32 *pri,*pai,*pxi;

  if(s(a)||s(x)) return KERR_TYPE;
  if(ta<=0 && tx<=0 && na!=nx) return KERR_LENGTH;

  switch(ta) {
  case 1:
    switch(tx) {
    case  1: r=t(1,(u32)safe_shift(ik(a),ik(x))); break;
    case -1: PRI(nx); PXI; i(nx,pri[i]=safe_shift(ik(a),pxi[i])); break;
    case  0: r=irecur2(shift_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case -1:
    switch(tx) {
    case  1: PRI(na); PAI; i(na,pri[i]=safe_shift(pai[i],ik(x))); break;
    case -1: PRI(na); PAI; PXI; i(na,pri[i]=safe_shift(pai[i],pxi[i])); break;
    case  0: r=irecur2(shift_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case 0:
    switch(tx) {
    case  1: r=irecur2(shift_,a,x); break;
    case -1: r=avdo(t(4,st(0xc7,"shift")),k_(a),k_(x),"'"); break;
    case  0: r=irecur2(shift_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  default: r=KERR_TYPE;
  }
  return knorm(r);
}

static K crypt_(char*(*f)(const char*,size_t,unsigned char*,unsigned char*,size_t*), K a, K x) {
  K r=0,ee,iv=0,key=0,*prk,*pak,*pxk,p;
  char *s=0,*e=0,*pxc,**pxs;
  size_t m;
  if(!a||!x) return KERR_TYPE;
  if(s(a)||s(x)) return KERR_TYPE;
  if(ta) return KERR_TYPE;
  if(na!=2) return KERR_LENGTH;
  PAK;
  i(na,if(T(pak[i])!=-1) return KERR_TYPE)
  if(n(pak[0])!=16) return kerror("iv");
  if(n(pak[1])!=32) return kerror("key");
  if(tx!=-3&&tx!=4&&tx!=-4&&tx!=0) return KERR_TYPE;
  iv=ci(pak[0]); if(E(iv)) return iv;
  key=ci(pak[1]); if(E(key)) return key;
  switch(tx) {
  case -3:
    PXC;
    e=f(pxc,nx,(u8*)px(iv),(u8*)px(key),&m);
    if(!e) { ee=KERR_DOMAIN; goto cleanup; }
    r=tnv(3,m,e);
    break;
  case  4:
    pxc=sk(x);
    e=f(pxc,strlen(pxc),(u8*)px(iv),(u8*)px(key),&m);
    if(!e) { ee=KERR_DOMAIN; goto cleanup; }
    r=tnv(3,m,e);
    break;
  case -4:
    PRK(nx);
    PXS;
    for(u64 i=0;i<nx;++i) {
      e=f(pxs[i],strlen(pxs[i]),(u8*)px(iv),(u8*)px(key),&m);
      if(!e) { ee=KERR_DOMAIN; goto cleanup; }
      p=tnv(3,m,e);
      prk[i]=p;
    }
    break;
  case  0:
    PRK(nx); PXK;
    i(nx,prk[i]=crypt_(f,a,pxk[i]);if(E(prk[i])){ ee=prk[i]; goto cleanup; });
    break;
  default: r=KERR_TYPE;
  }
goto noerr;
cleanup:
  _k(r); r=ee;
noerr:
  xfree(s);
  _k(iv); _k(key);
  return r;
}
K encrypt_(K a, K x) { return crypt_(aes256e,a,x); }
K decrypt_(K a, K x) { return crypt_(aes256d,a,x); }

static K serialize(K x, char **b, char **v, i32 *n, i32 *m) {
  K p;
  char **pxs,*pxc,cc;
  double *pxf;
  static int d;
  if(!x) return KERR_TYPE;
  if(++d>maxr) { --d; return KERR_STACK; }
  i32 t=tx,st=s(x),si=sizeof(i32),sd=sizeof(double),sn=sizeof(u64),dv,ii,*pxi;
  *n+=3*sizeof(i32);
  K *pxk;
  switch(t) {
  case  1: *n+=si; break;
  case  2: *n+=sd; break;
  case  3: *n+=si; break;
  case  4: *n+=1+strlen(sk(x)); break;
  case  6: case 10: break;
  case -4: PXS; *n+=sn; i(nx,*n+=1+strlen(pxs[i])); break;
  case -3: *n+=sn; *n+=nx; break;
  case -2: *n+=sn; *n+=nx*sd; break;
  case -1: *n+=sn; *n+=nx*si; break;
  case  0: *n+=sn; break;
  default: --d; return KERR_TYPE;
  }
  dv=*v-*b;
  while(dv+*n>*m) { *m<<=1; *b=xrealloc(*b,*m); *v=*b+dv; dv=*v-*b; }
  memcpy(*v,&t,si); *v+=si;
  memcpy(*v,&st,si); *v+=si;
  memset(*v,0,si); *v+=si; /* pad to 8 byte alignment for double */
  switch(t) {
  case  1: ii=ik(x); memcpy(*v,&ii,si); *v+=si; break;
  case  2: memcpy(*v,&fk(x),sd); *v+=sd; break;
  case  3: cc=ck(x); memcpy(*v,&cc,1); *v+=1; break;
  case  4: pxc=sk(x); memcpy(*v,pxc,1+strlen(pxc)); *v+=1+strlen(pxc); break;
  case  6: case 10: break;
  case -4: PXS; memcpy(*v,&nx,sn); *v+=sn; i(nx,memcpy(*v,pxs[i],1+strlen(pxs[i]));*v+=1+strlen(pxs[i])); break;
  case -3: PXC; memcpy(*v,&nx,sn); *v+=sn; memcpy(*v,pxc,nx); *v+=nx; break;
  case -2: PXF; memcpy(*v,&nx,sn); *v+=sn; memcpy(*v,pxf,nx*sd); *v+=nx*sd; break;
  case -1: PXI; memcpy(*v,&nx,sn); *v+=sn; memcpy(*v,pxi,nx*si); *v+=nx*si; break;
  case  0: PXK;
    if(0xc3==s(x))  {
      /* don't serialize lambda scope or parse result */
      memcpy(*v,&nx,sizeof(u64)); *v+=sizeof(u64);;
      i(nx,p=serialize((i==1||i==2)?null:pxk[i],b,v,n,m); if(E(p)) { --d; return p;})
    }
    else {
      memcpy(*v,&nx,sn); *v+=sn;
      i(nx,p=serialize(pxk[i],b,v,n,m); if(E(p)) { --d; return p;})
    }
    break;
  default: --d; return KERR_TYPE;
  }
  --d;
  return null;
}

static K deserialize(char **b,u64 *m) {
  K r=0,*prk,p;
  i32 t=0,st=0,*pri,ii;
  u64 si=sizeof(i32),sd=sizeof(double),sn=sizeof(u64),n=*m,count;
  char cc,**prs,*prc;
  double ff,*prf;
  if(n<si*3) return KERR_LENGTH;
  memcpy(&t,*b,si); *b+=si;
  memcpy(&st,*b,si); *b+=si;
  *b+=si; /* pad to 8 byte alignment for double */
  n-=3*si;
  switch(t) {
  case  1: if(n<si) return KERR_LENGTH; memcpy(&ii,*b,si); r=t(1,(u32)ii); *b+=si; break;
  case  2: if(n<sd) return KERR_LENGTH; memcpy(&ff,*b,sd); r=t2(ff); *b+=sd; break;
  case  3: if(n<1) return KERR_LENGTH; memcpy(&cc,*b,1); r=t(3,(u8)cc); *b+=1; break;
  case  4: if(n<1+strlen(*b)) return KERR_LENGTH; r=t(4,sp(*b)); *b+=1+strlen(*b); break;
  case  6: r=null; break;
  case 10: r=inull; break;
  case -4:
    if(n<sn) return KERR_LENGTH;
    memcpy(&count,*b,sn);
    *b+=sn; n-=sn;
    PRS(count);
    i(nr,size_t len=strnlen(*b,n); if(n<len) return KERR_LENGTH; prs[i]=sp(*b); n-=1+len; *b+=1+len) break;
  case -3:
    if(n<sn) return KERR_LENGTH;
    memcpy(&count,*b,sn);
    *b+=sn; n-=sn;
    PRC(count);
    if(n<nr) return KERR_LENGTH;
    memcpy(prc,*b,nr); *b+=nr; break;
  case -2:
    if(n<sn) return KERR_LENGTH;
    memcpy(&count,*b,sn);
    *b+=sn; n-=sn;
    PRF(count);
    if(n<nr*sd) return KERR_LENGTH;
    memcpy(prf,*b,nr*sd); *b+=nr*sd; break;
  case -1:
    if(n<sn) return KERR_LENGTH;
    memcpy(&count,*b,sn);
    *b+=sn; n-=sn;
    PRI(count);
    if(n<nr*si) return KERR_LENGTH;
    memcpy(pri,*b,nr*si); *b+=nr*si; break;
  case  0:
    if(n<sn) return KERR_LENGTH;
    memcpy(&count,*b,sn);
    *b+=sn; n-=sn;
    PRK(count);
    i(nr,p=deserialize(b,&n); if(E(p)) { _k(r); return p; } prk[i]=p)
    break;
  default: return KERR_TYPE;
  }
  *m=n;
  return r|(K)st<<48;
}

K bd_(K x) {
  K r=0,t;
  i32 n=4,m=4;
  char *b=xcalloc(m,1),*v=b,h[4]={2,0,0,0};
  memcpy(v,h,4); v+=4;
  t=serialize(x,&b,&v,&n,&m);
  if(E(t)) { xfree(b); return t; }
  r=tnv(3,(i32)(v-b),b);
  return r;
}

K db_(K x) {
  K r;
  char h[4],*b,*pxc;
  u64 n;
  if(tx!=-3) return KERR_TYPE;
  if(4>nx) return KERR_LENGTH;
  pxc=px(x);
  memcpy(h,pxc,4);
  if(h[0]!=2) {
    fprintf(stderr,"error: db_(): invalid serialization version\n");
    return KERR_TYPE;
  }
  if(nx<4) return KERR_LENGTH;
  b=4+pxc;
  n=nx-4;
  r=deserialize(&b,&n);
  return r;
}

K hb_(K x) {
  K r=0;
  char *prc,*pxc;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  //case  3: PRC(2); sprintf(prc,"%02x",ck(x)); break; /* breaks round trip */
  case -3: PRC(1+2*nx); PXC; i(nx,sprintf(prc,"%02x",(u8)pxc[i]);prc+=2); nr--; break;
  case  0: r=irecur1(hb_,x); break;
  default: r=KERR_TYPE;
  }
  return r;
}

K bh_(K x) {
  K r=0;
  char b[5],*prc,*pxc;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case -3:
    if(!nx) return tn(3,0);
    PRC(1+nx/2); PXC;
    if(nx==1) { sprintf(b,"0x0%c",pxc[0]); *prc=strtol(b,0,0); return r; }
    if(nx%2) { sprintf(b,"0x0%c",pxc[0]); *prc++=strtol(b,0,0); prc++; }
    i(nx/2, sprintf(b,"0x%c%c",pxc[0],pxc[1]); *prc++=strtol(b,0,0); pxc+=2; )
    nr--;
    break;
  case  0: r=irecur1(bh_,x); break;
  default: r=KERR_TYPE;
  }
  return r;
}

K zb_(K x) {
  K r;
  char *b,*pb,*pxc;
  size_t n;
  if(s(x)) return KERR_TYPE;
  if(tx!=-3) return KERR_TYPE;
  PXC;
  LZW *z=lzwc(pxc,nx);
  n=z->i/8+(z->i%8?1:0);
  size_t header = 1+3*sizeof(size_t);
  size_t total = header + n;
  b=xmalloc(sizeof(LZW)+n+1);
  pb=b;
  *b++=z->v;
  memcpy(b,&z->i,sizeof(size_t)); b+=sizeof(size_t);
  memcpy(b,&z->n,sizeof(size_t)); b+=sizeof(size_t);
  memcpy(b,&z->c,sizeof(size_t)); b+=sizeof(size_t);
  memcpy(b,z->b,n);
  lzwfree(z);
  r=tnv(3,total,pb);
  ((char*)px(r))[nr]=0; /* zero terminate */
  return r;
}

K bz_(K x) {
  K r;
  LZW *p,*q;
  char *pxc;
  size_t n;
  if(s(x)) return KERR_TYPE;
  if(tx!=-3) return KERR_TYPE;
  if(nx<1+sizeof(size_t)*3) return KERR_LENGTH;
  PXC;
  p=lzwnew();
  p->v=*pxc++;
  memcpy(&p->i,pxc,sizeof(size_t)); pxc+=sizeof(size_t);
  memcpy(&p->n,pxc,sizeof(size_t)); pxc+=sizeof(size_t);  // read it, but don't trust it
  memcpy(&p->c,pxc,sizeof(size_t)); pxc+=sizeof(size_t);

  // derive the byte count from i (bits) and clamp to actual payload
  size_t n_from_i = (p->i + 7) >> 3;
  size_t payload  = nx - (1 + 3*sizeof(size_t));
  if (n_from_i > payload) { lzwfree(p); return KERR_LENGTH; }
  p->n = n = n_from_i;  // OVERRIDE stream 'n'

  if (n > 32) p->b = xrealloc(p->b, n);
  memcpy(p->b, pxc, n);

  size_t total_bits = n * 8;
  size_t max_codes  = total_bits / 9 + 8; // slack for width bumps
  if (p->c == 0 || p->c > max_codes) p->c = max_codes;
  // require at least one 9-bit code worth of bits
  //if (total_bits < 9) { _k(x); lzwfree(p); return KERR_DOMAIN; }

  q=lzwd(p);
  lzwfree(p);
  if(!q) return KERR_DOMAIN;
  r=tnv(3,q->n,q->b);
  ((char*)px(r))[nr]=0; /* zero terminate */
  xfree(q);
  return r;
}

K md5_(K x) {
  K r=0,e,*prk,*pxk;
  char *prc,*pxc,**pxs;
  if(s(x)) return KERR_TYPE;
  if(tx==0) { PXK; i(nx,if(T(pxk[i])!=-3) return KERR_TYPE) }
  switch(tx) {
  case -3: PRC(32); PXC; md5(prc,(u8*)pxc,nx); break;
  case  4: PRC(32); pxc=sk(x); md5(prc,(u8*)pxc,strlen(pxc)); break;
  case -4: PRK(nx); PXS; i(nx,prk[i]=md5_(t(4,pxs[i]));EC(prk[i])) break;
  case  0: r=irecur1(md5_,x); break;
  default: r=KERR_TYPE;
  }
  return r;
cleanup:
  _k(r);
  return e;
}

K sha1_(K x) {
  K r=0,e,*prk,*pxk;
  char *prc,*pxc,**pxs;
  if(s(x)) return KERR_TYPE;
  if(tx==0) { PXK; i(nx,if(T(pxk[i])!=-3) return KERR_TYPE) }
  switch(tx) {
  case -3: PRC(40); PXC; sha1(prc,(u8*)pxc,nx); break;
  case  4: PRC(40); pxc=sk(x); sha1(prc,(u8*)pxc,strlen(pxc)); break;
  case -4: PRK(nx); PXS; i(nx,prk[i]=sha1_(t(4,pxs[i]));EC(prk[i])) break;
  case  0: r=irecur1(sha1_,x); break;
  default: r=KERR_TYPE;
  }
  return r;
cleanup:
  _k(r);
  return e;
}

K sha2_(K x) {
  K r=0,e,*prk,*pxk;
  char *prc,*pxc,**pxs;
  if(s(x)) return KERR_TYPE;
  if(tx==0) { PXK; i(nx,if(T(pxk[i])!=-3) return KERR_TYPE) }
  switch(tx) {
  case -3: PRC(64); PXC; sha2(prc,(u8*)pxc,nx); break;
  case  4: PRC(64); pxc=sk(x); sha2(prc,(u8*)pxc,strlen(pxc)); break;
  case -4: PRK(nx); PXS; i(nx,prk[i]=sha2_(t(4,pxs[i]));EC(prk[i])) break;
  case  0: r=irecur1(sha2_,x); break;
  default: r=KERR_TYPE;
  }
  return r;
cleanup:
  _k(r);
  return e;
}

static char* ge(char *k) {
#ifdef _WIN32
  char *p; _dupenv_s(&p,0,k); return p;
#else
  return getenv(k);
#endif
}

K getenv_(K x) {
  K r=null,e,*prk;
  char *p,**pxs;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case -3: p=ge((char*)px(x)); if(p)r=tnv(3,strlen(p),xstrndup(p,strlen(p))); break;
  case  4: p=ge(sk(x)); if(p)r=tnv(3,strlen(p),xstrndup(p,strlen(p))); break;
  case -4: PRK(nx); PXS; i(nx,prk[i]=getenv_(t(4,pxs[i])); EC(prk[i])) break;
  case  0: r=irecur1(getenv_,x); break;
  default: r=KERR_TYPE;
  }
  return r;
cleanup:
  _k(r);
  return e;
}

static void se(char *k, char *v) {
#ifdef _WIN32
  _putenv_s(k,v);
#else
  setenv(k,v,1);
#endif
}

K setenv_(K a, K x) {
  K e,*pak,*pxk;
  char *p,*q,**pas,**pxs;
  if(s(a)||s(x)) return KERR_TYPE;
  if((ta==0||ta==-4)&&(tx==0||tx==-4)&&(na!=nx)) return KERR_LENGTH;
  switch(ta) {
  case -3:
    switch(tx) {
    case -3: p=xstrndup(px(a),na); q=xstrndup(px(x),nx); break;
    case  4: p=xstrndup(px(a),na); q=xstrndup(sk(x),strlen(sk(x))); break;
    default: return KERR_TYPE;
    }
    se(p,q);
    xfree(p); xfree(q);
    break;
  case  4:
    switch(tx) {
    case -3: p=xstrndup(sk(a),strlen(sk(a))); q=xstrndup((char*)px(x),nx); break;
    case  4: p=xstrndup(sk(a),strlen(sk(a))); q=xstrndup(sk(x),strlen(sk(x))); break;
    default: return KERR_TYPE;
    }
    se(p,q);
    xfree(p); xfree(q);
    break;
  case -4:
    switch(tx) {
    case -4: PAS; PXS; i(nx,se(pas[i],pxs[i])); break;
    case  0: PAS; PXK; i(na,EC(setenv_(t(4,pas[i]),pxk[i]))); break;
    default: return KERR_TYPE;
    } break;
  case  0:
    switch(tx) {
    case -4: PAK; PXS; i(nx,EC(setenv_(pak[i],t(4,pxs[i])))); break;
    case  0: PAK; PXK; i(na,EC(setenv_(pak[i],pxk[i]))); break;
    default: return KERR_TYPE;
    } break;
  default: return KERR_TYPE;
  }
  return null;
cleanup:
  return e;
}

K in_(K a, K x) {
  u64 n;
  if(s(x)) return KERR_DOMAIN;
  if(tx>0) return KERR_DOMAIN;
  n=nx;
  K t=k(14,k_(x),k_(a));     /* x?a */
  if(E(t)) return t;
  return k(7,t,t(1,(u32)n)); /* t<nx */
}

K dvl_(K a, K x) {
  u64 n;
  if(s(a)||s(x)) return KERR_DOMAIN;
  if(ta!=6 && ta>0) return KERR_DOMAIN;
  if(tx>0) return KERR_DOMAIN;
  n=nx;
  K t=k(78,k_(x),k_(a)); /* x?/a */
  if(E(t)) return t;
  t=k(9,t(1,(u32)n),t);  /* (#nx)=t */
  if(E(t)) return t;
  t=k(5,0,t);            /* &t */
  if(E(t)) return t;
  t=k(13,k_(a),t);       /* a@t */
  return t;
}

K exit__(K x) {
  if(tx!=1) return KERR_TYPE;
  scope_free(cs);
  scope_free(gs);
  scope_free(ks);
  scope_free_all();
  dfree(C);
  dfree(Z);
  sf();
  mfree();
  kexit();
  pexit();
#ifdef FUZZING
  exit(0);
#endif
  exit(ik(x));
}

K exit_(K x) {
  if(tx!=1) return KERR_TYPE;
  EXIT=x;
  return k_(x);
}

K dir_(K x) {
  char *s,*p,*ss,*pp,*sr,*skd=sk(D),*psx;
  K d,q,t=0,es,fs;
  size_t n;

  if(ecount) return kerror("nonce");

  if(x!=null) {
    psx=sk(x);
    n=strlen(psx);
    if(n==2&&psx[0]=='.'&&psx[1]!='k') return kerror("reserved");
    if(n>2&&psx[0]=='.'&&psx[1]!='k'&&psx[2]=='.') return kerror("reserved");
    if(n==1&&gs==ks) return kerror("reserved");
  }

  if(x==null) printf("%s\n",skd);
  else if(x==t(4,sp("^"))) { /* go up one level */
    if(gs!=ks) {
      K *pgs=px(gs);
      _k(cs); cs=k_(pgs[0]);
      _k(gs); gs=k_(pgs[0]);
      pgs=px(gs);
      D=pgs[2];
    }
  }
  else {
    if(!scope_vktp(psx)) return kerror("domain");
    if(psx[0]=='.') s=xstrdup(psx);
    else { s=xmalloc(strlen(skd)+strlen(psx)+2); sprintf(s,"%s.%s",skd,psx); }
    size_t cap=strlen(s)+2;
    if(cap>255) { xfree(s); return kerror("length"); }
    ss=xstrdup(s); /* set global D at the end */
    pp=xmalloc(cap); /* incremental path for scope->k */
    es=k_(ks);         /* start at top of ktree */
    K *pes=px(es);
    d=k_(pes[1]);
    p=strtok_r(s,".",&sr);
    pp[0]=0;
    do {
      snprintf(pp+strlen(pp),cap-strlen(pp),"%s",".");
      snprintf(pp+strlen(pp),cap-strlen(pp),"%s",p);
      q=dget(d,sp(p));
      if(!q) { /* ktree path does not exist */
        t=dnew();
        (void)dset(d,sp(p),t);
        if(!(fs=scope_find(pp))) { _k(es); es=scope_newk(es,t(4,sp(pp))); }
        else { _k(es); es=k_(fs); }
        _k(d); d=t;
      }
      else if(0x80==s(q)) { /* ensure scope path exists */
        _k(d); d=q;
        t=scope_get(es,t(4,sp(p)));
        if(E(t)) { /* nope */
          if(!(fs=scope_find(pp))) { _k(es); es=scope_newk(es,t(4,sp(pp))); }
          else { _k(es); es=k_(fs); }
        }
        else if(!(fs=scope_find(pp))) {
          _k(es); es=scope_newk(es,t(4,sp(pp)));
          K *pes=px(es);
          _k(pes[1]);
          pes[1]=k_(q);
        }
        else { _k(es); es=k_(fs); }
        _k(t);
      }
      else { _k(q); _k(d); xfree(s); xfree(ss); xfree(pp); return kerror("type"); }
    } while((p=strtok_r(0,".",&sr)));
    _k(es);
    _k(d);
    _k(gs); gs=k_(es);
    _k(cs); cs=k_(es);
    D=t(4,sp(ss)); /* set global D */
    xfree(s); xfree(ss); xfree(pp);
  }
  return null;
}
