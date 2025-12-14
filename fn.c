#include "fn.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "p.h"
#include "av.h"

K fnestack[1024];
int fnestacki=-1;
static char* spf;

void fninit(void) {
  K f;
  f=fnnew("{x in\\y}"); dset(C,sp("lin"),f); fnfree(f);
  f=fnnew("{x dvl,y}"); dset(C,sp("dv"),f); fnfree(f);
  f=fnnew("{$[@x;. di[. x;&|/(!x)=/$[@y;,y;y]];x@&@[(#x)#1;y;:;0]]}"); dset(C,sp("di"),f); fnfree(f);
  f=fnnew("{(dj _ x%86400;100 sv 24 60 60 vs x!86400)}"); dset(C,sp("gtime"),f); fnfree(f);
  f=fnnew("{gtime lt x}"); dset(C,sp("ltime"),f); fnfree(f);
  f=fnnew("{t+x-lt t:x+x-lt x}"); dset(C,sp("tl"),f); fnfree(f);
  f=fnnew("{x dot\\y}"); dset(C,sp("mul"),f); fnfree(f);
  f=fnnew("{((2##*x)#1,&#*x) lsq x}"); dset(C,sp("inv"),f); fnfree(f);
  f=fnnew("{rint exp lgamma[1+x]-lgamma[1+y]+lgamma[1+x-y]}"); dset(C,sp("choose"),f); fnfree(f);
  f=fnnew("{x*rint y%x}"); dset(C,sp("round"),f); fnfree(f);
  f=fnnew("{if[nul~x;:nul];i:1+2*!_.5*#x:(0,,/(0,+/~+\\ep[<;0,\"[\"=y]-ep[<;(\"]\"=y$:)],0)+/x ss $[3=4:y;$y;y])_ x;,/$[7=4:z;@[x;i;z];4:z$:;@[x;i;:;(#i)#,z];@[x;i;:;z]]}"); dset(C,sp("ssr"),f); fnfree(f);
  f=fnnew("{_[-1;x;y]}"); dset(C,sp("ep"),f); fnfree(f);
  spf=sp("f");
}

K fnnew(char *s) {
  K f,*pf;
  char *pd;
  if(!s||*s!='{') return null;
  f=tn(0,5);
  pf=px(f);
  pf[0]=tn(3,strlen(s)); /* definition */
  pf[1]=null;            /* parse result */
  pf[2]=null;            /* scope */
  pf[3]=null;            /* av */
  pf[4]=t(1,0);          /* valence */
  pd=px(pf[0]);
  i(strlen(s),pd[i]=s[i])
  K p=fnd(f);
  if(p) { _k(f); return p; }
  return st(0xc3,f);
}

void fnfree(K f) {
  _k(f);
}

static K fncp_(K x) {
  K p,e;
  K f=tn(0,5);
  K *pf=px(f);
  K *px=px(x);
  pf[0]=kcp(px[0]); /* definition */
  if(E(pf[0])) {  e=pf[0]; _k(f); return e; }
  pf[1]=null;       /* parse result */
  pf[2]=null;       /* scope */
  pf[3]=kcp(px[3]); /* av */
  pf[4]=t(1,0);     /* valence */
  if((p=fnd(f))) { fnfree(f); return p; }
  return st(0xc3,f);
}
static K fncpproj_(K x) {
  K f=tn(0,3),e;
  K *pf=px(f);
  K *px=px(x);
  pf[0]=kcp(px[0]); /* lambda */
  if(E(pf[0])) {  e=pf[0]; _k(f); return e; }
  pf[1]=kcp(px[1]); /* pargs */
  if(E(pf[1])) {  e=pf[1]; _k(f); return e; }
  pf[2]=kcp(px[2]); /* av */
  if(E(pf[1])) {  e=pf[1]; _k(f); return e; }
  return st(0xc4,f);
}
K fncp(K x) {
  if(0xc3==s(x)) return fncp_(x);
  if(0xc4==s(x)) return fncpproj_(x);
  return KERR_TYPE;
}

#define SB(b,m,l,c) do { \
  if((l)==(m)) { (m)<<=1; (b)=xrealloc((b),(m)*sizeof(*(b))); } \
  (b)[(l)]=(c); \
} while (0)

K fnd(K f) {
  K p,r=0,*pf;
  char *ff=0,*b,**v,*g,*h;
  int i,j,s,n,q,vx,vy,vz,ffq=0,first=1,params=1;
  int bm=32,vm=32;
  pf=px(f);
  ff=px(pf[0]);
  s=j=n=q=vx=vy=vz=0;
  if(!*ff) return r;
  if(*ff!='{') return r;
  b=xcalloc(bm,1); v=xcalloc(vm*sizeof(char*),1);
  for(;*ff;++ff) {

    /* skip comments */
    if(s!=2 && first && *ff=='/') { ++ff; while(*ff&&*ff!='\n') ++ff; --ff; continue; }
    if(s!=2 && *ff== ' ' && ff[1] && ff[1]=='/') { ff+=2; while(*ff&&*ff!='\n') ++ff; --ff; continue; }
    first=0; if(*ff=='\n') first=1;

    if(*ff=='"') { params=0; while(*ff&&*ff=='"') ff=xeqs(ff); }
    if(!*ff) { r=KERR_PARSE; goto cleanup; }
    switch(s) {
    case 0:
      if(*ff=='{') s=1;
      else { r=KERR_PARSE; goto cleanup; }
      break;
    case 1:
      while(*ff&&isblank(*ff))++ff;
      if(*ff=='"') while(*ff&&*ff=='"') ff=xeqs(ff);
      if(!*ff) { r=KERR_PARSE; goto cleanup; }
      if(*ff=='}') s=2;
      else if(*ff=='{') { n++; s=9; }
      else if(*ff=='['&&params) s=3;
      else if(*ff=='.') s=11;
      else if(*ff=='`') s=101;
      else if(isalpha(*ff)) { s=7; SB(b,bm,j,*ff); j++; }
      else if(*ff<0) { r=KERR_PARSE; goto cleanup; }
      else s=6;
      params=0;
      break;
    case 101:
      if(isalnum(*ff)) s=101;
      else s=1;
      break;
    case 2:
      if(*ff&&strchr(" '/\\",*ff)) {
        char avb[256];
        i=0;
        while(*ff&&strchr("'/\\",*ff)) {
          avb[i++]=*ff++;
          if(i==256) { r=KERR_LENGTH; goto cleanup; }
        }
        avb[i]=0;
        pf[3]=tnv(3,i,xmemdup(avb,1+strlen(avb)));
        ff-=i; *ff=0; ff--;
        break;
      }
      else { r=KERR_PARSE; goto cleanup; }
    case 3:
      if(isalpha(*ff)) { s=4; SB(b,bm,j,*ff); j++; ffq=1; }
      else if(*ff==']') s=5;
      else { r=KERR_PARSE; goto cleanup; }
      break;
    case 4:
      if(isalnum(*ff)) { s=4; SB(b,bm,j,*ff); j++; }
      else if(*ff==']') {
        s=5; SB(b,bm,j,0); j=0; SB(v,vm,q,sp(b)); q++;
      }
      else if(*ff==';') {
        s=3; SB(b,bm,j,0); j=0; SB(v,vm,q,sp(b)); q++;
      }
      break;
    case 5:
      if(*ff=='}') s=2;
      else if(*ff=='{') { n++; s=8; }
      else if(*ff=='.') s=15;
      else if(*ff=='`') s=105;
      else if(isalpha(*ff)) { s=7; SB(b,bm,j,*ff); j++; }
      else s=6;
      break;
    case 105:
      if(isalnum(*ff)) s=105;
      else s=5;
      break;
    case 6:
      if(*ff=='}') s=2;
      else if(*ff=='.') s=16;
      else if(*ff=='`') s=106;
      else if(isalpha(*ff)) { s=7; SB(b,bm,j,*ff); j++; }
      else if(*ff=='{') { n++; s=8; }
      else if(*ff<0) { r=10; goto cleanup; }
      else s=6;
      break;
    case 106:
      if(isalnum(*ff)) s=106;
      else if(*ff=='`') s=106;
      else if(*ff=='}') s=2;
      else s=6;
      break;
    case 7:
      if(*ff=='}') {
        s=2; SB(b,bm,j,0); j=0;
        if(!ffq) { /* no implicit xyz if there are formal parameters */
          if(!vx && !strcmp(b,"x")) { vx=1; SB(v,vm,q,sp(b)); q++; }
          if(!vy && !strcmp(b,"y")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } vy=1; SB(v,vm,q,sp("y")); q++; }
          if(!vz && !strcmp(b,"z")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } if(!vy) { vy=1; SB(v,vm,q,sp("y")); q++; } vz=1; SB(v,vm,q,sp("z")); q++; }
        }
      }
      else if(isalnum(*ff)) { SB(b,bm,j,*ff); j++; }
      else if(*ff=='{') {
        s=10; SB(b,bm,j,0); j=0;
        if(!ffq) { /* no implicit xyz if there are formal parameters */
          if(!vx && !strcmp(b,"x")) { vx=1; SB(v,vm,q,sp(b)); q++; }
          if(!vy && !strcmp(b,"y")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } vy=1; SB(v,vm,q,sp("y")); q++; }
          if(!vz && !strcmp(b,"z")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } if(!vy) { vy=1; SB(v,vm,q,sp("y")); q++; } vz=1; SB(v,vm,q,sp("z")); q++; }
        }
      }
      else { /* no implicit xyz if there are formal parameters */
        s=6; SB(b,bm,j,0); j=0;
        if(!ffq) { /* no implicit xyz if there are formal parameters */
          if(!vx && !strcmp(b,"x")) { vx=1; SB(v,vm,q,sp(b)); q++; }
          if(!vy && !strcmp(b,"y")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } vy=1; SB(v,vm,q,sp("y")); q++; }
          if(!vz && !strcmp(b,"z")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } if(!vy) { vy=1; SB(v,vm,q,sp("y")); q++; } vz=1; SB(v,vm,q,sp("z")); q++; }
        }
      }
      break;
    case 8:
      if(*ff=='}') { n--; if(!n) s=6; }
      else if(*ff=='{') n++; /* nested function */
      break;
    case 9:
      if(*ff=='}') { n--; if(!n) s=5; }
      else if(*ff=='{') n++; /* nested function */
      break;
    case 10:
      if(*ff=='}') { n--; if(!n) s=7; }
      else if(*ff=='{') n++; /* nested function */
      break;
    case 11: /* .z.s */
      if(isalnum(*ff)) s=11;
      else if(*ff=='.') s=11;
      else s=1;
      break;
    case 15: /* .z.s */
      if(isalnum(*ff)) s=15;
      else if(*ff=='.') s=15;
      else s=5;
      break;
    case 16: /* .z.s */
      if(isalnum(*ff)) s=16;
      else if(*ff=='.') s=16;
      else s=6;
      break;
    default: { r=KERR_PARSE; goto cleanup; }
    }
  }

  pf[2]=scope_new(cs);
  if(vz&&q<3) { SB(v,vm,q,sp("y")); q++; }
  if(vz&&q<3) { SB(v,vm,q,sp("x")); q++; }
  if(vy&&q<2) { SB(v,vm,q,sp("x")); q++; }
  pf[4]=t(1,q);
  i(q,if((p=scope_set(pf[2],t(4,sp(v[i])),null))) { r=p; goto cleanup; })
  if(q!=(int)n(((K*)px(((K*)((K*)px(pf[2])))[1]))[0])) { r=KERR_PARSE; goto cleanup; }  // {[a;b;b]a,b}
  K locals=tn(4,q); K *plocals=px(locals);
  i(q,plocals[i]=(K)sp(v[i]))

  /* parse */
  h=px(pf[0]);
  ++h;
  while(*h&&isblank(*h))++h;
  if(*h&&*h=='[') while(*h&&*h++!=']');
  g=xcalloc(1,5+strlen(h));
  memcpy(g,h,1+strlen(h));
  if(*g) g[strlen(g)-1]='\n';
  int opencode0=opencode;
  opencode=0;
  K cs0=cs; cs=k_(pf[2]);
  pf[1]=pgparse(g,1,locals);
  _k(cs); cs=cs0;
  opencode=opencode0;
  _k(locals);
  if(E(pf[1])) { r=pf[1]; pf[1]=null; }
cleanup:
  xfree(b); xfree(v);
  return r;
}

// make a copy of r and parent scope
// closure only if parent scope == s0
// can't create a closure like this:
// f:{i+x}
// g:{i:0;f}
static K closure(K x, K s0, K closurescope) {
  K r=3,*px,s,*fs,*pr;
  if(0xc3==s(x)) {
    px=px(x);
    s=px[2];
    if(s==null) return x;
    fs=px(s);
    if(fs[0]!=s0) return x;
    r=fncp(x);
    if(E(r)) { _k(x); return r; }
    _k(x);
    pr=px(r);
    s=pr[2];
    fs=px(s);
    _k(fs[0]);
    fs[0]=k_(closurescope);
  }
  // too complicated. may circle back later.
  //else if(0xc4==s(x)) {
  //  px=px(x);
  //  f=px[0];
  //  if(0xc3!=s(f)) return x; // TODO: support projection of projection in closures?
  //  pf=px(f);
  //  s=pf[2];
  //  fs=px(s);
  //  if(fs[0]!=s0) return x;
  //  r=fncp(x);
  //  if(E(r)) { _k(x); return r; }
  //  closurescope=scope_cp(fs[0]);
  //  pc=px(closurescope);
  //  pc[3]=t(1,1);
  //  _k(x);
  //  pr=px(r);
  //  f=pr[0];
  //  pf=px(f);
  //  s=pf[2];
  //  fs=px(s);
  //  _k(fs[0]);
  //  fs[0]=closurescope;
  //}
  return r;
}

static char av2_[1024][256];
K fne_(K f, K x, char *av) {
  K r=0,e,*pd,*ps,*pf,cs0,*px,*pr,t,g,q,*pq,*pt,v;
  u32 j=0,xi=0;
  static int d=0;
  char *av2=av2_[d],*fav;
  u64 n,nn;

  if(++d>maxr) { r=KERR_STACK; --d; goto cleanup; }

  if(0xc4==s(f)) {
    if(av&&*av) {
      px=px(x);
      n=ik(val(f));
      if(n==0) r=avdo(f,0,null,av);
      else if(n==1) r=avdo(f,0,k_(px[0]),av);
      else if(n==2) {
        if(nx==2) r=avdo(f,k_(px[0]),k_(px[1]),av);
        else r=avdo(f,0,k_(px[0]),av);
      }
      else if(0x81==s(x)) r=avdo(f,0,k_(x),av);
      else { --d; _k(x); return KERR_VALENCE; }
      _k(x);
      return r;
    }
    pf=px(f);
    g=k_(pf[0]);
    q=pf[1]; pq=px(q);
    px=px(x);
    v=val(f);
    if(nx<(u64)ik(v)) {
      r=tn(0,3); pr=px(r);
      pr[0]=fncp(f); /* lambda projection */
      if(E(pr[0])) {  e=pr[0]; _k(r); _k(f); _k(x); _k(g); return e; }
      pr[1]=kcp(x);  /* pargs */
      if(E(pr[1])) {  e=pr[1]; _k(r); _k(f); _k(x); _k(g); return e; }
      pr[2]=null;    /* av */
      _k(f); _k(x); _k(g);
      --d;
      return st(0xc4,r);
    }

    u64 inc=0; i(n(q),if(pq[i]==inull)++inc)
    nn=n(q)-inc; //  if there are inulls, number of inulls must less than or equal to nx
    if(inc && inc>nx) { _k(f); _k(x); _k(g); return KERR_VALENCE; }
    t=tn(0,nn+nx); pt=px(t);
    i(n(q),pt[j++]=pq[i]==inull?k_(px[xi++]):k_(pq[i]))
    // now, use up the rest of px
    while(j<n(t)) pt[j++]=k_(px[xi++]);

    _k(x); x=t;
    _k(f); f=g;

    av2[0]=0;
    pf=px(f);
    if(0xc3==s(f)) g=pf[3];
    else if(0xc4==s(f)) g=pf[2];
    else { _k(f); _k(x); --d; return KERR_PARSE; }
    if(6!=T(g)) {
      fav=px(g);
      snprintf(av2+strlen(av2),sizeof(av2)-strlen(av2),"%s",fav);
    }
    if(av&&strlen(av)) snprintf(av2+strlen(av2),sizeof(av2)-strlen(av2),"%s",av);
    r=fne_(f,x,av2);
    --d;
    return r;
  }

  pf=px(f);
  px=px(x);
  n=(u64)ik(pf[4]);

  u64 inc=0; i(nx,if(px[i]==inull)++inc)
  nn=nx-inc; //  if there are inulls, number of inulls must less than or equal to nx
  if((nx<n||nn<n) && (!av||!strlen(av))) { /* project */
    t=fncp(f);
    if(E(t)) { _k(f); _k(x); --d; return t; }
    r=tn(0,3); pr=px(r);
    pr[0]=t;       /* lambda */
    pr[1]=kcp(x);  /* pargs */
    if(E(pr[1])) {  e=pr[1]; _k(r); _k(f); _k(x); return e; }
    pr[2]=null;    /* av */
    _k(f); _k(x);
    --d;
    return st(0xc4,r);
  }
  if(nx!=n && nx!=1 && n!=0 && (!av||!strlen(av))) { r=KERR_VALENCE; goto cleanup; }
  if(av&&*av) {
    if(inc) { r=KERR_VALENCE; goto cleanup; } /* TODO: {y}'[!5;] */
    if(n==0) r=avdo(k_(f),0,null,av);
    else if(n==1) r=avdo(k_(f),0,k_(px[0]),av);
    else if(n==2) {
      if(nx==2) r=avdo(k_(f),k_(px[0]),k_(px[1]),av);
      else r=avdo(k_(f),0,k_(px[0]),av);
    }
    else if(0x81==s(x)) r=avdo(k_(f),0,k_(x),av);
    else { r=KERR_VALENCE; --d; goto cleanup; }
  }
  else {
    if(6==T(pf[2])) { K p=fnd(f); if(p) { _k(f); _k(x); return p; } }
    ps=px(pf[2]);     // scope
    pd=px(ps[1]);     // scope dict
    cs0=cs; cs=k_(pf[2]);
    K pd1save=pd[1];  // save scope dict values
    u64 NX=nx;
    pd[1]=x;
    K ps5save=ps[5];
    ps[5]=t(1,1);
    n(pd[0])=n; n(pd[1])=n;
    fnestack[++fnestacki]=f;
    int opencode0=opencode;
    opencode=0;
    r=pgreduce(pf[1],0);
    opencode=opencode0;
    --fnestacki;
    if(0xc3==s(r)) {
      K closurescope=scope_cp(cs);
      if(E(closurescope)) {
        pd=px(ps[1]); pd[1]=pd1save; --localsi;
        _k(cs); cs=cs0;
        _k(r); r=closurescope;
        goto cleanup;
      }
      K *pc=px(closurescope); pc[3]=t(1,1);
      r=closure(r,cs,closurescope);
      _k(closurescope);
    }
    else if(0x80==s(r)) {
      K *pr=px(r);
      K v=pr[1]; /* values */
      K *pvv=px(v);
      u32 c=0;
      i(n(v),if(0xc3==s(pvv[i]))++c)
      if(c&&c==n(v)) {
        K closurescope=scope_cp(cs);
        if(E(closurescope)) {
          pd=px(ps[1]); pd[1]=pd1save; --localsi;
          _k(cs); cs=cs0;
          _k(r); r=closurescope;
          goto cleanup;
        }
        K *pc=px(closurescope); pc[3]=t(1,1);
        i(n(v),pvv[i]=closure(pvv[i],cs,closurescope))
        _k(closurescope);
      }
    }

    if(ik(ps[5])==0) _k(pd[1]);
    pd[1]=pd1save;
    ps[5]=ps5save;
    nx=NX;
    n(pd[0])=n(pd[1]); // locals could have been added, so sync key/val counts
    _k(cs); cs=cs0;
  }
  --d;
cleanup:
  _k(f);
  _k(x);
  return r;
}
K fne(K f, K x, char *av) {
  K *pf,g;
  char av2[256],*fav;

  if(0xc4==s(f)) return fne_(f,x,av);

  av2[0]=0;
  pf=px(f);
  g=pf[3];
  if(6!=T(g)) {
    fav=px(g);
    snprintf(av2+strlen(av2),sizeof(av2)-strlen(av2),"%s",fav);
  }
  if(av&&strlen(av))  snprintf(av2+strlen(av2),sizeof(av2)-strlen(av2),"%s",av);
  return fne_(f,x,av2);
}
