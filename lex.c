#include "lex.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "fn.h"
#include "h.h"

#ifdef _WIN32
#define strtok_r strtok_s
#endif

static char *P=":+-*%&|<>=~.!@?#_^,$'/\\";
static char *p,*p0,*p1;
static int line;
static int64_t iii;  /* lex in 64-bit space, clamp to 32-bit at end */
static double fff;

static void push(pgs *s, int tt, K tv) {
  if(s->tc==s->tm) {
    s->tm<<=1;
    s->t=xrealloc(s->t,s->tm*sizeof(int));
    s->vm<<=1;
    s->v=xrealloc(s->v,s->vm*sizeof(pn*));
  }
  s->t[s->tc]=tt;
  s->v[s->tc++]=pnnewi(s,0,0,tv,2,0,p1-p0,line);
  s->lt=tt;
  s->ll=0xc3==s(tv);
}

static int gn_(void) {
  char c,*q=p;
  int s=0;
  while(1) {
    switch(s) {
    case 0:
      if(!*p) s=10;
      else if(*p=='0') s=1;
      else if(isdigit(*p)) s=4;
      else if(*p=='-') s=5;
      else if(*p=='.') s=7;
      else s=10;
      break;
    case 1:
      if(!*p) s=11;
      else if(*p=='n'||*p=='i') s=2;
      else if(*p=='N'||*p=='I') s=3;
      else if(*p=='e'||*p=='E') s=8;
      else if(*p=='.') s=6;
      else if(isdigit(*p)) s=4;
      else s=11;
      break;
    case 2: s=12; break;
    case 3: s=11; break;
    case 4:
      if(!*p) s=11;
      else if(isdigit(*p)) s=4;
      else if(*p=='e'||*p=='E') s=8;
      else if(*p=='.') s=6;
      else s=11;
      break;
    case 5:
      if(!*p) s=10;
      else if(*p=='0') s=1;
      else if(isdigit(*p)) s=4;
      else if(*p=='.') s=7;
      else s=10;
      break;
    case 6:
      if(!*p) s=12;
      else if(isdigit(*p)) s=9;
      else if(*p=='e'||*p=='E') s=8;
      else s=12;
      break;
    case 7: if(*p&&isdigit(*p)) s=9; else s=10; break;
    case 8: if(*p&&(isdigit(*p)||*p=='-'||*p=='+')) s=9; else s=10; break;
    case 9:
      if(!*p) s=12;
      else if(isdigit(*p)) s=9;
      else if(*p=='e'||*p=='E') s=8;
      else s=12;
      break;
    case 10: return 0; /* error */
    case 11: /* int */
      if(p==q) return 0;
      c=*--p; *p=0;
      /* handle special int literals 0N, 0I, -0N, -0I */
      if(!strcmp(q,"0N")||!strcmp(q,"-0N")) iii=INT32_MIN;
      else if(!strcmp(q,"0I")) iii=INT32_MAX;
      else if(!strcmp(q,"-0I")) iii=INT32_MIN+1;
      else iii=strtoll(q,NULL,10);  /* 64-bit, no clamp - defer to gn() */
      *p=c;
      return 1;
    case 12: /* float */
      if(p==q) return 0;
      c=*--p; *p=0;
      fff=xstrtod(q);
      *p=c;
      return 2;
    }
    ++p;
  }
  return 0;
}

#define S2(x) ((x)[0]&&(x)[1])
#define S3(x) ((x)[0]&&(x)[1]&&(x)[2])
#define S4(x) ((x)[0]&&(x)[1]&&(x)[2]&&(x)[3])
static int gn(pgs *pgs) {
  int r0,r,i;
  int64_t *iv=0;  /* collect in 64-bit space */
  int ic=0,fc=0,imm=1,fm=1;
  double *fv=0;
  char *q;
  r0=gn_();
  if(*p&&isblank(*p)) {
    q=p;
    while(*p&&isblank(*p))++p;
    if((*p&&isdigit(*p))||(S2(p)&&(*p=='.'||*p=='-')&&isdigit(p[1]))) { /* convert to vector */
      if(r0==1) { imm<<=1; iv=xrealloc(iv,imm*sizeof(int64_t)); iv[ic++]=iii; }
      else if(r0==2) { fm<<=1; fv=xrealloc(fv,fm*sizeof(double)); fv[fc++]=fff; }
    }
    p=q;
    if(iv) {
      while(*p&&isblank(*p)) {
        q=p;
        while(*p&&isblank(*p))++p;
        r=gn_();
        if(r==1) {
          if(ic==imm) { imm<<=1; iv=xrealloc(iv,imm*sizeof(int64_t)); }
          iv[ic++]=iii;
        }
        else if(r==2) {
          fm=imm;
          fv=xrealloc(fv,fm*sizeof(double));
          for(i=0;i<ic;i++) {
            /* preserve null/inf when promoting int64 to double */
            if(iv[i]==INT32_MIN) fv[fc++]=NAN;
            else if(iv[i]==INT32_MAX) fv[fc++]=INFINITY;
            else if(iv[i]==INT32_MIN+1) fv[fc++]=-INFINITY;
            else fv[fc++]=(double)iv[i];
          }
          xfree(iv); iv=0; ic=0;
          if(fc==fm) { fm<<=1; fv=xrealloc(fv,fm*sizeof(double)); }
          fv[fc++]=fff;
          break;
        }
        else { p=q; break; }
      }
    }
    if(fv) {
      while(*p&&isblank(*p)) {
        q=p;
        while(*p&&isblank(*p))++p;
        r=gn_();
        if(r==1) {
          /* preserve null/inf when promoting int64 to double */
          if(iii==INT32_MIN) fff=NAN;
          else if(iii==INT32_MAX) fff=INFINITY;
          else if(iii==INT32_MIN+1) fff=-INFINITY;
          else fff=(double)iii;
          if(fc==fm) { fm<<=1; fv=xrealloc(fv,fm*sizeof(double)); }
          fv[fc++]=fff;
        }
        else if(r==2) {
          if(fc==fm) { fm<<=1; fv=xrealloc(fv,fm*sizeof(double)); }
          fv[fc++]=fff;
        }
        else { p=q; break; }
      }
    }
  }
  if(ic) {
    K x=tn(1,ic);
    int *pi=px(x);
    /* clamp int64 to int32 here */
    for(i=0;i<ic;i++) {
      if(iv[i]>INT32_MAX) pi[i]=INT32_MAX;
      else if(iv[i]<INT32_MIN) pi[i]=INT32_MIN;
      else pi[i]=(int)iv[i];
    }
    push(pgs,T014,x);
  }
  else if(fc) {
    K x=tn(2,fc);
    double *pf=px(x);
    i(fc,pf[i]=fv[i])
    push(pgs,T014,x);
  }
  else if(r0==1) {
    /* scalar int - clamp here too */
    int v;
    if(iii>INT32_MAX) v=INT32_MAX;
    else if(iii<INT32_MIN) v=INT32_MIN;
    else v=(int)iii;
    push(pgs,T014,t(1,(u32)v));
  }
  else if(r0==2) push(pgs,T014,t2(fff));
  if(iv) xfree(iv);
  if(fv) xfree(fv);
  return r0;
}

static int reserved(pgs *pgs, char *p, char *q) {
  if(S3(p)&&!strcmp(p,"nul")) {
    push(pgs,T014,null);
    if((q=strpbrk(q,"'/\\"))) /* nul\0 0 */
      push(pgs,T015,t(-3,st(0x85,tnv(3,strlen(q),xmemdup(q,1+strlen(q))))));
    return 1;
  }
  if(p==R_DRAW) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_DOT) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_VS) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_SV) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_ATAN2) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_DIV) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_AND) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_OR) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_SHIFT) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_ROT) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_XOR) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_EUCLID) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_ENCRYPT) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_DECRYPT) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_SETENV) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_RENAME) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_IN) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_DVL) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_SQR) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_ABS) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_SLEEP) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_IC) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_CI) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_DJ) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_JD) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_LT) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_LOG) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_EXP) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_SQRT) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_FLOOR) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_CEIL) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_SIN) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_COS) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_TAN) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_ASIN) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_ACOS) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_ATAN) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_AT) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_SS) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_SM) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_LSQ) { push(pgs,T015,t(4,st(0xc7,q))); return 1; }
  if(p==R_SINH) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_COSH) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_TANH) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_ERF) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_ERFC) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_GAMMA) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_LGAMMA) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_RINT) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_TRUNC) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_NOT) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_KV) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_VK) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_VAL) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_BD) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_DB) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_HB) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_BH) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_ZB) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_BZ) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_MD5) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_SHA1) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_SHA2) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_GETENV) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_SVD) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_LU) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_QR) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_EXIT) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_DEL) { push(pgs,T015,t(4,st(0xc6,q))); return 1; }
  if(p==R_DO) { push(pgs,T015,t(4,st(0xd1,q))); return 1; }
  if(p==R_WHILE) { push(pgs,T015,t(4,st(0xd2,q))); return 1; }
  if(p==R_IF) { push(pgs,T015,t(4,st(0xd3,q))); return 1; }
  if(p==R_LIN) { push(pgs,T015,t(4,st(0xca,q))); return 1; }
  if(p==R_DV) { push(pgs,T015,t(4,st(0xca,q))); return 1; }
  if(p==R_DI) { push(pgs,T015,t(4,st(0xca,q))); return 1; }
  if(p==R_MUL) { push(pgs,T015,t(4,st(0xca,q))); return 1; }
  if(p==R_EP) { push(pgs,T015,t(4,st(0xca,q))); return 1; }
  if(p==R_CHOOSE) { push(pgs,T015,t(4,st(0xca,q))); return 1; }
  if(p==R_ROUND) { push(pgs,T015,t(4,st(0xca,q))); return 1; }
  if(p==R_GTIME) { push(pgs,T015,t(4,st(0xc9,q))); return 1; }
  if(p==R_LTIME) { push(pgs,T015,t(4,st(0xc9,q))); return 1; }
  if(p==R_TL) { push(pgs,T015,t(4,st(0xc9,q))); return 1; }
  if(p==R_INV) { push(pgs,T015,t(4,st(0xc9,q))); return 1; }
  if(p==R_SSR) { push(pgs,T015,t(4,st(0xcb,q))); return 1; }
  return 0;
}

extern K dir_(K x);
static int gname(pgs *pgs) {
  char c,*q=p,*q0="";
  int s=*p=='.';
  while(1) {
    if(!*p) break;
    if(!s) { if(isalpha(*p)) { ++p; s=1; } else break; }
    else { if(isalnum(*p)) ++p; else if(*p=='.') { ++p; s=0; } else break; }
  }
  if(*p) { c=*p; *p=0; q0=sp(q); *p=c; }
  while(*p&&strchr("'/\\",*p)) ++p;
  if(*p) { c=*p; *p=0; q=sp(q); *p=c; }
  else { q=sp(q); }
  if(pgs->locals) {
    K *plocals=px(pgs->locals);
    i(n(pgs->locals),if(q==sk(plocals[i])){push(pgs,T014,t(1,st(0x40,(u32)i)));return 1;})
  }
  if(!reserved(pgs,sp(q0),q)) {
    push(pgs,T014,t(4,st(0x40,q)));
  }
  return 1;
}

static char *ss;
static int gsym_(void) {
  char *q,c,s=0;
  if(*p!='`') return 0;
  q=++p;
  while(1) {
    switch(s) {
    case 0:
      if(!*p) return 0;
      if(*p=='"') { ++q; s=2; }
      else if(*p=='.') s=4;
      else if(isalpha(*p)) s=3;
      else s=1;
      break;
    case 1:
      if(p==q) return 0;
      c=*--p; *p=0;
      ss=xunesc(q);
      *p=c;
      return 1;
    case 2:
      if(!*p) return 0;
      if(*p=='\\') { ++p; if(!*p) return 0; }
      else if(*p=='"') {
        c=*p; *p=0;
        ss=xunesc(q);
        *p++=c;
        return 1;
      }
      break;
    case 3:
      if(!*p) return 0;
      if(*p=='.') s=4;
      else if(!isalnum(*p)) s=1;
      break;
    case 4:
      if(!*p) return 0;
      if(isalnum(*p)) s=3;
      else s=1;
      break;
    }
    ++p;
  }
  return 1;
}

static int gsym(pgs *pgs) {
  int r;
  char *q,**sv=0,**pus;
  size_t sc=0,sm=1;
  K u;
  r=gsym_();
  // scan past spaces to see if there's another symbol,
  // but the problem is its possible to scan right up to a /comment, which has to be
  // preceeded by a space. hence the q=p/p=q stuff.
  q=p;
  while(*p&&isblank(*p))++p;
  if(*p=='`') {
    if(r==1) {
      sm<<=1;
      sv=xrealloc(sv,sm*sizeof(char*));
      sv[sc++]=sp(ss);
      xfree(ss);
    }
    else return r;
    while(*p&&*p=='`') {
      q=p;
      r=gsym_();
      if(r==1) {
        if(sc==sm) { sm<<=1; sv=xrealloc(sv,sm*sizeof(char*)); }
        sv[sc++]=sp(ss);
        xfree(ss);
      }
      else { p=q; break; }
      q=p;
      while(*p&&isblank(*p))++p;
      if(*p!='`') { p=q; break; }
    }
  }
  else { p=q; }
  if(sv) {
    if(sc==1) push(pgs,T014,t(4,sv[0]));
    else {
      u=tn(4,sc);
      pus=px(u);
      i(sc,*pus++=sv[i])
      push(pgs,T014,u);
    }
  }
  else if(r==1) { push(pgs,T014,t(4,sp(ss))); xfree(ss); }
  if(sv) xfree(sv);
  return r;
}

static int gc(pgs *pgs) {
  char s=0;
  int j=0,n;
  u8 o=0;
  K a;
  char *pac;
  n=32;
  ss=xmalloc(n);
  ++p;
  while(1) {
    if(!*p) { xfree(ss); return 0; }
    if(j>=n-1) { n<<=1; ss=xrealloc(ss,n); }
    switch(s) {
    case 0:
      if(*p=='"') s=10;
      else if(*p=='\\') s=1;
      else ss[j++]=*p;
      break;
    case 1: /* escape */
      if(*p=='b') { ss[j++]='\b'; s=0; }
      else if(*p=='t') { ss[j++]='\t'; s=0; }
      else if(*p=='n') { ss[j++]='\n'; s=0; }
      else if(*p=='r') { ss[j++]='\r'; s=0; }
      else if(*p=='"') { ss[j++]='\"'; s=0; }
      else if(*p=='\\') { ss[j++]='\\'; s=0; }
      else if(isdigit(*p)&&*p<='7') { o=*p-'0'; s=2; } /* octal */
      else { ss[j++]=*p; s=0; }
      break;
    case 2: /* octal */
      if(isdigit(*p)&&*p<='7') { o*=8; o+=*p-'0'; s=3; }
      else if(*p=='\\') { ss[j++]=o; s=1; }
      else if(*p=='"') { ss[j++]=o; s=10; }
      else { ss[j++]=o; ss[j++]=*p; s=0; }
      break;
    case 3: /* octal */
      if(isdigit(*p)&&*p<='7') { o*=8; o+=*p-'0'; ss[j++]=o; s=0; }
      else if(*p=='\\') { ss[j++]=o; s=1; }
      else if(*p=='"') { ss[j++]=o; s=10; }
      else { ss[j++]=o; ss[j++]=*p; s=0; }
      break;
    case 10: /* accept */
      if(j==1) push(pgs,T014,t(3,(u8)ss[0]));
      else {
        a=tn(3,j);
        pac=px(a);
        i(na,*pac++=ss[i])
        push(pgs,T014,a);
      }
      xfree(ss);
      return 1;
    default: xfree(ss); return 0; /* error */
    }
    ++p;
    if(!*p) { xfree(ss); return 0; }
  }
  xfree(ss);
  return 1;
}

static int gf(pgs *pgs) {
  char *q,c,s=0,first=1;
  K f;
  int fc=0;
  int gline00=gline;
  int startline=line;
  gline=fileline;
  q=p;
  while(1) {
     /* skip comments */
    if(first && *p=='/') { ++p; while(*p&&*p!='\n') ++p; continue; }
    if(*p== ' ' && p[1] && p[1]=='/') { p+=2; while(*p&&*p!='\n') ++p; continue; }
    first=0;

    switch(s) {
    case 0:
      if(!*p) return 0;
      if(*p=='\n') { ++line; first=1; }
      if(*p=='"') { p=xeqs(p); --p; }
      else if(*p=='}'&&fc==1) s=1;
      else if(*p=='}') fc--;
      else if(*p=='{') fc++;
      break;
    case 1:
      if(!*p||!strchr("'/\\",*p)) { --p; s=2; }
      break;
    case 2: /* accept */
      c=*p; *p=0;
      /* TODO: make this better
         fnnew() > fnd() > pgparse() > lex()
         p,p0,p1,line get stepped on. backup/restore for now
      */
      char *p_=p;
      char *p0_=p0;
      char *p1_=p1;
      int line_=line;
      int fileline0=fileline;
      fileline+=startline;
      f=fnnew(q);
      p=p_; p0=p0_; p1=p1_; line=line_;
      fileline=fileline0;
      if(E(f)) return 0;
      gline=gline00;
      push(pgs,T014,f);
      *p=c;
      return 1;
    }
    ++p;
  }
  gline=gline00;
  return 1;
}

static int gback(pgs *pgs, int load) {
  char c,*q,*puc;
  K u;
  size_t n;
  if(*p=='\r') ++p;
  if(!opencode) return 0;
  if(S2(p)&&p[1]=='\\') {
    if(load==2) { /* \l file */
      push(pgs,T012,0);
      *p=0;
    }
    else {
      p+=2;
      push(pgs,T015,254);
      push(pgs,T014,null); /* zero */
      push(pgs,T013,0);
      *p=0;
    }
  }
  else if((S2(p)&&strchr("\r\n",p[1]))||(*p&&!p[1])) {
    if(load) {
      push(pgs,T012,0);
      if(*p) ++p;
      *p=0;
    }
    else {
      help(0);
      ++p;
    }
  }
  else if(S2(p)&&strchr("0+'_.:-`?",p[1])) {
    help(p[1]);
    p+=2;
  }
  else if(S3(p)&&p[1]=='t'&&p[2]==' ') {
    p+=2;
    push(pgs,T015,253);
    push(pgs,T014,t(1,1));
    push(pgs,T012,0);
  }
  else if(S3(p)&&p[1]=='l'&&p[2]&&strchr(" \n\t",p[2])) {
    push(pgs,T015,252);
    p+=2;
    if(*p) {
      while(*p&&isblank(*p))++p;
      q=p;
      while(*p&&*p!='\n')++p;
      if(*p) {
        c=*p; *p=0;
        if((n=strlen(q))) {
          u=tn(3,n);
          puc=px(u);
          i(n,*puc++=q[i])
          push(pgs,T014,u);
        }
        else push(pgs,T014,0);
        *p=c;
      }
    }
  }
  else if(S3(p)&&p[1]=='p'&&p[2]&&strchr(" \n\t",p[2])) {
    push(pgs,T015,251);
    p+=2;
    if(*p) {
      while(*p&&isblank(*p))++p;
      q=p;
      if(*p&&isdigit(*p)) {
        while(*p&&isdigit(*p))++p;
        if(*p) {
          c=*p; *p=0;
          int prec=xatoi(q);
          if(!prec)prec=17;
          push(pgs,T014,t(1,(u32)prec));
          *p=c;
        }
      }
      else push(pgs,T014,null);
    }
  }
  else if(S3(p)&&p[1]=='v'&&p[2]=='\n') {
    p+=2;
    push(pgs,T015,250);
    push(pgs,T014,t(1,1));
    push(pgs,T012,0);
  }
  else if(S3(p)&&p[1]=='V'&&p[2]=='\n') {
    p+=2;
    push(pgs,T015,249);
    push(pgs,T014,t(1,1));
    push(pgs,T012,0);
  }
  else if(S4(p)&&p[1]=='z'&&p[2]=='c'&&p[3]&&strchr(" \n\t",p[3])) {
    push(pgs,T015,248);
    p+=3;
    if(*p) {
      while(*p&&isblank(*p))++p;
      q=p;
      if(*p&&isdigit(*p)) {
        while(*p&&isdigit(*p))++p;
        if(*p) {
          c=*p; *p=0;
          int zc=xatoi(q);
          push(pgs,T014,t(1,(u32)zc));
          *p=c;
        }
      }
      else push(pgs,T014,null);
    }
  }
  else if(S3(p)&&p[1]=='e'&&p[2]&&strchr(" \n\t",p[2])) {
    push(pgs,T015,247);
    p+=2;
    if(*p) {
      while(*p&&isblank(*p))++p;
      q=p;
      if(*p&&isdigit(*p)) {
        while(*p&&isdigit(*p))++p;
        if(*p) {
          c=*p; *p=0;
          int e=xatoi(q);
          if(e) e=1;
          push(pgs,T014,t(1,(u32)e));
          *p=c;
        }
      }
      else push(pgs,T014,null);
    }
  }
  else if(S3(p)&&p[1]=='d'&&p[2]&&strchr(" \n\t",p[2])) {
    push(pgs,T015,246);
    p+=2;
    if(*p) {
      while(*p&&isblank(*p))++p;
      q=p;
      while(*p&&*p!='\n')++p;
      if(*p) {
        c=*p; *p=0;
        if(!scope_vktp(q)&&strcmp(q,"^")) return 0;
        u=strlen(q)?t(4,sp(q)):null;
        push(pgs,T014,u);
        *p=c;
      }
    }
  }
  else return 0;
  return 1;
}

static void getmv(pgs *pgs, int t, K st) {
  K s;
  char *mv,*ps; int i=0; int m=4;
  mv=xmalloc(m);
  while(*p&&!strchr("'/\\",*p)) mv[i++]=*p++;
  while(*p&&strchr("'/\\",*p)) {
    if(i==m-1) { m<<=1; mv=xrealloc(mv,m); }
    mv[i++]=*p++;
  }
  mv[i]=0;
  s=tn(3,strlen(mv));
  ps=px(s);
  i(n(s),ps[i]=mv[i]);
  if(t==-3) push(pgs,T015,t(t,st(st,s)));
  else if(t==4) { push(pgs,T015,t(t,st(st,sp(ps)))); _k(s); }
  xfree(mv);
}

K lex(pgs *pgs, int load) {
  int f=1,b,s=0,first=1;
  int pcount=0,scount=0,ccount=0,qcount=0;
  char *q=pgs->p;
  char *q0=q;
  char *q1=q;
  char *tmp;
  line=0;
  while(*q) {
    switch(s) {
    case 0:
      /* skip comments */
      if(first && *q=='/') { ++q; while(*q&&*q!='\n') ++q; continue; }
      if(*q== ' ' && q[1] && q[1]=='/') { q+=2; while(*q&&*q!='\n') ++q; continue; }
      first=0;
      if(*q=='\n') {
        first=1;
        if(q[1]) { ++line; q1=q; }
        if(q[1]=='\\' && q[2]=='\n') q[1]=0; /* fold, ignore after \ */
        if(!pcount&&!scount&&!ccount&&!qcount) { q0=q+1; line=0; }
      }
      switch(*q) {
      case '(': ++pcount; break;
      case ')': --pcount; break;
      case '[': ++scount; break;
      case ']': --scount; break;
      case '{': ++ccount; break;
      case '}': --ccount; break;
      case '"': qcount=s=1; break;
      } break;
    case 1:
      switch(*q) {
      case '\\': s=2; break;
      case '"': qcount=s=0; break;
      } break;
    case 2: s=1; break;
    }
    if(pcount<0||scount<0||ccount<0) {
      gline=fileline;
      gline0=line;
      glinei=q-q0; if(glinei<0) glinei=0;
      gline0i=q-q1;
      if(*q1=='\n'||*q1=='\r') gline0i--;
      if(gline0i<0) gline0i=0;
      if(glinep) xfree(glinep);
      glinep=strtok_r(q1,"\n",&tmp); if(glinep) glinep=xstrdup(glinep);
      gline0p=strtok_r(q0,"\n",&tmp); if(gline0p) gline0p=xstrdup(gline0p);
      return kerror("unmatched error");
    }
    ++q;
  }
  if(pcount||scount||ccount||qcount) {
    gline=fileline;
    gline0=line;
    glinei=q-q0; if(glinei<0) glinei=0;
    gline0i=q-q1-2; if(gline0i<0) glinei=0;
    if(glinep) xfree(glinep);
    glinep=strtok_r(q1,"\n",&tmp); if(glinep) glinep=xstrdup(glinep);
    gline0p=strtok_r(q0,"\n",&tmp); if(gline0p) gline0p=xstrdup(gline0p);
    return kerror("open error");
  }

  line=0;
  p0=p1=p=pgs->p;
  while(1) {
    if(!*p) { push(pgs,T020,0); break; }
    b=isblank(*p);
    while(*p&&isblank(*p)) ++p;
    p1=p;
    if(*p=='\r') ++p;
    else if(*p=='\n') { ++p; if(f) push(pgs,T014,null); push(pgs,T013,0); f=1; ++line; p0=p1+1; continue; }
    else if(*p==';') { ++p; if(f) push(pgs,T014,0); push(pgs,T012,0); f=1; continue; }
    else if((b||f)&&*p=='/') { ++p; while(*p&&*p!='\n')++p; }
    else if(S2(p)&&isdigit(*p)&&p[1]==':') {
      if(S3(p)&&strchr("'/\\",p[2])) getmv(pgs,4,0xcc);
      else { char s[3]={*p,':',0}; p+=2; push(pgs,T015,t(4,st(0xcc,sp(s)))); }
    }
    else if(S2(p)&&!strncmp("::",p,2)) { p+=2; push(pgs,T015,t(4,st(0x82,sp("::")))); }
    else if(*p=='-') {
      if(S2(p)&&strchr("'/\\",p[1])) getmv(pgs,-3,0xc1);
      else if(S2(p)&&p[1]==':') { push(pgs,T015,t(1,st(0xce,*p))); p+=2; }
      else if(!(b|f)&&(pgs->lt==T014||pgs->lt==T017||pgs->lt==T019)&&!pgs->ll) { ++p; push(pgs,T015,'-'); }
      else if((S2(p)&&isdigit(p[1])) || (S3(p)&&p[1]=='.'&&isdigit(p[2]))) { if(!gn(pgs)) goto parseerror; }
      else { ++p; push(pgs,T015,'-'); }
    }
    else if(f&&*p=='\\') {
      if(!gback(pgs,load)) {
        if(opencode) {
#ifdef FUZZING
          if(system("echo fuzz_sandbox_stubbed")) return 0;
          while(*p&&*p!='\n')++p;
#else
          if(system(p+1)) return 0;
#endif
        }
        else { /* trace in lambda */
          push(pgs,T015,t(4,st(0xd8,sp("\\"))));
          ++p;
          continue;
        }
        while(*p&&*p!='\n')++p;
      }
    }
    else if(b&&*p=='\\') { /* trace in opencode 1 + \2 3 */
      push(pgs,T015,t(4,st(0xd8,sp("\\"))));
      ++p;
      continue;
    }
    else if((*p&&isdigit(*p))||(S2(p)&&*p=='.'&&isdigit(p[1]))) { if(!gn(pgs)) goto parseerror; }
    else if(*p&&strchr("'/\\",*p)) getmv(pgs,-3,0x85); /* ??? */
    else if(*p&&strchr(P,*p)) {
      if(S2(p)&&strchr("'/\\",p[1])) getmv(pgs,-3,0xc1);
      else if(S2(p)&&p[1]==':') { push(pgs,T015,t(1,st(0xce,*p))); p+=2; }
      else if(S2(p)&&*p=='.'&&isalpha(p[1])) gname(pgs);
      else { push(pgs,T015,*p); ++p; } }
    else if(*p=='(') { ++p; push(pgs,T016,0); }
    else if(*p==')') { ++p; push(pgs,T017,0); }
    else if(*p=='[') { ++p; push(pgs,T018,0); }
    else if(*p==']') { ++p; push(pgs,T019,0); if(*p&&strchr("'/\\",*p)) getmv(pgs,-3,0x85); }
    else if(*p=='{') { if(!gf(pgs)) goto parseerror; }
    else if(*p=='`') { if(!gsym(pgs)) goto parseerror; }
    else if(*p=='"') gc(pgs);
    else if(isalpha(*p)) gname(pgs);
    else goto parseerror;
    f=0;
  }
  return 0;
parseerror:
  glinei=p-p0; if(glinei<0) glinei=0;
  if(glinep) xfree(glinep);
  glinep=strtok_r(p1,"\n",&tmp); if(glinep) glinep=xstrdup(glinep);
  return kerror("parse");
}
