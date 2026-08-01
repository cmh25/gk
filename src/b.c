#include "b.h"
#include "endian.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#ifdef _WIN32
#include "systime.h"
#include "win_unistd.h"
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
#include "nt.h"
#include "repl.h"
#include "ipc.h"
#include "tmr.h"
#include "watch.h"

#ifdef _WIN32
#define strtok_r strtok_s
#endif

static K vlookup(K x) {
  K r=scope_get(cs,x);
  return r?r:KERR_VALUE;
}

/* tnv with size_t input; returns wsfull if n exceeds the vector length limit. */
static inline K tnc(i32 t, u64 n, void *v) {
  if(n>=(u64)VMAX) return KERR_WSFULL;
  return tnv(t,(i64)n,v);
}

const binfo2 BDYAD[] = {
  {&R_DRAW,draw},{&R_VS,vs},{&R_SV,sv},{&R_ATAN2,atan2_},{&R_DIV,div_},
  {&R_AND,and_},{&R_OR,or_},{&R_XOR,xor_},{&R_HYPOT,hypot_},{&R_GCD,gcd_},
  {&R_LCM,lcm_},{&R_MODINV,modinv_},{&R_ROT,rot_},{&R_SHIFT,shift_},
  {&R_DOT,dotp},{&R_MUL,mul_},{&R_AT,at_},{&R_SM,sm},{&R_SS,ss},{&R_LSQ,lsq_},
  {&R_ENCRYPT,encrypt_},{&R_DECRYPT,decrypt_},{&R_RENAME,rename_},
  {&R_SETENV,setenv_},{&R_IN,in_},{&R_LIN,lin_},{&R_DVL,dvl_},{&R_BIN,bin_},{&R_BINL,binl_},
};
const binfo1 BMONAD[] = {
  {&R_SLEEP,sleep_},{&R_IC,ic},{&R_CI,ci},{&R_DJ,dj},{&R_JD,jd},{&R_LT,lt},
  {&R_LOG,log_},{&R_EXP,exp_},{&R_ABS,abs_},{&R_SQR,square},{&R_SQRT,sqrt_},
  {&R_FLOOR,floor_},{&R_CEIL,ceil_},{&R_SIN,sin_},{&R_COS,cos_},{&R_TAN,tan_},
  {&R_ASIN,asin_},{&R_ACOS,acos_},{&R_ATAN,atan_},{&R_SINH,sinh_},
  {&R_COSH,cosh_},{&R_TANH,tanh_},{&R_ERF,erf_},{&R_ERFC,erfc_},
  {&R_GAMMA,gamma_},{&R_LGAMMA,lgamma_},{&R_RINT,rint_},{&R_TRUNC,trunc_},
  {&R_NOT,not__},{&R_KV,kv_},{&R_VK,vk_},{&R_VAL,val_},{&R_DEL,del_},
  {&R_BD,bd_},{&R_DB,db_},{&R_HB,hb_},{&R_BH,bh_},{&R_ZB,zb_},{&R_BZ,bz_},
  {&R_MD5,md5_},{&R_SHA1,sha1_},{&R_SHA2,sha2_},{&R_SVD,svd_},{&R_LU,lu_},
  {&R_QR,qr_},{&R_LDU,ldu_},{&R_RREF,rref_},{&R_DET,det_},{&R_MAG,mag_},
  {&R_PRIME,prime_},{&R_FACTOR,factor_},{&R_GETENV,getenv_},{&R_EXIT,exit_},
  {&R_TIMER,timer_},{&R_WATCH,watch_},
};
const int NBDYAD=sizeof(BDYAD)/sizeof(*BDYAD);
const int NBMONAD=sizeof(BMONAD)/sizeof(*BMONAD);
int bi_lookup(char *nm, u64 *sub) {
  for(int i=0;i<NBDYAD;i++)  if(nm==*BDYAD[i].nm)  { *sub=0xc7; return i; }
  for(int i=0;i<NBMONAD;i++) if(nm==*BMONAD[i].nm) { *sub=0xc6; return i; }
  return -1;
}
static K biv(char *nm) { u64 sub; int i=bi_lookup(sp(nm),&sub); return t(1,st(sub,i)); }

K builtin(K f, K a, K x) {
  K r=0,t;
  char *cf,*a2;
  static i32 d=0;

  if(0x85==s(a)||0x85==s(x)) { _k(a); _k(x); return KERR_TYPE; }
  if(s(a)==0x40) { a=vlookup(a); if(E(a)) { _k(x); return a; } }
  if(s(x)==0x40) { x=vlookup(x); if(E(x)) { _k(a); return x; } }

  if(++d>maxr || (!(d&7)&&stack_low())) { --d; _k(a); _k(x); return KERR_STACK; }

  if(1==T(f)) {
    /* int-index builtin: 0xc7 dyad / 0xc6 monad, ik(f) = table index */
    u32 idx=(u32)ik(f);
    if(0xc7==s(f)) {
      if(!x) r=KERR_TYPE;
      else if(!a) { /* project -> 0xd9(verb,(x,)) wrapper (see legacy note) */
        K verb_k=t(1,st(0xc7,idx));
        t=kcp(x);
        if(E(t)) { --d; _k(a); _k(x); return t; }
        K args=tn(0,1); K *pa=px(args);
        pa[0]=t; args=st(0x81,args);
        r=wrap_proj(verb_k, args);
      }
      else r=BDYAD[idx].f(a,x);
    }
    else { /* 0xc6 monad */
      if(!x) r=KERR_TYPE;
      else r=BMONAD[idx].f(x);
    }
    --d; _k(a); _k(x); return r;
  }

  /* type-4 symbol verbs: file verbs (0xcc/0xcd) and do/while/if.  Adverb
     gluing is fully gone (file verbs de-glued in lexer; p.c routes postfix
     adverbs through avdo via pgreduce_ cases 0xcc/0xcd), so the name never
     contains '/\\ and no split is needed -- just re-intern for dispatch. */
  cf=sk(f);
  if(255<strlen(cf)) { --d; _k(a); _k(x); return KERR_VALUE; }
  a2=sp(cf);
  if(0xcc==s(f)) {
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
      K verb_k=t(4,st(0xcd,sp(a2)));
      K args=tn(0,1); K *pa=px(args);
      pa[0]=x;
      args=st(0x81,args);
      r=wrap_proj(verb_k, args);
      x=0; /* args plist took ownership of x */
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
  i32 n=1,*pai;
  i64 *paj;
  if(!a||!x) { e=KERR_TYPE; goto cleanup; }
  if(s(a)||s(x)) { e=KERR_TYPE; goto cleanup; }
  if(ta==8) { /* long-atom count -> big draw; ? (v.c draw) does i64-count
                 validation and the deal/roll/float-range dispatch */
    r=k(14,tj(jk(a)),k_(x));
    if(r<EMAX) { e=r; r=0; goto cleanup; }
    return r;
  }
  if(ta!=0 && ta!=1 && ta!=-1 && ta!=-8) { e=KERR_TYPE; goto cleanup; }
  /* tx==9 (real): draw[n;1.0e] was the one hole in the roll/deal type grid --
     `0?1.0e` worked (v.c's draw) but the named form type-errored */
  if(tx!=1 && tx!=2 && tx!=8 && tx!=9 && ta!=-1) { e=KERR_TYPE; goto cleanup; }
  switch(ta) {
  case  0: if(na) { e=KERR_TYPE; goto cleanup; } r=k(14,t(1,(u32)n),k_(x)); /* n?x */ break;
  case  1: n=ik(a); r=k(14,t(1,(u32)n),k_(x)); /* n?x */ break;
  case -1: case -8:
    switch(tx) {
    case 1: case 2: case 8: case 9: /* case 9 (real) was missing: a shaped
      real draw draw[2 3;1.0e] type-errored though the scalar-count
      draw[6;1.0e] and 0?1.0e worked */
      if(ta==-1) {
        PAI; i(na,if(pai[i]<0 || pai[i]==INT32_MAX){e=KERR_DOMAIN;goto cleanup;})
      } else {
        PAJ; i(na,if(paj[i]<0 || paj[i]==J_INF){e=KERR_DOMAIN;goto cleanup;})
      }
      p=k(21,3,k_(a)); EC(p); // */a
      q=k(14,p,k_(x)); EC(q); // p?x
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
  K r=0,e,*prk,*pak,*pxk;
  i32 *pri,*pai,*pxi;
  i64 *pxj,*paj,*prj;
  float *pae,*pre;
  double *prf,*paf;
  char *prc,*pac,**prs,**pas;

  if(!a||!x) return KERR_TYPE;
  if(0x80==s(a)) return k(13,k_(a),k_(x));  /* dict: @ already null-fills a missing key */
  if(!s(a)&&4==ta) {                        /* symbol handle: deref, then index */
    /* A handle can deref to another handle (`a:`b;`b:`a`), so this chain is
       runtime data, not syntactic nesting -- a cycle recurses forever.  Bound
       it with the shared depth guard (irecur2's list path already is); a cyclic
       or pathologically long chain trips a stack error instead of overflowing. */
    static int hd=0;
    if(++hd>maxr || (!(hd&7)&&stack_low())) { --hd; return KERR_STACK; }
    K v=vlookup(a);
    if(E(v)) { --hd; return v==KERR_VALUE?at_(null,x):v; }  /* undefined handle -> nul, the identity vector */
    r=at_(v,x); _k(v);
    --hd;
    return r;
  }
  if(!s(a)&&6==ta) return k(13,k_(a),k_(x)); /* nul: the identity vector has no out-of-range */
  if(s(a)||s(x)||aa) return KERR_RANK;
  if(tx==6) return k_(a);
  if(!tx&&!nx) return tn(0,0);

  switch(ta) {
  case -1:
    switch(tx) {
    case  1: PAI; r=t(1,(u32)((ik(x)<0||(u64)(ik(x))>=na)?INT32_MIN:pai[ik(x)])); break;
    case -1: PRI(nx); PAI; PXI; i(nx, pri[i]=(pxi[i]<0||(u64)pxi[i]>=na)?INT32_MIN:pai[pxi[i]]); break;
    case  8: PAI; r=t(1,(u32)((jk(x)<0||(u64)(jk(x))>=na)?INT32_MIN:pai[jk(x)])); break;
    case -8: PRI(nx); PAI; pxj=px(x); i(nx, pri[i]=(pxj[i]<0||(u64)pxj[i]>=na)?INT32_MIN:pai[pxj[i]]); break;
    case  0: r=irecur2(at_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case -2:
    switch(tx) {
    case  1: PAF; r=t2((ik(x)<0||(u64)(ik(x))>=na)?NAN:paf[ik(x)]); break;
    case -1: PRF(nx); PAF; PXI; i(nx, prf[i]=(pxi[i]<0||(u64)pxi[i]>=na)?NAN:paf[pxi[i]]); break;
    case  8: PAF; r=t2((jk(x)<0||(u64)(jk(x))>=na)?NAN:paf[jk(x)]); break;
    case -8: PRF(nx); PAF; pxj=px(x); i(nx, prf[i]=(pxj[i]<0||(u64)pxj[i]>=na)?NAN:paf[pxj[i]]); break;
    case  0: r=irecur2(at_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case -3:
    switch(tx) {
    case  1: PAC; r=t(3,(ik(x)<0||(u64)(ik(x))>=na)?' ':(u8)pac[ik(x)]); break;
    case -1: PRC(nx); PAC; PXI; i(nx, prc[i]=(pxi[i]<0||(u64)pxi[i]>=na)?' ':pac[pxi[i]]); break;
    case  8: PAC; r=t(3,(jk(x)<0||(u64)(jk(x))>=na)?' ':(u8)pac[jk(x)]); break;
    case -8: PRC(nx); PAC; pxj=px(x); i(nx, prc[i]=(pxj[i]<0||(u64)pxj[i]>=na)?' ':pac[pxj[i]]); break;
    case  0: r=irecur2(at_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case -4:
    switch(tx) {
    case  1: PAS; r=t(4,(ik(x)<0||(u64)(ik(x))>=na)?sp(""):pas[ik(x)]); break;
    case -1: PRS(nx); PAS; PXI; i(nx, prs[i]=(pxi[i]<0||(u64)pxi[i]>=na)?sp(""):pas[pxi[i]]); break;
    case  8: PAS; r=t(4,(jk(x)<0||(u64)(jk(x))>=na)?sp(""):pas[jk(x)]); break;
    case -8: PRS(nx); PAS; pxj=px(x); i(nx, prs[i]=(pxj[i]<0||(u64)pxj[i]>=na)?sp(""):pas[pxj[i]]); break;
    case  0: r=irecur2(at_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case -8:
    switch(tx) {
    case  1: PAJ; r=tj((ik(x)<0||(u64)(ik(x))>=na)?J_NULL:paj[ik(x)]); break;
    case -1: PRJ(nx); PAJ; PXI; i(nx, prj[i]=(pxi[i]<0||(u64)pxi[i]>=na)?J_NULL:paj[pxi[i]]); break;
    case  8: PAJ; r=tj((jk(x)<0||(u64)(jk(x))>=na)?J_NULL:paj[jk(x)]); break;
    case -8: PRJ(nx); PAJ; pxj=px(x); i(nx, prj[i]=(pxj[i]<0||(u64)pxj[i]>=na)?J_NULL:paj[pxj[i]]); break;
    case  0: r=irecur2(at_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case -9:
    switch(tx) {
    case  1: PAE; r=te((ik(x)<0||(u64)(ik(x))>=na)?(float)NAN:pae[ik(x)]); break;
    case -1: PRE(nx); PAE; PXI; i(nx, pre[i]=(pxi[i]<0||(u64)pxi[i]>=na)?(float)NAN:pae[pxi[i]]); break;
    case  8: PAE; r=te((jk(x)<0||(u64)(jk(x))>=na)?(float)NAN:pae[jk(x)]); break;
    case -8: PRE(nx); PAE; pxj=px(x); i(nx, pre[i]=(pxj[i]<0||(u64)pxj[i]>=na)?(float)NAN:pae[pxj[i]]); break;
    case  0: r=irecur2(at_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case  0: /* list: index the top level (a conforms over x, never over a) */
    PAK;
    switch(tx) {
    case  1: { i64 v=ik(x); r=(v<0||(u64)v>=na)?null:k_(pak[v]); } break;
    case  8: { i64 v=jk(x); r=(v<0||(u64)v>=na)?null:k_(pak[v]); } break;
    case -1: PRK(nx); PXI; i(nx, prk[i]=(pxi[i]<0||(u64)pxi[i]>=na)?null:k_(pak[pxi[i]])) break;
    case -8: PRK(nx); pxj=px(x); i(nx, prk[i]=(pxj[i]<0||(u64)pxj[i]>=na)?null:k_(pak[pxj[i]])) break;
    case  0: {
      static i32 d=0;
      if(++d>maxr || (!(d&7)&&stack_low())) { --d; r=KERR_STACK; break; }
      PRK(nx); PXK;
      i(nx, e=at_(a,pxk[i]); if(E(e)) { --d; goto cleanup; } prk[i]=e)
      --d;
    } break;
    default: r=KERR_TYPE;
    } break;
  default: r=KERR_TYPE;
  }
  return knorm(r);
cleanup:
  _k(r);
  return e;
}

/* Iterative glob with backtracking to the LAST star only (si/sj): when a
   token mismatches, the last star eats one more subject char and matching
   resumes after it.  One bookmark suffices -- any match reachable by giving
   an earlier star a longer eat is also reachable from the last one.  The
   old version recursed on every subject position per star, which went
   exponential on runs of stars (fuzz: sm["a+;x";"a****...**[\]"] never
   returned); this is O(an*xn) worst case.  Per-token semantics unchanged. */
static i32 sm_(char *a, char *x, i64 an, i64 xn, i64 i, i64 j) {
  i64 j2,si=-1,sj=-1;
  i32 m;
  while(i<an) {
    if(j<xn && x[j]=='*') { sj=++j; si=i; continue; }  /* bookmark; eat zero first */
    m=0; j2=j;
    if(j<xn) {
      if(x[j]=='?') { m=1; j2=j+1; }
      else if(x[j]=='[' && j+2<xn && x[j+2]==']') { /* escaped: [c] literal (incl []]) */
        m=a[i]==x[j+1]; j2=j+3;
      }
      else if(x[j]=='[' && strchr(x+j,']')) {  /* unterminated [ stays literal */
        j2=j+1;
        if(x[j2]=='^') {
          m=1; for(++j2;j2<xn&&x[j2]!=']';j2++) if(a[i]==x[j2]) { m=0; break; }
        }
        else if(j2+2<xn && x[j2+1]=='-') {                     /* [p-q] range, as in ss_ */
          char p=x[j2]; char q=x[j2+2]; j2+=2;
          m=a[i]>=p&&a[i]<=q;
        }
        else {
          m=0; for(;j2<xn&&x[j2]!=']';j2++) if(a[i]==x[j2]) { m=1; break; }
        }
        while(j2<xn&&x[j2]!=']') ++j2;
        ++j2;                                                  /* past the ']' */
      }
      else { m=a[i]==x[j]; j2=j+1; }
    }
    if(m) { ++i; j=j2; }
    else if(sj>=0) { j=sj; i=++si; }
    else return 0;
  }
  while(j<xn&&x[j]=='*') ++j;   /* only * matches "" */
  return j>=xn;
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
    case  0: r=avdo(biv("sm"),k_(a),k_(x),"'"); break;
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

static i64 ss_(char *a, char *x, i64 an, i64 xn, i64 i) {
  i64 m=1,j,k=i;
  for(j=0;j<xn;j++) {
    if(k==an) break;
    if(x[j]=='?') { }
    else if(x[j]=='[' && j+2<xn && x[j+2]==']') { /* escaped: [c] literal */
      ++j;
      if(a[k]!=x[j]) { m=0; break; }
      ++j;
    }
    else if(x[j]=='[' && strchr(x+j,']')) {  /* unterminated [ stays literal */
      if(x[++j]=='^') {
        m=1; for(++j;j<xn&&x[j]!=']';j++) if(a[k]==x[j]) { m=0; break; }
      }
      else if(j+2<xn && x[j+1]=='-') {              /* [p-q] range */
        char p=x[j]; char q=x[j+2]; j+=2;
        m=a[k]>=p&&a[k]<=q;
      }
      else {
        m=0; for(;j<xn&&x[j]!=']';j++) if(a[k]==x[j]) { m=1; break; }
      }
      while(j<xn&&x[j]!=']') ++j;
      if(!m) break;
    }
    else if(a[k]!=x[j]) { m=0; break; }
    ++k;
  }
  return m&&j==xn ? k-i : 0;
}

K ss(K a, K x) {
  K r=0;
  u64 i,n=0;
  i64 m,s,*prj;     /* match positions accumulated as i64 (narrowed to int below
                       when #a<=2^31, so small-haystack output stays an int vector) */
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
      PRJ(2);
      s0[0]=ck(x); s0[1]=0;
      s=1;
      for(i=0;i<na;i++) {
        if((m=ss_(pac,s0,na,s,i))) {
          if(n==nr) { r=kresize(r,n<<1); prj=px(r); }
          prj[n++]=i;
          i+=m-1;
        }
      }
      nr=n;
      break;
    case  4:
      PRJ(2);
      pxc=sk(x);
      s=strlen(pxc);
      for(i=0;i<na;i++) {
        if((m=ss_(pac,pxc,na,s,i))) {
          if((i==0||!isalpha((unsigned char)pac[i-1])) && ((i+m<na&&!isalpha((unsigned char)pac[i+m]))||i+m==na)) {
            if(n==nr) { r=kresize(r,n<<1); prj=px(r); }
            prj[n++]=i;
            i+=m-1;
          }
        }
      }
      nr=n;
      break;
    case -3:
      PRJ(2);
      PXC;
      for(i=0;i<na;i++) {
        if((m=ss_(pac,pxc,na,nx,i))) {
          if(n==nr) { r=kresize(r,n<<1); prj=px(r); }
          prj[n++]=i;
          i+=m-1;
        }
      }
      nr=n;
      break;
    case -4: r=avdo(biv("ss"),k_(a),k_(x),"/"); break;
    case  0: r=avdo(biv("ss"),k_(a),k_(x),"/"); break;
    default: r=KERR_TYPE;
    }
    /* positions fit i32 when #a<=2^31: narrow long result to an int vector so
       small-haystack output is unchanged (an int vector). big haystack keeps
       long positions. */
    if(T(r)==-8 && na<=BIGV) {
      K r2=tn(1,nr); i32 *p2=px(r2); prj=px(r);
      i(nr,p2[i]=(i32)prj[i]);
      _k(r); r=r2;
    }
    break;
  case  0:
    switch(tx) {
    case  3: r=irecur2(ss,a,x); break;
    case  4: r=irecur2(ss,a,x); break;
    case -3: r=avdo(biv("ss"),k_(a),k_(x),"\\"); break;
    case -4: r=avdo(biv("ss"),k_(a),k_(x),"'"); break;
    case  0: r=avdo(biv("ss"),k_(a),k_(x),"'"); break;
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

/* A radix must be an ordinary integer, not one of the null/infinity
   sentinels. Keep this check shared by vs/sv and by scalar/mixed radices so
   promotion cannot turn an i32 domain error into an accepted i64 operation. */
static int radix_has_sentinel(K a) {
  if(ta==1) {
    i32 v=ik(a);
    return v==INT32_MAX || v==INT32_MIN || v==INT32_MIN+1;
  }
  if(ta==8) {
    i64 v=jk(a);
    return v==J_INF || v==J_NULL || v==J_NINF;
  }
  if(ta==-1) {
    i32 *p=px(a);
    for(u64 i=0;i<na;i++)
      if(p[i]==INT32_MAX || p[i]==INT32_MIN || p[i]==INT32_MIN+1) return 1;
  }
  if(ta==-8) {
    i64 *p=px(a);
    for(u64 i=0;i<na;i++)
      if(p[i]==J_INF || p[i]==J_NULL || p[i]==J_NINF) return 1;
  }
  return 0;
}

/* digit count of unsigned value v in base b (b>=2) */
static u32 vwidthj(u64 v, u64 b) { u32 w=1; while(v>=b) { v/=b; ++w; } return w; }
/* read element j of an int/long vector as i64; widening preserves the three
   sentinel values rather than treating their i32 bit patterns as numbers */
static i64 vreadj(K x, u64 j) { return tx==-8 ? ((i64*)px(x))[j] : ji(((i32*)px(x))[j]); }

/* long base-encode (vs) -- any of base/value is long, result digits long.
   mirrors the int vs_: scalar base x scalar value -> digit vector (empty for
   0); scalar base x vector value -> flipped/padded matrix; vector base ->
   mixed-radix digits. value as a general list is left to the int path. */
static K vsj(K a, K x) {
  if(!(ta==1||ta==8||ta==-1||ta==-8)) return KERR_TYPE;
  if(!(tx==1||tx==8||tx==-1||tx==-8)) return KERR_TYPE;
  if(ta==1 || ta==8) {                       /* scalar base */
    i64 bb = ta==8 ? jk(a) : ji(ik(a));
    if(bb<=1) return KERR_DOMAIN;
    u64 b=(u64)bb;
    if(tx==1 || tx==8) {                     /* scalar value */
      u64 v = tx==8 ? (u64)jk(x) : (u64)ji(ik(x));
      if(v==0) return tn(8,0);               /* vs[b;0] -> empty (matches int) */
      u32 w=vwidthj(v,b);
      K r=tn(8,w); i64 *pr=px(r);
      for(i32 i=(i32)w-1;i>=0;i--) { pr[i]=(i64)(v%b); v/=b; }
      return r;
    }
    u64 nval=nx; i32 w=1;                    /* vector value -> flipped matrix */
    for(u64 j=0;j<nval;j++) { u32 ww=vwidthj((u64)vreadj(x,j),b); if((i32)ww>w) w=ww; }
    K r=tn(0,w); K *pr=px(r);
    for(i32 d=0;d<w;d++) pr[d]=tn(8,nval);
    for(u64 j=0;j<nval;j++) {
      u64 v=(u64)vreadj(x,j);
      for(i32 d=w-1;d>=0;d--) { ((i64*)px(pr[d]))[j]=(i64)(v%b); v/=b; }
    }
    return r;
  }
  /* vector base = mixed radix; scalar value -> digit vector, vector value ->
     matrix (L rows x nval cols), mirroring the int path so long round-trips */
  u64 L=na; if(!L) return KERR_DOMAIN;
  /* place unsigned: the running product of bases overflows i64 for a wide
     mixed radix -- UB -- so accumulate in u64 (bases are >=0 after the domain
     check).  Downstream already reads place through (u64) casts. */
  i64 *base=xmalloc(L*sizeof(i64)); u64 *place=xmalloc(L*sizeof(u64));
  for(u64 i=0;i<L;i++) {
    base[i]=vreadj(a,i);
    if(base[i]<0 || (i && base[i]<1)) { xfree(base); xfree(place); return KERR_DOMAIN; }
  }
  place[L-1]=1;
  for(i32 i=(i32)L-2;i>=0;i--) place[i]=place[i+1]*(u64)base[i+1];
  for(u64 i=0;i<L;i++) if(place[i]==0) { xfree(base); xfree(place); return KERR_DOMAIN; }
  if(tx==1 || tx==8) {                       /* scalar value -> digit vector */
    u64 xx = tx==8 ? (u64)jk(x) : (u64)ji(ik(x));
    K r=tn(8,L); i64 *pr=px(r);
    for(u64 i=0;i<L;i++) {
      u64 pl=(u64)place[i], m=xx/pl; xx-=m*pl;
      pr[i] = base[i] ? (i64)(m%(u64)base[i]) : (i64)m;
    }
    xfree(base); xfree(place);
    return r;
  }
  u64 nval=nx;                               /* vector value -> matrix */
  K r=tn(0,L); K *pr=px(r);
  for(u64 i=0;i<L;i++) pr[i]=tn(8,nval);
  for(u64 j=0;j<nval;j++) {
    u64 xx=(u64)vreadj(x,j);
    for(u64 i=0;i<L;i++) {
      u64 pl=(u64)place[i], m=xx/pl; xx-=m*pl;
      ((i64*)px(pr[i]))[j] = base[i] ? (i64)(m%(u64)base[i]) : (i64)m;
    }
  }
  xfree(base); xfree(place);
  return r;
}

K vs(K a, K x) {
  i32 w,*pai,z;
  K *pxk;
  if(s(a)||s(x)) return KERR_TYPE;
  if(radix_has_sentinel(a)) return KERR_DOMAIN;
  if(ta==8 || ta==-8 || tx==8 || tx==-8) return vsj(a,x);
  if((ta==1&&ik(a)<=1)) return KERR_DOMAIN;
  if(ta==-1) {
    if(!na) return KERR_DOMAIN;
    pai=px(a);
    i(na,if(pai[i]<0) return KERR_DOMAIN)
    i(na,if(i&&pai[i]<1) return KERR_DOMAIN)
  }
  if(tx==0) {
    pxk=px(x);
    i(nx,if(s(pxk[i])||(1!=T(pxk[i])&&-1!=T(pxk[i]))) return KERR_DOMAIN)
  }

  if(ta==1) w=maxw(x,ik(a));
  else if(ta==-1) { w=0; pai=px(a); i(na,z=maxw(x,pai[i]);if(w<z)w=z) }
  else return KERR_TYPE;
  return vs_(a,x,w);
}

static K sv_(K a, K x);
K sv(K a, K x) {
  static i32 d=0;
  if(++d>maxr || (!(d&7)&&stack_low())) { --d; return KERR_STACK; }
  K r=sv_(a,x);
  --d;
  return r;
}
static K sv_(K a, K x) {
  K r=0,e,q=0,p=0,s=0,*pxk,*prk;
  i32 *pxi;
  u32 j;
  double jf,*pxf;
  if(s(a)||s(x)) return KERR_TYPE;
  if(radix_has_sentinel(a)) return KERR_DOMAIN;
  /* long base or long digit-vector with a scalar base: i64 Horner, long
     result (e.g. sv[256j;bytes] builds a 64-bit value without overflow) */
  if((ta==8 || tx==8 || tx==-8) && (ta==1 || ta==8)) {
    if(tx==8) return tj(jk(x));
    if(tx==-1 || tx==-8) {
      i64 base = ta==8 ? jk(a) : ji(ik(a));
      /* Horner in u64: a value that doesn't fit i64 (base decode of a huge
         digit string) overflows -- signed overflow is UB (traps under ubsan),
         so accumulate unsigned, which wraps with defined 2's-complement bits
         identical to the production result. */
      u64 acc = 0, ub = (u64)base; i64 *pj; i32 *pi;
      if(tx==-8) { pj=px(x); i(nx, acc=acc*ub+(u64)pj[i]) }
      else { pi=px(x); i(nx, acc=acc*ub+(u64)ji(pi[i])) }
      return tj((i64)acc);
    }
  }
  /* mixed-radix vector base with a long base or long digits: i64, long
     result. value = sum(digit[i] * place[i]); place[i]=prod of later bases */
  if((ta==-8 || tx==-8) && (ta==-1 || ta==-8) && (tx==-1 || tx==-8)) {
    if(na!=nx) return KERR_LENGTH;
    u64 L=na; if(!L) return tj(0);
    /* place/acc unsigned: the place products and the weighted sum overflow
       i64 on an over-wide value -- UB -- so compute in u64 (base is >=0 after
       the domain check).  Same defined-wrap result as production. */
    i64 *base=xmalloc(L*sizeof(i64)); u64 *place=xmalloc(L*sizeof(u64)), acc=0;
    for(u64 i=0;i<L;i++) {
      base[i]=vreadj(a,i);
      if(base[i]<0 || (i && base[i]<1)) { xfree(base); xfree(place); return KERR_DOMAIN; }
    }
    place[L-1]=1;
    for(i32 i=(i32)L-2;i>=0;i--) place[i]=place[i+1]*(u64)base[i+1];
    for(u64 i=0;i<L;i++) if(place[i]==0) { xfree(base); xfree(place); return KERR_DOMAIN; }
    for(u64 i=0;i<L;i++) acc += (u64)vreadj(x,i) * place[i];
    xfree(base); xfree(place);
    return tj((i64)acc);
  }
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
    /* empty base vector: `*\|a` is empty, so the n(p)--/n(s)-- below would
       underflow the count to SIZE_MAX and the following reverse would try a
       ~2^64-byte allocation.  No places => the decode is 0, matching the long
       sv path's `if(!L) return tj(0)`. */
    if(!na) { r=t(1,0); break; }
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
    case -1: {
      pxi=px(x);
      if(na!=nx) { e=KERR_LENGTH; goto cleanup; }
      u64 xn=nx;                      // x is the CALLER's vector -- its length
      q=k(6,0,k_(a)); EC(q);          // |a   must be restored before we return
      s=k(22,3,q); q=0; EC(s);        // *\q
      n(s)--;
      q=k(6,0,s); s=0; EC(q);         // |p
      n(x)=xn-1;                      // view x as its leading xn-1 digits for q*x
      r=k(3,q,k_(x)); q=0;            // q*x  (borrows x at the shortened length)
      n(x)=xn;                        // RESTORE before any error exit -- the old
      EC(r);                          // n(x)-- left the caller's vector truncated
      r=k(21,1,r); EC(r);             // +/r
      r=k(1,r,t(1,(u32)pxi[xn-1])); EC(r); // + the last digit.  (u32): without
                                      // it a NEGATIVE digit sign-extends into
                                      // bits 48-55, so s(x) read subtype 0xFF
                                      // and `10 10 sv 0 -1` type-errored while
                                      // `10 sv 0 -1` worked.
      break;
    }
    default: r=KERR_TYPE;
    } break;
  case  8:
  case -8:
    /* long base, matrix digits (list of rows after flip): decode each row,
       mirrors the int case-0 arms so long round-trips like int */
    if(tx==0) { pxk=px(x); r=tn(0,n(x)); prk=px(r); i(nr, prk[i]=sv(a,pxk[i]); EC(prk[i])) }
    else r=KERR_TYPE;
    break;
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
  i64 *pxj;
  float *pxe,*pre,ef;
  double f,*prf,*pxf;
  if(!x) { e=KERR_TYPE; goto cleanup; }
  if(s(x)) { e=KERR_TYPE; goto cleanup; }
  switch(tx) {
  case  1: f=fi(ik(x))*fi(ik(x)); r=t2(f); break;
  case  2: f=-fk(x); f*=f; r=t2(f); break;
  case  8: f=fj(jk(x)); f*=f; r=t2(f); break;
  case  9: ef=ek(x); r=te(ef*ef); break;
  case -1: PRF(nx); PXI; i(nx,*prf++=*pxi*fi(*pxi);++pxi) break;
  case -2: PRF(nx); PXF; i(nx,*prf++=*pxf**pxf;++pxf) break;
  case -8: PRF(nx); PXJ; i(nx,f=fj(pxj[i]);prf[i]=f*f) break;
  case -9: PRE(nx); PXE; i(nx,ef=pxe[i];pre[i]=ef*ef) break;
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
  i64 jj,*prj,*pxj;
  float ef,*pre,*pxe;
  double f,g,*prf,*pxf;
  if(!x) { e=KERR_TYPE; goto cleanup; }
  if(s(x)) { e=KERR_TYPE; goto cleanup; }
  switch(tx) {
  case  1: i=ik(x); j=i==INT32_MIN?INT32_MIN:i==INT32_MIN+1?INT32_MAX:i<0?-i:i; r=t(1,(u32)j); break;
  case  2: f=fk(x); g=isinf(f)&&f<0.0?INFINITY:f<0.0?-f:f; r=t2(g); break;
  case  8: jj=jk(x); r=tj(jj==J_NULL?J_NULL:jj==J_NINF?J_INF:jj<0?-jj:jj); break;
  case  9: ef=ek(x); r=te(isinf(ef)&&ef<0.0f?(float)INFINITY:ef<0.0f?-ef:ef); break;
  case -1: PRI(nx); PXI; i(nx,j=*pxi++; *pri++=j==INT32_MIN?INT32_MIN:j==INT32_MIN+1?INT32_MAX:j<0?-j:j); break;
  case -2: PRF(nx); PXF; i(nx,f=*pxf++; *prf++=isinf(f)&&f<0.0?INFINITY:f<0.0?-f:f); break;
  case -8: PRJ(nx); PXJ; i(nx,jj=pxj[i]; prj[i]=jj==J_NULL?J_NULL:jj==J_NINF?J_INF:jj<0?-jj:jj); break;
  case -9: PRE(nx); PXE; i(nx,ef=pxe[i]; pre[i]=isinf(ef)&&ef<0.0f?(float)INFINITY:ef<0.0f?-ef:ef); break;
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
  double d = (tx == 1) ? fi(ik(x)) : fk(x);   /* fi(): sleep 0I slept 24.8 DAYS */
  if(!isfinite(d) || d < 0) return KERR_DOMAIN;
#ifdef FUZZING
  return null;  /* a fuzzed `sleep 5e17` is not a hang worth saving */
#endif
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
  if(0x81==s(x)) {  /* all constants: no work, but the count (source order,
                       px[0]) must still be an int/long like the live path */
    return intatom(px[0]) ? null : KERR_TYPE;
  }
  a=pgreduce_(px[i],&q);
  if(E(a)) return a;
  if(!intatom(a)) { _k(a); return KERR_TYPE; }
  c = ta==8 ? jk(a) : ik(a);  /* use each integer type's encoded count */
  _k(a);
  if(c<0) return r;
  while(c-->0) {
    for(i=nx-2;i>=0;i--) {
#ifdef FUZZING
      if(--gk_budget<0) return kerror("limit");
#endif
      p=pgreduce_(px[i],&q); if(!p)p=null;
      EC(p);
      if(RETURN) return p;
      _k(p);
      if(EXIT) return kerror("abort");
    }
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
  if(0x81==s(x)) {
    K cc=px[0]; i32 tc=T(cc);
    if(!intatom(cc)) return KERR_TYPE;
    if(!(tc==8 ? jk(cc) : ik(cc))) return null;   /* false condition: 0 iterations */
    for(;;) {  /* while[1;1] */
#ifdef FUZZING
      if(--gk_budget<0) return kerror("limit");
#endif
      if(STOP) { STOP=0; return kerror("stop"); }
    }
  }
  a=pgreduce_(px[nx-1],&q);
  if(E(a)) return a;
  if(!intatom(a)) { _k(a); return KERR_TYPE; }
  i64 c = ta==8 ? jk(a) : ik(a); _k(a);
  while(c) {
#ifdef FUZZING
    if(--gk_budget<0) return kerror("limit");
#endif
    for(i=nx-2;i>=0;i--) {
      p=pgreduce_(px[i],&q); if(!p)p=null;
      EC(p);
      if(RETURN) return p;
      _k(p);
      if(EXIT) return kerror("abort");
    }
    a=pgreduce_(px[nx-1],&q);
    if(E(a)) return a;
    if(!intatom(a)) { _k(a); return KERR_TYPE; }
    c = ta==8 ? jk(a) : ik(a); _k(a);
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
  if(0x81==s(x)) {  /* all constants: no work, but the condition (source
                       order, px[0]) must still be an int/long */
    return intatom(px[0]) ? null : KERR_TYPE;
  }
  a=pgreduce_(px[i],&q);
  if(E(a)) return a;
  if(!intatom(a)) { _k(a); return KERR_TYPE; }
  i64 c = ta==8 ? jk(a) : ik(a); _k(a);
  if(c)
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
      a=k_(px[i++]);
      if(E(a)) return a;
      if(!intatom(a)) { _k(a); return KERR_TYPE; }
      i64 c = ta==8 ? jk(a) : ik(a); _k(a);
      if(c&&i<=nx-1) { _k(r); r=k_(px[i]); if(!r) r=null; break; }
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
      if(E(a)) { _k(r); return a; }
      if(!intatom(a)) { _k(a); return KERR_TYPE; }
      i64 c = ta==8 ? jk(a) : ik(a); _k(a);
      if(c&&i>=0) { _k(r); r=pgreduce_(px[i],&q); if(!r) r=null; break; }
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
  case -1: r=avdo(biv("dj"),0,k_(x),"'"); break;
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
  case  1: {
    i32 dv=ik(x);
    /* A sentinel is not a YYYYMMDD date.  Decomposing one gave a
       plausible-looking day number instead of a null: jd 0N -> -79179540,
       jd 0I -> 77692941, jd -0I -> -79179539.  The inverse dj lands on 0N for
       these already (via its out-of-range clamp), so mirror that here. */
    if(dv==INT32_MIN||dv==INT32_MAX||dv==INT32_MIN+1) { r=t(1,(u32)INT32_MIN); break; }
    year = dv/10000;
    month = (dv/100)%100;
    day = dv%100;
    g = (14-month)/12;
    y = (i64)year+4800-g;
    m = month+12*g-3;
    val = (i64)day + (153*m+2)/5 + y*365 + y/4 - y/100 + y/400 - 32045 - 2464329;
    // val is days; it already fits in 64-bit. pack to i32 domain.
    if(val<INT32_MIN || val>INT32_MAX) r=t(1, (u32)INT32_MIN);
    else r=t(1, (u32)(i32)val); }
    break;
  case -1: r=avdo(biv("jd"),0,k_(x),"'"); break;
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
  case  1: {
    i32 lv=ik(x);
    /* Sentinels are not timestamps, and the timezone shift can also push an
       ordinary timestamp ONTO one.  Both directions were unguarded, and both
       are invisible under TZ=UTC (o==0), which is presumably how it survived:
       TZ=Asia/Tokyo gave `lt 0N` -> -2147451248 and, worse, turned the
       ordinary `lt 2147451247` into 0I; TZ=America/New_York turned
       `lt -2147465648` into 0N. */
    if(lv==INT32_MIN||lv==INT32_MAX||lv==INT32_MIN+1) { r=t(1,(u32)lv); break; }
    t = lv+2051222400l; /* 2035-1970 */
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
    q=(i64)lv+(i64)o;
    /* a shifted value that lands on a sentinel is out of range, not infinite */
    if(q>=INT32_MAX||q<=(i64)INT32_MIN+1) { r=t(1,(u32)INT32_MIN); break; }
    r = t(1,(u32)q);
    break; }
  case -1: r=avdo(biv("lt"),0,k_(x),"'"); break;
  case  0: r=irecur1(lt,x); break;
  default: return KERR_TYPE;
  }
  return knorm(r);
}

/* j2f: long/f32 atom or vector -> a fresh float64 K; other types pass
 * through as a new ref. Used by float-output builtins (atan2/hypot) to
 * accept longs/reals by promoting to double without expanding their whole
 * type matrix. */
static K j2f(K x) {
  K r; double *prf; i64 *pxj; float *pxe;
  if(tx==8) return t2(fj(jk(x)));
  if(tx==-8) { r=tn(2,nx); prf=px(r); PXJ; i(nx,prf[i]=fj(pxj[i])) return r; }
  if(tx==9) return t2((double)ek(x));
  if(tx==-9) { r=tn(2,nx); prf=px(r); PXE; i(nx,prf[i]=(double)pxe[i]) return r; }
  return k_(x);
}

/* i2e: int atom/vector -> a fresh real(f32) K; real (9/-9) passes through as a
 * new ref. Mirror of j2f for the f32-stays-f32 path of atan2/hypot (used only
 * when operands are pure int/real -- no f64 or long present). */
static K i2e(K x) {
  K r; float *pre; i32 *pxi;
  if(tx==1) return te(ei(ik(x)));
  if(tx==-1) { r=tn(9,nx); pre=px(r); PXI; i(nx,pre[i]=ei(pxi[i])) return r; }
  return k_(x);
}

/* flatie: x is a flat int(1/-1) or real(9/-9) atom/vector -- exactly the set
 * i2e converts to a real K. The real-stays-real fast path is only safe when
 * BOTH operands are flatie; a general list (or any other type) must fall
 * through to the recursing/f64 path or ehypot_/eatan2_ would read it as a
 * float buffer and over-read the other operand. */
static int flatie(K x) { return tx==1||tx==-1||tx==9||tx==-9; }

/* real-op-real -> real dyad; operands already real (9/-9). F0f is the
 * f32-precision libm fn. Atoms inline; vec/vec needs equal length. */
#define EC2D(NAME,F0f) \
static K NAME(K a,K x) { \
  K r=0; float *pre,*pae,*pxe; \
  if(ta<0&&tx<0&&na!=nx) return KERR_LENGTH; \
  if(ta==9&&tx==9) return te(F0f(ek(a),ek(x))); \
  else if(ta==9) { float A=ek(a); PRE(nx); PXE; i(nx,pre[i]=F0f(A,pxe[i])) } \
  else if(tx==9) { float X=ek(x); PRE(na); PAE; i(na,pre[i]=F0f(pae[i],X)) } \
  else { PRE(na); PAE; PXE; i(na,pre[i]=F0f(pae[i],pxe[i])) } \
  return knorm(r); \
}
EC2D(ehypot_,hypotf)
EC2D(eatan2_,atan2f)

/*
 *  F: function
 * F0: function to compute
*/
/* F0: f64 math fn; F0f: its f32-precision variant (real stays real) */
#define MC1D(F,F0,F0f) \
K F(K x) { \
  K r=0,e; \
  i32 *pxi; \
  i64 *pxj; \
  float *pxe,*pre; \
  double *prf,*pxf; \
  if(s(x)) return KERR_TYPE; \
  switch(tx) { \
  case  1: r=t2(F0(fi(ik(x)))); break; \
  case  2: r=t2(F0(fk(x))); break; \
  case  8: r=t2(F0(fj(jk(x)))); break; \
  case  9: r=te(F0f(ek(x))); break; \
  case  0: r=irecur1(F,x); break; \
  case -1: r=tn(2,nx); prf=px(r); PXI; i(nx,prf[i]=F0(fi(pxi[i]))) break; \
  case -2: r=tn(2,nx); prf=px(r); PXF; i(nx,prf[i]=F0(pxf[i])) break; \
  case -8: r=tn(2,nx); prf=px(r); PXJ; i(nx,prf[i]=F0(fj(pxj[i]))) break; \
  case -9: r=tn(9,nx); pre=px(r); PXE; i(nx,pre[i]=F0f(pxe[i])) break; \
  default: { e=KERR_TYPE; goto cleanup; } \
  } \
  return knorm(r); \
cleanup: \
  _k(r); \
  return e; \
}
MC1D(log_,log,logf)
MC1D(exp_,exp,expf)
MC1D(sqrt_,sqrt,sqrtf)
MC1D(floor_,floor,floorf)
MC1D(ceil_,ceil,ceilf)
MC1D(sin_,sin,sinf)
MC1D(cos_,cos,cosf)
MC1D(tan_,tan,tanf)
MC1D(asin_,asin,asinf)
MC1D(acos_,acos,acosf)
MC1D(atan_,atan,atanf)
MC1D(sinh_,sinh,sinhf)
MC1D(cosh_,cosh,coshf)
MC1D(tanh_,tanh,tanhf)
MC1D(erf_,erf,erff)
MC1D(erfc_,erfc,erfcf)
/* gamma has poles at the non-positive integers and at -inf; the value there is
   undefined, so it must be NaN (C99/C11 F.10.5.4 -- tgamma(negative integer)
   and tgamma(-inf) return NaN).  glibc obeys this, but newlib (Cygwin) returns
   +inf instead, which diverges per-platform.  Guard the poles ourselves so
   gamma is deterministic everywhere: `x<0 && x==floor(x)` catches the negative
   integers and -inf (floor(-inf)==-inf) while leaving 0 (->+inf) and NaN alone. */
static double gktgamma(double x) { return (x<0.0 && x==floor(x)) ? NAN : tgamma(x); }
static float  gktgammaf(float x) { return (x<0.0f && x==floorf(x)) ? (float)NAN : tgammaf(x); }
MC1D(gamma_,gktgamma,gktgammaf)
MC1D(lgamma_,lgamma,lgammaf)
MC1D(rint_,rint,rintf)
MC1D(trunc_,trunc,truncf)

/*
 *  F: function
 * F0: function to compute
*/
#define MC2D(F,F0,EF) \
K F(K a,K x) { \
  K r=0; \
  i32 *pai,*pxi; \
  double *prf,*paf,*pxf; \
  if(s(a)||s(x)) return KERR_TYPE; \
  { char hl=ta==8||ta==-8||tx==8||tx==-8, he=ta==9||ta==-9||tx==9||tx==-9, \
         hf=ta==2||ta==-2||tx==2||tx==-2; \
    if(he&&!hl&&!hf&&flatie(a)&&flatie(x)) { K A=i2e(a),X=i2e(x); r=EF(A,X); _k(A); _k(X); return r; } \
    if(hl||he) { K A=j2f(a),X=j2f(x); r=F(A,X); _k(A); _k(X); return r; } } \
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
    case  0: r=avdo(biv(#F0),k_(a),k_(x),"'"); break; \
    default: r=KERR_TYPE; break; \
    } break; \
  case -2: \
    switch(tx) { \
    case  1: PRF(na); PAF; i(na,prf[i]=F0(paf[i],fi(ik(x)))) break; \
    case  2: PRF(na); PAF; i(na,prf[i]=F0(paf[i],fk(x))) break; \
    case -1: PRF(na); PAF; PXI; i(na,prf[i]=F0(paf[i],fi(pxi[i]))) break; \
    case -2: PRF(na); PAF; PXF; i(na,prf[i]=F0(paf[i],pxf[i])) break; \
    case  0: r=avdo(biv(#F0),k_(a),k_(x),"'"); break; \
    default: r=KERR_TYPE; break; \
    } break; \
  case  0: \
    switch(tx) { \
    case  1: r=irecur2(F,a,x); break; \
    case  2: r=irecur2(F,a,x); break; \
    case -1: r=avdo(biv(#F0),k_(a),k_(x),"'"); break; \
    case -2: r=avdo(biv(#F0),k_(a),k_(x),"'"); break; \
    case  0: r=irecur2(F,a,x); break; \
    default: r=KERR_TYPE; break; \
    } break; \
  default: r=KERR_TYPE; break; \
  } \
  return knorm(r); \
}
MC2D(atan2_,atan2,eatan2_)

/* `div` by zero yields infinity, exactly as `%` does (ref.md: "Division by zero
   yields infinity, not an error") -- it used to raise `domain`, so two integer
   ops with a zero divisor disagreed (`5%0` was 0i but `5 div 0` errored).  The
   sign follows the numerator and 0 div 0 is null, again mirroring `%`, whose
   0%0 is 0n.  Sentinels: 0I/0N are INT_MAX/INT_MIN, -0I is INT_MIN+1 (x.c).
   The result width follows the usual promotion, so a long operand anywhere
   gives 0Ij.  INT_MIN/-1 still overflows to `range`: that is a real overflow,
   not a zero divisor. */
static i32 divi_(i32 a, i32 b) {
  if(b) return a/b;
  return a>0 ? INT32_MAX : a<0 ? INT32_MIN+1 : INT32_MIN;
}
static i64 divj_(i64 a, i64 b) {
  if(b) return a/b;
  return a>0 ? INT64_MAX : a<0 ? INT64_MIN+1 : INT64_MIN;
}

/* integer ops. Accept int (type 1) and long (type 8); any long operand
   promotes the result to long (computed in i64). Float is a type error.
   OI/OJ are the i32 and i64 combiners: a plain operator for and/or/xor, the
   zero-aware helpers above for div. */
#define MC2IO(F,F0,OI,OJ,N) \
K F(K a, K x) { \
  K r=0; \
  i32 *pri,*pai,*pxi; \
  i64 *prj,*paj,*pxj; \
  if(s(a)||s(x)) return KERR_TYPE; \
  if((N)) {  /* INT_MIN/-1 overflow -> range (zero divisor is handled in div*_) */ \
    if((ta==1 && INT32_MIN==ik(a)) || (ta==8 && INT64_MIN==jk(a))) { \
      if(tx==1 && -1==ik(x)) return KERR_RANGE; \
      if(tx==8 && -1==jk(x)) return KERR_RANGE; \
      if(tx==-1) { PXI; i(nx,if(-1==pxi[i]) return KERR_RANGE) } \
      if(tx==-8) { PXJ; i(nx,if(-1==pxj[i]) return KERR_RANGE) } \
    } \
    if(ta==-1 || ta==-8) { \
      i32 b=0; \
      if(ta==-1) { PAI; i(na,if(INT32_MIN==pai[i]) {b=1;break;}) } \
      else { PAJ; i(na,if(INT64_MIN==paj[i]) {b=1;break;}) } \
      if(b && tx==1 && -1==ik(x)) return KERR_RANGE; \
      if(b && tx==8 && -1==jk(x)) return KERR_RANGE; \
      if(b && tx==-1) { PXI; i(nx,if(-1==pxi[i]) return KERR_RANGE) } \
      if(b && tx==-8) { PXJ; i(nx,if(-1==pxj[i]) return KERR_RANGE) } \
    } \
  } \
  if(ta<=0 && tx<=0 && na!=nx) return KERR_LENGTH; \
  switch(ta) { \
  case  1: \
    switch(tx) { \
    case  1: r=t(1,(u32)OI(ik(a),ik(x))); break; \
    case  8: r=tj(OJ(ji(ik(a)),jk(x))); break; \
    case -1: PRI(nx); PXI; i(nx,pri[i]=OI(ik(a),pxi[i])) break; \
    case -8: PRJ(nx); PXJ; { i64 A=ji(ik(a)); i(nx,prj[i]=OJ(A,pxj[i])) } break; \
    case  0: r=irecur2(F,a,x); break; \
    default: r=KERR_TYPE; \
    } break; \
  case  8: \
    switch(tx) { \
    case  1: r=tj(OJ(jk(a),ji(ik(x)))); break; \
    case  8: r=tj(OJ(jk(a),jk(x))); break; \
    case -1: PRJ(nx); PXI; { i64 A=jk(a); i(nx,prj[i]=OJ(A,ji(pxi[i]))) } break; \
    case -8: PRJ(nx); PXJ; { i64 A=jk(a); i(nx,prj[i]=OJ(A,pxj[i])) } break; \
    case  0: r=irecur2(F,a,x); break; \
    default: r=KERR_TYPE; \
    } break; \
  case -1: \
    switch(tx) { \
    case  1: PRI(na); PAI; i(na,pri[i]=OI(pai[i],ik(x))) break; \
    case  8: PRJ(na); PAI; { i64 X=jk(x); i(na,prj[i]=OJ(ji(pai[i]),X)) } break; \
    case -1: PRI(na); PAI; PXI; i(na,pri[i]=OI(pai[i],pxi[i])) break; \
    case -8: PRJ(na); PAI; PXJ; i(na,prj[i]=OJ(ji(pai[i]),pxj[i])) break; \
    case  0: r=avdo(biv(#F0),k_(a),k_(x),"'"); break; \
    default: r=KERR_TYPE; \
    } break; \
  case -8: \
    switch(tx) { \
    case  1: PRJ(na); PAJ; { i64 X=ji(ik(x)); i(na,prj[i]=OJ(paj[i],X)) } break; \
    case  8: PRJ(na); PAJ; { i64 X=jk(x); i(na,prj[i]=OJ(paj[i],X)) } break; \
    case -1: PRJ(nx); PAJ; PXI; i(nx,prj[i]=OJ(paj[i],ji(pxi[i]))) break; \
    case -8: PRJ(nx); PAJ; PXJ; i(nx,prj[i]=OJ(paj[i],pxj[i])) break; \
    case  0: r=avdo(biv(#F0),k_(a),k_(x),"'"); break; \
    default: r=KERR_TYPE; \
    } break; \
  case  0: \
    switch(tx) { \
    case  1: r=irecur2(F,a,x); break; \
    case  8: r=irecur2(F,a,x); break; \
    case -1: r=avdo(biv(#F0),k_(a),k_(x),"'"); break; \
    case -8: r=avdo(biv(#F0),k_(a),k_(x),"'"); break; \
    case  0: r=irecur2(F,a,x); break; \
    default: r=KERR_TYPE; \
    } break; \
  default: r=KERR_TYPE; \
  } \
  return knorm(r); \
}
#define OP_AND(a,b) ((a)&(b))
#define OP_OR(a,b)  ((a)|(b))
#define OP_XOR(a,b) ((a)^(b))
MC2IO(div_,div,divi_,divj_,1)
MC2IO(and_,and,OP_AND,OP_AND,0)
MC2IO(or_,or,OP_OR,OP_OR,0)
MC2IO(xor_,xor,OP_XOR,OP_XOR,0)

K hypot_(K a,K x) {
  K r=0;
  double af,xf,*prf,*paf,*pxf;
  i32 *pai,*pxi;

  if(s(a)||s(x)) return KERR_TYPE;
  { char hl=ta==8||ta==-8||tx==8||tx==-8, he=ta==9||ta==-9||tx==9||tx==-9,
         hf=ta==2||ta==-2||tx==2||tx==-2;
    if(he&&!hl&&!hf&&flatie(a)&&flatie(x)) { K A=i2e(a),X=i2e(x); r=ehypot_(A,X); _k(A); _k(X); return r; }
    if(hl||he) { K A=j2f(a),X=j2f(x); r=hypot_(A,X); _k(A); _k(X); return r; } }
  if(ta<0 && tx<0 && na!=nx) return KERR_LENGTH;

  switch(ta) {
  case  1:
    switch(tx) {
    case  1: r=t2(hypot(fi(ik(a)),fi(ik(x)))); break;
    case  2: r=t2(hypot(fi(ik(a)),fk(x))); break;
    case -1: PRF(nx); PXI; af=fi(ik(a)); i(nx,prf[i]=hypot(af,fi(pxi[i]))) break;
    case -2: PRF(nx); PXF; af=fi(ik(a)); i(nx,prf[i]=hypot(af,pxf[i])) break;
    case  0: r=irecur2(hypot_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case  2:
    switch(tx) {
    case  1: r=t2(hypot(fk(a),fi(ik(x)))); break;
    case  2: r=t2(hypot(fk(a),fk(x))); break;
    case -1: PRF(nx); PXI; af=fk(a); i(nx,prf[i]=hypot(af,fi(pxi[i]))) break;
    case -2: PRF(nx); PXF; af=fk(a); i(nx,prf[i]=hypot(af,pxf[i])) break;
    case  0: r=irecur2(hypot_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case -1:
    switch(tx) {
    case  1: PRF(na); PAI; xf=fi(ik(x)); i(na,prf[i]=hypot(fi(pai[i]),xf)) break;
    case  2: PRF(na); PAI; xf=fk(x); i(na,prf[i]=hypot(fi(pai[i]),xf)) break;
    case -1: PRF(na); PAI; PXI; i(na,prf[i]=hypot(fi(pai[i]),fi(pxi[i]))) break;
    case -2: PRF(na); PAI; PXF; i(na,prf[i]=hypot(fi(pai[i]),pxf[i])) break;
    case  0: r=avdo(biv("hypot"),k_(a),k_(x),"'"); break;
    default: r=KERR_TYPE;
    } break;
  case -2:
    switch(tx) {
    case  1: PRF(na); PAF; xf=fi(ik(x)); i(na,prf[i]=hypot(paf[i],xf)) break;
    case  2: PRF(na); PAF; xf=fk(x); i(na,prf[i]=hypot(paf[i],xf)) break;
    case -1: PRF(na); PAF; PXI; i(na,prf[i]=hypot(paf[i],fi(pxi[i]))) break;
    case -2: PRF(na); PAF; PXF; i(na,prf[i]=hypot(paf[i],pxf[i])) break;
    case  0: r=avdo(biv("hypot"),k_(a),k_(x),"'"); break;
    default: r=KERR_TYPE;
    } break;
  case  0:
    switch(tx) {
    case  1: r=irecur2(hypot_,a,x); break;
    case  2: r=irecur2(hypot_,a,x); break;
    case -1: r=avdo(biv("hypot"),k_(a),k_(x),"'"); break;
    case -2: r=avdo(biv("hypot"),k_(a),k_(x),"'"); break;
    case  0: r=irecur2(hypot_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  default: r=KERR_TYPE;
  }
  return knorm(r);
}

K not__(K x) {
  K r=0;
  i32 *pri,*pxi;
  i64 *prj,*pxj;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case  1: r=t(1,(u32)~ik(x)); break;
  case  8: r=tj(~jk(x)); break;
  case  0: r=irecur1(not__,x); break;
  case -1: PRI(nx); PXI; i(nx,pri[i]=~pxi[i]) break;
  case -8: PRJ(nx); PXJ; i(nx,prj[i]=~pxj[i]) break;
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

/* map an int/long shift count to the other width, preserving the
   0N / 0I / -0I sentinels (which select identity / max / 1) */
/* widen a shift/rot count i32 -> i64, sentinel-aware.  (The reverse narrowing
   helper became dead when shift/rot began promoting on an i64 count.) */
static inline i64 shcount_i(i32 c) {
  if(c==INT32_MIN)   return J_NULL;
  if(c==INT32_MAX)   return J_INF;
  if(c==INT32_MIN+1) return J_NINF;
  return (i64)c;
}

static inline i64 raw_rot64(i64 v, i64 sh) {
  u64 u = (u64)v;
  u64 s = (u64)sh & 63;  /* normalize to [0..63] */
  if(!s) return v;
  return (i64)((u << s) | (u >> (64 - s)));
}

static inline i64 safe_rot64(i64 v, i64 sh) {
  if(sh == J_NULL) return v;    /* 0Nj shift count -> identity */
  if(sh == J_INF)  sh = 63;     /* 0Ij  -> rotate-by-63 */
  if(sh == J_NINF) sh = 1;      /* -0Ij -> rotate-by-1 */
  return raw_rot64(v, sh);
}

K rot_(K a, K x) {
  K r=0;
  i32 *pri,*pai,*pxi;
  i64 *prj,*paj,*pxj;

  if(s(a)||s(x)) return KERR_TYPE;
  if(ta<=0 && tx<=0 && na!=nx) return KERR_LENGTH;

  switch(ta) {
  case 1:
    switch(tx) {
    case  1: r=t(1,(u32)safe_rot(ik(a),ik(x))); break;
    /* i64 count promotes to i64 (either-operand-i64 rule); the value widens
       sentinel-aware, and a 64-bit rotate differs from a 32-bit one */
    case  8: r=tj(safe_rot64(ji(ik(a)),jk(x))); break;
    case -1: PRI(nx); PXI; i(nx,pri[i]=safe_rot(ik(a),pxi[i])); break;
    case -8: PRJ(nx); PXJ; { i64 V=ji(ik(a)); i(nx,prj[i]=safe_rot64(V,pxj[i])) } break;
    case  0: r=irecur2(rot_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case 8:
    switch(tx) {
    case  1: r=tj(safe_rot64(jk(a),shcount_i(ik(x)))); break;
    case  8: r=tj(safe_rot64(jk(a),jk(x))); break;
    case -1: PRJ(nx); PXI; { i64 V=jk(a); i(nx,prj[i]=safe_rot64(V,shcount_i(pxi[i]))) } break;
    case -8: PRJ(nx); PXJ; { i64 V=jk(a); i(nx,prj[i]=safe_rot64(V,pxj[i])) } break;
    case  0: r=irecur2(rot_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case -1:
    switch(tx) {
    case  1: PRI(na); PAI; i(na,pri[i]=safe_rot(pai[i],ik(x))); break;
    case  8: PRJ(na); PAI; { i64 S=jk(x); i(na,prj[i]=safe_rot64(ji(pai[i]),S)) } break;
    case -1: PRI(na); PAI; PXI; i(na,pri[i]=safe_rot(pai[i],pxi[i])); break;
    case -8: PRJ(na); PAI; PXJ; i(na,prj[i]=safe_rot64(ji(pai[i]),pxj[i])); break;
    case  0: r=avdo(biv("rot"),k_(a),k_(x),"'"); break;
    default: r=KERR_TYPE;
    } break;
  case -8:
    switch(tx) {
    case  1: PRJ(na); PAJ; { i64 S=shcount_i(ik(x)); i(na,prj[i]=safe_rot64(paj[i],S)) } break;
    case  8: PRJ(na); PAJ; { i64 S=jk(x); i(na,prj[i]=safe_rot64(paj[i],S)) } break;
    case -1: PRJ(na); PAJ; PXI; i(na,prj[i]=safe_rot64(paj[i],shcount_i(pxi[i]))); break;
    case -8: PRJ(na); PAJ; PXJ; i(na,prj[i]=safe_rot64(paj[i],pxj[i])); break;
    case  0: r=avdo(biv("rot"),k_(a),k_(x),"'"); break;
    default: r=KERR_TYPE;
    } break;
  case 0:
    switch(tx) {
    case  1: r=irecur2(rot_,a,x); break;
    case  8: r=irecur2(rot_,a,x); break;
    case -1: r=avdo(biv("rot"),k_(a),k_(x),"'"); break;
    case -8: r=avdo(biv("rot"),k_(a),k_(x),"'"); break;
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

static inline i64 raw_shift64(i64 v, i64 sh) {
  u64 u = (u64)v;
  if(sh >= 0) u <<= (sh & 63);
  else u >>= ((-sh) & 63);
  return (i64)u;
}

static inline i64 safe_shift64(i64 v, i64 sh) {
  if(sh == J_NULL) return v;   /* 0Nj shift count -> identity */
  if(sh == J_INF)  return (v & 1) ? INT64_MIN : 0;  /* 0Ij shift count */
  if(sh == J_NINF) return 0;   /* -0Ij shift count -> always 0 */
  return raw_shift64(v, sh);
}

K shift_(K a, K x) {
  K r=0;
  i32 *pri,*pai,*pxi;
  i64 *prj,*paj,*pxj;

  if(s(a)||s(x)) return KERR_TYPE;
  if(ta<=0 && tx<=0 && na!=nx) return KERR_LENGTH;

  switch(ta) {
  case 1:
    switch(tx) {
    case  1: r=t(1,(u32)safe_shift(ik(a),ik(x))); break;
    /* i64 count promotes the result to i64 (either-operand-i64 rule, matching
       and/or/xor/div/gcd/lcm); the value widens sentinel-aware so 0N shift 3j
       == 0Nj shift 3 */
    case  8: r=tj(safe_shift64(ji(ik(a)),jk(x))); break;
    case -1: PRI(nx); PXI; i(nx,pri[i]=safe_shift(ik(a),pxi[i])); break;
    case -8: PRJ(nx); PXJ; { i64 V=ji(ik(a)); i(nx,prj[i]=safe_shift64(V,pxj[i])) } break;
    case  0: r=irecur2(shift_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case 8:
    switch(tx) {
    case  1: r=tj(safe_shift64(jk(a),shcount_i(ik(x)))); break;
    case  8: r=tj(safe_shift64(jk(a),jk(x))); break;
    case -1: PRJ(nx); PXI; { i64 V=jk(a); i(nx,prj[i]=safe_shift64(V,shcount_i(pxi[i]))) } break;
    case -8: PRJ(nx); PXJ; { i64 V=jk(a); i(nx,prj[i]=safe_shift64(V,pxj[i])) } break;
    case  0: r=irecur2(shift_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case -1:
    switch(tx) {
    case  1: PRI(na); PAI; i(na,pri[i]=safe_shift(pai[i],ik(x))); break;
    case  8: PRJ(na); PAI; { i64 S=jk(x); i(na,prj[i]=safe_shift64(ji(pai[i]),S)) } break;
    case -1: PRI(na); PAI; PXI; i(na,pri[i]=safe_shift(pai[i],pxi[i])); break;
    case -8: PRJ(na); PAI; PXJ; i(na,prj[i]=safe_shift64(ji(pai[i]),pxj[i])); break;
    case  0: r=irecur2(shift_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case -8:
    switch(tx) {
    case  1: PRJ(na); PAJ; { i64 S=shcount_i(ik(x)); i(na,prj[i]=safe_shift64(paj[i],S)) } break;
    case  8: PRJ(na); PAJ; { i64 S=jk(x); i(na,prj[i]=safe_shift64(paj[i],S)) } break;
    case -1: PRJ(nx); PAJ; PXI; i(nx,prj[i]=safe_shift64(paj[i],shcount_i(pxi[i]))); break;
    case -8: PRJ(nx); PAJ; PXJ; i(nx,prj[i]=safe_shift64(paj[i],pxj[i])); break;
    case  0: r=irecur2(shift_,a,x); break;
    default: r=KERR_TYPE;
    } break;
  case 0:
    switch(tx) {
    case  1: r=irecur2(shift_,a,x); break;
    case  8: r=irecur2(shift_,a,x); break;
    case -1: r=avdo(biv("shift"),k_(a),k_(x),"'"); break;
    case -8: r=avdo(biv("shift"),k_(a),k_(x),"'"); break;
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
  i(na,if(s(pak[i])||T(pak[i])!=-1) return KERR_TYPE)
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
    r=tnc(3,m,e);
    if(E(r)) { xfree(e); ee=r; r=0; goto cleanup; }
    break;
  case  4:
    pxc=sk(x);
    e=f(pxc,strlen(pxc),(u8*)px(iv),(u8*)px(key),&m);
    if(!e) { ee=KERR_DOMAIN; goto cleanup; }
    r=tnc(3,m,e);
    if(E(r)) { xfree(e); ee=r; r=0; goto cleanup; }
    break;
  case -4:
    PRK(nx);
    PXS;
    for(u64 i=0;i<nx;++i) {
      e=f(pxs[i],strlen(pxs[i]),(u8*)px(iv),(u8*)px(key),&m);
      if(!e) { ee=KERR_DOMAIN; goto cleanup; }
      p=tnc(3,m,e);
      if(E(p)) { xfree(e); ee=p; goto cleanup; }
      prk[i]=p;
    }
    break;
  case  0: {
    static i32 d=0;
    if(++d>maxr || (!(d&7)&&stack_low())) { --d; ee=KERR_STACK; goto cleanup; }
    PRK(nx); PXK;
    i(nx,prk[i]=crypt_(f,a,pxk[i]);if(E(prk[i])){ --d; ee=prk[i]; goto cleanup; });
    --d;
  } break;
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

/* A closure's captured environment, flattened into one dict.
 *
 * A lambda's scope slot[0] is the environment its FREE variables resolve
 * against (see fncp_).  For a closure that is the enclosing lambda's frame; for
 * a plain lambda it is a namespace scope.  Walk the chain from there collecting
 * bindings, inner first so an inner name shadows an outer one, and STOP at the
 * first namespace scope -- globals belong to the far side and are resolved
 * there, exactly as they always were.  So a plain lambda captures nothing and
 * its scope slot still serializes as `null`: non-closure blobs stay
 * byte-identical, and the wire format is unchanged for them.
 *
 * Returns 0 when there is nothing to capture.  A frame holding the very lambda
 * being serialized (or a ring member -- see fncap_strip below) is a cycle in
 * the object graph, but never on the wire: the self is skipped and ring
 * members ship env-less, so the blob is always acyclic and serialize's depth
 * guard is only a backstop for pathological nesting. */
/* fncap_ring: does this binding hold, anywhere on its conversion surface
   (see the surface comment in fn.c), a lambda parented to sc -- a member of
   the frame's reference ring?  Depth overflow answers yes: the strip below
   copes, and shipping as-is would recurse around the ring. */
static int fncap_ring(K w, K sc) {
  static int d=0;
  int r=0; u64 j; K *p;
  if(++d>maxr || (!(d&7)&&stack_low())) { --d; return 1; }
  switch(s(w)) {
  case 0xc3: { K scp=((K*)px(w))[2]; r=6!=T(scp)&&((K*)px(scp))[0]==sc; break; }
  case 0xd9: case 0xd7: p=px(w); r=fncap_ring(p[0],sc)||fncap_ring(p[1],sc); break;
  case 0xda: p=px(w); r=fncap_ring(p[0],sc); break;
  case 0xc5: p=px(w); r=fncap_ring(p[0],sc); break; /* slot 0 = list of composed fns */
  case 0x80: { p=px(w); K vl=p[1]; K *pv=px(vl);
    for(j=0;j<n(vl)&&!r;++j) r=fncap_ring(pv[j],sc);
    break; }
  default:
    if(0==T(w)&&(0==s(w)||0x81==s(w))) { p=px(w); for(j=0;j<n(w)&&!r;++j) r=fncap_ring(p[j],sc); }
  }
  --d;
  return r;
}

/* Copy a ring-holding binding for shipping: rebuild the containers on the
   surface -- wrapper chains, held-args lists, dicts -- sharing everything
   that needs no change (serialize only reads them), with each ring member
   replaced by a DEFINITION-ONLY lambda copy.  Env-less is the wire form of
   "parented to the frame it travels in": the receiver's fnrestore re-parents
   it to the restored frame, whose dict is this very blob, so the ring is
   rebuilt on the far side with nothing cyclic ever on the wire.  Depth
   overflow ships the node as-is; serialize's own depth guard then reports an
   honest `stack` error. */
static K fncap_strip(K w, K sc) {
  static int d=0;
  K r,*pr; u64 j;
  if(++d>maxr || (!(d&7)&&stack_low())) { --d; return k_(w); }
  switch(s(w)) {
  case 0xc3: {
    K scp=((K*)px(w))[2];
    if(6==T(scp)||((K*)px(scp))[0]!=sc) { r=k_(w); break; }
    r=tn(0,4); pr=px(r);
    pr[0]=k_(((K*)px(w))[0]);   /* definition text */
    pr[1]=null; pr[2]=null; pr[3]=FN_VF(0,0);
    r=st(0xc3,r);
    break; }
  case 0xd9: case 0xd7:
    r=tn(0,2); pr=px(r);
    pr[0]=fncap_strip(((K*)px(w))[0],sc);
    pr[1]=fncap_strip(((K*)px(w))[1],sc);
    r=st(s(w),r);
    break;
  case 0xda: case 0xc5:       /* 0xc5: slot 0 = list of composed fns */
    r=tn(0,2); pr=px(r);
    pr[0]=fncap_strip(((K*)px(w))[0],sc);
    pr[1]=k_(((K*)px(w))[1]);   /* the adverb string */
    r=st(s(w),r);
    break;
  case 0x80: {
    K *pw=px(w); K ky=pw[0]; K vl=pw[1]; char **pk=px(ky); K *pv=px(vl);
    r=dnew();
    for(j=0;j<n(ky);++j) { K v2=fncap_strip(pv[j],sc); (void)dset(r,pk[j],v2); _k(v2); }
    break; }
  default:
    if(0==T(w)&&(0==s(w)||0x81==s(w))) {
      r=tn(0,n(w)); pr=px(r);
      { K *pw=px(w); for(j=0;j<n(w);++j) pr[j]=fncap_strip(pw[j],sc); }
      if(0x81==s(w)) r=st(0x81,r);
    }
    else r=k_(w);
  }
  --d;
  return r;
}

static K fncap(K f) {
  K *pf=px(f), sc, *ps, d=0, *pdk, ky, vl, *pvl, g, cs0, e;
  char **pks;
  u64 j;
  if(6==T(pf[2])) return 0;      /* never realized -- nothing captured yet */
  ps=px(pf[2]);
  sc=ps[0];
  while(sc && sc!=null) {
    /* A scope frame is a 6-slot plist; ps[2]/ps[1] below assume it.  fncap
       serializes closures that can originate from an untrusted db blob (or
       capture a transient IPC-dispatch frame), so don't trust the shape: a
       short/non-list frame ends the walk rather than over-reading. */
    if(s(sc) || T(sc)!=0 || n(sc)<6) break;
    ps=px(sc);
    if(null!=ps[2]) break;       /* a namespace scope: globals stop here */
    if(0x80==s(ps[1])) {
      pdk=px(ps[1]); ky=pdk[0]; vl=pdk[1];
      pks=px(ky); pvl=px(vl);
      /* a forged scope dict can have keys longer than values (dget/dset index
         values by a key slot); bound to the values count too */
      for(j=0;j<n(ky)&&j<n(vl);++j) {
        if(pvl[j]==f) continue;  /* the frame holds the very lambda we are
                                    capturing for (`f:{...}` beside its own
                                    definition).  Shipping it inside its own
                                    environment is a cycle -- and it is already
                                    being serialized, so skip it.  (A WRAPPER
                                    around it -- `g:f 1` -- is a different
                                    binding and must ship: it lands in the
                                    ring branch below and goes env-less.) */
        g=fn_inner(pvl[j]);
        /* A sibling lambda that has never run has no environment of its own
           yet, but its free names are this frame's.  Give it one NOW, against
           the frame it lives in, so the recursive serialize below captures
           them into ITS blob -- it then travels self-contained, and the
           receiver never has to link it back to our frame (which is what would
           make the object graph cyclic; see fnrestore). */
        if(0xc3==s(g) && 6==T(((K*)px(g))[2])) {
          cs0=cs; cs=k_(sc);
          e=fnd(g);
          _k(cs); cs=cs0;
          if(e) { if(d) dfree(d); return 0; }  /* unparseable: capture nothing */
        }
        if(!d) d=dnew();
        e=dget(d,pks[j]);
        if(e) { _k(e); continue; }             /* inner shadows outer */
        /* A binding holding a lambda parented to THIS frame -- bare, under a
           wrapper, in a projection's held args, or nested in a list/dict
           (closure_siblings / fnrestore made it a member of the frame's
           reference ring) -- must ship env-less: capturing it as-is would
           recurse around the ring -- serialize(f) captures g, whose blob
           captures f, forever (mutually recursive siblings) -- until the
           depth guard turned it into a `stack` error.  Its free names are
           exactly this frame's bindings, which THIS blob already carries, and
           the receiver's fnrestore re-parents an env-less lambda to the
           restored frame -- rebuilding the ring on the far side, siblings and
           recursion intact. */
        if(fncap_ring(pvl[j],sc)) {
          K g2=fncap_strip(pvl[j],sc);
          (void)dset(d,pks[j],g2); _k(g2);
          continue;
        }
        (void)dset(d,pks[j],pvl[j]);
      }
    }
    sc=ps[0];
  }
  return d;
}

static K serialize(K x, char **b, char **v, i64 *n, i64 *m) {
  K p;
  char **pxs,*pxc,cc;
  double *pxf;
  static int d;
  if(!x) return KERR_TYPE;
  if(++d>maxr || (!(d&7)&&stack_low())) { --d; return KERR_STACK; }
  i32 t=tx,st=s(x),si=sizeof(i32),sd=sizeof(double),sn=sizeof(u64),ii,*pxi;
  i64 dv;
  /* A 2:-linked C function (0xdc) holds a raw, process-local code pointer in
     its slot 0 (io.c linkcall CALLs it).  Unlike a lambda (ships its text), a
     dict, or a builtin verb (ships an index/name), there is nothing meaningful
     to write down: the address is invalid in any other process, run, or after
     the .so moves.  Serializing it would only ever produce a blob that db must
     reject (and that a hostile peer forges into an arbitrary-call).  Refuse at
     the source -- a value that cannot round-trip is not serializable data.
     (Catches it nested too: a closure/list capturing a linked fn hits this.)
     Not gated on t==0: db's reject is unconditional, so keep the two sides
     symmetric rather than trusting a linked fn to always be list-shaped. */
  if(st==0xdc) { --d; return KERR_TYPE; }
  *n+=3*sizeof(i32);
  K *pxk;
  switch(t) {
  case  1: *n+=si; break;
  case  2: *n+=sd; break;
  case  8: *n+=sd; break;
  case  9: *n+=si; break;
  case  3: *n+=si; break;
  case  4: *n+=1+strlen(sk(x)); break;
  case  6: case 10: break;
  case -4: PXS; *n+=sn; i(nx,*n+=1+strlen(pxs[i])); break;
  case -3: *n+=sn; *n+=(i64)nx; break;
  case -2: *n+=sn; *n+=(i64)nx*sd; break;
  case -8: *n+=sn; *n+=(i64)nx*sd; break;
  case -9: *n+=sn; *n+=(i64)nx*si; break;
  case -1: *n+=sn; *n+=(i64)nx*si; break;
  case  0: *n+=sn; break;
  default: --d; return KERR_TYPE;
  }
  dv=*v-*b;
  /* bd output is a K char vector; cap at the vector length limit (VMAX) */
  if(dv+*n>=VMAX) { --d; return KERR_WSFULL; }
  while(dv+*n>*m) {
    i64 nm=*m<<1;
    if(nm>VMAX) nm=VMAX;
    *m=nm; *b=xrealloc(*b,(size_t)*m); *v=*b+dv; dv=*v-*b;
  }
  gk_st_arr(*v,&t,1,si); *v+=si;
  gk_st_arr(*v,&st,1,si); *v+=si;
  memset(*v,0,si); *v+=si; /* pad to 8 byte alignment for double */
  switch(t) {
  case  1: ii=ik(x); gk_st_arr(*v,&ii,1,si); *v+=si; break;
  case  2: gk_st_arr(*v,&fk(x),1,sd); *v+=sd; break;
  case  8: gk_st_arr(*v,&jk(x),1,sd); *v+=sd; break;
  case  9: { float ef=ek(x); gk_st_arr(*v,&ef,1,si); *v+=si; } break;
  case  3: cc=ck(x); memcpy(*v,&cc,1); *v+=1; break;
  case  4: pxc=sk(x); memcpy(*v,pxc,1+strlen(pxc)); *v+=1+strlen(pxc); break;
  case  6: case 10: break;
  case -4: PXS; gk_st_arr(*v,&nx,1,sn); *v+=sn; i(nx,memcpy(*v,pxs[i],1+strlen(pxs[i]));*v+=1+strlen(pxs[i])); break;
  case -3: PXC; gk_st_arr(*v,&nx,1,sn); *v+=sn; memcpy(*v,pxc,nx); *v+=nx; break;
  case -2: PXF; gk_st_arr(*v,&nx,1,sn); *v+=sn; gk_st_arr(*v,pxf,nx,sd); *v+=nx*sd; break;
  case -8: { i64 *pxj=px(x); gk_st_arr(*v,&nx,1,sn); *v+=sn; gk_st_arr(*v,pxj,nx,sd); *v+=nx*sd; } break;
  case -9: { float *pxe=px(x); gk_st_arr(*v,&nx,1,sn); *v+=sn; gk_st_arr(*v,pxe,nx,si); *v+=nx*si; } break;
  case -1: PXI; gk_st_arr(*v,&nx,1,sn); *v+=sn; gk_st_arr(*v,pxi,nx,si); *v+=nx*si; break;
  case  0: PXK;
    if(0xc3==s(x))  {
      /* The parse result (slot 1) is rebuilt from the definition text on the
         far side, so it is never shipped.  The scope (slot 2) is not shippable
         either -- it chains up through the globals into the ktree, and is
         cyclic -- but a CLOSURE's captured bindings are, so ship those as a
         dict in its place (fncap above).  A plain lambda captures nothing and
         still writes `null` here, exactly as before. */
      K cap=fncap(x);
      gk_st_arr(*v,&nx,1,sizeof(u64)); *v+=sizeof(u64);;
      i(nx,p=serialize(i==1?null:(i==2?(cap?cap:null):pxk[i]),b,v,n,m);
           if(E(p)) { _k(cap); --d; return p;})
      _k(cap);
    }
    else {
      /* a dict ships its 3 semantic slots (keys;values;capacity); the
         hash-index cache (slot 4, dict.c) is runtime-only, so the wire
         format is unchanged.  db re-adds a null slot on arrival. */
      u64 nw=(0x80==st && nx>3)?3:nx;
      gk_st_arr(*v,&nw,1,sn); *v+=sn;
      i(nw,p=serialize(pxk[i],b,v,n,m); if(E(p)) { --d; return p;})
    }
    break;
  default: --d; return KERR_TYPE;
  }
  --d;
  return null;
}

/* Projection argument containers are either an ordinary general list or the
   0x81 plist spelling used by bracket application.  Other T=0 subtypes are
   functions/dicts with private slots, not argument lists. */
static int bd_arglist(K x) {
  return T(x)==0 && (!s(x) || s(x)==0x81);
}

/* Runtime wrapper slots that hold verbs must contain a real callable K.
   ISF also admits the raw <256 primitive representation, but serialize cannot
   emit such an inline value as a child object: all stored primitives use the
   ordinary K atom spelling with subtype 0xc0. */
static int bd_callable(K x) {
  return x>=256 && ISF(x);
}

static int bd_adverbs(K x) {
  if(s(x) || T(x)!=-3) return 0;
  char *p=px(x);
  for(u64 i=0;i<n(x);++i)
    if(p[i]!='\'' && p[i]!='/' && p[i]!='\\') return 0;
  return 1;
}

/* Pre-Pass-3b-3 IPC/serialize encoded a builtin projection as 0xc8
   with shape (verb_sym_K, bound_arg, null).  Pass 3b-3 / Pass 4
   replaced these with 0xd9(verb_sym_K, [bound_arg]) -- the av slot
   was always null on legacy 0xc8 producers and is dropped. */
static K bd_legacy_c8_to_d9(K r) {
  K *pr=px(r);
  if(n(r)<2 || !bd_callable(pr[0])) { _k(r); return KERR_PARSE; }
  K verb=k_(pr[0]);
  K bound=k_(pr[1]);
  _k(r);
  K args=tn(0,1); ((K*)px(args))[0]=bound;
  args=st(0x81,args);
  K w=tn(0,2); K *pw=px(w);
  pw[0]=verb; pw[1]=args;
  return st(0xd9,w);
}

/* Pre-Pass-3b-2 IPC/serialize encoded primitive arity-3/4 projections
   as 0xd5/0xd6 with a flat (verb, arg0, arg1, arg2[, arg3]) tuple.
   Pass 3b-2 / Pass 4 replaced these with 0xd9(verb, [arg0, arg1,
   arg2[, arg3]]) -- same args, but wrapped in an extra plist. */
static K bd_legacy_d5d6_to_d9(K r) {
  K *pr=px(r);
  u64 arity=n(r)-1;
  if(arity<3 || !bd_callable(pr[0])) { _k(r); return KERR_PARSE; }
  K verb=k_(pr[0]);
  K args=tn(0,arity); K *pa=px(args);
  for(u64 i=0;i<arity;++i) pa[i]=k_(pr[1+i]);
  args=st(0x81,args);
  _k(r);
  K w=tn(0,2); K *pw=px(w);
  pw[0]=verb; pw[1]=args;
  return st(0xd9,w);
}

/* Pre-Pass-3b-5 IPC/serialize encoded a lambda projection as a t==0
   plist with subtype 0xc4 holding (lambda, pargs, av). Pass 3b-5
   replaced them with 0xd9 (f;args), with the av (if any) lifted to an
   outer 0xda(0xd9, av) wrapper. Any disk file written via 1: or IPC
   payload received from a pre-Pass-3b-5 peer still has the old
   encoding, so transform the legacy 3-tuple on read.  Returns the new
   K, freeing the legacy. */
static K bd_legacy_c4_to_d9(K r) {
  K *pr=px(r);
  if(n(r)<3 || !bd_callable(pr[0]) || !bd_arglist(pr[1])
     || (pr[2]!=null && !bd_adverbs(pr[2]))) {
    _k(r); return KERR_PARSE;
  }
  K lam=k_(pr[0]);
  K pargs=k_(pr[1]);
  K av=k_(pr[2]);
  _k(r);
  K w=tn(0,2); K *pw=px(w);
  pw[0]=lam; pw[1]=pargs;
  K d9=st(0xd9,w);
  if(T(av)==-3 && n(av)>0) {
    K w2=tn(0,2); K *pw2=px(w2);
    pw2[0]=d9; pw2[1]=av;
    return st(0xda,w2);
  }
  _k(av);
  return d9;
}

/* Backward-compat fixup for the Issue #2 pass 1a wrapper migration:
   0xc1 (+/x) and 0xc2 (a+/x) modified-verb K's used to be plain -3
   strings carrying "verb_char + adverbs" (e.g. "+/'"). Pass 1a
   replaced them with 0xda (f;av) tuples. Any disk file written via
   1: or IPC payload received from a pre-Pass-1a peer still has the
   old encoding, so peel the legacy string into the new wrapper on
   read. New writes serialize as 0xda directly so this only ever
   triggers for legacy data. Returns the new K, freeing the legacy. */
static K bd_legacy_mv_to_da(K r, int dyad) {
  static const char *P=":+-*%&|<>=~.!@?#_^,$'/\\";
  char *mv=(char*)px(r);
  if(!*mv) { _k(r); return KERR_PARSE; }
  const char *vp=strchr(P,*mv);
  if(!vp || vp-P>20) { _k(r); return KERR_PARSE; }
  for(char *p=mv+1;*p;++p)
    if(*p!='\'' && *p!='/' && *p!='\\') { _k(r); return KERR_PARSE; }
  int vi=(int)(vp-P);
  K f=t(1,st(0xc0,(K)((dyad?64:32)+vi)));
  int alen=(int)strlen(mv+1);
  K av=tn(3,alen);
  if(alen) memcpy((char*)px(av),mv+1,alen);
  K w=tn(0,2); K *pw=px(w);
  pw[0]=f; pw[1]=av;
  _k(r);
  return st(0xda,w);
}

/* Nesting depth of the deserialize recursion; see the guard at its one
   recursive call site.  Reset at every top-level entry (db_buf_) so a depth
   error on one blob cannot leak a raised count into the next. */
static i32 dsdepth=0;

static K deserialize(char **b,u64 *m) {
  K r=0,*prk,p;
  i32 t=0,st=0,*pri,ii;
  u64 si=sizeof(i32),sd=sizeof(double),sn=sizeof(u64),n=*m,count;
  char cc,**prs,*prc;
  double ff,*prf;
  if(n<si*3) return KERR_LENGTH;
  gk_ld_arr(&t,*b,1,si); *b+=si;
  gk_ld_arr(&st,*b,1,si); *b+=si;
  /* The subtype is an 8-bit field (s(x) = x>>48 & 0xff); serialize never emits
     more.  A hostile blob (untrusted IPC/db payload) that sets high bits gets
     them OR'd into the K via `r|(K)st<<48` below -- past bit 55 that lands in
     the TYPE field, forging e.g. an int atom into a fake heap vector with an
     attacker-picked low-48 "pointer" (a type-confusion primitive: a crafted
     st=0x30e8 on an int atom deserialized to 0x31e8000000000001, whose k_
     dereferenced address 0x1).  Reject anything a real subtype can't be. */
  if((u32)st > 0xff) return KERR_TYPE;
  /* Whitelist the subtype to exactly what serialize emits: plain data (0), a
     dict (0x80), a packed args plist (0x81), or a verb/function/projection
     (0xc0..0xdc, each shape-validated further below).  Every OTHER value is an
     INTERNAL-only tag the wire never legitimately carries -- a parse node
     (0x40..0x45), a raw adverb (0x85), an eval state -- and the reducer/apply
     machinery interprets those structurally, dereferencing the value's payload
     as a pointer (a crafted t=1/st=0x41 int atom made the `$[...]` reducer read
     its payload as a parse-node ko -> wild deref).  A hostile blob is the only
     source; reject up front so no downstream consumer can be fed one. */
  if(st && st!=0x80 && st!=0x81 && !(st>=0xc0 && st<=0xdc)) return KERR_TYPE;
  *b+=si; /* pad to 8 byte alignment for double */
  n-=3*si;
  switch(t) {
  /* every case must decrement n by exactly what it advances *b, not only by
     the count-header: a nested list (case 0) threads the SAME n across its
     siblings, so a payload left unsubtracted lets a later sibling's n<...
     guard pass while *b has already run past a truncated buffer -- a heap
     over-read on any corrupt/short blob (db of untrusted IPC data). */
  case  1: if(n<si) return KERR_LENGTH; gk_ld_arr(&ii,*b,1,si); r=t(1,(u32)ii); *b+=si; n-=si; break;
  case  2: if(n<sd) return KERR_LENGTH; gk_ld_arr(&ff,*b,1,sd); r=t2(ff); *b+=sd; n-=sd; break;
  case  8: if(n<sd) return KERR_LENGTH; { i64 jj; gk_ld_arr(&jj,*b,1,sd); r=tj(jj); *b+=sd; n-=sd; } break;
  case  9: if(n<si) return KERR_LENGTH; { float ef; gk_ld_arr(&ef,*b,1,si); r=te(ef); *b+=si; n-=si; } break;
  case  3: if(n<1) return KERR_LENGTH; memcpy(&cc,*b,1); r=t(3,(u8)cc); *b+=1; n-=1; break;
  case  4: { size_t len=strnlen(*b,n); if(len>=n) return KERR_LENGTH; r=t(4,sp(*b)); *b+=1+len; n-=1+len; } break;  /* strnlen not strlen: an untrusted (IPC) blob need not be nul-terminated within the buffer */
  case  6: r=null; break;
  case 10: r=inull; break;
  case -4:
    if(n<sn) return KERR_LENGTH;
    gk_ld_arr(&count,*b,1,sn);
    *b+=sn; n-=sn;
    /* min 1 byte/elem (the trailing nul); guards i32 overflow in PRS too */
    if(count>n||count>=(u64)VMAX) return KERR_LENGTH;
    PRS(count);
    i(nr,size_t len=strnlen(*b,n); if(len>=n) { _k(r); return KERR_LENGTH; } prs[i]=sp(*b); n-=1+len; *b+=1+len) break;  /* len>=n (not n<len, which strnlen makes impossible): need a nul WITHIN the remaining n bytes, else n-=1+len underflows.  _k(r): free the partially-built symbol vector on a mid-loop truncation, else it leaks (the only per-element return in deserialize). */
  case -3:
    if(n<sn) return KERR_LENGTH;
    gk_ld_arr(&count,*b,1,sn);
    *b+=sn; n-=sn;
    if(count>n||count>=(u64)VMAX) return KERR_LENGTH;
    PRC(count);
    memcpy(prc,*b,nr); *b+=nr; n-=nr; break;
  case -2:
    if(n<sn) return KERR_LENGTH;
    gk_ld_arr(&count,*b,1,sn);
    *b+=sn; n-=sn;
    if(count>n/sd||count>=(u64)VMAX) return KERR_LENGTH;
    PRF(count);
    gk_ld_arr(prf,*b,nr,sd); *b+=nr*sd; n-=nr*sd; break;
  case -8:
    if(n<sn) return KERR_LENGTH;
    gk_ld_arr(&count,*b,1,sn);
    *b+=sn; n-=sn;
    if(count>n/sd||count>=(u64)VMAX) return KERR_LENGTH;
    { i64 *prj; PRJ(count); gk_ld_arr(prj,*b,nr,sd); *b+=nr*sd; n-=nr*sd; } break;
  case -9:
    if(n<sn) return KERR_LENGTH;
    gk_ld_arr(&count,*b,1,sn);
    *b+=sn; n-=sn;
    if(count>n/si||count>=(u64)VMAX) return KERR_LENGTH;
    { float *pre; PRE(count); gk_ld_arr(pre,*b,nr,si); *b+=nr*si; n-=nr*si; } break;
  case -1:
    if(n<sn) return KERR_LENGTH;
    gk_ld_arr(&count,*b,1,sn);
    *b+=sn; n-=sn;
    if(count>n/si||count>=(u64)VMAX) return KERR_LENGTH;
    PRI(count);
    gk_ld_arr(pri,*b,nr,si); *b+=nr*si; n-=nr*si; break;
  case  0:
    if(n<sn) return KERR_LENGTH;
    gk_ld_arr(&count,*b,1,sn);
    *b+=sn; n-=sn;
    /* every nested element occupies at least 3*sizeof(i32) bytes (header) */
    if(count>n/(3*si)||count>=(u64)VMAX) return KERR_LENGTH;
    PRK(count);
    /* Depth guard, mirroring serialize's at b.c:1912.  Without it a nested
       blob recurses once per ~20 wire bytes, so a ~1MB message or file blows
       the C stack -- and this is untrusted input (db x, 2:/1: file read, and
       an unauthenticated IPC frame via db_buf).  Guarding HERE rather than at
       function entry is deliberate: depth only grows through this one call,
       and deserialize has ~60 return points, so an entry/exit counter would
       have to be threaded through every one of them. */
    if(++dsdepth>maxr || (!(dsdepth&7)&&stack_low())) { --dsdepth; _k(r); return KERR_STACK; }
    i(nr,p=deserialize(b,&n); if(E(p)) { --dsdepth; _k(r); return p; } prk[i]=p)
    --dsdepth;
    break;
  default: return KERR_TYPE;
  }
  *m=n;
  /* Every subtype rides exactly ONE base type: serialize only ever emits 0xc3
     on a list, 0xc6 on an int atom, 0xd8 on a symbol, and so on.  Each guard
     below (and each consumer in fe.c/fn.c/k.c) reads the payload the way that
     one pairing implies -- so pin the base type HERE, before any of them.
     Checking shape alone is not enough, because every such check is itself
     keyed on the base type and is simply SKIPPED on a mismatch: a list subtype
     on an atom slid past the `t==0` guards (a t=2/0xc3 float atom reached
     kprint_'s closure arm, which read the double's bits as a ko* -> SEGV; a
     t=1/0xdc int atom rebuilt the arbitrary-call primitive the 0xdc reject
     exists to kill), and symmetrically an atom subtype on a LIST slid past the
     `t>0` guards (a t=0/0xc6 list skipped the BMONAD bound check, so apply
     CALLed a table read off the end).  Deserializing 0xc0..0xdc x every base
     type crashed in 68 combinations before this switch; the default arm makes
     the pairing exhaustive, so a new subtype must name its base type to ship.
     Reject rather than coerce: a mismatch is never a value serialize wrote. */
  switch(st) {
  case 0: break;                                    /* plain data: any base type */
  case 0x80: case 0x81:                             /* dict; packed-args plist */
  case 0xc3: case 0xc4: case 0xc5: case 0xc8:       /* lambda/closure; legacy projections; composition */
  case 0xd0: case 0xd4: case 0xd5: case 0xd6:       /* fixed-dyad keeper; legacy primitive projections */
  case 0xd7: case 0xd9: case 0xda:                  /* (f;args)/(f;av) wrappers */
    if(t!=0) { _k(r); return KERR_TYPE; } break;
  case 0xc1: case 0xc2:                             /* legacy modified verb: verb char + arg string */
    if(t!=-3) { _k(r); return KERR_TYPE; } break;
  case 0xc0: case 0xc6: case 0xc7:                  /* verb char; int-index builtins (payload = table index) */
  case 0xce: case 0xcf:                             /* `+:` / `a+:` assign forms */
    if(t!=1) { _k(r); return KERR_TYPE; } break;
  case 0xc9: case 0xca: case 0xcb: case 0xcc: case 0xcd:
  case 0xd1: case 0xd2: case 0xd3: case 0xd8:       /* name-based verbs: payload = interned symbol */
    if(t!=4) { _k(r); return KERR_TYPE; } break;
  case 0xdb: _k(r); return KERR_TYPE;               /* retired in Pass 1b (replaced by 0xda): no consumer */
  case 0xdc: _k(r); return KERR_TYPE;               /* raw process-local code pointer: never deserializable */
  default: _k(r); return KERR_TYPE;
  }
  /* Structural subtypes route the deserialized value to a handler that reads
     fixed slots (fnrestore, the legacy converters); a hostile blob can bless
     a short/empty list with such a subtype and drive those reads out of
     bounds (a crafted 0xc3 on a <3-slot list overran fnrestore's pf[2]; a
     0xd5/0xd6 on an empty list underflowed n(r)-1 to a huge arity).  The
     converters that already length-check (c4/c8/d4) are fine; guard the rest
     to the exact shape their serialize side emits before routing. */
  if(t==-3 && (st==0xc1 || st==0xc2)) {
    /* legacy modified-verb K (see bd_legacy_mv_to_da above): a verb char
       plus an argument string -- needs at least the verb byte */
    if(!n(r)) { _k(r); return KERR_TYPE; }
    *m=n;
    return bd_legacy_mv_to_da(r, st==0xc2);
  }
  if(t==0 && st==0xc3) {
    /* a closure ships its captured bindings as a dict in the scope slot; turn
       them back into a frozen parent frame (fnrestore).  A plain lambda has
       `null` there and passes straight through.  A serialized lambda is
       always 4 slots (fncap ships n(x)) with slot 0 the definition TEXT (a
       char vector, which fnd re-parses); a forged slot 0 of any other type is
       dereferenced by fnd as a raw ko* (px(pf[0])) -> wild/OOB read. */
    if(n(r)!=4) { _k(r); return KERR_TYPE; }
    K *pf=(K*)px(r);
    if(s(pf[0]) || T(pf[0])!=-3 || pf[1]!=null
       || (pf[2]!=null && s(pf[2])!=0x80)
       || s(pf[3]) || T(pf[3])!=1) {
      _k(r); return KERR_TYPE;
    }
    return fnrestore(r|(K)0xc3<<48);
  }
  if(t==0 && st==0xc4) {
    /* legacy lambda projection (see bd_legacy_c4_to_d9 above) */
    return bd_legacy_c4_to_d9(r);
  }
  if(t==0 && st==0xc8) {
    /* legacy builtin projection (see bd_legacy_c8_to_d9 above) */
    return bd_legacy_c8_to_d9(r);
  }
  /* Pre-Pass-3b-1 IPC/serialize encoded a primitive 2-arg projection as 0xd4
     (verb, args).  Pass 3b-1 / Pass 4 replaced these with 0xd9, which has the
     SAME shape, so the legacy fixup is a pure subtype rebrand -- and therefore
     must satisfy 0xd9's own arity guard below.  It used to convert-and-return
     right here, which skipped that guard: an empty list became a 0-slot 0xd9
     whose consumers read slots 0 and 1 -> heap over-read.  Rebrand and fall
     through instead, so the one guard covers both spellings by construction. */
  if(t==0 && st==0xd4) st=0xd9;
  if(t==0 && (st==0xd5 || st==0xd6)) {
    /* legacy primitive arity-3/4 projection (bd_legacy_d5d6_to_d9): it forms
       arity=n(r)-1, so an empty list underflows to a huge arity -- guard */
    if(!n(r)) { _k(r); return KERR_TYPE; }
    return bd_legacy_d5d6_to_d9(r);
  }
  if(t==0 && st==0x80) {
    /* a dict ships (keys; values; capacity).  dget/dset/dvals index the
       values vector by a key-slot index bounded only against the KEY count,
       so a hostile blob whose values vector is shorter than its keys vector
       drives an out-of-bounds read/write; the 3 slots are also read
       unconditionally (atcb/dotcb/valuecb), so a <3-slot forged dict
       over-reads.  Reject anything that isn't a real dict shape here. */
    if(!r || E(r) || n(r)<3) { _k(r); return KERR_TYPE; }
    K *pk=px(r);
    /* Pin slot 1's TYPE before reading its count, and pin it to the general
       list dnew() always builds (dict.c: v=tn(0,DMAX)).  Two bugs otherwise:
       n(pk[1]) is a raw deref of the low 48 bits, so for a non-heap element
       (int/char/null/f32) it reads 8 bytes at the blob's own inline payload --
       an arbitrary-address read inside db itself; and comparing only the
       COUNT let a -1 int vector (4 bytes/elt) or -3 char vector (1 byte/elt)
       through, after which dict.c's pv=px(pd[1]) / k_(pv[i]) walk it 8 bytes
       at a time -- reading past the buffer and treating attacker bytes as a
       tagged K, so k_() does ++((ko*)(b(48)&x))->r: a +1 write at any address.
       Reachable from an unauthenticated IPC frame via db_buf.  || is
       left-to-right and short-circuits, so the type test guards the count. */
    if(s(pk[0]) || T(pk[0])!=-4 || s(pk[1]) || T(pk[1])!=0
       || n(pk[0])!=n(pk[1])) { _k(r); return KERR_TYPE; }
    /* dicts ship 3 payload slots; the in-memory form carries a 4th (the
       hash-index cache, dict.c).  Re-add it as null -- and drop any crafted
       extra slots from a hostile blob, which dget/dset would otherwise
       treat as a table. */
    u64 j,n0=n(r);
    K *pr4;
    if(n0==3) { r=kresize(r,4); pr4=px(r); pr4[3]=null; }
    else { pr4=px(r); for(j=3;j<n0;j++) { _k(pr4[j]); pr4[j]=null; } n(r)=4; }
    /* Slot 2 is the key/value allocation capacity (dset's resize threshold).
       The wire ships the SENDER's capacity (e.g. 64 after doubling), but the
       receive-side vectors are rebuilt exactly count-sized.  A stale capacity
       > the real allocation makes dset's append skip its resize and write past
       the vector -- a heap overflow (and an arbitrary one from a hostile blob).
       The rebuilt vectors are exactly count-sized, so the true capacity is the
       key count; resetting it is always safe (next append just resizes). */
    _k(pr4[2]); pr4[2]=t(1,(u32)n(pr4[0]));
  }
  /* The remaining structural subtypes attach to a t=0 list whose fixed slots
     the apply/eval machinery (fe.c/fn.c) dereferences as verbs, args, or (for
     0xdc) a raw C function pointer -- none re-checked downstream, because only
     trusted internal code builds these.  A hostile db/IPC blob is the one
     place untrusted bytes forge one, so validate the shape here:
       (0xdc -- a raw process-local code pointer -- is rejected outright by the
       base-type pin above, so it never reaches here.)
       0xd9/0xda/0xd0/0xd7  (f;args)/(f;av) wrappers: consumers read slots 0,1.
       0xc5  composition: fc reads slot 0 as a sub-list of verbs. */
  if(t==0) {
    u64 nlen=n(r);
    K *pw=(K*)px(r);
    switch(st) {
    case 0xd9:
      /* kprint_ (k.c:446) walks slot 1 as a general list -- `K *pg2=px(g2);
         i(n(g2),kprint_(pg2[i],...))` -- with no type test of its own, so a
         forged ATOM in slot 1 makes px() dereference the blob's own inline
         payload: merely DISPLAYING the decoded value segfaults.  p.c/fe.c
         only ever build slot 1 as a type-0 list (plain, or 0x81-subtyped via
         wrap_proj), so pin both fields to the shape the apply path expects. */
      if(nlen!=2 || !bd_callable(pw[0]) || !bd_arglist(pw[1])) {
        _k(r); return KERR_TYPE;
      }
      break;
    case 0xda:
      /* The operand may intentionally be a noun (e.g. 5'), but the suffix is
         an exact parser adverb string. */
      if(nlen!=2 || !bd_adverbs(pw[1])) {
        _k(r); return KERR_TYPE;
      }
      break;
    case 0xd0:
      if(nlen!=2 || !bd_callable(pw[1])) { _k(r); return KERR_TYPE; }
      break;
    case 0xd7:
      /* Seeded over/scan accepts only the three inner shapes fe() handles. */
      if(nlen!=2 || (s(pw[1])!=0xc3 && s(pw[1])!=0xd9 && s(pw[1])!=0xda)) {
        _k(r); return KERR_TYPE;
      }
      break;
    case 0xc5:
      /* a composition is a 2-slot list: slot 0 the (non-empty) list of
         composed fns, slot 1 the adverb char-vector.  Both slots are read
         downstream -- kprint_ and the fe.c av-peel dereference px[1] (the av),
         and fc reads px[0][n-1] -- so nlen>=1 was not enough: a 1-slot 0xc5
         drove kprint_'s px[1] off the end of the list (heap over-read), and an
         empty slot-0 list underflows fc's n(f)-1 to a -1 index.  Pin the exact
         shape serialize emits (see p.c: tn(0,2), pw[0]=fn-list, pw[1]=tn(3,0)). */
      if(nlen!=2 || s(pw[0]) || T(pw[0])!=0 || !n(pw[0])
         || !bd_adverbs(pw[1])) { _k(r); return KERR_TYPE; }
      K *pf=px(pw[0]);
      for(u64 i=0;i<n(pw[0]);++i)
        if(!bd_callable(pf[i])) { _k(r); return KERR_TYPE; }
      break;
    }
  }
  /* The int-index builtins (0xc6 monad, 0xc7 dyad) use the atom's payload
     DIRECTLY as a BMONAD/BDYAD table index, so a forged index yields a garbage
     function pointer that apply then CALLS (a second RCE beyond 0xdc) or that
     print dereferences.  The base type is pinned to t=1 above; bound the index
     itself here.  (0xc0 is reduced modulo 32; k() bounds invalid opcodes and
     the printer bounds its P-table lookup.  Name-based verbs only need the
     t=4 pin, so neither needs an additional payload check here.) */
  switch(st) {
  case 0xc6: if((u32)ik(r)>=(u32)NBMONAD) { _k(r); return KERR_TYPE; } break;
  case 0xc7: if((u32)ik(r)>=(u32)NBDYAD)  { _k(r); return KERR_TYPE; } break;
  }
  return r|(K)st<<48;
}

/* Serialize x into a caller-provided growable buffer. *buf may be NULL
 * (then *cap must be 0); on return *buf is non-NULL and *len is the
 * total bytes written (including the 4-byte stream header). *cap is the
 * current backing-allocation size (>= *len). *cap and *buf may be reused
 * across calls to amortize the alloc + page-fault cost on hot paths.
 * Returns 0 on success, an error K on failure (buffer state preserved
 * but contents undefined on error). */
K bd_into(K x, char **buf, u64 *cap, u64 *len) {
  i64 m = (i64)*cap, n = 4;
  if(m < 4) { m = 4; *buf = xrealloc(*buf, (size_t)m); }
  char *b = *buf, *v = b, h[4] = {3,0,0,0};  /* v3: adds 64-bit int (type 8) */
  memcpy(v, h, 4); v += 4;
  K t = serialize(x, &b, &v, &n, &m);
  *buf = b; *cap = (u64)m; *len = (u64)(v - b);
  return t;  /* `null` on success, error K on failure */
}

K bd_(K x) {
  K r;
  char *b = NULL;
  u64 cap = 0, len = 0;
  K e = bd_into(x, &b, &cap, &len);
  if(E(e)) { xfree(b); return e; }
  r = tnc(3, len, b);
  if(E(r)) { xfree(b); return r; }
  return r;
}

/* Deserialize directly from a raw byte buffer. Lets callers (the IPC
 * recv path) skip wrapping the receive buffer in a K char vector, which
 * matters because that wrapper would force the receive buffer to be
 * adopted by the K vector and re-allocated on every message. */
K db_buf(const char *buf, u64 nbytes) {
  if(nbytes < 4) return KERR_LENGTH;
  /* accept v3 (current) and v2 (gk v1): the v2->v3 change was purely additive
     (i64/f32 type arms), so a v2 blob -- which never contains those types --
     deserializes identically. v2 always writes v3. */
  if(buf[0] != 3 && buf[0] != 2) return KERR_TYPE;
  char *b = (char*)buf + 4;
  u64 n = nbytes - 4;
  dsdepth=0;
  return deserialize(&b, &n);
}

K db_(K x) {
  if(s(x) || tx != -3) return KERR_TYPE;
  return db_buf((const char*)px(x), nx);
}

K hb_(K x) {
  K r=0;
  char *prc,*pxc;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  //case  3: PRC(2); sprintf(prc,"%02x",ck(x)); break; /* breaks round trip */
  case -3:
    if(nx>(VMAX-1)/2) return KERR_WSFULL;
    PRC(1+2*nx); PXC; i(nx,sprintf(prc,"%02x",(u8)pxc[i]);prc+=2); nr--; break;
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
    /* hex is exactly two chars per byte, so odd input is malformed: it does
       not round-trip (hb bh "abc" would be "0abc", not "abc").  The old code
       tried to pad a leading zero but did it wrong (dropped/garbled a byte);
       a length error is the honest answer. */
    if(nx%2) return KERR_LENGTH;
    PRC(nx/2); PXC;
    i(nx/2, sprintf(b,"0x%c%c",pxc[0],pxc[1]); *prc++=strtol(b,0,0); pxc+=2; )
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
  r=tnc(3,total,pb);
  if(E(r)) { xfree(pb); return r; }
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
  r=tnc(3,q->n,q->b);
  if(E(r)) { xfree(q->b); xfree(q); return r; }
  ((char*)px(r))[nr]=0; /* zero terminate */
  xfree(q);
  return r;
}

K md5_(K x) {
  K r=0,e,*prk;
  char *prc,*pxc,**pxs;
  if(s(x)) return KERR_TYPE;
  /* general lists recurse via case 0/irecur1 below (a non-char/sym leaf
     hits the default type error); no flat-list-only pre-check */
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
  K r=0,e,*prk;
  char *prc,*pxc,**pxs;
  if(s(x)) return KERR_TYPE;
  /* general lists recurse via case 0/irecur1 below (a non-char/sym leaf
     hits the default type error); no flat-list-only pre-check */
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
  K r=0,e,*prk;
  char *prc,*pxc,**pxs;
  if(s(x)) return KERR_TYPE;
  /* general lists recurse via case 0/irecur1 below (a non-char/sym leaf
     hits the default type error); no flat-list-only pre-check */
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

/* Cap env var NAME length and BATCH (vector) element count.  Env names are
 * tiny in practice and the OS bounds the whole env block (ARG_MAX); this just
 * rejects absurd (e.g. big-vector) inputs with a clean length error instead of
 * a pointless huge alloc.  VALUES are intentionally NOT capped here (real ones
 * like LS_COLORS/PATH exceed this). */
#define ENVMAX 4096

/* Caller must xfree() (or free()) the result. NULL if unset. */
static char* ge(char *k) {
#ifdef _MSC_VER
  /* MSVC deprecates getenv; _dupenv_s mallocs (ge_free uses free()).  MinGW
     (incl. msvcrt-based MINGW64, which has no _dupenv_s) takes the getenv path. */
  char *p=0; _dupenv_s(&p,0,k); return p;
#else
  char *p=getenv(k);
  return p ? xstrndup(p,strlen(p)) : 0;
#endif
}

static void ge_free(char *p) {
  if(!p) return;
#ifdef _MSC_VER
  free(p);
#else
  xfree(p);
#endif
}

K getenv_(K x) {
  K r=null,e,*prk;
  char *p,**pxs;
  if(s(x)) return KERR_TYPE;
  switch(tx) {
  case -3: if(nx>ENVMAX) return KERR_LENGTH; p=ge((char*)px(x)); if(p){r=tnv(3,strlen(p),xstrndup(p,strlen(p))); ge_free(p);} break;
  case  4: p=ge(sk(x));        if(p){r=tnv(3,strlen(p),xstrndup(p,strlen(p))); ge_free(p);} break;
  case -4: if(nx>ENVMAX) return KERR_LENGTH; PRK(nx); PXS; i(nx,prk[i]=getenv_(t(4,pxs[i])); EC(prk[i])) break;
  case  0: if(nx>ENVMAX) return KERR_LENGTH; r=irecur1(getenv_,x); break;
  default: r=KERR_TYPE;
  }
  return r;
cleanup:
  _k(r);
  return e;
}

/* Returns 0 on success, else an errno value. */
static int se(char *k, char *v) {
#ifdef _WIN32
  return _putenv_s(k,v);               /* 0, or an errno value */
#else
  return setenv(k,v,1)==0 ? 0 : errno; /* normalize -1 to errno */
#endif
}

/* Map a setenv/_putenv_s failure to a sensible K error.  The realistic causes
 * are EINVAL (empty name or a name containing '=') -> domain, and ENOMEM ->
 * wsfull.  (NB setenv does NOT enforce ARG_MAX -- a long value just succeeds.) */
static K se_err(int rc) {
  return rc==ENOMEM ? KERR_WSFULL : KERR_DOMAIN;
}

K setenv_(K a, K x) {
  K e,*pak,*pxk;
  char *p,*q,**pas,**pxs;
  if(s(a)||s(x)) return KERR_TYPE;
  if((ta==0||ta==-4)&&(tx==0||tx==-4)&&(na!=nx)) return KERR_LENGTH;
  switch(ta) {
  case -3:
    if(na>ENVMAX) return KERR_LENGTH;   /* name length; value (nx) uncapped */
    switch(tx) {
    case -3: p=xstrndup(px(a),na); q=xstrndup(px(x),nx); break;
    case  4: p=xstrndup(px(a),na); q=xstrndup(sk(x),strlen(sk(x))); break;
    default: return KERR_TYPE;
    }
    {int rc=se(p,q); xfree(p); xfree(q); if(rc) return se_err(rc);}
    break;
  case  4:
    switch(tx) {
    case -3: p=xstrndup(sk(a),strlen(sk(a))); q=xstrndup((char*)px(x),nx); break;
    case  4: p=xstrndup(sk(a),strlen(sk(a))); q=xstrndup(sk(x),strlen(sk(x))); break;
    default: return KERR_TYPE;
    }
    {int rc=se(p,q); xfree(p); xfree(q); if(rc) return se_err(rc);}
    break;
  case -4:
    if(na>ENVMAX) return KERR_LENGTH;   /* batch count; values uncapped */
    switch(tx) {
    case -4: PAS; PXS; i(nx,{int rc=se(pas[i],pxs[i]); if(rc) return se_err(rc);}); break;
    case  0: PAS; PXK; i(na,EC(setenv_(t(4,pas[i]),pxk[i]))); break;
    default: return KERR_TYPE;
    } break;
  case  0:
    if(na>ENVMAX) return KERR_LENGTH;   /* batch count; values uncapped */
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

static inline i64 bin1j(void *v, i32 t, i64 n, i64 y) {
  i64 lo=0,hi=n,m;
  while(lo<hi) { m=lo+((hi-lo)>>1);
    if(((t==-1)?ji(((i32*)v)[m]):((i64*)v)[m])<y) lo=m+1; else hi=m; }
  return lo;
}
static inline i64 bin1f(void *v, i32 t, i64 n, f64 y) {
  i64 lo=0,hi=n,m;
  f64 am;
  while(lo<hi) { m=lo+((hi-lo)>>1);
    /* fi()/fj(), not plain casts: an int/long HAYSTACK entry that is a
       sentinel must widen to the float sentinel, or the ordering the search
       relies on disagrees with grade and with `<`.  (f32->f64 is exact for
       IEEE inf/NaN, so that arm stays a plain cast.) */
    am=(t==-1)?fi(((i32*)v)[m]):(t==-8)?fj(((i64*)v)[m]):(t==-2)?((f64*)v)[m]:(f64)((f32*)v)[m];
    /* total order: 0n sorts below everything (matches grade and the < verb),
       but C < is always false on NaN, so a null in the haystack would stop
       the search from ever moving right */
    if(am<y || (isnan(am) && !isnan(y))) lo=m+1; else hi=m; }
  return lo;
}
K bin_(K a, K x) {
  if(s(a)||s(x)) return KERR_DOMAIN;
  if(ta>0) return KERR_RANK;
  if(ta!=-1&&ta!=-2&&ta!=-8&&ta!=-9) return KERR_DOMAIN;
  if(tx!=1&&tx!=2&&tx!=8&&tx!=9) return KERR_TYPE;
  void *v=px(a);
  i64 c;
  if((ta==-1||ta==-8)&&(tx==1||tx==8))
    c=bin1j(v,ta,(i64)na,tx==1?ji(ik(x)):jk(x));
  else
    /* same for the NEEDLE: `1e10 2e10 bin 0I` gave 0 while `bin 0i` gave 2. */
    c=bin1f(v,ta,(i64)na,tx==1?fi(ik(x)):tx==8?fj(jk(x)):tx==2?fk(x):(f64)ek(x));
  return na>BIGV?tj(c):t(1,(u32)c);
}
K binl_(K a, K x) {
  K r=0;
  i32 *pri=0;
  i64 *prj=0;
  if(!s(x)&&tx>0) return bin_(a,x);
  if(s(a)||s(x)) return KERR_DOMAIN;
  if(ta>0) return KERR_RANK;
  if(ta!=-1&&ta!=-2&&ta!=-8&&ta!=-9) return KERR_DOMAIN;
  if(tx!=-1&&tx!=-2&&tx!=-8&&tx!=-9) return KERR_TYPE;
  void *v=px(a), *w=px(x);
  i64 n=(i64)na, nn=(i64)nx, c;
  int ints=(ta==-1||ta==-8)&&(tx==-1||tx==-8);
  int big=n>(i64)BIGV;
  if(big) { PRJ(nn); } else { PRI(nn); }
  if(E(r)) return r;
  for(i64 j=0;j<nn;++j) {
    if(ints) c=bin1j(v,ta,n,tx==-1?ji(((i32*)w)[j]):((i64*)w)[j]);
    else { f64 y=tx==-1?fi(((i32*)w)[j]):tx==-8?fj(((i64*)w)[j]):tx==-2?((f64*)w)[j]:(f64)((f32*)w)[j];  /* binl needles: see bin_ */
           c=bin1f(v,ta,n,y); }
    if(big) prj[j]=c; else pri[j]=(i32)c;
  }
  return r;
}

/* a lin x : elementwise whole-value membership -- (x?/a)<#x, the
   elementwise twin of in_.  The batched x?/y fast path (findr, k.core/v.c)
   hashes x once, so this is O(#a+#x). */
K lin_(K a, K x) {
  u64 n;
  if(s(x)) return KERR_DOMAIN;
  if(tx>0) return KERR_DOMAIN;
  n=nx;
  K t=k(78,k_(x),k_(a));     /* x?/a */
  if(E(t)) return t;
  return k(7,t,n>BIGV?tj((i64)n):t(1,(u32)n)); /* t<#x */
}

K in_(K a, K x) {
  u64 n;
  if(s(x)) return KERR_DOMAIN;
  if(tx>0) return KERR_DOMAIN;
  n=nx;
  K t=k(14,k_(x),k_(a));     /* x?a */
  if(E(t)) return t;
  return k(7,t,n>BIGV?tj((i64)n):t(1,(u32)n)); /* t<nx */
}

K dvl_(K a, K x) {
  u64 n;
  if(s(a)||s(x)) return KERR_DOMAIN;
  if(ta!=6 && ta>0) return KERR_DOMAIN;
  if(tx>0) return KERR_DOMAIN;
  n=nx;
  K t=k(78,k_(x),k_(a)); /* x?/a */
  if(E(t)) return t;
  t=k(9,n>BIGV?tj((i64)n):t(1,(u32)n),t);  /* (#nx)=t */
  if(E(t)) return t;
  t=k(5,0,t);            /* &t */
  if(E(t)) return t;
  t=k(13,k_(a),t);       /* a@t */
  return t;
}

K exit__(K x) {
  if(s(x)||tx!=1) return KERR_TYPE;
  /* Tear down IPC before K state: ipc_shutdown closes any live conns
   * (fires .m.c handlers, which need K state alive) and frees the
   * persistent send-scratch buffer. */
  ipc_shutdown();
  tmr_shutdown();
  watch_shutdown();
  cc_shutdown();
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
  link_shutdown();
#ifdef FUZZING
  exit(0);
#endif
  exit(ik(x));
}

K exit_(K x) {
  if(s(x)||tx!=1) return KERR_TYPE;
  EXIT=x;
  return k_(x);
}

K dir_(K x) {
  char *s,*p,*ss,*pp,*sr,*skd=sk(D),*psx;
  K d,q,t=0,es,fs;
  size_t n;

  if(ecount && x!=null) return kerror("domain"); /* read-only \d (display) is safe while
                                                     suspended; changing namespace is not */
  if(x!=null && (s(x)||T(x)!=4)) return KERR_TYPE;

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
      gcache_clear(); /* gs changed: cache keys aren't scope-qualified */
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
        if(!(fs=scope_find(pp))) { _k(es); es=scope_newk(es,t(4,sp(pp)));
          if(E(es)) { _k(d); xfree(s); xfree(ss); xfree(pp); return es; } }
        else { _k(es); es=k_(fs); }
        _k(d); d=t;
      }
      else if(0x80==s(q)) { /* ensure scope path exists */
        _k(d); d=q;
        t=scope_get(es,t(4,sp(p)));
        if(E(t)) { /* nope */
          if(!(fs=scope_find(pp))) { _k(es); es=scope_newk(es,t(4,sp(pp)));
            if(E(es)) { _k(d); xfree(s); xfree(ss); xfree(pp); return es; } }
          else { _k(es); es=k_(fs); }
        }
        else if(!(fs=scope_find(pp))) {
          _k(es); es=scope_newk(es,t(4,sp(pp)));
          if(E(es)) { _k(d); _k(t); xfree(s); xfree(ss); xfree(pp); return es; }
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
    gcache_clear(); /* gs changed: cache keys aren't scope-qualified */
  }
  return null;
}
