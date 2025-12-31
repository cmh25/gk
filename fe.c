#include "fe.h"
#include <stdio.h>
#include <string.h>
#include "k.h"
#include "av.h"
#include "b.h"
#include "fn.h"

static char avb[256];
static K stripav(K x, char **av) {
  char a2[256];
  u8 t=tx;
  u8 st=s(x);
  **av=0;
  if(strlen(sk(x))>255) return KERR_LENGTH;
  snprintf(a2,sizeof(a2),"%s",sk(x));
  char *p=strpbrk(a2,"'/\\");
  if(p) { snprintf(*av,256,"%s",p); *p=0; }
  return t(t,st(st,sp(a2)));
}

K fc(K f, K a, K x, char *av) {
  K r=0,e,p,f2,*pf,*pf2;
  if(av&&*av) r=avdo(k_(f),k_(a),k_(x),av);
  else {
    pf=px(f);
    f2=pf[0];
    pf2=px(f2);
    i32 i=n(f2)-1;
    r=fe(k_(pf2[i]),k_(a),k_(x),av);
    EC(r);
    for(i--;i>=0;--i) {
      p=fe(k_(pf2[i]),0,r,av);
      EC(p);
      r=p;
    }
  }
  _k(a); _k(x);
  return r;
cleanup:
  _k(a); _k(x);
  return e;
}

K fe(K f, K a, K x, char *av) {
  K r=3,f0,f1,*pf,*pf1,*pr,t,*pt,xx,*pxx,fav;
  char *P=":+-*%&|<>=~.!@?#_^,$'/\\";
  char ff=f;
  char *mv,*s;

  if(0x45==s(a)||0x45==s(x)) { _k(f); _k(a); _k(x); return KERR_TYPE; }

  if(0xd0==s(f)&&(!av||!*av)) {
    if(a) { _k(f); _k(a); _k(x); return KERR_VALENCE; }
    pf=px(f);
    a=k_(pf[0]);
    K f2=k_(pf[1]);
    _k(f); f=f2;
    if(0x81==s(x)) {
      if(nx!=1) { _k(f); _k(a); _k(x); return KERR_VALENCE; }
      K *px=px(x);
      K x2=k_(px[0]);
      _k(x); x=x2;
    }
  }

  if(0xc0==s(f)) ff=ck(f)%32;
  else if(!s(f)&&f>32) ff=strchr(P,ck(f))-P; /* r19 primitive verbs are literal '+' '-' etc. */

  if(!a) {
    switch(s(f)) {
    case 0xc1: case 0xc2:
      mv=px(f);
      s=strchr(P,*mv);
      if(*++mv && s-P<=20) r=avdo(s-P,a,x,mv);
      else { _k(a); _k(x); r=KERR_PARSE; }
      break;
    case 0xc3: case 0xc4:
      if(0x81==s(x)) r=fne(k_(f),x,av);
      else {
        xx=tn(0,1); pxx=px(xx);
        pxx[0]=x;
        r=fne(k_(f),xx,av);
      }
      break;
    case 0xc6:
      if(0x81==s(x)&&n(x)==1) { K *px=px(x); r=builtin(f,0,k_(px[0])); _k(x); }
      else if(0x81==s(x)) { _k(x); r=KERR_VALENCE; }
      else r=builtin(f,0,x);
      break;
    case 0xc7: case 0xc8:
      if(0x81==s(x)&&n(x)==1) { K *px=px(x); r=builtin(f,0,k_(px[0])); _k(x); }
      else if(0x81==s(x)&&n(x)==2)  { K *px=px(x); r=builtin(f,k_(px[0]),k_(px[1])); _k(x); }
      else if(0x81==s(x)) { _k(x); r=KERR_VALENCE; }
      else r=builtin(f,0,x);
      break;
    case 0xcc:
      r=builtin(f,0,x);
      break;
    case 0xc5:
      r=fc(f,0,x,av);
      break;
    case 0xd4:
      pf=px(f);
      f1=pf[1]; pf1=px(f1);
      if(pf1[0]==inull && pf1[1]==inull) {
        if(0x81==s(x)&&n(x)==1) {
          r=tn(0,2); pr=px(r);
          pr[0]=kcp(f); if(E(pr[0])) { r=pr[0]; _k(r); _k(x); break; }
          pr[1]=kcp(x); if(E(pr[1])) { r=pr[1]; _k(r); _k(x); break; }
          r=st(0xd9,r);
          _k(x);
        }
        else if(0x81==s(x)) { _k(x); r=KERR_VALENCE; }
        else {
          r=tn(0,2); pr=px(r);
          t=tn(0,1); pt=px(t);
          pt[0]=kcp(x); if(E(pt[0])) { r=pt[0]; _k(t); _k(r); _k(x); break; }
          pr[0]=kcp(f); if(E(pr[0])) { r=pr[0]; _k(t); _k(r); _k(x); break; }
          pr[1]=st(0x81,t);
          r=st(0xd9,r);
          _k(x);
        }
        break;
      }
      switch(s(pf[0])) {
      case 0xc5:
        if(pf1[0]==inull) {
          if(0x81==s(x)&&n(x)==1) { K *px=px(x); r=fc(pf[0],k_(px[0]),k_(pf1[1]),av); _k(x); }
          else if(0x81==s(x)) { _k(x); r=KERR_VALENCE; }
          else r=fc(pf[0],x,k_(pf1[1]),av);
        }
        else if(pf1[1]==inull) {
          if(0x81==s(x)&&n(x)==1) { K *px=px(x); r=fc(pf[0],k_(pf1[0]),k_(px[0]),av); _k(x); }
          else if(0x81==s(x)) { _k(x); r=KERR_VALENCE; }
          else r=fc(pf[0],k_(pf1[0]),x,av);
        }
        else { _k(f); _k(x); return KERR_VALENCE; }
        break;
      case 0xc7:
        if(pf1[0]==inull) {
          if(0x81==s(x)&&n(x)==1) { K *px=px(x); r=builtin(pf[0],k_(px[0]),k_(pf1[1])); _k(x); }
          else if(0x81==s(x)) { _k(x); r=KERR_VALENCE; }
          else r=builtin(pf[0],x,k_(pf1[1]));
        }
        else if(pf1[1]==inull) {
          if(0x81==s(x)&&n(x)==1) { K *px=px(x); r=builtin(pf[0],k_(pf1[0]),k_(px[0])); _k(x); }
          else if(0x81==s(x)) { _k(x); r=KERR_VALENCE; }
          else r=builtin(pf[0],k_(pf1[0]),x);
        }
        else { _k(f); _k(x); return KERR_VALENCE; }
        break;
      default:
        ff=ck(pf[0])%32;
        if(pf1[0]==inull) {
          if(0x81==s(x)&&n(x)==1) { K *px=px(x); r=k(ff,k_(px[0]),k_(pf1[1])); _k(x); }
          else if(0x81==s(x)) { _k(x); r=KERR_VALENCE; }
          else r=k(ff,x,k_(pf1[1]));
        }
        else if(pf1[1]==inull) {
          if(0x81==s(x)&&n(x)==1) { K *px=px(x); r=k(ff,k_(pf1[0]),k_(px[0])); _k(x); }
          else if(0x81==s(x)) { _k(x); r=KERR_VALENCE; }
          else r=k(ff,k_(pf1[0]),x);
        }
        else { _k(f); _k(x); return KERR_VALENCE; }
      }
      break;
    case 0xd0:
      r=avdo(k_(f),0,x,av);
      break;
    case 0xd7:
      pf=px(f);
      f0=k_(pf[0]);
      f1=k_(pf[1]);
      pf1=px(f1);
      if(0xc3==s(f1)) fav=pf1[3];
      else if(0xc4==s(f1)) fav=pf1[2];
      else { _k(x); r=KERR_PARSE; break; }
      char *favp=-3==T(fav)?(char*)px(fav):"";
      if((!strcmp(favp,"/") || !strcmp(favp,"\\"))) {
        K f1_=kcp(f1); _k(f1); f1=f1_;
        if(E(f1)) { _k(f0); _k(x); r=f1; break; }
        K *pf1=px(f1);
        if(0xc3==s(f1)) { _k(pf1[3]); pf1[3]=null; }
        else if(0xc4==s(f1)) { _k(pf1[2]); pf1[2]=null; }
        if(s(f0)==0 && T(f0)==1 && !strcmp(favp,"/")) { r=overmonadn(f1,f0,x,""); quiet=0; }
        else if(s(f0)==0xc3 && !strcmp(favp,"/")) { r=overmonadb(f1,f0,x,""); quiet=0; }
        else if(s(f0)==0 && T(f0)==1 && !strcmp(favp,"\\")) { r=scanmonadn(f1,f0,x,""); quiet=0; }
        else if(s(f0)==0xc3 && !strcmp(favp,"\\")) { r=scanmonadb(f1,f0,x,""); quiet=0; }
        else { _k(f0); _k(f1); _k(x); r=KERR_TYPE; break; }
      }
      break;
    case 0xd5:;
      K f2=kcp(f); if(E(f2)) { _k(f); _k(x); return f2; }
      _k(f); f=f2;
      pf=px(f);
      if(0x81==s(x)) {
        _k(x);
        r=KERR_TYPE; // TODO
      }
      else {
        if(pf[1]==inull) pf[1]=x;
        else if(pf[2]==inull) pf[2]=x;
        else if(pf[3]==inull) pf[3]=x;
        if(ck(pf[0])%32==13) r=kamendi3(k_(pf[1]),k_(pf[2]),k_(pf[3]));
        else if(ck(pf[0])%32==11) r=kamend3(k_(pf[1]),k_(pf[2]),k_(pf[3]));
        else if(ck(pf[0])%32==16) r=kslide(k_(pf[2]),k_(pf[1]),k_(pf[3]),"");
        else { _k(x); r=KERR_TYPE; }
      }
      break;
    case 0xd6:
      f2=kcp(f); if(E(f2)) { _k(f); _k(x); return f2; }
      _k(f); f=f2;
      pf=px(f);
      if(0x81==s(x)) {
        _k(x);
        r=KERR_TYPE; // TODO
      }
      else {
        if(pf[1]==inull) pf[1]=x;
        else if(pf[2]==inull) pf[2]=x;
        else if(pf[3]==inull) pf[3]=x;
        else if(pf[4]==inull) pf[4]=x;
        if(ck(pf[0])%32==13) r=kamendi4(k_(pf[1]),k_(pf[2]),k_(pf[3]),k_(pf[4]));
        else if(ck(pf[0])%32==11) r=kamend4(k_(pf[1]),k_(pf[2]),k_(pf[3]),k_(pf[4]));
        else { _k(x); r=KERR_TYPE; }
      }
      break;
    default:
      if(!ff) {
        if(0x81==s(x)) { _k(f); _k(x); return KERR_TYPE; }
        else r=x;
      }
      else if(ff>0&&ff<32) {
        if(0x81==s(x)) {
          K *px=px(x);
          if(nx==1) r=k(ff,0,k_(px[0]));
          else if(nx==2) r=k(ff,k_(px[0]),k_(px[1]));
          else if(nx==3&&ff==13) r=kamendi3(k_(px[0]),k_(px[1]),k_(px[2]));
          else if(nx==4&&ff==13) r=kamendi4(k_(px[0]),k_(px[1]),k_(px[2]),k_(px[3]));
          else if(nx==3&&ff==11) r=kamend3(k_(px[0]),k_(px[1]),k_(px[2]));
          else if(nx==4&&ff==11) r=kamend4(k_(px[0]),k_(px[1]),k_(px[2]),k_(px[3]));
          else if(nx==3&&ff==16) r=kslide(k_(px[1]),k_(px[0]),k_(px[2]),"");
          else r=KERR_VALENCE;
          _k(x);
        }
        else if(av && *av) {
          int n=strlen(av);
          if(n>32) { _k(a); _k(x); return KERR_LENGTH; }
          if(n==1) {
            int avf=strchr(P,*av)-P;
            r=k(avf,ff,x);
          }
          else {
            char av2[256];
            snprintf(av2,sizeof(av2),"%s",av);
            av2[n-1]=0;
            r=avdo(ff,0,x,av2);
          }
        }
        else r=k(ff,0,x);
      }
      else _k(x);
    }
  }
  else {
    switch(s(f)) {
    case 0xc1: case 0xc2:
      if(a==inull || x==inull) {
        r=tn(0,2); pr=px(r);
        t=tn(0,2); pt=px(t);
        pt[0]=a;
        pt[1]=x;
        pr[0]=k_(f);
        pr[1]=st(0x81,t);
        r=st(0xd9,r);
      }
      else {
        mv=px(f);
        s=strchr(P,*mv);
        if(*++mv && s-P<=20) r=avdo(s-P,a,x,mv);
        else { _k(a); _k(x); r=KERR_PARSE; }
      }
      break;
    case 0xc3: case 0xc4:
      xx=tn(0,2); pxx=px(xx);
      pxx[0]=a; pxx[1]=x;
      r=fne(k_(f),xx,av);
      break;
    case 0xc7: case 0xcd:
      if(a==inull || x==inull) {
        r=tn(0,2); pr=px(r);
        t=tn(0,2); pt=px(t);
        pt[0]=a;
        pt[1]=x;
        pr[0]=f;
        pr[1]=st(0x81,t);
        r=st(0xd4,r);
      }
      else r=builtin(f,a,x);
      break;
    case 0xc5:
      if(a==inull || x==inull) {
        r=tn(0,2); pr=px(r);
        t=tn(0,2); pt=px(t);
        pt[0]=a;
        pt[1]=x;
        pr[0]=k_(f);
        pr[1]=st(0x81,t);
        r=st(0xd4,r);
      }
      else r=fc(f,a,x,av);
      break;
    case 0xc6: { /* 5 sin\2 */
      char *av=avb;
      K f2=stripav(f,&av);
      if(s(a)==0 && T(a)==1 && !strcmp(av,"/")) r=overmonadn(f2,a,x,"");
      else if(s(a)==0xc3 && !strcmp(av,"/")) r=overmonadb(f2,a,x,"");
      else if(s(a)==0 && T(a)==1 && !strcmp(av,"\\")) r=scanmonadn(f2,a,x,"");
      else if(s(a)==0xc3 && !strcmp(av,"\\")) r=scanmonadb(f2,a,x,"");
      else { _k(a); _k(x); r=KERR_TYPE; }
      break;
      }
    default:
      if(!ff) { _k(a); r=x; }
      else if(ff>0&&ff<32) {
        if(a==inull || x==inull) {
          r=tn(0,2); pr=px(r);
          t=tn(0,2); pt=px(t);
          pt[0]=a;
          pt[1]=x;
          pr[0]=t(1,st(0xc0,(u32)ff+32));
          pr[1]=st(0x81,t);
          r=st(0xd4,r);
        }
        else r=k(ff,a,x);
      }
      else { _k(a); _k(x); }
    }
  }
  _k(f);
  return r;
}
