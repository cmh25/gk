#include "p.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "node.h"
#include "x.h"
#include "ops.h"
#include "timer.h"
#include "sym.h"
#include "fn.h"

/*
# k3 ll(1)
ss > s sz
sz > | s sz
s > e se | se
se > ';' | '\n'
e > o av ez
ez > | e | plist
o > N | V | klist
av > | AV
plist > '[' elist ']' pz
pz > | e | AV pz2 | plist
pz2 > | e | plist
klist > '(' elist ')'
elist > ee elistz
elistz > | se ee elistz
ee > | e
*/

static int debug=0;
char **lines;
int linei,linem,*linef,*linefn;
char **fnames;
int fnamesi,fnamesm;
static char *ln;

static int LL[16][26]={
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,0,0,0,-1,-1,-1,0,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,1,1,1,1,-1,-1,-1,1,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,3,3,3,3,-1,-1,-1,3,-1,2},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,5,5,4,4,-1,-1,-1,4,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,6,7,-1,-1,-1,-1,-1,-1,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,8,8,-1,-1,-1,8,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,9,9,10,10,-1,11,9,10,9,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,12,13,-1,-1,-1,14,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,15,15,15,15,16,15,15,15,15,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,17,-1,-1,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,18,18,19,19,20,21,18,19,18,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,22,22,23,23,-1,24,22,23,22,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,25,-1,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,26,26,26,26,-1,-1,26,26,26,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,28,28,-1,-1,-1,-1,27,-1,27,-1},
{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,29,29,30,30,-1,-1,29,30,29,-1}
};

static int RT[31][4]={
{T001},
{T003,T002},
{-1},
{T003,T002},
{T005,T004},
{T004},
{T016},
{T017},
{T007,T008,T006},
{-1},
{T005},
{T009},
{T018},
{T019},
{T012},
{-1},
{T020},
{T021,T013,T022,T010},
{-1},
{T005},
{T020,T011},
{T009},
{-1},
{T005},
{T009},
{T023,T013,T024},
{T015,T014},
{-1},
{T004,T015,T014},
{-1},
{T005}
};

#define RCSIZE 31
static int RC[RCSIZE]={1,2,0,2,2,1,1,1,3,0,1,1,1,1,1,0,1,4,0,1,2,1,0,1,1,3,2,0,3,0,1};
int quiet,quiet2,btime;

static void r000(pgs *s) { /* $a > ss */
  (void)s;
  if(debug) printf("r000()\n");
}
static void r001(pgs *s) { /* ss > s sz */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r001()\n");
  node *a,*b,**aa;
  b=V[vi--];
  a=V[vi];
  if(b->k&&b->k->t==16) {
    node_free(b);
    aa=xmalloc(sizeof(node*));
    aa[0]=a;
    V[vi]=node_new(aa,1,0);
  }
  else {
    node_append(a,b);
    V[vi]=b;
  }
  s->vi=vi;
}
static void r002(pgs *s) { /* sz > */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r002()\n");
  V[++vi]=node_new(0,0,inull);
  s->vi=vi;
}
static void r003(pgs *s) { /* sz > s sz */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r003()\n");
  node *a,*b,**aa;
  b=V[vi--];
  a=V[vi];
  if(b->k&&b->k->t==16) {
    aa=xmalloc(sizeof(node*));
    aa[0]=a;
    V[vi]=node_new(aa,1,0);
    node_free(b);
  }
  else {
    node_append(a,b);
    V[vi]=b;
  }
  s->vi=vi;
}
static void r004(pgs *s) { /* s > e se */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r004()\n");
  node *a,*b;
  b=V[vi--];
  a=V[vi];
  if(b->k->t==20) a->q=1;
  node_free(b);
  s->vi=vi;
}
static void r005(pgs *s) { /* s > se */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r005()\n");
  if(V[vi]->k->t==20||V[vi]->k->t==21) V[vi]->k->t=6;
  s->vi=vi;
}
static void r006(pgs *s) { /* se > ';' */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r006()\n");
  V[vi]=node_new(0,0,knew(20,0,0,0,0,0));
  s->vi=vi;
}
static void r007(pgs *s) { /* se > '\n' */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r007()\n");
  V[vi]=node_new(0,0,knew(21,0,0,0,0,0));
  s->vi=vi;
}
static void r008(pgs *s) { /* e > o av ez */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r008()\n");
  node *a,*b,*c,**aa;
  c=V[vi--];
  b=V[vi--];
  a=V[vi];
  aa=xmalloc(sizeof(node*)*3);
  aa[0]=a;
  aa[1]=b;
  aa[2]=c;
  V[vi]=node_newli(aa,3,0,a->line,a->linei);
  s->vi=vi;
}
static void r009(pgs *s) { /* ez > */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r009()\n");
  V[++vi]=node_new(0,0,inull);
  s->vi=vi;
}
static void r010(pgs *s) { /* ez > e */
  (void)s;
  if(debug) printf("r010()\n");
}
static void r011(pgs *s) { /* ez > plist */
  (void)s;
  if(debug) printf("r011()\n");
}
static void r012(pgs *s) { /* o > N */
  (void)s;
  if(debug) printf("r012()\n");
}
static void r013(pgs *s) { /* o > V */
  (void)s;
  if(debug) printf("r013()\n");
}
static void r014(pgs *s) { /* o > klist */
  (void)s;
  if(debug) printf("r014()\n");
}
static void r015(pgs *s) { /* av > */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r015()\n");
  V[++vi]=node_new(0,0,inull);
  s->vi=vi;
}
static void r016(pgs *s) { /* av > AV */
  (void)s;
  if(debug) printf("r016()\n");
}
static void r017(pgs *s) { /* plist > '[' elist ']' pz */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r017()\n");
  node *a,*b,*c,*d,**aa;
  d=V[vi--];
  c=V[vi--];
  b=V[vi--];
  a=V[vi];
  if(d->t==13) { /* rhs assign */
    aa=xmalloc(sizeof(node*)*2);
    node_free(a);
    node_free(c);
    b->t=11;
    aa[0]=b;
    aa[1]=d;
    V[vi]=node_new(aa,2,0);
  }
  else if(d->t==14) { /* []'2 3 */
    aa=xmalloc(sizeof(node*)*2);
    node_free(a);
    node_free(c);
    b->t=11;
    aa[0]=b;
    aa[1]=d;
    V[vi]=node_new(aa,2,0);
  }
  else if(d->t==11) { /* plist plist */
    node_free(a);
    node_free(c);
    if(b->t==12) {
      b->t=11;
      aa=xmalloc(sizeof(node*)*2);
      aa[0]=b;
      aa[1]=d;
      V[vi]=node_new(aa,2,0);
      V[vi]->t=10; /* plist list */
    }
  }
  else if(d->t==10) { /* list of plist */
    bt=11;
    node_prepend(b,d);
    V[vi]=d;
  }
  else {
    if(d->k&&d->k->t==16) {
      node_free(a);
      node_free(c);
      node_free(d);
      b->t=11;
      V[vi]=b;
    }
    else if(b->t==12) {
      node_free(a);
      node_free(c);
      b->t=11;
      aa=xmalloc(sizeof(node*)*2);
      aa[0]=b;
      aa[1]=d;
      V[vi]=node_new(aa,2,0);
      V[vi]->t=18;
    }
    else {
      node_free(a);
      node_free(c);
      node_prepend(d,b);
      b->t=11;
      V[vi]=b;
    }
  }
  s->vi=vi;
}
static void r018(pgs *s) { /* pz > */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r018()\n");
  V[++vi]=node_new(0,0,inull);
  s->vi=vi;
}
static void r019(pgs *s) { /* pz > e */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r019()\n");
  if(V[vi]->a[0]->k->t==7) V[vi]->t=13;
}
static void r020(pgs *s) { /* pz > AV pz2 */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r020()\n");
  node *a,*b,**aa;
  b=V[vi--];
  a=V[vi];
  aa=xmalloc(sizeof(node*)*3);
  aa[0]=a;
  aa[1]=node_new(0,0,inull);
  aa[2]=b;
  V[vi]=node_new(aa,3,0);
  V[vi]->t=14;
  s->vi=vi;
}
static void r021(pgs *s) { /* pz > plist */
  (void)s;
  if(debug) printf("r021()\n");
}
static void r022(pgs *s) { /* pz2 > */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r022()\n");
  V[++vi]=node_new(0,0,inull);
  s->vi=vi;
}
static void r023(pgs *s) { /* pz2 > e */
  (void)s;
  if(debug) printf("r023()\n");
}
static void r024(pgs *s) { /* pz2 > plist */
  (void)s;
  if(debug) printf("r024()\n");
}
static void r025(pgs *s) { /* klist > '(' elist ')' */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r025()\n");
  node *a,*b,*c;
  c=V[vi--];
  b=V[vi--];
  a=V[vi];
  node_free(a);
  node_free(c);
  if(b->k&&b->k->t==16) b->k=knew(0,0,0,0,0,0);
  V[vi]=b;
  s->vi=vi;
}
static void r026(pgs *s) { /* elist > ee elistz */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r026()\n");
  node *a,*b,**aa;
  b=V[vi--];
  a=V[vi];
  if(b->k&&b->k->t==16) {
    node_free(b);
    aa=xmalloc(sizeof(node*));
    aa[0]=a;
    V[vi]=node_new(aa,1,0);
  }
  else {
    node_append(a,b);
    V[vi]=b;
  }
  V[vi]->t=12;
  s->vi=vi;
}
static void r027(pgs *s) { /* elistz > */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r027()\n");
  V[++vi]=node_new(0,0,inull);
  s->vi=vi;
}
static void r028(pgs *s) { /* elistz > se ee elistz */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r028()\n");
  node *a,*b,*d,**aa;
  b=V[vi--];
  a=V[vi--];
  d=V[vi];
  node_free(d);
  if(b->k&&b->k->t==16) {
    node_free(b);
    aa=xmalloc(sizeof(node*));
    aa[0]=a;
    V[vi]=node_new(aa,1,0);
  }
  else {
    node_append(a,b);
    V[vi]=b;
  }
  s->vi=vi;
}
static void r029(pgs *s) { /* ee > */
  node **V=s->V;
  int vi=s->vi;
  if(debug) printf("r029()\n");
  V[++vi]=node_new(0,0,inull);
  s->vi=vi;
}
static void r030(pgs *s) { /* ee > e */
  (void)s;
  if(debug) printf("r030()\n");
}

static void (*F[31])(pgs *s)={r000,r001,r002,r003,r004,r005,r006,r007,r008,r009,r010,r011,r012,r013,r014,r015,r016,r017,r018,r019,r020,r021,r022,r023,r024,r025,r026,r027,r028,r029,r030};

static void push(pgs *s, int t, node *v) {
  if(s->tc==s->tm) { s->tm<<=1; s->t=xrealloc(s->t,s->tm*sizeof(int)); }
  if(s->tc==s->vm) { s->vm<<=1; s->v=xrealloc(s->v,s->vm*sizeof(node*)); }
  s->t[s->tc]=t;
  s->v[s->tc++]=v;
  s->lt=t;
}

static int ii;
static double ff;
static char *p;
static int gn_(void) {
  char c,*q=p;
  int s=0;
  while(1) {
    switch(s) {
    case 0:
      if(*p=='0') s=1;
      else if(isdigit(*p)) s=4;
      else if(*p=='-') s=5;
      else if(*p=='.') s=7;
      else s=10;
      break;
    case 1:
      if(*p=='n'||*p=='i') s=2;
      else if(*p=='N'||*p=='I') s=3;
      else if(*p=='e'||*p=='E') s=8;
      else if(*p=='.') s=6;
      else if(isdigit(*p)) s=4;
      else s=11;
      break;
    case 2: s=12; break;
    case 3: s=11; break;
    case 4:
      if(isdigit(*p)) s=4;
      else if(*p=='e'||*p=='E') s=8;
      else if(*p=='.') s=6;
      else s=11;
      break;
    case 5:
      if(*p=='0') s=1;
      else if(isdigit(*p)) s=4;
      else if(*p=='.') s=7;
      else s=10;
      break;
    case 6:
      if(isdigit(*p)) s=9;
      else if(*p=='e'||*p=='E') s=8;
      else s=12;
      break;
    case 7: if(isdigit(*p)) s=9; else s=10; break;
    case 8: if(isdigit(*p)||*p=='-'||*p=='+') s=9; else s=10; break;
    case 9:
      if(isdigit(*p)) s=9;
      else if(*p=='e'||*p=='E') s=8;
      else s=12;
      break;
    case 10: return 0; /* error */
    case 11: /* int */
      c=*--p; *p=0;
      ii=xatoi(q);
      *p=c;
      return 1;
    case 12: /* double */
      c=*--p; *p=0;
      ff=xstrtod(q,0);
      *p=c;
      return 2;
    }
    ++p;
  }
  return 0;
}

static int gn(pgs *pgs) {
  int r,*iv=0,i;
  int ic=0,fc=0,im=1,fm=1;
  double *fv=0;
  char *q;
  r=gn_();
  if(*p==' ') {
    q=p;
    while(*p==' ')p++;
    if(isdigit(*p)||((*p=='.'||*p=='-')&&isdigit(p[1]))) { /* convert to vector */
      if(r==1) { im<<=1; iv=xrealloc(iv,im*sizeof(int)); iv[ic++]=ii; }
      else if(r==2) { fm<<=1; fv=xrealloc(fv,fm*sizeof(double)); fv[fc++]=ff; }
    }
    p=q;
    if(iv) {
      while(*p==' ') {
        q=p;
        while(*p==' ')p++;
        r=gn_();
        if(r==1) {
          if(ic==im) { im<<=1; iv=xrealloc(iv,im*sizeof(int)); }
          iv[ic++]=ii;
        }
        else if(r==2) {
          fm=im;
          fv=xrealloc(fv,fm*sizeof(double));
          for(i=0;i<ic;i++) fv[fc++]=I2F(iv[i]);
          xfree(iv); iv=0; ic=0;
          if(fc==fm) { fm<<=1; fv=xrealloc(fv,fm*sizeof(double)); }
          fv[fc++]=ff;
          break;
        }
        else { p=q; break; }
      }
    }
    if(fv) {
      while(*p==' ') {
        q=p;
        while(*p==' ')p++;
        r=gn_();
        if(r==1) {
          ff=I2F(ii);
          if(fc==fm) { fm<<=1; fv=xrealloc(fv,fm*sizeof(double)); }
          fv[fc++]=ff;
        }
        else if(r==2) {
          if(fc==fm) { fm<<=1; fv=xrealloc(fv,fm*sizeof(double)); }
          fv[fc++]=ff;
        }
        else { p=q; break; }
      }
    }
  }
  if(ic) push(pgs,T018,node_new(0,0,knew(-1,ic,iv,0,0,0)));
  else if(fc) push(pgs,T018,node_new(0,0,knew(-2,fc,fv,0,0,0)));
  else if(r==1) push(pgs,T018,node_new(0,0,knew(1,0,0,ii,0,0)));
  else if(r==2) push(pgs,T018,node_new(0,0,knew(2,0,0,0,ff,0)));
  if(iv) xfree(iv);
  if(fv) xfree(fv);
  return r;
}

static char *ss;
static int gsym_(void) {
  char *q,c,s=0;
  if(*p!='`') return 0;
  q=++p;
  while(1) {
    switch(s) {
    case 0:
      if(*p=='"') { ++q; s=2; }
      else if(*p=='.') s=4;
      else if(isalpha(*p)) s=3;
      else s=1;
      break;
    case 1:
      c=*--p; *p=0;
      ss=xunesc(q);
      *p=c;
      if(*p=='"')p++;
      return 1;
    case 2:
      if(*p=='\\') ++p;
      else if(*p=='"') {
        c=*p; *p=0;
        ss=xunesc(q);
        *p++=c;
        return 1;
      }
      break;
    case 3:
      if(*p=='.') s=4;
      else if(!isalnum(*p)) s=1;
      break;
    case 4:
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
  char *q,**sv=0;
  size_t sc=0,sm=1;
  r=gsym_();
  if(*p==' '||*p=='`') {
    q=p;
    while(*p==' ')p++;
    if(*p=='`') sm<<=1;
    sv=xrealloc(sv,sm*sizeof(char*));
    sv[sc++]=sp(ss);
    xfree(ss);
    p=q;
    if(sv) {
      while(*p==' '||*p=='`') {
        q=p;
        while(*p==' ')p++;
        r=gsym_();
        if(r==1) {
          if(sc==sm) { sm<<=1; sv=xrealloc(sv,sm*sizeof(char*)); }
          sv[sc++]=sp(ss);
          xfree(ss);
        }
        else { p=q; break; }
      }
    }
  }
  if(sv) {
    if(sc==1) push(pgs,T018,node_new(0,0,knew(4,0,sv[0],0,0,0)));
    else push(pgs,T018,node_new(0,0,knew(-4,sc,sv,0,0,0)));
  }
  else if(r==1) { push(pgs,T018,node_new(0,0,knew(4,0,ss,0,0,0))); xfree(ss); }
  if(sv) xfree(sv);
  return r;
}

static int gc(pgs *pgs) {
  char s=0;
  int j=0,n;
  unsigned char o;
  n=32;
  ss=xmalloc(n);
  ++p;
  while(1) {
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
      else if(isdigit(*p)&&*p<='7') { o=*p-48; s=2; } /* octal */
      else { ss[j++]=*p; s=0; }
      break;
    case 2: /* octal */
      if(isdigit(*p)&&*p<='7') { o*=8; o+=*p-48; s=3; }
      else if(*p=='\\') { ss[j++]=o; s=1; }
      else if(*p=='"') { ss[j++]=o; s=10; }
      else { ss[j++]=o; ss[j++]=*p; s=0; }
      break;
    case 3: /* octal */
      if(isdigit(*p)&&*p<='7') { o*=8; o+=*p-48; ss[j++]=o; s=0; }
      else if(*p=='\\') { ss[j++]=o; s=1; }
      else if(*p=='"') { ss[j++]=o; s=10; }
      else { ss[j++]=o; ss[j++]=*p; s=0; }
      break;
    case 10: /* accept */
      if(j==0) { push(pgs,T018,node_new(0,0,knew(-3,0,0,0,0,0))); }
      else if(j==1) { push(pgs,T018,node_new(0,0,knew(3,0,0,ss[0],0,0))); }
      else push(pgs,T018,node_new(0,0,knew(-3,j,ss,0,0,0)));
      xfree(ss);
      return 1;
    default: return 0; /* error */
    }
    ++p;
  }
  return 1;
}

static int gname(pgs *pgs, int v) {
  char c,*q=p;
  int s=*p=='.';
  while(1) {
    if(!s) { if(isalpha(*p)) { p++; s=1; } else break; }
    else { if(isalnum(*p)) p++; else if(*p=='.') { p++; s=0; } else break; }
  }
  c=*p; *p=0;
  push(pgs,T018,node_newli(0,0,knew(v?99:4,0,sp(q),0,0,0),linei,p-ln));
  *p=c;
  return 1;
}

static int gp(pgs *pgs) {
  char *q,c;
  int a=0;
  q=p++;
  if(*p==':') a=1; /* indicate +: */
  c=*p; *p=0;
  if(a) push(pgs,T019,node_newli(0,0,knew(57,0,fnnew(q),*q,0,0),linei,p-ln));
  else push(pgs,T019,node_newli(0,0,knew(7,0,fnnew(q),*q,0,0),linei,p-ln));
  *p=c;
  if(a) ++p;
  return 1;
}

static int gf(pgs *pgs) {
  char *q,c,s=0;
  fn *f;
  int fc=0;
  q=p;
  while(1) {
    if(*p=='"') p=xeqs(p);
    switch(s) {
    case 0:
      if(*p=='}'&&fc==1) s=1;
      else if(*p=='}') fc--;
      else if(*p=='{') fc++;
      break;
    case 1: /* accept */
      c=*p; *p=0;
      f=fnnew(q);
      push(pgs,T019,node_new(0,0,knew(37,0,f,0,0,0)));
      *p=c;
      return 1;
    default: return 0; /* error */
    }
    ++p;
  }
  return 1;
}

static int gav(pgs *pgs) {
  char *q,c;
  q=p++;
  while(*p=='\''||*p=='/'||*p=='\\') ++p;
  c=*p; *p=0;
  push(pgs,T020,node_new(0,0,knew(47,strlen(q),xstrdup(q),0,0,0)));
  *p=c;
  if(*p==':') return 0; /* k32 adverb use? */
  else return 1;
}

static int gcon(pgs *pgs, int n, int t) {
  char *q,c;
  int fc=0;
  p+=n; /*  while[ do[ if[ $[ */
  q=p;
  while(1) {
    if(*p=='"') p=xeqs(p);
    if(*p==']'&&fc==0) {
      c=*p; *p=0;
      push(pgs,T018,node_new(0,0,knew(t,0,q,0,0,0)));
      *p=c;
      ++p; /* ] */
      return 1;
    }
    else if(*p==']') fc--;
    else if(*p=='[') fc++;
    ++p;
  }
  return 1;
}

static int gback(pgs *pgs) {
  char *q,c;
  int prec,err;
  if(p[1]=='\n') {
    push(pgs,T019,node_new(0,0,knew(7,0,fnnew("abort"),176,0,0)));
    push(pgs,T018,node_new(0,0,null));
    p++;
  }
  else if(p[1]=='\\') {
    push(pgs,T019,node_new(0,0,knew(7,0,fnnew("exit"),141,0,0)));
    push(pgs,T018,node_new(0,0,knew(1,0,0,0,0,0)));
    p+=2;
  }
  else if(p[1]=='e'&&p[2]&&strchr(" \n\t",p[2])) {
    push(pgs,T019,node_new(0,0,knew(7,0,fnnew("\\e"),175,0,0)));
    p+=2;
    while(*p==' ')p++;
    q=p;
    if(isdigit(*p)) {
      while(isdigit(*p))p++;
      c=*p; *p=0;
      err=xatoi(q);
      if(!err)err=0;
      push(pgs,T018,node_new(0,0,knew(1,0,0,err,0,0)));
      *p=c;
    }
    else push(pgs,T018,node_new(0,0,null));
  }
  else if(p[1]=='t'&&p[2]&&strchr(" \n\t",p[2])) { p+=2; push(pgs,T019,node_new(0,0,knew(7,0,fnnew("\\t"),174,0,0))); }
  else if(p[1]=='p'&&p[2]&&strchr(" \n\t",p[2])) {
    push(pgs,T019,node_new(0,0,knew(7,0,fnnew("\\p"),142,0,0)));
    p+=2;
    while(*p==' ')p++;
    q=p;
    if(isdigit(*p)) {
      while(isdigit(*p))p++;
      c=*p; *p=0;
      prec=xatoi(q);
      if(!prec)prec=17;
      push(pgs,T018,node_new(0,0,knew(1,0,0,prec,0,0)));
      *p=c;
    }
    else push(pgs,T018,node_new(0,0,null));
  }
  else if(p[1]=='v'&&p[2]&&strchr(" \n\t",p[2])) {
    push(pgs,T019,node_new(0,0,knew(7,0,fnnew("\\v"),143,0,0)));
    p+=2;
    push(pgs,T018,node_new(0,0,null));
  }
  else if(p[1]=='V'&&p[2]&&strchr(" \n\t",p[2])) {
    push(pgs,T019,node_new(0,0,knew(7,0,fnnew("\\v"),143,0,0)));
    p+=2;
    push(pgs,T018,node_newli(0,0,knew(4,0,sp("-l"),0,0,0),linei,p-ln));
  }
  else if(p[1]=='d'&&p[2]&&strchr(" \n\t",p[2])) {
    push(pgs,T019,node_newli(0,0,knew(7,0,fnnew("\\d"),177,0,0),linei,p-ln));
    p+=2;
    while(*p==' ')p++;
    if(isalpha(*p)||(*p=='.'&&isalpha(p[1]))) gname(pgs,0);
    else if(*p=='^') { push(pgs,T018,node_new(0,0,knew(3,0,0,'^',0,0))); ++p; }
    else push(pgs,T018,node_new(0,0,null));
  }
  else if(p[1]=='l'&&p[2]&&strchr(" \n\t",p[2])) {
    push(pgs,T019,node_newli(0,0,knew(7,0,fnnew("\\l"),184,0,0),linei,p-ln));
    p+=2;
    while(*p==' ')p++;
    q=p;
    while(*p&&*p!='\n')p++;
    c=*p; *p=0;
    if(strlen(q )) push(pgs,T018,node_newli(0,0,knew(4,0,sp(q),0,0,0),linei,p-ln));
    else push(pgs,T018,node_new(0,0,null));
    *p=c;
  }
  else if(strchr("0+'_.:-`?",p[1])) { /* help */
    push(pgs,T019,node_new(0,0,knew(7,0,fnnew(""),179,0,0)));
    push(pgs,T018,node_new(0,0,k3(*(p+1))));
    p+=2;
  }
  else return 0;
  return 1;
}

static int lex(pgs *pgs) {
  int f=1,s=0;
  ln=p=pgs->p;
  if(!linem) {
    linem=2;
    lines=xmalloc(linem*sizeof(char*));
    linef=xmalloc(linem*sizeof(int*));
    linefn=xmalloc(linem*sizeof(int*));
  }
  while(1) {
    s=*p==' ';
    while(*p==' ') p++;
    if((s||f)&&*p=='/') { while(*++p!='\n'){}; continue; }
    if(*p=='\n') {
      push(pgs,T017,0);
      f=1;
      if(linei==linem) {
        linem<<=1;
        lines=xrealloc(lines,linem*sizeof(char*));
        linef=xrealloc(linef,linem*sizeof(int*));
        linefn=xrealloc(linefn,linem*sizeof(int*));
      }
      lines[linei]=xstrndup(ln,p-ln);
      linef[linei]=pgs->ffli; /* first line of fn */
      linefn[linei++]=pgs->fni; /* filename index */
      ln=++p;
      continue;
    }
    else if(*p==';') { ++p; push(pgs,T016,0); }
    else if(*p=='(') { ++p; push(pgs,T023,0); }
    else if(*p==')') { ++p; push(pgs,T024,0); }
    else if(*p=='[') { ++p; push(pgs,T021,0); }
    else if(*p==']') { ++p; push(pgs,T022,0); }
    else if(*p=='{') gf(pgs);
    else if(!strncmp(p,"while[",6)) gcon(pgs,6,80);
    else if(!strncmp(p,"do[",3)) gcon(pgs,3,81);
    else if(!strncmp(p,"if[",3)) gcon(pgs,3,82);
    else if(!strncmp(p,"$[",2)) gcon(pgs,2,83);
    else if(!strncmp("0:",p,2)) { p+=2; push(pgs,T019,node_newli(0,0,knew(7,0,fnnew("0:"),128,0,0),linei,p-ln)); }
    else if(!strncmp("1:",p,2)) { p+=2; push(pgs,T019,node_newli(0,0,knew(7,0,fnnew("1:"),129,0,0),linei,p-ln)); }
    else if(!strncmp("2:",p,2)) { p+=2; push(pgs,T019,node_newli(0,0,knew(7,0,fnnew("2:"),131,0,0),linei,p-ln)); }
    else if(!strncmp("4:",p,2)) { p+=2; push(pgs,T019,node_newli(0,0,knew(7,0,fnnew("4:"),133,0,0),linei,p-ln)); }
    else if(!strncmp("5:",p,2)) { p+=2; push(pgs,T019,node_newli(0,0,knew(7,0,fnnew("5:"),134,0,0),linei,p-ln)); }
    else if(!strncmp("6:",p,2)) { p+=2; push(pgs,T019,node_newli(0,0,knew(7,0,fnnew("6:"),135,0,0),linei,p-ln)); }
    else if(isdigit(*p)||(*p=='.'&&isdigit(p[1]))) gn(pgs);
    else if(*p=='-') {
      if(p[1]==':') gp(pgs);
      else if(!(s|f)&&(pgs->lt==T018||pgs->lt==T024)) { ++p; push(pgs,T019,node_newli(0,0,knew(7,0,fnnew("-"),'-',0,0),linei,p-ln)); }
      else if(isdigit(p[1])||(p[1]=='.'&&isdigit(p[2]))) gn(pgs);
      else { ++p; push(pgs,T019,node_newli(0,0,knew(7,0,fnnew("-"),'-',0,0),linei,p-ln)); }
    }
    else if((*p=='.'&&isalpha(p[1]))||isalpha(*p)) gname(pgs,1);
    else if(*p&&strchr("+*%&|<>=^!~,#_$?@.:",*p)) gp(pgs);
    else if(!*p) { push(pgs,T025,0); break; }
    else if(f&&*p=='\\') {
      if(!gback(pgs)) {
        if(system(p+1)) return 0;
        while(*p!='\n')++p;
      }
    }
    else if(!strncmp("`0:",p,3)) { p+=3; push(pgs,T019,node_newli(0,0,knew(7,0,fnnew("`0:"),136,0,0),linei,p-ln)); }
    else if(!strncmp("`3:",p,3)) { p+=3; push(pgs,T019,node_newli(0,0,knew(7,0,fnnew("`3:"),137,0,0),linei,p-ln)); }
    else if(!strncmp("`4:",p,3)) { p+=3; push(pgs,T019,node_newli(0,0,knew(7,0,fnnew("`4:"),138,0,0),linei,p-ln)); }
    else if(!strncmp("`8:",p,3)) { p+=3; push(pgs,T019,node_newli(0,0,knew(7,0,fnnew("`8:"),139,0,0),linei,p-ln)); }
    else if(*p=='`') gsym(pgs);
    else if(*p=='"') gc(pgs);
    else if(f&&*p=='\'') gp(pgs);
    else if(*p=='\''||*p=='/'||*p=='\\') { /* check for k32 adverb use */
      if(*(p+1)&&*(p+1)==':') { printf("lex\n"); return 0; }
      else if(!gav(pgs)) { printf("lex\n"); return 0; }
    }
    else { printf("lex\n"); return 0; }
    f=0;
  }
  return 1;
}

pgs* pgnew(void) {
  pgs *s=xmalloc(sizeof(pgs));
  s->Sm=s->Rm=s->Vm=1024;
  s->tm=s->vm=1024;
  s->S=xmalloc(s->Sm*sizeof(int));
  s->R=xmalloc(s->Rm*sizeof(int));
  s->V=xmalloc(s->Vm*sizeof(node*));
  s->t=xmalloc(s->tm*sizeof(int));
  s->v=xmalloc(s->vm*sizeof(node*));
  s->ffli=-1;
  s->fni=-1;
  return s;
}

void pgfree(pgs *s) {
  if(s->p) xfree(s->p);
  if(s->S) xfree(s->S);
  if(s->R) xfree(s->R);
  if(s->V) xfree(s->V);
  if(s->t) xfree(s->t);
  if(s->v) xfree(s->v);
  xfree(s);
}

node* pgparse(pgs *s) {
  int i,r;
  s->ti=s->tc=0;
  s->si=s->ri=s->vi=s->lt=-1;
  if(!lex(s)) { DO(s->tc,node_free(s->v[i])); return 0; }
  s->S[++s->si]=T000; /* $a */
  while(1) {
    if(s->si>=s->Sm-RCSIZE+2) { s->Sm<<=1; s->S=xrealloc(s->S,s->Sm*sizeof(int)); }
    if(s->ri>=s->Rm-1) { s->Rm<<=1; s->R=xrealloc(s->R,s->Rm*sizeof(int)); }
    if(s->vi>=s->Vm-1) { s->Vm<<=1; s->V=xrealloc(s->V,s->Vm*sizeof(node*)); }
    if(s->S[s->si]==s->t[s->ti]) {
      s->V[++s->vi]=s->v[s->ti++];
      --s->si;
    }
    else {
      r=LL[s->S[s->si--]][s->t[s->ti]];
      if(r==-1) { printf("parse\n"); return 0; }
      s->R[++s->ri]=r;
      s->S[++s->si]=-2; /* reduction marker */
      for(i=RC[r]-1;i>=0;i--) s->S[++s->si]=RT[r][i];
    }
    while(s->si>=0&&s->S[s->si]==-2) { (*F[s->R[s->ri--]])(s); --s->si; }
    if(s->si<0) break;
  }
  return s->V[s->vi];
}
