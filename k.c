#include "k.h"
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include "scope.h"
#include "fn.h"
#include "ms.h"
#ifndef _WIN32
#include <sys/resource.h>
#endif
#include "b.h"
#include "fe.h"
#include "io.h"
#include "av.h"

i32 maxr=MAXR;
char *E[EMAX]={"nyi","rank","length","type","value","range","domain","valence","index","int","parse","nonce","stack","reserved","wsfull"};
i32 zeroclamp;

static char *P=":+-*%&|<>=~.!@?#_^,$'/\\";

/* reserved symbols */
char *R_NUL,*R_DRAW,*R_DOT,*R_VS,*R_SV,*R_ATAN2,*R_DIV,*R_AND,*R_OR,*R_SHIFT,*R_ROT,*R_XOR,*R_EUCLID,*R_ENCRYPT,*R_DECRYPT,*R_SETENV,*R_RENAME,*R_SQR,*R_ABS,*R_SLEEP,*R_IC,*R_CI,*R_DJ,*R_JD,*R_LT,*R_LOG,*R_EXP,*R_SQRT,*R_FLOOR,*R_CEIL,*R_SIN,*R_COS,*R_TAN,*R_ASIN,*R_ACOS,*R_ATAN,*R_AT,*R_SS,*R_SM,*R_LSQ,*R_SINH,*R_COSH,*R_TANH,*R_ERF,*R_ERFC,*R_GAMMA,*R_LGAMMA,*R_RINT,*R_TRUNC,*R_NOT,*R_KV,*R_VK,*R_VAL,*R_BD,*R_DB,*R_HB,*R_BH,*R_ZB,*R_BZ,*R_MD5,*R_SHA1,*R_SHA2,*R_GETENV,*R_SVD,*R_LU,*R_QR,*R_EXIT,*R_DEL,*R_DO,*R_WHILE,*R_IF,*R_IN,*R_DVL,*R_LIN,*R_DVL,*R_DV,*R_DI,*R_GTIME,*R_LTIME,*R_TL,*R_MUL,*R_INV,*R_CHOOSE,*R_ROUND,*R_SSR,*R_EP;

void kinit(void) {
#ifndef _WIN32
  struct rlimit rl;
  if(getrlimit(RLIMIT_STACK,&rl)) {
    fprintf(stderr,"error: kinit() failed\n");
    exit(1);
  }
  maxr=rl.rlim_cur / 10500;
  if(maxr>1000) maxr=1000;
#ifdef ASAN_ENABLED
  maxr=100;
#endif
#endif
  /* reserved */
  R_NUL=sp("nul");
  R_DRAW=sp("draw");
  R_DOT=sp("dot");
  R_VS=sp("vs");
  R_SV=sp("sv");
  R_ATAN2=sp("atan2");
  R_DIV=sp("div");
  R_AND=sp("and");
  R_OR=sp("or");
  R_SHIFT=sp("shift");
  R_ROT=sp("rot");
  R_XOR=sp("xor");
  R_EUCLID=sp("euclid");
  R_ENCRYPT=sp("encrypt");
  R_DECRYPT=sp("decrypt");
  R_SETENV=sp("setenv");
  R_RENAME=sp("rename");
  R_SQR=sp("sqr");
  R_ABS=sp("abs");
  R_SLEEP=sp("sleep");
  R_IC=sp("ic");
  R_CI=sp("ci");
  R_DJ=sp("dj");
  R_JD=sp("jd");
  R_LT=sp("lt");
  R_LOG=sp("log");
  R_EXP=sp("exp");
  R_SQRT=sp("sqrt");
  R_FLOOR=sp("floor");
  R_CEIL=sp("ceil");
  R_SIN=sp("sin");
  R_COS=sp("cos");
  R_TAN=sp("tan");
  R_ASIN=sp("asin");
  R_ACOS=sp("acos");
  R_ATAN=sp("atan");
  R_AT=sp("at");
  R_SS=sp("ss");
  R_SM=sp("sm");
  R_LSQ=sp("lsq");
  R_SINH=sp("sinh");
  R_COSH=sp("cosh");
  R_TANH=sp("tanh");
  R_ERF=sp("erf");
  R_ERFC=sp("erfc");
  R_GAMMA=sp("gamma");
  R_LGAMMA=sp("lgamma");
  R_RINT=sp("rint");
  R_TRUNC=sp("trunc");
  R_NOT=sp("not");
  R_KV=sp("kv");
  R_VK=sp("vk");
  R_VAL=sp("val");
  R_BD=sp("bd");
  R_DB=sp("db");
  R_HB=sp("hb");
  R_BH=sp("bh");
  R_ZB=sp("zb");
  R_BZ=sp("bz");
  R_MD5=sp("md5");
  R_SHA1=sp("sha1");
  R_SHA2=sp("sha2");
  R_GETENV=sp("getenv");
  R_SVD=sp("svd");
  R_LU=sp("lu");
  R_QR=sp("qr");
  R_EXIT=sp("exit");
  R_DEL=sp("del");
  R_DO=sp("do");
  R_WHILE=sp("while");
  R_IF=sp("if");
  R_IN=sp("in");
  R_DVL=sp("dvl");
  R_LIN=sp("lin");
  R_DVL=sp("dvl");
  R_DV=sp("dv");
  R_DI=sp("di");
  R_GTIME=sp("gtime");
  R_LTIME=sp("ltime");
  R_TL=sp("tl");
  R_MUL=sp("mul");
  R_INV=sp("inv");
  R_CHOOSE=sp("choose");
  R_ROUND=sp("round");
  R_SSR=sp("ssr");
  R_EP=sp("ep");
}

static i32 vname(char *s, i32 n) {
  i32 i,a=1,v=1;
  if(s[0]=='.'&&n>1) a=0;
  for(i=0;i<n;i++) {
    if(a && !isalpha(s[i])) v=0;
    else if(!isalnum(s[i]) && s[i]!='.') v=0;
    if(s[i]=='.') a=1;
    else a=0;
  }
  return v;
}

static i32 ffix(char *ds, i32 a0) {
  if(strchr(ds,'.')||strchr(ds,'e')) return 0;
  if(!strcmp(ds,"inf")) { sprintf(ds,"0i"); return 0; }
  if(!strcmp(ds,"nan")) { sprintf(ds,"0n"); return 0; }
  if(!strcmp(ds,"-inf")) { sprintf(ds,"-0i"); return 0; }
  return a0;
}

static void pc_(u8 c) {
  if((c<32||c>126)) {
    if(c==8) mprintf("\\b");
    else if(c==9) mprintf("\\t");
    else if(c==10) mprintf("\\n");
    else if(c==13) mprintf("\\r");
    else mprintf("\\%03o",c);
  }
  else {
    if(c==34) mprintf("\\\"");
    else if(c==92) mprintf("\\\\");
    else mprintf("%c",c);
  }
}
static void pc(K x, char *s, char *e) {
  mprintf("%s\"",s); pc_(ck(x)); mprintf("\"%s",e);
}
static void pca(K x, char *s, char *e) {
  char *pxc=px(x);
  if(nx==1) mprintf("%s,\"",s);
  else mprintf("%s\"",s);
  i(nx,pc_(pxc[i]))
  mprintf("\"%s",e);
}
static void pi_(i32 i) {
  if(i==INT32_MAX) mprintf("0I");
  else if(i==INT32_MIN) mprintf("0N");
  else if(i==INT32_MIN+1) mprintf("-0I");
  else mprintf("%d",i);
}
static void pi(K x, char *s, char *e) {
  mprintf("%s",s); pi_(ik(x)); mprintf("%s",e);
}
static void pia(K x, char *s, char *e) {
  i32 *pxi=px(x);
  if(!nx) mprintf("%s!0%s",s,e);
  else if(1==nx) { mprintf("%s,",s); pi_(pxi[0]); mprintf("%s",e); }
  else i(nx,pi(pxi[i],i?"":s,i<nx-1?" ":e))
}

static void pf_(double f) {
  char ds[256];
  if(isinf(f)&&f>0.0) mprintf("0i");
  else if(isnan(f)) mprintf("0n");
  else if(isinf(f)&&f<0.0) mprintf("-0i");
  else {
    if(zeroclamp && fabs(f)<1e-12) f=0.0;
    sprintf(ds,"%0.*g",precision,f==0.0?0.0:f);
    if(!strchr(ds,'.')&&!strchr(ds,'e')) snprintf(ds+strlen(ds),sizeof(ds)-strlen(ds),".0");
    mprintf("%s",ds);
  }
}
static void pf(K x, char *s, char *e) {
  mprintf("%s",s); pf_(fk(x)); mprintf("%s",e);
}
static void pfa(K x, char *s, char *e) {
  i32 a0=1;
  u64 i;
  char ds[256];
  double f,*pxf=px(x);
  if(!nx) mprintf("%s0#0.0%s",s,e);
  else if(1==nx) { mprintf("%s,",s); pf_(pxf[0]); mprintf("%s",e); }
  else {
    for(i=0;i<nx;i++) {
      f=pxf[i];
      if(isinf(f)&&f>0.0) { sprintf(ds,"0i"); a0=0; }
      else if(isnan(f)) { sprintf(ds,"0n"); a0=0; }
      else if(isinf(f)&&f<0.0) { sprintf(ds,"-0i"); a0=0; }
      else {
        if(zeroclamp && fabs(f)<1e-12) f=0.0;
        sprintf(ds,"%0.*g",precision,f==0.0?0.0:f); a0=ffix(ds,a0);
      }
      if(i==nx-1) {
        if(a0) snprintf(ds+strlen(ds),sizeof(ds)-strlen(ds),".0");
        mprintf("%s%s%s",ds,i?"":s,e);
      }
      else mprintf("%s%s ",i?"":s,ds);
    }
  }
}

static void ps_(char *s) {
  char *e;
  i32 v;
  if(!s) { mprintf("`"); return; }
  v=vname(s,strlen(s));
  e=xesc(s);
  if(!v) mprintf("`\"%s\"",e);
  else mprintf("`%s",e);
  xfree(e);
}
static void ps(K x, char *s, char *e) {
  mprintf("%s",s); ps_(sk(x)); mprintf("%s",e);
}
static void psa(K x, char *s, char *e) {
  char **pxs=px(x);
  if(!nx) mprintf("%s0#`%s",s,e);
  else if(nx==1) { mprintf("%s,",s); ps_(pxs[0]); mprintf("%s",e); }
  else { mprintf("%s",s); i(nx,ps_(pxs[i]); if(i<nx-1)mprintf(" ")) mprintf("%s",e); }
}

const char* kprint_(K x, char *s, char *e, char *s0) {
  K *px,g;
  i32 sm=32,sp=0;
  char *s2;
  typedef struct { K x; char *s,*e,*s0; } SF;

  SF *stack=xmalloc(sizeof(SF)*sm);
  if(!stack) abort();
  stack[sp++] = (SF){ x, xstrdup(s), xstrdup(e), xstrdup(s0) };

  while(sp>0) {
    SF *f=&stack[--sp];
    K x=f->x;
    char *s=f->s;
    char *e=f->e;
    char *s0=f->s0;

    if(!x) { mprintf("%s%s",s,e); if(s) xfree(s); if(e) xfree(e); if(s0) xfree(s0); continue; }

    switch(s(x)) {
    case 0xc0:;
      u32 c=ck(x)%32;
      if(c<strlen(P)) mprintf("%s%c%s",s,P[ck(x)%32],e);
      else mprintf("%s%s",s,e);
      break;
    case 0xc1: mprintf("%s%s%s",s,(char*)px(x),e); break;
    case 0xc2: mprintf("%s%s%s",s,(char*)px(x),e); break;
    case 0xc3:
      px=px(x);
      mprintf("%s%s",s,(char*)px(px[0]));
      if(6!=T(px[3])) mprintf("%s%s",px(px[3]),e);
      else mprintf("%s",e);
      break;
    case 0xc4:
      px=px(x);
      kprint_(px[0],s,"",s0);
      g=px[1];
      if(6!=T(g)) {
        K *pg=px(g);
        mprintf("[");
        i(n(g),kprint_(pg[i],"","",s0);if(i<n(g)-1) mprintf(";"))
        mprintf("]");
      }
      kprint_(px[2],"","",s0);
      mprintf(e);
      break;
    case 0xc5:
      px=px(x);
      K f=px[0]; K *pff=px(f);
      K av=px[1];
      if(n(av)) {
        mprintf("%s(",s);
        i(n(f),kprint_(pff[i],"","",""))
        mprintf(")%s%s",(char*)px(av),e);
      }
      else {
        mprintf("%s",s);
        i(n(f),kprint_(pff[i],"","",""))
        mprintf("%s",e);
      }
      break;
    case 0xc6: mprintf("%s%s%s",s,sk(x),e); break;
    case 0xc7: mprintf("%s%s%s",s,sk(x),e); break;
    case 0xc8:
      px=px(x);
      mprintf("%s%s[",s,sk(px[0]));
      s2=xmalloc(2+strlen(s0));
      sprintf(s2,"%s ",s0);
      char *e2=xmalloc(2+strlen(e));
      sprintf(e2,"]%s",e);
      if(sp==sm) { stack=xrealloc(stack,sizeof(SF)*(sm*=2)); }
      stack[sp++] = (SF){ 0, xstrdup(""), e2, xstrdup(s2) };
      if(sp==sm) { stack=xrealloc(stack,sizeof(SF)*(sm*=2)); }
      stack[sp++] = (SF){ px[1], xstrdup(""), xstrdup(""), xstrdup(s2) };
      xfree(s2);
      break;
    case 0xc9: mprintf("%s%s%s",s,sk(x),e); break;
    case 0xca: mprintf("%s%s%s",s,sk(x),e); break;
    case 0xcb: mprintf("%s%s%s",s,sk(x),e); break;
    case 0xcc: mprintf("%s%s%s",s,sk(x),e); break;
    //case 0xcd: mprintf("%s%s%s",s,sk(x),e); break;
    case 0xce: mprintf("%s%c:%s",s,ck(x),e); break;
    //case 0xcf: mprintf("%s%c:%s",s,ck(x),e); break;
    case 0xd0:
      px=px(x);
      kprint_(px[0],s,"","");
      if(s(px[1])) kprint_(px[1],"",e,"");
      else mprintf("%s%c%s","",P[ck(px[1])%32],e);
      break;
    case 0xd1: mprintf("%s%s%s",s,sk(x),e); break;
    case 0xd2: mprintf("%s%s%s",s,sk(x),e); break;
    case 0xd3: mprintf("%s%s%s",s,sk(x),e); break;
    case 0xd4:
      px=px(x);
      kprint_(px[0],s,"",s0);
      g=px[1];
      K *pg=px(g);
      mprintf("[");
      i(n(g),kprint_(pg[i],"","",s0);if(i<n(g)-1) mprintf(";"))
      mprintf("]");
      mprintf(e);
      break;
    case 0xd5:
      px=px(x);
      kprint_(px[0],s,"",s0);
      mprintf("[");
      kprint_(px[1],s,";",s0);
      kprint_(px[2],s,";",s0);
      kprint_(px[3],s,"",s0);
      mprintf("]");
      mprintf(e);
      break;
    case 0xd6:
      px=px(x);
      kprint_(px[0],s,"",s0);
      mprintf("[");
      kprint_(px[1],s,";",s0);
      kprint_(px[2],s,";",s0);
      kprint_(px[3],s,";",s0);
      kprint_(px[4],s,"",s0);
      mprintf("]");
      mprintf(e);
      break;
    case 0xd7:
      px=px(x);
      kprint_(px[0],s,"",s0);
      kprint_(px[1],"",e,s0);
      break;
    case 0x80:
      mprintf("%s.",s);
      s2=xmalloc(2+strlen(s0));
      sprintf(s2,"%s ",s0);
      px=px(x);
      K g=px[2]; px[2]=null;
      K y=k(1,0,k_(b(48)&x)); /* transpose */
      if(E(y)) { if(y<EMAX) y=kerror(E[y]); kprint_(y,"kprint: ",e,s2); }
      else kprint_(y,"",e,s2);
      px[2]=g;
      xfree(s2);
      _k(y);
      break;
    case 0x82: mprintf("%s::%s",s,e); break;
    case 0x84:;
      i32 ee=0;
      if(strcmp(sk(x),"abort")) {
        i(EMAX,if(!strcmp(sk(x),E[i])) { ee=1; break; })
        if(ee) fprintf(stderr,"%s%s error%s",s,sk(x),e);
        else fprintf(stderr,"%s%s%s",s,sk(x),e);
      }
      break;
    case 0x85: mprintf("%s%s%s",s,(char*)px(x),e); break;
    case 0:
      switch(tx) {
      case  1: pi(x,s,e); break;
      case  2: pf(x,s,e); break;
      case  3: pc(x,s,e); break;
      case  4: ps(x,s,e); break;
      case  6: case 10: mprintf("%s%s",s,e); break;
      case -1: pia(x,s,e); break;
      case -2: pfa(x,s,e); break;
      case -3: pca(x,s,e); break;
      case -4: psa(x,s,e); break;
      case  0:
        if(nx==1) {
          px=px(x);
          char *s2=xmalloc(2+strlen(s0));
          sprintf(s2,"%s ",s0);
          mprintf("%s,",s);
          if(sp==sm) { stack=xrealloc(stack,sizeof(SF)*(sm*=2)); }
          stack[sp++] = (SF){ px[0], xstrdup(""), xstrdup(e), s2 };
        }
        else {
          i32 a=1;
          px=px(x);
          char t;
          u64 ncv=0;
          mprintf("%s(",s);
          char *s2=xmalloc(2+strlen(s0));
          sprintf(s2,"%s ",s0);
          i(nx,t=T(px[i]);if(px[i]&&!s(px[i])&&t<=0&&t!=-3&&n(px[i])){a=0;break;}) /* (1;2.0;"asdf") */
          i(nx,t=T(px[i]);if(t==-3)++ncv); if(nx==ncv) a=0; /* ("asdf";"asdf") */
          i(nx,if(px[i]&&0x80==s(px[i])&&n(((K*)px(px[i]))[0])){a=0;break;}) /* .,(`a;1) */

          char *e2=xmalloc(2+strlen(e));
          sprintf(e2,")%s",e);
          if(sp==sm) { stack=xrealloc(stack,sizeof(SF)*(sm*=2)); }
          stack[sp++] = (SF){ 0, xstrdup(""), e2, xstrdup(s2) };

          if(a) {
            for(i64 i=nx-1;i>=0;--i) {
              if(sp==sm) { stack=xrealloc(stack,sizeof(SF)*(sm*=2)); }
              stack[sp++] = (SF){ px[i], xstrdup(""), xstrdup((u64)i==nx-1?"":";"), xstrdup(s2) };
            }
          }
          else {
            for(i64 i=nx-1;i>=0;--i) {
              if(sp==sm) { stack=xrealloc(stack,sizeof(SF)*(sm*=2)); }
              stack[sp++] = (SF){ px[i], i?xstrdup(s2):xstrdup(""), xstrdup((u64)i==nx-1?"":"\n"), xstrdup(s2) };
            }
          }
          xfree(s2);
        }
        break;
      } break;
    }
    if(s) xfree(s);
    if(e) xfree(e);
    if(s0) xfree(s0);
  }
  xfree(stack);
  return mbuffer();
}

void kprint(K x, char *s, char *e, char *s0) {
  mreset();
  const char *t=kprint_(x,s,e,s0);
  fputs(t,stdout);
  _k(x);
}

i32 kreserved(char *p) {
  if(p==R_NUL) return 1;
  if(p==R_DRAW) return 1;
  if(p==R_DOT) return 1;
  if(p==R_VS) return 1;
  if(p==R_SV) return 1;
  if(p==R_ATAN2) return 1;
  if(p==R_DIV) return 1;
  if(p==R_AND) return 1;
  if(p==R_OR) return 1;
  if(p==R_SHIFT) return 1;
  if(p==R_ROT) return 1;
  if(p==R_XOR) return 1;
  if(p==R_EUCLID) return 1;
  if(p==R_ENCRYPT) return 1;
  if(p==R_DECRYPT) return 1;
  if(p==R_SETENV) return 1;
  if(p==R_RENAME) return 1;
  if(p==R_SQR) return 1;
  if(p==R_ABS) return 1;
  if(p==R_SLEEP) return 1;
  if(p==R_IC) return 1;
  if(p==R_CI) return 1;
  if(p==R_DJ) return 1;
  if(p==R_JD) return 1;
  if(p==R_LT) return 1;
  if(p==R_LOG) return 1;
  if(p==R_EXP) return 1;
  if(p==R_SQRT) return 1;
  if(p==R_FLOOR) return 1;
  if(p==R_CEIL) return 1;
  if(p==R_SIN) return 1;
  if(p==R_COS) return 1;
  if(p==R_TAN) return 1;
  if(p==R_ASIN) return 1;
  if(p==R_ACOS) return 1;
  if(p==R_ATAN) return 1;
  if(p==R_AT) return 1;
  if(p==R_SS) return 1;
  if(p==R_SM) return 1;
  if(p==R_LSQ) return 1;
  if(p==R_SINH) return 1;
  if(p==R_COSH) return 1;
  if(p==R_TANH) return 1;
  if(p==R_ERF) return 1;
  if(p==R_ERFC) return 1;
  if(p==R_GAMMA) return 1;
  if(p==R_LGAMMA) return 1;
  if(p==R_RINT) return 1;
  if(p==R_TRUNC) return 1;
  if(p==R_NOT) return 1;
  if(p==R_KV) return 1;
  if(p==R_VK) return 1;
  if(p==R_VAL) return 1;
  if(p==R_BD) return 1;
  if(p==R_DB) return 1;
  if(p==R_HB) return 1;
  if(p==R_BH) return 1;
  if(p==R_ZB) return 1;
  if(p==R_BZ) return 1;
  if(p==R_MD5) return 1;
  if(p==R_SHA1) return 1;
  if(p==R_SHA2) return 1;
  if(p==R_GETENV) return 1;
  if(p==R_SVD) return 1;
  if(p==R_LU) return 1;
  if(p==R_QR) return 1;
  if(p==R_EXIT) return 1;
  if(p==R_DEL) return 1;
  if(p==R_DO) return 1;
  if(p==R_WHILE) return 1;
  if(p==R_IF) return 1;
  if(p==R_IN) return 1;
  if(p==R_DVL) return 1;
  if(p==R_LIN) return 1;
  if(p==R_DVL) return 1;
  if(p==R_DV) return 1;
  if(p==R_DI) return 1;
  if(p==R_GTIME) return 1;
  if(p==R_LTIME) return 1;
  if(p==R_TL) return 1;
  if(p==R_MUL) return 1;
  if(p==R_INV) return 1;
  if(p==R_CHOOSE) return 1;
  if(p==R_ROUND) return 1;
  if(p==R_SSR) return 1;
  if(p==R_EP) return 1;
  return 0;
}

K val(K x) {
  K r,g,*px;
  i32 n;
  switch(s(x)) {
  case 0xc0: r=t(1,2); break;
  case 0xc1: r=t(1,2); break;
  case 0xc2: r=t(1,2); break;
  case 0xc3: px=px(b(48)&x); n=ik(px[4]); r=t(1,n); break;
  case 0xc4:
    px=px(b(48)&x);
    n=val(px[0]);
    g=px[1];
    if(6!=T(g)) { K *pg=px(g); i(n(g),if(pg[i]!=inull)--n) }
    r=t(1,n);
    break;
  case 0xc5: r=t(1,2); break;
  case 0xc6: r=t(1,1); break;
  case 0xc7: r=t(1,2); break;
  case 0xc8: r=t(1,1); break;
  case 0xc9: r=t(1,1); break;
  case 0xca: r=t(1,2); break;
  case 0xcb: r=t(1,3); break;
  case 0xcc: r=t(1,2); break;
  case 0xcd: r=t(1,2); break;
  case 0xce: r=t(1,1); break;
  case 0xcf: r=t(1,2); break;
  case 0xd0: r=t(1,1); break;
  case 0xd4: r=t(1,1); break;
  case 0xd5: px=px(x); n=0; i(nx,if(px[i]==inull)++n) r=t(1,n); break;
  case 0xd6: px=px(x); n=0; i(nx,if(px[i]==inull)++n) r=t(1,n); break;
  case 0xd7: r=t(1,1); break;
  case 0x82: r=t(1,2); break;
  default: r=KERR_TYPE;
  }
  return r;
}

static K kamendi3v(K d, K i, K f);
static K kamendi3d(K d, K i, K f) {
  K r,e,*pdu,v,*pv,p=0,t;
  char ti,**pis,*s;
  typedef struct { K i; u64 j; } sf;
  i32 sm=32,sp=0;
  sf *stack=0;
  static int dd=0;
  if(++dd>maxr) { e=KERR_STACK; --dd; goto cleanup; }

  ti=T(i);

  if(s(i)) { e=KERR_TYPE; goto cleanup; }
  if(ti!=4 && ti!=-4 && ti!=6 && ti!=0) { e=KERR_TYPE; goto cleanup; }
  if(d==ktree && i==null) { e=KERR_RESERVED; goto cleanup; }
  if(d==ktree && ti==4 && strlen(sk(i))==1 && strcmp(sk(i),"k")) { e=KERR_RESERVED; goto cleanup; }

  pdu=px(d);
  switch(ti) {
  case  4:
    if(0x80!=s(d)) { e=KERR_TYPE; goto cleanup; }
    s=sk(i);
    if(!strlen(s)) { e=KERR_DOMAIN; goto cleanup; }
    t=dget(d,s);if(!t)t=null;
    r=fe(k_(f),0,t,0);EC(r);
    e=dset(d,s,r); _k(r); if(E(e)) goto cleanup;
    break;
  case -4:
    if(0x80!=s(d)) { e=KERR_TYPE; goto cleanup; }
    pis=px(i);
    i(n(i),if(!strlen(pis[i])) { e=KERR_DOMAIN; goto cleanup; })
    for(u64 j=0;j<n(i);++j) {
      t=dget(d,pis[j]); if(!t)t=null;
      r=fe(k_(f),0,t,0);EC(r);
      e=dset(d,pis[j],r); _k(r); if(E(e)) goto cleanup;
    }
    break;
  case  6:
    if(0x80==s(d)) {
      v=pdu[1]; pv=px(v);
      i(n(v),r=fe(k_(f),0,k_(pv[i]),0);EC(r);_k(pv[i]);pv[i]=r)
    }
    else if(s(d)) { e=KERR_TYPE; goto cleanup; }
    else if(T(d)<=0 &&n(d)) d=kamendi3v(d,k_(i),k_(f));
    else if(T(d)>0) { e=KERR_TYPE; goto cleanup; }
    break;
  case 0: {
    stack=xmalloc(sizeof(sf)*sm);
    stack[sp++]=(sf){i,0};
    while(sp) {
      sf *pf=&stack[sp-1];
      K i_=pf->i;
      K *piu=px(i_);
      while(pf->j<n(i_)) {
        K k=piu[pf->j++];
        if(T(k)==0) {
          if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
          stack[sp++]=(sf){k,0};
          goto continue_outer;
        }
        else if(T(k)==4) {
          t=dget(d,sk(k)); if(!t) t=null;
          r=fe(k_(f),0,t,0); EC(r);
          e=dset(d,sk(k),r); _k(r); if(E(e)) goto cleanup;
        }
        else if(T(k)==-4) {
          char **ks=px(k);
          for(u64  m=0;m<n(k);++m) {
            if(!strlen(ks[m])) { e=KERR_DOMAIN; goto cleanup; }
            t=dget(d,ks[m]); if(!t) t=null;
            r=fe(k_(f),0,t,0); EC(r);
            e=dset(d,ks[m],r); _k(r); if(E(e)) goto cleanup;
          }
        }
        else if(k==null) {
          d=kamendi3d(d,k,k_(f));
          if(E(d)) { e=d; goto cleanup; }
        }
        else { e=KERR_TYPE; goto cleanup; }
      }
      --sp;
    continue_outer:;
    }
    xfree(stack);
    break;
  }
  default: _k(d); d=KERR_TYPE;
  }
  _k(i); _k(f);
  --dd;
  return knorm(d);
cleanup:
  if(stack) xfree(stack);
  if(p) _k(p);
  _k(d); _k(i); _k(f);
  return e;
}

static K kamendi3v(K d, K i, K f) {
  i32 *pii=0;
  K r,e,t,*pdu=0;
  char td,ti;
  typedef struct { K i; u64 j; } sf;
  i32 sm=32,sp=0;
  sf *stack=0;
  static int dd=0;
  if(++dd>maxr) { e=KERR_STACK; --dd; goto cleanup; }

  td=T(d);
  ti=T(i);
  if(s(i)) { e=KERR_TYPE; goto cleanup; }

  switch(ti) {
  case  1: if(ik(i)<0||(K)ik(i)>=n(d)) { e=KERR_INDEX; goto cleanup; } break;
  case -1: pii=px(i); i(n(i),if(pii[i]<0||(K)pii[i]>=n(d)) { e=KERR_INDEX; goto cleanup; }) break;
  case  6: if(!n(d)) { _k(i); _k(f); --dd; return d; }
  }

  if(td) { t=kmix(d); EC(t); _k(d); d=t; }
  pdu=px(d);
  switch(ti) {
  case  1: r=fe(k_(f),0,k_(pdu[ik(i)]),0); EC(r); _k(pdu[ik(i)]); pdu[ik(i)]=r; break;
  case -1: i(n(i),r=fe(k_(f),0,k_(pdu[pii[i]]),0); EC(r); _k(pdu[pii[i]]); pdu[pii[i]]=r) break;
  case  6: i(n(d),r=fe(k_(f),0,k_(pdu[i]),0); EC(r); _k(pdu[i]); pdu[i]=r) break;
  case  0: {
    stack=xmalloc(sizeof(sf)*sm);
    stack[sp++]=(sf){i,0};
    while(sp) {
      sf *pf=&stack[sp-1];
      K i_=pf->i;
      K *piu=px(i_);
      while(pf->j<n(i_)) {
        K i2=piu[pf->j++];
        if(s(i2)) { e=KERR_TYPE; goto cleanup; }
        if(T(i2)==0) {
          if(sp==sm) stack=xrealloc(stack,sizeof(sf)*(sm*=2));
          stack[sp++]=(sf){i2,0};
          goto continue_outer;
        }
        else if(T(i2)==1) {
          i32 k=ik(i2);
          if((K)k>=n(d)) { e=KERR_INDEX; goto cleanup; }
          r=fe(k_(f),0,k_(pdu[k]),0); EC(r);
          _k(pdu[k]); pdu[k]=r;
        }
        else if(T(i2)==-1) {
          i32 *pxi=px(i2);
          for(u64 j=0;j<n(i2);++j) {
            i32 k=pxi[j];
            if((K)k>=n(d)) { e=KERR_INDEX; goto cleanup; }
            r=fe(k_(f),0,k_(pdu[k]),0); EC(r);
            _k(pdu[k]); pdu[k]=r;
          }
        }
        else if(T(i2)==6) {
          for(u64 j=0;j<n(d);++j) {
            r=fe(k_(f),0,k_(pdu[j]),0); EC(r);
            _k(pdu[j]); pdu[j]=r;
          }
        }
        else { e=KERR_TYPE; goto cleanup; }
      }
      --sp;
  continue_outer:;
    }
    xfree(stack);
    break;
  }
  default: _k(d); d=KERR_TYPE;
  }
  _k(i); _k(f);
  --dd;
  return knorm(d);
cleanup:
  if(stack) xfree(stack);
  _k(d); _k(i); _k(f);
  return e;
}

K kamendi3(K d, K i, K f) {
  K r,e,sym=0,t,*prk;

  if(d==inull||i==inull||f==inull) {
    r=tn(0,4); prk=px(r);
    K g=t(1,st(0xc0,45)); // @
    prk[0]=g;
    prk[1]=kcp(d); if(E(prk[1])) { e=prk[1]; _k(r); goto cleanup; }
    prk[2]=kcp(i); if(E(prk[2])) { e=prk[2]; _k(r); goto cleanup; }
    prk[3]=kcp(f); if(E(prk[3])) { e=prk[3]; _k(r); goto cleanup; }
    _k(d); _k(i); _k(f);
    return st(0xd5,r);
  }

  if(!d) { e=KERR_TYPE; goto cleanup; }
  if(!ISF(f)) { e=KERR_TYPE; goto cleanup; }
  if(4==T(d)&&!s(d)) {
    K d2=scope_get(gs,d); sym=d; d=d2;
    //if(!d) d=null;
    //EC(d);
    if(E(d)) d=null;
  }
  if((T(d)<=0||T(d)==2) && 1<((ko*)(b(48)&d))->r) { K d2=kcp(d); EC(d2); _k(d); d=d2; }

  if((s(d) && s(d)!=0x80 && !ISF(d))) { e=KERR_TYPE; goto cleanup; }

  if(T(d)==6) {
    if(T(i)==6) { _k(f); return d; }
    if(T(i)<=0 && n(i)==0) { _k(i); _k(f); return d; }
    _k(i); _k(f); return KERR_INDEX;
  }

  if(ISF(d) && 0xc0==s(f) && 32==ck(f)) { /* error trap */
    int e0=EFLAG; EFLAG=0;
    t=k(13,d,i); /* d@i */
    EFLAG=e0;
    r=tn(0,2); prk=px(r);
    if(t<EMAX) { prk[0]=t(1,1); prk[1]=tnv(3,strlen(E[t]),xmemdup(E[t],1+strlen(E[t]))); }
    else if(E(t)) { prk[0]=t(1,1); prk[1]=tnv(3,strlen(sk(t)),xmemdup(sk(t),1+strlen(sk(t)))); }
    else { prk[0]=t(1,0); prk[1]=t; }
    r=knorm(r);
  }
  else if(ISF(d)) { e=KERR_RANK; goto cleanup; }
  else if(0x80==s(d)) r=kamendi3d(d,i,f);
  else if(T(d)<=0) r=kamendi3v(d,i,f);
  else { e=KERR_RANK; goto cleanup; }

  if(E(r)) return r;
  else if(sym) { scope_set(gs,sym,r); _k(r); return sym; }
  else return r;
cleanup:
  _k(d); _k(i); _k(f);
  return e;
}

static K kamendi4d(K d, K i, K f, K y) {
  i32 *pyi,b=0;
  K r,e,*pdu,*pyu,v,*pv,p=0,*pp,t;
  double *pyf;
  char ti,ty,Ty,*pyc,**pys,**pis,*s;
  typedef struct { K i,y; u64 j; } sf;
  i32 sm=32,sp=0;
  sf *stack=0;
  static int dd=0;

  if(++dd>maxr) { e=KERR_STACK; --dd; goto cleanup; }

  ti=T(i);
  ty=T(y);
  Ty=ty; if(s(y)) { if(!VST(y)) { e=KERR_TYPE; goto cleanup; } Ty=15; }
  if(s(i)) { e=KERR_TYPE; goto cleanup; }
  if(ti!=4 && ti!=-4 && ti!=6 && ti!=0) { e=KERR_TYPE; goto cleanup; }
  if(d==ktree && i==null) { e=KERR_RESERVED; goto cleanup; }
  if(d==ktree && ti==4 && strlen(sk(i))==1 && strcmp(sk(i),"k")) { e=KERR_RESERVED; goto cleanup; }
  if(f&&ck(f)!=':') b=1;

  pdu=px(d);
  switch(ti) {
  case  4:
    s=sk(i);
    if(!strlen(s)) { e=KERR_DOMAIN; goto cleanup; }
    if(b) { t=dget(d,s);if(!t)t=null; r=fe(k_(f),t,k_(y),0);EC(r); }
    else { r=kcp(y); if(E(r)) { e=r; goto cleanup; } }
    e=dset(d,s,r); _k(r); if(E(e)) goto cleanup;
    break;
  case -4:
    pis=px(i);
    i(n(i),if(!strlen(pis[i])) { e=KERR_DOMAIN; goto cleanup; })
    switch(Ty) {
    case  1: case 2: case 3: case 4: case 6:
      for(u64 j=0;j<n(i);++j) {
        if(b) { t=dget(d,pis[j]);if(!t)t=null;r=fe(k_(f),t,k_(y),0);EC(r); }
        else r=k_(y);
        e=dset(d,pis[j],r); _k(r); if(E(e)) goto cleanup;
      }
      break;
    case 15:
      for(u64 j=0;j<n(i);++j) {
        if(b) { t=dget(d,pis[j]);if(!t)t=null;r=fe(k_(f),t,k_(y),0);EC(r); }
        else { r=kcp(y); EC(r); }
        e=dset(d,pis[j],r); _k(r); if(E(e)) goto cleanup;
      }
      break;
    case -1: pyi=px(y);
      if(n(i)!=n(y)) { e=KERR_LENGTH; goto cleanup; }
      for(u64 j=0;j<n(i);++j) {
        if(b) { t=dget(d,pis[j]);if(!t)t=null;r=fe(k_(f),t,t(1,(u32)pyi[j]),0);EC(r); }
        else r=t(1,(u32)pyi[j]);
        e=dset(d,pis[j],r); _k(r); if(E(e)) goto cleanup;
      }
      break;
    case -2: pyf=px(y);
      if(n(i)!=n(y)) { e=KERR_LENGTH; goto cleanup; }
      for(u64 j=0;j<n(i);++j) {
        if(b) { t=dget(d,pis[j]);if(!t)t=null;r=fe(k_(f),t,t2(pyf[j]),0);EC(r); }
        else r=t2(pyf[j]);
        e=dset(d,pis[j],r); _k(r); if(E(e)) goto cleanup;
      }
      break;
    case -3: pyc=px(y);
      if(n(i)!=n(y)) { e=KERR_LENGTH; goto cleanup; }
      for(u64 j=0;j<n(i);++j) {
        if(b) { t=dget(d,pis[j]);if(!t)t=null;r=fe(k_(f),t,t(3,(u8)pyc[j]),0);EC(r); }
        else r=t(3,(u8)pyc[j]);
        e=dset(d,pis[j],r); _k(r); if(E(e)) goto cleanup;
      }
      break;
    case -4: pys=px(y);
      if(n(i)!=n(y)) { e=KERR_LENGTH; goto cleanup; }
      for(u64 j=0;j<n(i);++j) {
        if(b) { t=dget(d,pis[j]);if(!t)t=null;r=fe(k_(f),t,t(4,pys[j]),0);EC(r); }
        else r=t(4,pys[j]);
        e=dset(d,pis[j],r); _k(r); if(E(e)) goto cleanup;
      }
      break;
    case  0: pyu=px(y);
      if(n(i)!=n(y)) { e=KERR_LENGTH; goto cleanup; }
      for(u64 j=0;j<n(i);++j) {
        if(b) { t=dget(d,pis[j]);if(!t)t=null;r=fe(k_(f),t,k_(pyu[j]),0);EC(r); }
        else r=k_(pyu[j]);
        e=dset(d,pis[j],r); _k(r); if(E(e)) goto cleanup;
      }
      break;
    } break;
  case  6:
    v=pdu[1]; pv=px(v);
    switch(Ty) {
    case -1: case -2: case -3: case -4: case 0:
      if(n(v)!=n(y)) { e=KERR_LENGTH; goto cleanup; }
      p=kmix(y); EC(p);
      pp=px(p);
      if(b) i(n(p),r=fe(k_(f),k_(pv[i]),k_(pp[i]),0);EC(r);_k(pv[i]);pv[i]=r)
      else i(n(p),_k(pv[i]);pv[i]=pp[i];pp[i]=0)
       _k(p);
      break;
    case 1: case 2: case 3: case 4: case 6:
      if(b) i(n(v),r=fe(k_(f),k_(pv[i]),k_(y),0);EC(r);_k(pv[i]);pv[i]=r)
      else i(n(v),_k(pv[i]);pv[i]=k_(y))
      break;
    case 15:
      if(b) i(n(v),r=fe(k_(f),k_(pv[i]),k_(y),0);EC(r);_k(pv[i]);pv[i]=r)
      else i(n(v),_k(pv[i]);pv[i]=kcp(y);EC(pv[i]))
      break;
    } break;
  case  0: {
    stack=xmalloc(sizeof(sf)*sm);
    stack[sp++]=(sf){i,y,0};
    while(sp) {
      sf *pf=&stack[sp-1];
      K i_=pf->i;
      K y_=pf->y;
      K *piu=px(i_);
      char Ty_=T(y_); if(s(y_)) { if(!VST(y_)) { e=KERR_TYPE; goto cleanup; } Ty_=10; }
      switch(Ty_) {
      case -1: pyi=px(y_);
        if(n(i_)!=n(y_)) { e=KERR_LENGTH; goto cleanup; }
        i(n(i_),d=kamendi4d(d,k_(piu[i]),k_(f),t(1,(u32)pyi[i]));EC(d))
        break;
      case -2: pyf=px(y_);
        if(n(i_)!=n(y_)) { e=KERR_LENGTH; goto cleanup; }
        i(n(i_),d=kamendi4d(d,k_(piu[i]),k_(f),t2(pyf[i]));EC(d))
        break;
      case -3: pyc=px(y_);
        if(n(i_)!=n(y_)) { e=KERR_LENGTH; goto cleanup; }
        i(n(i_),d=kamendi4d(d,k_(piu[i]),k_(f),t(3,(u8)pyc[i]));EC(d))
        break;
      case -4: pys=px(y_);
        if(n(i_)!=n(y_)) { e=KERR_LENGTH; goto cleanup; }
        i(n(i_),d=kamendi4d(d,k_(piu[i]),k_(f),t(4,pys[i]));EC(d))
        break;
      case 1: case 2: case 3: case 4: case 6: case 15:
        while(pf->j<n(i_)) {
          K i2=piu[pf->j++];
          if(s(i2)) { e=KERR_TYPE; goto cleanup; }
          if(T(i2)==0) {
            if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
            stack[sp++]=(sf){i2,y_,0};
            goto continue_outer;
          }
          else {
            d=kamendi4d(d,k_(i2),k_(f),k_(y));
            if(E(d)) { e=d; goto cleanup; }
          }
        }
        break;
      case 0:
        if(n(i_)!=n(y_)) { e=KERR_LENGTH; goto cleanup; }
        K *pyu=px(y_);
        while(pf->j<n(i_)) {
          K i2=piu[pf->j];
          K y2=pyu[pf->j++];
          if(s(i2)) { e=KERR_TYPE; goto cleanup; }
          if(T(i2)==0 && T(y2)==0) {
            if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
            stack[sp++]=(sf){i2,y2,0};
            goto continue_outer;
          }
          else {
            d=kamendi4d(d,k_(i2),k_(f),k_(y2));
            if(E(d)) { e=d; goto cleanup; }
          }
        }
        break;
      default: { e=KERR_TYPE; goto cleanup; }
      }
      --sp;
continue_outer:;
    }
    xfree(stack);
    }
    break;
  default: _k(d); d=KERR_TYPE;
  }
  _k(i); _k(f); _k(y);
  --dd;
  return d;
cleanup:
  if(stack) xfree(stack);
  if(p) _k(p);
  _k(d); _k(i); _k(f); _k(y);
  return e;
}

#define ASSIGN_LOOP(N, IDXEXPR, LHS, RHS, RAW, EXPECTED, KNPACK) \
  for(u64 j=0;j<(N);++j) { \
    i32 idx=(IDXEXPR); \
    if(pdu) { \
      r=fe(k_(f),k_(pdu[idx]),RHS,0); EC(r); \
      _k(pdu[idx]); pdu[idx]=r; \
    } \
    else { \
      r=fe(k_(f),LHS,RHS,0); EC(r); \
      if(T(r)==EXPECTED) { RAW[idx]=KNPACK; _k(r); } \
      else { t=kmix(d); EC(t); _k(d); d=t; pdu=px(d); _k(pdu[idx]); pdu[idx]=r; \
      } \
    } \
  }
#define AMEND_CASE(PRE, N, IDXEXPR, LHS, RHS, RAW, EXPECTED, KNPACK, DIRECT) \
  PRE; \
  if(b) { ASSIGN_LOOP(N, IDXEXPR, LHS, RHS, RAW, EXPECTED, KNPACK); } \
  else { DIRECT; }

static K kamendi4v(K d, K i, K f, K y) {
  i32 *pii=0,*pdi,*pyi,b=0;
  K r,e,t,*pdu=0,*pyu;
  double *pdf,*pyf;
  char td,ti,ty,Ty,*pdc,**pds,*pyc,**pys;
  typedef struct { K i,y; u64 j; } sf;
  i32 sm=32,sp=0;
  sf *stack=0;
  static int dd=0;
  if(++dd>maxr) { e=KERR_STACK; --dd; goto cleanup; }

  td=T(d);
  ti=T(i);
  ty=T(y);
  Ty=ty; if(s(y)) { if(!VST(y)) { e=KERR_TYPE; goto cleanup; } Ty=15; }
  if(s(i)) { e=KERR_TYPE; goto cleanup; }

  switch(ti) {
  case  1: if(ik(i)<0||(K)ik(i)>=n(d)) { e=KERR_INDEX; goto cleanup; } break;
  case -1: pii=px(i); i(n(i),if(pii[i]<0||(K)pii[i]>=n(d)) { e=KERR_INDEX; goto cleanup; }) break;
  }

  if(ti<=0 && Ty<=0 && n(i)!=n(y)) { e=KERR_LENGTH; goto cleanup; }

  if(f&&ck(f)!=':') b=1;

  if(td&&ti&&abs(td)==abs(Ty)) {
    switch(td) {
    case -1: pdi=px(d);
      switch(ti) {
      case  1:
        if(b) { r=fe(k_(f),t(1,(u32)pdi[ik(i)]),k_(y),0); EC(r); }
        else r=k_(y);
        if(T(r)==1) pdi[ik(i)]=ik(r);
        else {
          t=kmix(d); EC(t);
          _k(d); d=t; pdu=px(d); pdu[ik(i)]=r;
        }
        break;
      case -1:
        switch(Ty) {
        case  1: AMEND_CASE(/* */, n(i), pii[j], t(1,(u32)pdi[idx]), y, pdi, 1, ik(r), i(n(i), pdi[pii[i]]=y)); break;
        case -1: AMEND_CASE(pyi=px(y), n(i), pii[j], t(1,(u32)pdi[idx]), t(1,(u32)pyi[j]), pdi, 1, ik(r), i(n(i), pdi[pii[i]]=pyi[i])); break;
        default: _k(d); d=KERR_TYPE;
        } break;
      case  6:
        switch(Ty) {
        case  1: AMEND_CASE(/* */, n(d), j, t(1,(u32)pdi[idx]), y, pdi, 1, ik(r), i(n(d), pdi[i]=y)); break;
        case -1:
          if(n(d)!=n(y)) { e=KERR_LENGTH; goto cleanup; }
          AMEND_CASE(pyi=px(y), n(d), j, t(1,(u32)pdi[idx]), t(1,(u32)pyi[j]), pdi, 1, ik(r), i(n(d), pdi[i]=pyi[i])); break;
        default: _k(d); d=KERR_TYPE;
        } break;
      } break;
    case -2: pdf=px(d);
      switch(ti) {
      case  1:
        if(b) { r=fe(k_(f),t2(pdf[ik(i)]),k_(y),0); EC(r); }
        else r=k_(y);
        if(T(r)==2) { pdf[ik(i)]=fk(r); _k(r); }
        else {
          t=kmix(d); EC(t);
          _k(d); d=t; pdu=px(d); _k(pdu[ik(i)]); pdu[ik(i)]=r;
        }
        break;
      case -1:
        switch(Ty) {
        case 2: AMEND_CASE(/* */, n(i), pii[j], t2(pdf[idx]), k_(y), pdf, 2, fk(r), i(n(i), pdf[pii[i]]=fk(y))); break;
        case -2: AMEND_CASE(pyf=px(y), n(i), pii[j], t2(pdf[idx]), t2(pyf[j]), pdf, 2, fk(r), i(n(i), pdf[pii[i]]=pyf[i])); break;
        default: _k(d); d=KERR_TYPE;
        } break;
      case  6:
        switch(Ty) {
        case 2: AMEND_CASE(/* */, n(d), j, t2(pdf[idx]), k_(y), pdf, 2, fk(r), i(n(d), pdf[i]=fk(y))); break;
        case -2:
          if(n(d)!=n(y)) { e=KERR_LENGTH; goto cleanup; }
          AMEND_CASE(pyf=px(y), n(d), j, t2(pdf[idx]), t2(pyf[j]), pdf, 2, fk(r), i(n(d), pdf[i]=pyf[i])); break;
        default: _k(d); d=KERR_TYPE;
        } break;
      } break;
    case -3: pdc=px(d);
      switch(ti) {
      case  1:
        if(b) { r=fe(k_(f),t(3,(u8)pdc[ik(i)]),k_(y),0); EC(r); }
        else r=k_(y);
        if(T(r)==3) pdc[ik(i)]=ck(r);
        else {
          t=kmix(d); EC(t);
          _k(d); d=t; pdu=px(d); pdu[ik(i)]=r;
        }
        break;
      case -1:
        switch(Ty) {
        case 3: AMEND_CASE(/* */, n(i), pii[j], t(3,(u8)pdc[idx]), y, pdc, 3, ck(r), i(n(i), pdc[pii[i]]=y)); break;
        case -3: AMEND_CASE(pyc=px(y), n(i), pii[j], t(3,(u8)pdc[idx]), t(3,(u8)pyc[j]), pdc, 3, ck(r), i(n(i), pdc[pii[i]]=pyc[i])); break;
        default: _k(d); d=KERR_TYPE;
        } break;
      case  6:
        switch(Ty) {
        case 3: AMEND_CASE(/* */, n(d), j, t(3,(u8)pdc[idx]), y, pdc, 3, ck(r), i(n(d), pdc[i]=y)); break;
        case -3:
          if(n(d)!=n(y)) { e=KERR_LENGTH; goto cleanup; }
          AMEND_CASE(pyc=px(y), n(d), j, t(3,(u8)pdc[idx]), t(3,(u8)pyc[j]), pdc, 3, ck(r), i(n(d), pdc[i]=pyc[i])); break;
        default: _k(d); d=KERR_TYPE;
        } break;
      } break;
    case -4: pds=px(d);
      switch(ti) {
      case  1:
        if(b) { r=fe(k_(f),t(4,pds[ik(i)]),k_(y),0); EC(r); }
        else r=k_(y);
        if(T(r)==4) pds[ik(i)]=sk(r);
        else {
          t=kmix(d); EC(t);
          _k(d); d=t; pdu=px(d); pdu[ik(i)]=r;
        }
        break;
      case -1:
        switch(Ty) {
        case 4: AMEND_CASE(/* */, n(i), pii[j], t(4,pds[idx]), y, pds, 4, sk(r), i(n(i), pds[pii[i]]=sk(y))); break;
        case -4: AMEND_CASE(pys=px(y), n(i), pii[j], t(4,pds[idx]), t(4,pys[j]), pds, 4, sk(r), i(n(i), pds[pii[i]]=pys[i])); break;
        default: _k(d); d=KERR_TYPE;
        } break;
      case  6:
        switch(Ty) {
        case 4: AMEND_CASE(/* */, n(d), j, t(4,pds[idx]), y, pds, 4, sk(r), i(n(d), pds[i]=sk(y))); break;
        case -4:
          if(n(d)!=n(y)) { e=KERR_LENGTH; goto cleanup; }
          AMEND_CASE(pys=px(y), n(d), j, t(4,pds[idx]), t(4,pys[j]), pds, 4, sk(r), i(n(d), pds[i]=pys[i])); break;
        default: _k(d); d=KERR_TYPE;
        } break;
      } break;
    }
  }
  else {
    if(td) { t=kmix(d); EC(t); _k(d); d=t; }
    pdu=px(d);
    switch(ti) {
    case  1:
      if(b) { r=fe(k_(f),k_(pdu[ik(i)]),k_(y),0); EC(r); _k(pdu[ik(i)]); pdu[ik(i)]=r; }
      else { _k(pdu[ik(i)]); pdu[ik(i)]=kcp2(y); EC(pdu[ik(i)]); }
      break;
    case -1:
      switch(Ty) {
      case  1: case 2: case 3: case 4: case 6:
        if(b) { i(n(i),r=fe(k_(f),k_(pdu[pii[i]]),k_(y),0); EC(r); _k(pdu[pii[i]]); pdu[pii[i]]=r) }
        else { i(n(i),_k(pdu[pii[i]]); pdu[pii[i]]=k_(y)) }
        break;
      case 15:
        if(b) i(n(i),r=fe(k_(f),k_(pdu[pii[i]]),k_(y),0); EC(r); _k(pdu[pii[i]]); pdu[pii[i]]=r)
        else i(n(i),_k(pdu[pii[i]]);pdu[pii[i]]=kcp(y);EC(pdu[pii[i]]))
        break;
      case -1: pyi=px(y);
        if(b) i(n(i),r=fe(k_(f),k_(pdu[pii[i]]),t(1,(u32)pyi[i]),0); EC(r); _k(pdu[pii[i]]); pdu[pii[i]]=r)
        else i(n(i),_k(pdu[pii[i]]);pdu[pii[i]]=t(1,(u32)pyi[i]))
        break;
      case -2: pyf=px(y);
        if(b) i(n(i),r=fe(k_(f),k_(pdu[pii[i]]),t2(pyf[i]),0); EC(r); _k(pdu[pii[i]]); pdu[pii[i]]=r)
        else i(n(i),_k(pdu[pii[i]]);pdu[pii[i]]=t2(pyf[i]))
        break;
      case -3: pyc=px(y);
        if(b) i(n(i),r=fe(k_(f),k_(pdu[pii[i]]),t(3,(u8)pyc[i]),0); EC(r); _k(pdu[pii[i]]); pdu[pii[i]]=r)
        else i(n(i),_k(pdu[pii[i]]);pdu[pii[i]]=t(3,(u8)pyc[i]))
        break;
      case -4: pys=px(y);
        if(b) i(n(i),r=fe(k_(f),k_(pdu[pii[i]]),t(4,pys[i]),0); EC(r); _k(pdu[pii[i]]); pdu[pii[i]]=r)
        else i(n(i),_k(pdu[pii[i]]);pdu[pii[i]]=t(4,pys[i]))
        break;
      case  0: pyu=px(y);
        if(b) i(n(i),r=fe(k_(f),k_(pdu[pii[i]]),k_(pyu[i]),0); EC(r); _k(pdu[pii[i]]); pdu[pii[i]]=r)
        else i(n(i),_k(pdu[pii[i]]);pdu[pii[i]]=kcp2(pyu[i]);EC(pdu[pii[i]]))
        break;
      } break;
    case  6:
      switch(Ty) {
      case  1: case 2: case 3: case 4: case 6:
        if(b) { i(n(d),r=fe(k_(f),k_(pdu[i]),k_(y),0); EC(r); _k(pdu[i]); pdu[i]=r) }
        else { i(n(d),_k(pdu[i]); pdu[i]=k_(y)) }
        break;
      case 15:
        if(b) i(n(d),r=fe(k_(f),k_(pdu[i]),k_(y),0); EC(r); _k(pdu[i]); pdu[i]=r)
        else i(n(d),_k(pdu[i]);pdu[i]=kcp(y);EC(pdu[i]))
        break;
      case -1: pyi=px(y);
        if(n(d)!=n(y)) { e=KERR_LENGTH; goto cleanup; }
        if(b) i(n(d),r=fe(k_(f),k_(pdu[i]),t(1,(u32)pyi[i]),0); EC(r); _k(pdu[i]); pdu[i]=r)
        else i(n(d),_k(pdu[i]);pdu[i]=t(1,(u32)pyi[i]))
        break;
      case -2: pyf=px(y);
        if(n(d)!=n(y)) { e=KERR_LENGTH; goto cleanup; }
        if(b) i(n(d),r=fe(k_(f),k_(pdu[i]),t2(pyf[i]),0); EC(r); _k(pdu[i]); pdu[i]=r)
        else i(n(d),_k(pdu[i]);pdu[i]=t2(pyf[i]))
        break;
      case -3: pyc=px(y);
        if(n(d)!=n(y)) { e=KERR_LENGTH; goto cleanup; }
        if(b) i(n(d),r=fe(k_(f),k_(pdu[i]),t(3,(u8)pyc[i]),0); EC(r); _k(pdu[i]); pdu[i]=r)
        else i(n(d),_k(pdu[i]);pdu[i]=t(3,(u8)pyc[i]))
        break;
      case -4: pys=px(y);
        if(n(d)!=n(y)) { e=KERR_LENGTH; goto cleanup; }
        if(b) i(n(d),r=fe(k_(f),k_(pdu[i]),t(4,pys[i]),0); EC(r); _k(pdu[i]); pdu[i]=r)
        else i(n(d),_k(pdu[i]);pdu[i]=t(4,pys[i]))
        break;
      case  0: pyu=px(y);
        if(n(d)!=n(y)) { e=KERR_LENGTH; goto cleanup; }
        if(b) i(n(d),r=fe(k_(f),k_(pdu[i]),k_(pyu[i]),0); EC(r); _k(pdu[i]); pdu[i]=r)
        else i(n(d),_k(pdu[i]);pdu[i]=kcp2(pyu[i]);EC(pdu[i]))
        break;
      } break;
    case  0: {
      stack=xmalloc(sizeof(sf)*sm);
      stack[sp++]=(sf){i,y,0};
      while(sp) {
        sf *pf=&stack[sp-1];
        K i_=pf->i;
        K y_=pf->y;
        K *piu=px(i_);
        char Ty_=T(y_); if(s(y_)) { if(!VST(y_)) { e=KERR_TYPE; goto cleanup; } Ty_=10; }
        switch(Ty_) {
        case -1: pyi=px(y_);
          if(n(i_)!=n(y_)) { e=KERR_LENGTH; goto cleanup; }
          i(n(i_),d=kamendi4v(d,k_(piu[i]),k_(f),t(1,(u32)pyi[i]));EC(d))
          break;
        case -2: pyf=px(y_);
          if(n(i_)!=n(y_)) { e=KERR_LENGTH; goto cleanup; }
          i(n(i_),d=kamendi4v(d,k_(piu[i]),k_(f),t2(pyf[i]));EC(d))
          break;
        case -3: pyc=px(y_);
          if(n(i_)!=n(y_)) { e=KERR_LENGTH; goto cleanup; }
          i(n(i_),d=kamendi4v(d,k_(piu[i]),k_(f),t(3,(u8)pyc[i]));EC(d))
          break;
        case -4: pys=px(y_);
          if(n(i_)!=n(y_)) { e=KERR_LENGTH; goto cleanup; }
          i(n(i_),d=kamendi4v(d,k_(piu[i]),k_(f),t(4,pys[i]));EC(d))
          break;
        case 1: case 2: case 3: case 4: case 6: case 15:
          while(pf->j<n(i_)) {
            K i2=piu[pf->j++];
            if(s(i2)) { e=KERR_TYPE; goto cleanup; }
            if(T(i2)==0) {
              if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
              stack[sp++]=(sf){i2,y_,0};
              goto continue_outer;
            }
            else {
              d=kamendi4v(d,k_(i2),k_(f),k_(y));
              if(E(d)) { e=d; goto cleanup; }
            }
          }
          break;
        case 0:
          if(n(i_)!=n(y_)) { e=KERR_LENGTH; goto cleanup; }
          K *pyu=px(y_);
          while(pf->j<n(i_)) {
            K i2=piu[pf->j];
            K y2=pyu[pf->j++];
            if(T(i2)==0 && T(y2)==0) {
              if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
              stack[sp++]=(sf){i2,y2,0};
              goto continue_outer;
            }
            else {
              d=kamendi4v(d,k_(i2),k_(f),k_(y2));
              if(E(d)) { e=d; goto cleanup; }
            }
          }
          break;
        default: { e=KERR_TYPE; goto cleanup; }
        }
        --sp;
  continue_outer:;
      }
      xfree(stack);
      }
      break;
    default: _k(d); d=KERR_TYPE;
    }
  }
  _k(i); _k(f); _k(y);
  --dd;
  return knorm(d);
cleanup:
  if(stack) xfree(stack);
  _k(d); _k(i); _k(f); _k(y);
  return e;
}

K kamendi4(K d, K i, K f, K y) {
  K r,e,sym=0,*prk;

  if(d==inull||i==inull||f==inull||y==inull) {
    r=tn(0,5); prk=px(r);
    K g=t(1,st(0xc0,45)); // @
    prk[0]=g;
    prk[1]=kcp(d); if(E(prk[1])) { e=prk[1]; _k(r); goto cleanup; }
    prk[2]=kcp(i); if(E(prk[2])) { e=prk[2]; _k(r); goto cleanup; }
    prk[3]=kcp(f); if(E(prk[3])) { e=prk[3]; _k(r); goto cleanup; }
    prk[4]=kcp(y); if(E(prk[4])) { e=prk[4]; _k(r); goto cleanup; }
    _k(d); _k(i); _k(f); _k(y);
    return st(0xd6,r);
  }

  if(!d) { e=KERR_TYPE; goto cleanup; }
  if(!ISF(f)) { e=KERR_TYPE; goto cleanup; }
  if(4==T(d)) {
    K d2=scope_get(gs,d); sym=d; d=d2;
    if(E(d)) d=null;
    else if((T(d)<=0||T(d)==2) && 1<((ko*)(b(48)&d))->r) { K d2=kcp(d); _k(d); d=d2; }
  }
  else { K d2=kcp(d); _k(d); d=d2; }
  EC(d);

  if(T(d)>0) { e=KERR_RANK; goto cleanup; }
  if((s(d)&&s(d)!=0x80)) { e=KERR_TYPE; goto cleanup; }

  if(0x80==s(d)) r=kamendi4d(d,i,f,y);
  else r=kamendi4v(d,i,f,y);

  if(E(r)) return r;
  else if(sym) { scope_set(gs,sym,r); _k(r); return sym; }
  else return r;
cleanup:
  _k(d); _k(i); _k(f); _k(y);
  return e;
}

static K kamend3_(K d, K i, K f) {
  K r,e,*pdu,t,i0,i2,*pv,keys;
  char **pk;
  typedef struct { K d,i,r,ri,p; u64 j; } sf;
  i32 sm=32,sp=0,*pi;
  sf *stack=0;
  static int dd=0;
  if(++dd>maxr) { e=KERR_STACK; --dd; goto cleanup; }

#define PROPAGATE_RESULT(x) do { \
    if(sp == 1) { _k(pf->d); pf->d=(x); break; } \
    sf *p=&stack[pf->p]; \
    _k(p->r); p->r=(x); \
  } while(0)

  if(T(i)<=0 && !n(i)) { /* amend entire */
    r=fe(f,0,d,0);
    _k(i);
    --dd;
    return r;
  }

  stack = xmalloc(sizeof(sf)*sm);
  stack[sp++] = (sf){k_(d), k_(i), 0, 0, 0, 0};

  while(sp) {
    sf *pf = &stack[sp-1];
    K d_ = pf->d;
    K i_ = pf->i;
    K r_ = pf->r;
    K ri_ = pf->ri;

    if(r_) {
      if(!ri_) {
        PROPAGATE_RESULT(k_(r_)); goto freeframe;
      }
      if(0x80 == s(d_)) {
        if(T(ri_) == 4) {
          (void)dset(d_, sk(ri_), r_);
          PROPAGATE_RESULT(k_(d_)); goto freeframe;
        }
        else if(T(ri_) == -4) {
          pk=px(ri_);
          (void)dset(d_, pk[pf->j], r_);
          _k(pf->r); pf->r=0;
          if(++pf->j==n(ri_)) {
            PROPAGATE_RESULT(k_(d_)); goto freeframe;
          }
        }
      }
      else {
        pdu=px(d_);
        if(T(ri_) == 1) {
          _k(pdu[ik(ri_)]); pdu[ik(ri_)]=k_(r_);
          PROPAGATE_RESULT(k_(d_)); goto freeframe;
        }
        else if(T(ri_) == -1) {
          pi=px(ri_);
          _k(pdu[pi[pf->j]]); pdu[pi[pf->j]]=k_(r_);
          _k(pf->r); pf->r=0;
          if(++pf->j==n(ri_)) {
            PROPAGATE_RESULT(k_(d_)); goto freeframe;
          }
        }
        else if(T(ri_) == 6) {
          _k(pdu[pf->j]); pdu[pf->j]=k_(r_);
          _k(pf->r); pf->r=0;
          if(++pf->j==n(d)) {
            PROPAGATE_RESULT(k_(d_)); goto freeframe;
          }
        }
      }
    }

    switch(T(d_)) {
    case  6:
      switch(T(i_)) {
      case 1:
        r=fe(k_(f),0,k_(i_),0);
        PROPAGATE_RESULT(r);
        break;
      case 4:
        t=fe(k_(f),0,null,0);
        if(E(t)) { e=t; goto cleanup; }
        r=dnew();
        (void)dset(r,sk(i_),t); _k(t);
        PROPAGATE_RESULT(r);
        break;
      case 6:
        r=fe(k_(f),0,tn(0,0),0);
        PROPAGATE_RESULT(r);
        break;
      default: e=KERR_TYPE; goto cleanup;
      }
      break;
    case -1: case -2: case -3: case -4:
      switch(T(i_)) {
      case  1:
        r=kamendi3v(k_(d_),k_(i_),k_(f)); EC(r);
        PROPAGATE_RESULT(r);
        break;
      case -1:
        if(n(i_)!=1) { e=KERR_RANK; goto cleanup; }
        i0=k(3,0,k_(i_)); EC(i0);
        r=kamendi3v(k_(d_),i0,k_(f)); EC(r);
        PROPAGATE_RESULT(r);
        break;
      case  6:
        r=kamendi3v(k_(d_),k_(i_),k_(f)); EC(r);
        PROPAGATE_RESULT(r);
        break;
      case  0:
        if(n(i_)!=1) { e=KERR_RANK; goto cleanup; }
        i0=k(3,0,k_(i_)); EC(i0);
        r=kamendi3v(k_(d_),i0,k_(f)); EC(r);
        PROPAGATE_RESULT(r);
        break;
      default: e=KERR_TYPE; goto cleanup;
      } break;
    case 0:
      if(0x80 == s(d_)) {
        switch(T(i_)) {
        case  4:
          t=dget(d_,sk(i_)); if(!t) t=dnew();
          r=fe(k_(f),0,t,0); EC(r);
          e=dset(d_,sk(i_),r); _k(r); if(E(e)) goto cleanup;
          PROPAGATE_RESULT(k_(d_));
          break;
        case -4:
          i0=k(3,0,k_(i_)); EC(i0);
          i2=k(16,t(1,1),k_(i_));
          if(E(i2)) { _k(i0); e=i2; goto cleanup; }
          t=dget(d_,sk(i0)); if(!t) t=dnew();
          _k(pf->ri); pf->ri=i0;
          if(n(i2)) {
            if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
            stack[sp]=(sf){t, i2, 0, 0, sp-1, 0}; ++sp;
            continue;
          }
          else {
            _k(i2);
            r=fe(k_(f),0,t,0); EC(r);
            e=dset(d_,sk(i0),r); _k(r); if(E(e)) goto cleanup;
            PROPAGATE_RESULT(k_(d_));
            break;
          }
        case  6:
          pdu=px(d_);
          pk=px(pdu[0]); pv=px(pdu[1]);
          for(u64 i=0;i<n(pdu[0]);++i) {
            r=fe(k_(f),0,k_(pv[i]),0); EC(r);
            e=dset(d_,pk[i],r); _k(r); if(E(e)) goto cleanup;
          }
          PROPAGATE_RESULT(k_(d_));
          break;
        case  0:
          i0=k(3,0,k_(i_)); EC(i0);
          i2=k(16,t(1,1),k_(i_));
          if(E(i2)) { _k(i0); e=i2; goto cleanup; }
          t=kmix(i2); if(E(t)) { _k(i0); _k(i2); e=t; goto cleanup; }
          _k(i2); i2=t;
          switch(T(i0)) {
          case 4:
            t=dget(d_,sk(i0)); if(!t) t=null;
            _k(pf->ri); pf->ri=i0;
            if(n(i2)) {
              if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
              stack[sp]=(sf){t, i2, 0, 0, sp-1, 0}; ++sp;
              continue;
            }
            else {
              _k(i2);
              r=fe(k_(f),0,t,0); EC(r);
              e=dset(d_,sk(i0),r); _k(r); if(E(e)) goto cleanup;
              PROPAGATE_RESULT(k_(d_));
              break;
            }
          case -4:
            if(n(i2)) {
              char **pi0=px(i0);
              _k(pf->ri); pf->ri=i0;
              if(pf->j>=n(i0)) { _k(i2); e=KERR_INDEX; goto cleanup; }
              t=dget(d_,pi0[pf->j]); if(!t) t=null;
              if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
              stack[sp]=(sf){t, i2, 0, 0, sp-1, 0}; ++sp;
              continue;
            }
            else {
              _k(i2);
              char **pi0=px(i0);
              for(u64 i=0;i<n(i0);++i) {
                t=dget(d_,pi0[i]); if(!t) t=null;
                r=fe(k_(f),0,t,0);
                if(E(r)){ _k(i0); e=r; goto cleanup; }
                e=dset(d_,pi0[i],r); _k(r); if(E(e)) { _k(i0); goto cleanup; }
              }
              _k(i0);
              PROPAGATE_RESULT(k_(d_));
              break;
            }
          case  6:
            pdu=px(d_);
            keys=pdu[0];
            pk=px(keys);
            if(n(i2)) {
              _k(pf->ri); pf->ri=k_(keys);
              if(pf->j>=n(keys)) { e=KERR_INDEX; goto cleanup; }
              t=dget(d_,pk[pf->j]); if(!t) t=null;
              if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
              stack[sp]=(sf){t, i2, 0, 0, sp-1, 0}; ++sp;
              continue;
            }
            else {
              _k(i2);
              for(u64 i=0;i<n(keys);++i) {
                t=dget(d_,pk[i]); if(!t) t=null;
                r=fe(k_(f),0,t,0); EC(r);
                e=dset(d_,pk[i],r); _k(r); if(E(e)) goto cleanup;
              }
              PROPAGATE_RESULT(k_(d_));
              break;
            }
          case  0:;
            K *pi0u=px(i0);
            K *pi2=px(i2);
            for(u64 i=0;i<n(i0);++i) {
              K q=tn(0,1+n(i2));
              K *pq=px(q);
              pq[0]=k_(pi0u[i]);
              for(u64 j=0;j<n(i2);++j) { pq[1+j]=k_(pi2[j]); }
              r=kamend3_(k_(d_),q,k_(f));
              if(E(r)) { _k(i0); _k(i2); e=r; goto cleanup; }
              _k(r);
            }
            _k(i0); _k(i2);
            PROPAGATE_RESULT(k_(d_));
            break;
          default: _k(i0); _k(i2); e=KERR_TYPE; goto cleanup;
          } break;
        default: e=KERR_TYPE; goto cleanup;
        }
      }
      else if(s(d_)) {
        e=KERR_TYPE;
        goto cleanup;
      }
      else {
        pdu=px(d_);
        switch(T(i_)) {
        case  1:
          if(ik(i_)<0||(K)(ik(i_))>=n(d_)) { e=KERR_INDEX; goto cleanup; }
          t=k(13,k_(d_),k_(i_)); EC(t);
          r=fe(k_(f),0,t,0); EC(r);
          _k(pdu[ik(i_)]); pdu[ik(i_)]=r;
          PROPAGATE_RESULT(k_(d_));
          break;
        case -1:
          i0=k(3,0,k_(i_)); EC(i0); /* *i_ */
          i2=k(16,t(1,1),k_(i_));   /* 1_i_ */
          if(E(i2)) { _k(i0); e=i2; goto cleanup; }
          t=k(13,k_(d_),i0);  /* d_@i0 */
          if(E(t)){ _k(i2); e=t; goto cleanup; }
          _k(pf->ri); pf->ri=i0;
          if(n(i2)) {
            if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
            stack[sp]=(sf){t, i2, 0, 0, sp-1, 0}; ++sp;
            continue;
          }
          else {
            _k(i2);
            r=fe(k_(f),0,t,0); EC(r);
            _k(pdu[ik(i0)]); pdu[ik(i0)]=r;
            PROPAGATE_RESULT(k_(d_));
            break;
          }
        case  6:
          i(n(d_), r=fe(k_(f),0,k_(pdu[i]),0); EC(r); _k(pdu[i]); pdu[i]=r)
          PROPAGATE_RESULT(k_(d_));
          break;
        case  0:
          i0=k(3,0,k_(i_)); EC(i0);
          i2=k(16,t(1,1),k_(i_));
          if(E(i2)) { _k(i0); e=i2; goto cleanup; }
          t=kmix(i2); if(E(t)) { _k(i0); e=t; goto cleanup; }
          _k(i2); i2=t;
          switch(T(i0)) {
          case  1:
            if(ik(i0)<0||(K)(ik(i0))>=n(d_)) { _k(i0); _k(i2); e=KERR_INDEX; goto cleanup; }
            t=k_(pdu[ik(i0)]);
            _k(pf->ri); pf->ri=i0;
            if(n(i2)) {
              if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
              stack[sp]=(sf){t, i2, 0, 0, sp-1, 0}; ++sp;
              continue;
            }
            else {
              _k(i2);
              r=fe(k_(f),0,t,0); EC(r);
              _k(pdu[ik(i0)]); pdu[ik(i0)]=r;
              PROPAGATE_RESULT(k_(d_));
              break;
            }
          case -1:;
            i32 *pi0=px(i0);
            i(n(i0),if(pi0[i]<0||(K)pi0[i]>=n(d_)) { _k(i0); _k(i2); e=KERR_INDEX; goto cleanup; })
            if(n(i2)) {
              if(pf->j>=n(i0)) { _k(i0); _k(i2); e=KERR_INDEX; goto cleanup; }
              _k(pf->ri); pf->ri=i0;
              if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
              stack[sp]=(sf){k_(pdu[pi0[pf->j]]), i2, 0, 0, sp-1, 0}; ++sp;
              continue;
              }
            else {
              _k(i0); _k(i2);
              i(n(i0),r=fe(k_(f),0,k_(pdu[pi0[i]]),0); EC(r); _k(pdu[pi0[i]]); pdu[pi0[i]]=r)
              PROPAGATE_RESULT(k_(d_));
              break;
            }
          case  6:
            if(n(i2)) {
              if(pf->j>=n(d_)) { _k(i0); _k(i2); e=KERR_INDEX; goto cleanup; }
              pf->r=tn(0,n(d_));
              _k(pf->ri); pf->ri=i0;
              if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
              stack[sp]=(sf){k_(pdu[pf->j]), i2, 0, 0, sp-1, 0}; ++sp;
              continue;
            }
            else {
              _k(i0); _k(i2);
              i(n(d_),r=fe(k_(f),0,k_(pdu[i]),0); EC(r); _k(pdu[i]); pdu[i]=r)
              PROPAGATE_RESULT(k_(d_));
              break;
            }
          case  0:;
            K *pi0u=px(i0);
            K *pi2=px(i2);
            for(u64 i=0;i<n(i0);++i) {
              K q=tn(0,1+n(i2));
              K *pq=px(q);
              pq[0]=k_(pi0u[i]);
              for(u64 j=0;j<n(i2);++j) { pq[1+j]=k_(pi2[j]); }
              r=kamend3_(k_(d_),q,k_(f));
              if(E(r)) { _k(i0); _k(i2); e=r; goto cleanup; }
              _k(r);
            }
            _k(i2);
            PROPAGATE_RESULT(k_(d_));
            _k(i0);
            break;
          default: _k(t); _k(i0); e=KERR_TYPE; goto cleanup;
          }
          break;
        default: e=KERR_TYPE; goto cleanup;
        }
      }
      break;
    default: e=KERR_RANK; goto cleanup;
    }
freeframe:
    --sp;
    if(sp>0) _k(stack[sp].d);
    _k(stack[sp].i);
    _k(stack[sp].r);
    _k(stack[sp].ri);
  }
  _k(d); _k(i); _k(f);
  d=stack[0].d;
  xfree(stack);
  --dd;
  return knorm(d);
cleanup:
  if(stack) {
    while(sp--) {
      _k(stack[sp].d);
      _k(stack[sp].i);
      _k(stack[sp].r);
      _k(stack[sp].ri);
    }
    xfree(stack);
  }
  _k(d); _k(i); _k(f);
  return e;
}

K kamend3(K d, K i, K f) {
  K r,e,sym=0,t,*prk;
  int kt=0;

  if(d==inull||i==inull||f==inull) {
    r=tn(0,4); prk=px(r);
    K g=t(1,st(0xc0,43)); // .
    prk[0]=g;
    prk[1]=kcp(d); if(E(prk[1])) { e=prk[1]; _k(r); goto cleanup; }
    prk[2]=kcp(i); if(E(prk[2])) { e=prk[2]; _k(r); goto cleanup; }
    prk[3]=kcp(f); if(E(prk[3])) { e=prk[3]; _k(r); goto cleanup; }
    _k(d); _k(i); _k(f);
    return st(0xd5,r);
  }

  if(!d) { e=KERR_TYPE; goto cleanup; }
  if(s(i)) { e=KERR_TYPE; goto cleanup; }
  if(!ISF(f)) { e=KERR_TYPE; goto cleanup; }
  if(4==T(d)&&!s(d)) {
    if('.'==*sk(d)||0==*sk(d)) kt=1;
    K d2=scope_get(gs,d); sym=d; d=d2;
    if(E(d)) d=null;
  }
  if(kt) ;
  else if((T(d)<=0||T(d)==2) && 1<((ko*)(b(48)&d))->r) { K d2=kcp(d); EC(d2); _k(d); d=d2; }

  // cannot amend entire ktree
  if(d==ktree && T(i)<=0 && n(i)==0) { e=KERR_RESERVED; goto cleanup; }

  if(T(i)<=0 && !n(i)) r=kamend3_(d,i,f); /* amend entire */
  else if(ISF(d) && 0xc0==s(f) && 32==ck(f)) { /* error trap */
    int e0=EFLAG; EFLAG=0;
    t=k(11,d,i); /* d . i */
    EFLAG=e0;
    r=tn(0,2); prk=px(r);
    if(t<EMAX) { prk[0]=t(1,1); prk[1]=tnv(3,strlen(E[t]),xmemdup(E[t],1+strlen(E[t]))); }
    else if(E(t)) { prk[0]=t(1,1); prk[1]=tnv(3,strlen(sk(t)),xmemdup(sk(t),1+strlen(sk(t)))); }
    else { prk[0]=t(1,0); prk[1]=t; }
    r=knorm(r);
  }
  else if((T(d)>0&&T(d)!=6) || ISF(d)) { e=KERR_RANK; goto cleanup; }
  else r=kamend3_(d,i,f);

  if(E(r)) return r;
  else if(sym && kt) { _k(r); return sym; }
  else if(sym) { scope_set(gs,sym,r); _k(r); return sym; }
  else return r;
cleanup:
  _k(d); _k(i); _k(f);
  return e;
}

static K kamend4_(K d, K i, K f, K y) {
  K r,e,*pdu,t,i0,i2,v,*pv,keys,ym,*pym;
  char **pk;
  typedef struct { K d,i,y,r,ri,p; u64 j; } sf;
  i32 sm=32,sp=0,*pi;
  sf *stack=0;
  static int dd=0;
  if(++dd>maxr) { e=KERR_STACK; --dd; goto cleanup; }

#define PROPAGATE_RESULT(x) do { \
    if(sp == 1) { _k(pf->d); pf->d=(x); break; } \
    sf *p=&stack[pf->p]; \
    _k(p->r); p->r=(x); \
  } while(0)

  if(T(i)<=0 && !n(i)) { /* amend entire */
    r=fe(k_(f),d,y,0);
    _k(i); _k(f);
    --dd;
    return r;
  }

  stack = xmalloc(sizeof(sf)*sm);
  stack[sp++] = (sf){k_(d), k_(i), k_(y), 0, 0, 0, 0};

  while(sp) {
    sf *pf = &stack[sp-1];
    K d_ = pf->d;
    K i_ = pf->i;
    K y_ = pf->y;
    K r_ = pf->r;
    K ri_ = pf->ri;

    if(r_) {
      pf->r=knorm(pf->r); r_=pf->r;
      if(!ri_) {
        PROPAGATE_RESULT(k_(r_)); goto freeframe;
      }
      if(0x80 == s(d_)) {
        if(T(ri_) == 4) {
          (void)dset(d_, sk(ri_), r_);
          PROPAGATE_RESULT(k_(d_)); goto freeframe;
        }
        else if(T(ri_) == -4) {
          pk=px(ri_);
          (void)dset(d_, pk[pf->j], r_);
          _k(pf->r); pf->r=0;
          if(++pf->j==n(ri_)) {
            PROPAGATE_RESULT(k_(d_)); goto freeframe;
          }
        }
      }
      else {
        pdu=px(d_);
        if(T(ri_) == 1) {
          _k(pdu[ik(ri_)]); pdu[ik(ri_)]=k_(r_);
          PROPAGATE_RESULT(k_(d_)); goto freeframe;
        }
        else if(T(ri_) == -1) {
          pi=px(ri_);
          _k(pdu[pi[pf->j]]); pdu[pi[pf->j]]=k_(r_);
          _k(pf->r); pf->r=0;
          if(++pf->j==n(ri_)) {
            PROPAGATE_RESULT(k_(d_)); goto freeframe;
          }
        }
        else if(T(ri_) == 6) {
          _k(pdu[pf->j]); pdu[pf->j]=k_(r_);
          _k(pf->r); pf->r=0;
          if(++pf->j==n(d)) {
            PROPAGATE_RESULT(k_(d_)); goto freeframe;
          }
        }
      }
    }

    switch(T(d_)) {
    case  6:
      r=null;
      PROPAGATE_RESULT(r);
      break;
    case -1: case -2: case -3: case -4:
      if(i_==inull)i_=null;
      switch(T(i_)) {
      case  1:
        r=kamendi4v(k_(d_),k_(i_),k_(f),k_(y_)); EC(r);
        PROPAGATE_RESULT(r);
        break;
      case -1:
        if(n(i_)!=1) { e=KERR_RANK; goto cleanup; }
        i0=k(3,0,k_(i_)); EC(i0);
        r=kamendi4v(k_(d_),i0,k_(f),k_(y_)); EC(r);
        PROPAGATE_RESULT(r);
        break;
      case  6:
        r=kamendi4v(k_(d_),k_(i_),k_(f),k_(y_)); EC(r);
        PROPAGATE_RESULT(r);
        break;
      case  0:
        if(n(i_)!=1) { e=KERR_RANK; goto cleanup; }
        i0=k(3,0,k_(i_)); EC(i0);
        if(i0==inull)i0=null;
        r=kamendi4v(k_(d_),i0,k_(f),k_(y_)); EC(r);
        PROPAGATE_RESULT(r);
        break;
      default: e=KERR_TYPE; goto cleanup;
      } break;
    case 0:
      if(0x80 == s(d_)) {
        if(i_==inull)i_=null;
        switch(T(i_)) {
        case  4:
          t=dget(d_,sk(i_)); if(!t) t=dnew();
          r=fe(k_(f),t,k_(y_),0); EC(r);
          e=dset(d_,sk(i_),r); _k(r); if(E(e)) goto cleanup;
          PROPAGATE_RESULT(k_(d_));
          break;
        case -4:
          i0=k(3,0,k_(i_)); EC(i0);
          i2=k(16,t(1,1),k_(i_));
          if(E(i2)) { _k(i0); e=i2; goto cleanup; }
          t=dget(d_,sk(i0)); if(!t) t=dnew();
          _k(pf->ri); pf->ri=i0;
          if(n(i2)) {
            if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
            stack[sp]=(sf){t, i2, k_(y_), 0, 0, sp-1, 0}; ++sp;
            continue;
          }
          else {
            _k(i2);
            r=fe(k_(f),t,k_(y_),0); EC(r);
            e=dset(d_,sk(i0),r); _k(r); if(E(e)) goto cleanup;
            PROPAGATE_RESULT(k_(d_));
            break;
          }
        case  6:
          pdu=px(d_);
          pk=px(pdu[0]);
          v=pdu[1]; pv=px(v);
          if(T(y_)<=0&&!s(y_)&&n(v)!=n(y_)) { e=KERR_LENGTH; goto cleanup; }
          if(T(y_)<=0&&!s(y_)) {
            ym=kmix(y_); EC(ym);
            pym=px(ym);
            for(u64 i=0;i<n(v);++i) {
              r=fe(k_(f),k_(pv[i]),k_(pym[i]),0); if(E(r)) { _k(ym); e=r; goto cleanup; };
              e=dset(d_,pk[i],r); _k(r); if(E(e)) goto cleanup;
            }
            _k(ym);
          }
          else {
            for(u64 i=0;i<n(v);++i) {
              r=fe(k_(f),k_(pv[i]),k_(y_),0); EC(r);
              e=dset(d_,pk[i],r); _k(r); if(E(e)) goto cleanup;
            }
          }
          PROPAGATE_RESULT(k_(d_));
          break;
        case  0:
          if(n(i_)==1) { /* special case of one item list i */
            i0=k(3,0,k_(i_)); EC(i0);
            r=kamendi4d(k_(d_),i0,k_(f),k_(y_)); EC(r);
            PROPAGATE_RESULT(r);
            break;
          }
          i0=k(3,0,k_(i_)); EC(i0);
          i2=k(16,t(1,1),k_(i_));
          if(E(i2)) { _k(i0); e=i2; goto cleanup; }
          t=kmix(i2); if(E(t)) { _k(i0); e=t; goto cleanup; }
          _k(i2); i2=t;
          if(i0==inull)i0=null;
          switch(T(i0)) {
          case 4:
            t=dget(d_,sk(i0)); if(!t) t=null;
            _k(pf->ri); pf->ri=i0;
            if(n(i2)) {
              if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
              stack[sp]=(sf){t, i2, k_(y), 0, 0, sp-1, 0}; ++sp;
              continue;
            }
            else {
              _k(i2);
              r=fe(k_(f),t,k_(y_),0); EC(r);
              e=dset(d_,sk(i0),r); _k(r); if(E(e)) goto cleanup;
              PROPAGATE_RESULT(k_(d_));
              break;
            }
          case -4:
            if(T(y_)<=0&&!s(y_)) {
              if(n(y_)!=n(i0)) { _k(i0); _k(i2); e=KERR_LENGTH; goto cleanup; }
              ym=kmix(y_); if(E(ym)) { _k(i0); _k(i2); e=KERR_LENGTH; goto cleanup; }
              pym=px(ym);
            }
            else ym=0;
            if(n(i2)) {
              char **pi0=px(i0);
              _k(pf->ri); pf->ri=i0;
              if(pf->j>=n(i0)) { _k(i2); e=KERR_INDEX; goto cleanup; }
              t=dget(d_,pi0[pf->j]); if(!t) t=null;
              if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
              stack[sp]=(sf){t, i2, k_(ym?pym[pf->j]:y_), 0, 0, sp-1, 0}; ++sp;
              _k(ym);
              continue;
            }
            else {
              _k(i2);
              char **pi0=px(i0);
              for(u64 i=0;i<n(i0);++i) {
                t=dget(d_,pi0[i]); if(!t) t=null;
                r=fe(k_(f),t,k_(ym?pym[i]:y_),0);
                if(E(r)) { _k(i0); _k(ym); e=r; goto cleanup; }
                e=dset(d_,pi0[i],r); _k(r); if(E(e)) { _k(i0); _k(ym); goto cleanup; }
              }
              _k(i0); _k(ym);
              PROPAGATE_RESULT(k_(d_));
              break;
            }
          case  6:
            pdu=px(d_);
            keys=pdu[0];
            pk=px(keys);
            if(T(y_)<=0&&!s(y_)) {
              if(n(y_)!=n(keys)) { _k(i2); e=KERR_LENGTH; goto cleanup; }
              ym=kmix(y_); if(E(ym)) { _k(i2); e=KERR_LENGTH; goto cleanup; }
              pym=px(ym);
            }
            else ym=0;
            if(n(i2)) {
              _k(pf->ri); pf->ri=k_(keys);
              if(pf->j>=n(keys)) { _k(i2); e=KERR_INDEX; goto cleanup; }
              t=dget(d_,pk[pf->j]); if(!t) t=null;
              if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
              stack[sp]=(sf){t, i2, k_(ym?pym[pf->j]:y_), 0, 0, sp-1, 0}; ++sp;
              _k(ym);
              continue;
            }
            else {
              _k(i2);
              for(u64 i=0;i<n(keys);++i) {
                t=dget(d_,pk[i]); if(!t) t=null;
                r=fe(k_(f),t,k_(ym?pym[i]:y_),0);
                if(E(r)) { _k(ym); e=r; goto cleanup; }
                e=dset(d_,pk[i],r); _k(r); if(E(e)) { _k(ym); goto cleanup; }
              }
              PROPAGATE_RESULT(k_(d_));
              _k(ym);
              break;
            }
          case  0:
            if(T(y_)<=0&&!s(y_)) {
              if(n(y_)!=n(i0)) { _k(i0); _k(i2); e=KERR_LENGTH; goto cleanup; }
              ym=kmix(y_); if(E(ym)) { _k(i0); _k(i2); e=KERR_LENGTH; goto cleanup; }
              pym=px(ym);
            }
            else ym=0;
            K *pi0=px(i0);
            for(u64 i=0;i<n(i0);++i) {
              K q=tn(0,1+n(i2));
              K *pq=px(q);
              pq[0]=k_(pi0[i]);
              K *pi2=px(i2);
              for(u64 j=0;j<n(i2);++j) { pq[1+j]=k_(pi2[j]); }
              r=kamend4_(k_(d_),q,k_(f),k_(ym?pym[i]:y_));
              if(E(r)) { _k(i0); _k(i2); _k(ym); e=r; goto cleanup; }
              _k(r);
            }
            _k(i2);
            PROPAGATE_RESULT(k_(d_));
            _k(i0); _k(ym);
            break;
          default: _k(i0); _k(i2); e=KERR_TYPE; goto cleanup;
          } break;
        default: e=KERR_TYPE; goto cleanup;
        }
      }
      else if(s(d_)) {
        e=KERR_TYPE;
        goto cleanup;
      }
      else {
        pdu=px(d_);
        if(i_==inull)i_=null;
        switch(T(i_)) {
        case  1:
          if(ik(i_)<0||(K)(ik(i_))>=n(d_)) { e=KERR_INDEX; goto cleanup; }
          t=k(13,k_(d_),k_(i_)); EC(t);
          r=fe(k_(f),t,k_(y_),0); EC(r);
          _k(pdu[ik(i_)]); pdu[ik(i_)]=r;
          PROPAGATE_RESULT(k_(d_));
          break;
        case -1:
          i0=k(3,0,k_(i_)); EC(i0); /* *i_ */
          i2=k(16,t(1,1),k_(i_));   /* 1_i_ */
          if(E(i2)) { _k(i0); e=i2; goto cleanup; }
          t=k(13,k_(d_),i0);  /* d_@i0 */
          if(E(t)){ _k(i2); e=t; goto cleanup; }
          _k(pf->ri); pf->ri=i0;
          if(n(i2)) {
            if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
            stack[sp]=(sf){t, i2, k_(y_), 0, 0, sp-1, 0}; ++sp;
            continue;
          }
          else {
            _k(i2);
            r=fe(k_(f),t,k_(y_),0); EC(r);
            _k(pdu[ik(i0)]); pdu[ik(i0)]=r;
            PROPAGATE_RESULT(k_(d_));
            break;
          }
        case  6:
          if(T(y_)<=0&&!s(y_)&&n(d_)!=n(y_)) { e=KERR_LENGTH; goto cleanup; }
          if(T(y_)<=0&&!s(y_)) {
            ym=kmix(y_); EC(ym);
            pym=px(ym);
            i(n(d), r=fe(k_(f),k_(pdu[i]),k_(pym[i]),0); if(E(r)) { _k(ym); e=r; goto cleanup; }; _k(pdu[i]); pdu[i]=r)
            _k(ym);
          }
          else i(n(d), r=fe(k_(f),k_(pdu[i]),k_(y_),0); EC(r); _k(pdu[i]); pdu[i]=r)
          PROPAGATE_RESULT(k_(d_));
          break;
        case  0:
          if(n(i_)==1) { /* special case of one item list i */
            i0=k(3,0,k_(i_)); EC(i0);
            r=kamendi4v(k_(d_),i0,k_(f),k_(y_)); EC(r);
            PROPAGATE_RESULT(r);
            break;
          }
          i0=k(3,0,k_(i_)); EC(i0);
          i2=k(16,t(1,1),k_(i_));
          if(E(i2)) { _k(i0); e=i2; goto cleanup; }
          t=kmix(i2); if(E(t)) { _k(i0); e=t; goto cleanup; }
          _k(i2); i2=t;
          if(i0==inull)i0=null;
          switch(T(i0)) {
          case  1:
            if(ik(i0)<0||(K)(ik(i0))>=n(d_)) { _k(i0); _k(i2); e=KERR_INDEX; goto cleanup; }
            t=k_(pdu[ik(i0)]);
            _k(pf->ri); pf->ri=i0;
            if(n(i2)) {
              if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
              stack[sp]=(sf){t, i2, k_(y_), 0, 0, sp-1, 0}; ++sp;
              continue;
            }
            else {
              _k(i2);
              r=fe(k_(f),t,k_(y_),0); EC(r);
              _k(pdu[ik(i0)]); pdu[ik(i0)]=r;
              PROPAGATE_RESULT(k_(d_));
              break;
            }
          case -1:
            if(!n(i0)) { _k(i0); _k(i2); PROPAGATE_RESULT(k_(d_)); break; }
            if(T(y_)<=0&&!s(y_)) {
              if(n(y_)!=n(i0)) { _k(i0); _k(i2); e=KERR_LENGTH; goto cleanup; }
              ym=kmix(y_); if(E(ym)) { _k(i0); _k(i2); e=KERR_LENGTH; goto cleanup; }
              pym=px(ym);
            }
            else ym=0;
            i32 *pi0=px(i0);
            i(n(i0),if(pi0[i]<0||(K)pi0[i]>=n(d_)) { _k(i0); _k(i2); _k(ym); e=KERR_INDEX; goto cleanup; })
            if(n(i2)) {
              if(pf->j>=n(i0)) { _k(i0); _k(i2); _k(ym); e=KERR_INDEX; goto cleanup; }
              _k(pf->ri); pf->ri=i0;
              if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
              stack[sp]=(sf){k_(pdu[pi0[pf->j]]), i2, k_(ym?pym[pf->j]:y_), 0, 0, sp-1, 0}; ++sp;
              _k(ym);
              continue;
            }
            else {
              _k(i2);
              i(n(i0),r=fe(k_(f),k_(pdu[pi0[i]]),k_(ym?pym[i]:y_),0); EC(r); _k(pdu[pi0[i]]); pdu[pi0[i]]=r)
              _k(i0); _k(ym);
              PROPAGATE_RESULT(k_(d_));
              break;
            }
          case  6:
            pdu=px(d_);
            if(T(y_)<=0&&!s(y_)) {
              if(n(y_)!=n(d_)) { _k(i0); _k(i2); e=KERR_LENGTH; goto cleanup; }
              ym=kmix(y_); if(E(ym)) { _k(i0); _k(i2); e=ym; goto cleanup; }
              pym=px(ym);
            }
            else ym=0;
            if(n(i2)) {
              if(pf->j>=n(d_)) { _k(i0); _k(i2); _k(ym); e=KERR_INDEX; goto cleanup; }
              _k(pf->ri); pf->ri=i0;
              if(sp==sm) { stack=xrealloc(stack,sizeof(sf)*(sm*=2)); }
              stack[sp]=(sf){k_(pdu[pf->j]), i2, k_(ym?pym[pf->j]:y_), 0, 0, sp-1, 0}; ++sp;
              _k(ym);
              continue;
            }
            else {
              _k(i2);
              i(n(d_),r=fe(k_(f),k_(pdu[i]),k_(ym?pym[i]:y_),0); EC(r); _k(pdu[i]); pdu[i]=r)
              _k(i0); _k(ym);
              PROPAGATE_RESULT(k_(d_));
              break;
            }
          case  0:
            if(T(y_)<=0&&!s(y_)) {
              if(n(y_)!=n(i0)) { _k(i0); _k(i2); e=KERR_LENGTH; goto cleanup; }
              ym=kmix(y_); if(E(ym)) { _k(i0); _k(i2); e=ym; goto cleanup; }
              pym=px(ym);
            }
            else ym=0;
            K *pi0u=px(i0);
            K *pi2=px(i2);
            for(u64 i=0;i<n(i0);++i) {
              K q=tn(0,1+n(i2));
              K *pq=px(q);
              pq[0]=k_(pi0u[i]);
              for(u64 j=0;j<n(i2);++j) { pq[1+j]=k_(pi2[j]); }
              r=kamend4_(k_(d_),q,k_(f),k_(ym?pym[i]:y_));
              if(E(r)) { _k(i0); _k(i2); _k(ym); e=r; goto cleanup; }
              _k(r);
            }
            _k(i0); _k(i2); _k(ym);
            PROPAGATE_RESULT(k_(d_));
            break;
          default: _k(t); _k(i0); e=KERR_TYPE; goto cleanup;
          }
          break;
        default: e=KERR_TYPE; goto cleanup;
        }
      }
      break;
    default: e=KERR_RANK; goto cleanup;
    }
freeframe:
    --sp;
    if(sp>0) _k(stack[sp].d);
    _k(stack[sp].i);
    _k(stack[sp].y);
    _k(stack[sp].r);
    _k(stack[sp].ri);
  }
  _k(d); _k(i); _k(f); _k(y);
  d=stack[0].d;
  xfree(stack);
  --dd;
  return knorm(d);
cleanup:
  if(stack) {
    while(sp--) {
      _k(stack[sp].d);
      _k(stack[sp].i);
      _k(stack[sp].y);
      _k(stack[sp].r);
      _k(stack[sp].ri);
    }
    xfree(stack);
  }
  _k(d); _k(i); _k(f); _k(y);
  return e;
}

K kamend4(K d, K i, K f, K y) {
  K r,e,sym=0,*prk;
  int kt=0;

  if(d==inull||i==inull||f==inull||y==inull) {
    r=tn(0,5); prk=px(r);
    K g=t(1,st(0xc0,43)); // .
    prk[0]=g;
    prk[1]=kcp(d); if(E(prk[1])) { e=prk[1]; _k(r); goto cleanup; }
    prk[2]=kcp(i); if(E(prk[2])) { e=prk[2]; _k(r); goto cleanup; }
    prk[3]=kcp(f); if(E(prk[3])) { e=prk[3]; _k(r); goto cleanup; }
    prk[4]=kcp(y); if(E(prk[4])) { e=prk[4]; _k(r); goto cleanup; }
    _k(d); _k(i); _k(f); _k(y);
    return st(0xd6,r);
  }

  if(!d) { e=KERR_TYPE; goto cleanup; }
  if(s(i)) { e=KERR_TYPE; goto cleanup; }
  if(!ISF(f)) { e=KERR_TYPE; goto cleanup; }
  if(4==T(d)&&!s(d)) {
    if('.'==*sk(d)||0==*sk(d)) kt=1;
    K d2=scope_get(gs,d); sym=d; d=d2;
    if(E(d)) d=null;
    else if(kt) ;
    else if((T(d)<=0||T(d)==2) && 1<((ko*)(b(48)&d))->r) { K d2=kcp(d); _k(d); d=d2; }
  }
  else { K d2=kcp(d); _k(d); d=d2; }
  EC(d);

  // cannot amend entire ktree
  if(d==ktree && T(i)<=0 && n(i)==0) { e=KERR_RESERVED; goto cleanup; }

  r=kamend4_(d,i,f,y);

  if(E(r)) return r;
  else if(sym && kt) { _k(r); return sym; }
  else if(sym) { scope_set(gs,sym,r); _k(r); return sym; }
  else return r;
cleanup:
  _k(d); _k(i); _k(f); _k(y);
  return e;
}

K kslide(K f, K a, K x, char *av) {
  K r=0,*prk,xm=0,*pxm,e;
  u32 n,s,v=2,m;
  K ff=f;
  char Tx;

  if(T(f)==10||T(a)==10||T(x)==10) {
    r=tn(0,4); prk=px(r);
    K g=t(1,st(0xc0,48)); // _
    prk[0]=g; prk[1]=a; prk[2]=f; prk[3]=x;
    return st(0xd5,r);
  }

  Tx=tx; if(!Tx&&s(x)) { if(!VST(x)) { e=KERR_TYPE; goto cleanup; } Tx=15; }
  if(ta!=1) { e=KERR_TYPE; goto cleanup; }
  if(!ik(a)) { e=KERR_TYPE; goto cleanup; }
  if(Tx<=0&&!nx) { _k(f); _k(a); _k(x); return tn(0,0); }
  if(ta<=0&&!na) { _k(f); _k(a); _k(x); return tn(0,0); }
  if(Tx<0) { xm=kmix(x); EC(xm); }
  else xm=x;

  if(0xc0==s(f)) ff=ck(f)%32;

  n=av?strlen(av):0;
  if(Tx>0) {
    if(n) r=avdo(ff,k_(x),k_(x),av);
    else r=fe(k_(ff),k_(x),k_(x),av);
  }
  else {
    s=abs(ik(a));
    m=(nx-v+s)/s;
    if((m-1)*s+v!=n(xm)) { e=7; goto cleanup; }
    PRK(m);
    pxm=px(xm);
    if(n) {
      prk[0]=avdo(ff,k_(pxm[0]),k_(a),av); EC(prk[0]);
      i(nx-1, prk[i+1]=avdo(ff,k_(pxm[i+1]),k_(pxm[i]),av); EC(prk[i+1]))
    }
    else if(ik(a)<0) i(m, prk[i]=fe(k_(ff),k_(pxm[i*s+1]),k_(pxm[i*s]),av); EC(prk[i]))
    else if(ik(a)>0) i(m, prk[i]=fe(k_(ff),k_(pxm[i*s]),k_(pxm[i*s+1]),av); EC(prk[i]))
  }

  if(xm!=x) _k(xm);
  _k(f); _k(a); _k(x);
  return knorm(r);
cleanup:
  if(xm!=x) _k(xm);
  _k(f); _k(r); _k(a); _k(x);
  return e;
}

void kdump(i32 l) {
  i32 t,c,m=4,n,nn,refcount;
  u32 i;
  K v;
  //if(T(gs)) return;
  //if(n(gs)!=6) return;
  K *pgs=px(gs);
  K d=pgs[1];
  if(0x80!=s(d)) return;
  if(n(d)!=3) return;
  K keys=((K*)px(d))[0];
  K vals=((K*)px(d))[1];
  if(s(keys)||T(keys)!=-4) return;
  if(s(vals)||T(vals)!=0) return;
  if(n(keys)!=n(vals)) return;
  char **pkeys=px(keys);
  K *pvals=px(vals);
  char *s,*p;
  if(l) {
    i(n(keys),n=strlen(pkeys[i]);if(m<n)m=n);
    for(i=0;i<n(keys);i++) {
      c=nn=refcount=0;
      v=pvals[i];
      t=T(v);
      if(ISF(v)) t=7;
      if(s(v)==0x80) t=5;
      mreset();
      if(T(v)<=0&&!s(v)&&n(v)>100) { c=n(v); n(v)=100; }
      const char *s0=kprint_(v,"","","");
      if(T(v)<=0&&!s(v)&&c) n(v)=c;
      s=xstrdup(s0);
      s=xrealloc(s,strlen(s)+3);
      if(strlen(s)>30) { s[30]=0; snprintf(s+strlen(s),sizeof(s)-strlen(s),"..."); }
      if(t==5||t==0) i(strlen(s),if(s[i]=='\n')s[i]=';');
      if((p=strchr(s,'\n'))) { *p=0; snprintf(s+strlen(s),sizeof(s)-strlen(s),"..."); }
      if(t<=0&&!s(v)) nn=n(v);
      if(t<=0||t==2) refcount=((ko*)(b(48)&v))->r;
      if(t==6) printf("%-*s t:[%2d] c:[%10d] r:[%2d]\n",m,pkeys[i],t,nn,refcount);
      else printf("%-*s t:[%2d] c:[%10d] r:[%2d] %s\n",m,pkeys[i],t,nn,refcount,s);
      xfree(s);
    }
  }
  else {
    if(n(keys)) printf("%s",pkeys[0]);
    for(i=1;i<n(keys);i++) printf(" %s",pkeys[i]);
    printf("\n");
  }
}

u64 khashcb(K x) {
  u64 r=2654435761;
  K *pxk;
  switch(s(x)) {
  case 0x80: r=r+khash(b(48)&x); break;
  case 0x81: r=r+khash(x&(K)0xff00ffffffffffff); break;
  case 0x82: r=r+khash(x&(K)0xff00ffffffffffff); break;
  case 0x85: r=r+khash(x&(K)0xff00ffffffffffff); break;
  case 0xc0: r=r+(u64)ck(x)*2654435761U; break;
  case 0xc1: r=r+khash(x&(K)0xff00ffffffffffff); break;
  case 0xc2: r=r+khash(x&(K)0xff00ffffffffffff); break;
  case 0xc3: PXK; r=r+khash(pxk[0]); r^=r+khash(pxk[3]); break;
  case 0xc4: PXK; r=r+khashcb(pxk[0]); r^=r+khash(b(48)&pxk[1]); r^=r+khash(pxk[2]); break;
  case 0xc5: PXK; i(nx,r^=r+khash(pxk[i])) break;
  case 0xc6: r=r+khash(x&(K)0xff00ffffffffffff); break;
  case 0xc7: r=r+khash(x&(K)0xff00ffffffffffff); break;
  case 0xc8: PXK; r=r+khashcb(pxk[0]); r^=r+khash(pxk[1]); r^=r+khash(pxk[2]); break;
  case 0xcc: r=r+khash(x&(K)0xff00ffffffffffff); break;
  case 0xcd: r=r+khash(x&(K)0xff00ffffffffffff); break;
  case 0xce: r=r+khash(x&(K)0xff00ffffffffffff); break;
  case 0xcf: r=r+khash(x&(K)0xff00ffffffffffff); break;
  case 0xd0: r=r+khash(x&(K)0xff00ffffffffffff); break;
  case 0xd1: r=r+khash(x&(K)0xff00ffffffffffff); break;
  case 0xd2: r=r+khash(x&(K)0xff00ffffffffffff); break;
  case 0xd3: r=r+khash(x&(K)0xff00ffffffffffff); break;
  case 0xd4: r=r+khash(x&(K)0xff00ffffffffffff); break;
  case 0xd5: r=r+khash(x&(K)0xff00ffffffffffff); break;
  case 0xd6: r=r+khash(x&(K)0xff00ffffffffffff); break;
  case 0xd7: r=r+khash(x&(K)0xff00ffffffffffff); break;
  default:
    fprintf(stderr,"error: unsupported type in khashcb()\n");
    exit(1);
  }
  return r;
}

i32 kcmprcb(K a,K x) {
  i32 r=0,at=ta,xt=tx;
  static int d=0;
  if(++d>maxr) { --d; return KERR_STACK; }
  if(ISF(a)) at=7;
  else if(s(a)==0x80) at=5;
  if(ISF(x)) xt=7;
  else if(s(x)==0x80) xt=5;

  if(at<xt) r=-1;
  else if(at>xt) r=1;
  else if(at==5) r=dcmp(a,x);
  else {
    K p=fivecolon(0,a);
    K q=fivecolon(0,x);
    r=kcmpr(p,q);
    _k(p); _k(q);
  }
  --d;
  return r;
}

K kcpcb(K x) {
  K r=KERR_TYPE,p;
  static int d=0;
  if(++d>maxr) { --d; return KERR_STACK; }
  switch(s(x)) {
  case 0x80: r=dcp(x); break;
  case 0x81: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0x81); break;
  case 0x82: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0x82); break;
  case 0x83: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0x83); break;
  case 0x85: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0x85); break;
  case 0xc0: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xc0); break;
  case 0xc1: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xc1); break;
  case 0xc2: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xc2); break;
  case 0xc3: r=fncp(x); break;
  case 0xc4: r=fncp(x); break;
  case 0xc5: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xc5); break;
  case 0xc6: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xc6); break;
  case 0xc7: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xc7); break;
  case 0xc8: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xc8); break;
  case 0xcc: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xcc); break;
  case 0xcd: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xcd); break;
  case 0xce: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xce); break;
  case 0xcf: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xcf); break;
  case 0xd0: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xd0); break;
  case 0xd1: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xd1); break;
  case 0xd2: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xd2); break;
  case 0xd3: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xd3); break;
  case 0xd4: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xd4); break;
  case 0xd5: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xd5); break;
  case 0xd6: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xd6); break;
  case 0xd7: p=kcp(x&(K)0xff00ffffffffffff); if(E(p)) r=p; else r=set_sx(p,0xd7); break;
  }
  --d;
  return r;
}

void kdp(K x) {
  kprint(k_(x),"","\n","");
}
