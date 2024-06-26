#include "fn.h"
#include <ctype.h>
#include <string.h>
#include "ops.h"
#include "x.h"
#include "sym.h"
#include "sys.h"
#include "av.h"
#include "avopt.h"
#include "p.h"
#include "io.h"
#include "os.h"

int DEPTH;

K* (*dt2[256])(K *a, K *b);
K* (*dt1[256])(K *a);
K* (*dt2avo[256])(K *a, char *av);
K* (*dt2avo2[256])(K *a, K *b, char *av);

fn* fnnew(char *s) {
  fn *f=xmalloc(sizeof(fn));
  f->d=xstrdup(s);
  f->v=0;
  f->n=0;
  f->s_=0;
  f->s=0;
  f->av=xstrdup("");
  f->q = 0;
  f->l = 0;
  f->a = 0;
  f->c = 0;
  f->cn = 0;
  f->i = 0;
  return f;
}

void fnfree(fn *f) {
  xfree(f->d);
  if(f->n) node_free(f->n);
  if(f->av) xfree(f->av);
  if(f->l) kfree(f->l);
  if(f->a) kfree(f->a);
  if(f->c) { DO(f->cn,kfree(f->c[i])); xfree(f->c); }
  xfree(f);
}

fn* fncp(fn *f) {
  fn *g=fnnew(f->d);
  g->v=f->v;
  xfree(g->av);
  g->av=xstrdup(f->av);
  g->q=f->q;
  if(f->l) g->l = kcp(f->l);
  if(f->a) g->a = kcp(f->a);
  if(f->c) {
    g->cn=f->cn;
    g->c=xmalloc(sizeof(K*)*g->cn);
    DO(g->cn,g->c[i]=kcp(f->c[i]));
  }
  g->i=f->i;
  return g;
}

void fninit(void) {
  C=dnew();

  dt1['+']=flip_;
  dt1['-']=negate_;
  dt1['*']=first_;
  dt1['%']=reciprocal_;
  dt1['&']=where_;
  dt1['|']=reverse_;
  dt1['<']=upgrade_;
  dt1['>']=downgrade_;
  dt1['=']=group_;
  dt1['^']=shape_;
  dt1['!']=enumerate_;
  dt1['~']=not_;
  dt1[',']=enlist_;
  dt1['#']=count_;
  dt1['_']=flr_;
  dt1['$']=format_;
  dt1['?']=unique_;
  dt1['@']=atom_;
  dt1['.']=value_;
  dt1[':']=return_;
  dt1['\'']=signal_;
  dt1[128]=zerocolon1_;
  dt1[129]=onecolon1_;
  dt1[131]=twocolon1_;
  dt1[133]=fourcolon1_;
  dt1[134]=fivecolon1_;
  dt1[135]=sixcolon1_;
  dt1[136]=b0colon1_;
  dt1[137]=b3colon1_;
  dt1[138]=b4colon1_;
  dt1[139]=b8colon1_;
  dt1[141]=exit1_;
  dt1[142]=precision1_;
  dt1[143]=kdump1_;
  dt1[144]=sleep1_;
  dt1[145]=ci1_;
  dt1[146]=ic1_;
  dt1[147]=dj1_;
  dt1[148]=jd1_;
  dt1[149]=lt1_;
  dt1[150]=log1_;
  dt1[151]=exp1_;
  dt1[152]=abs1_;
  dt1[153]=sqr1_;
  dt1[154]=sqrt1_;
  dt1[155]=floor1_;
  dt1[156]=ceil1_;
  dt1[157]=sin1_;
  dt1[158]=cos1_;
  dt1[159]=tan1_;
  dt1[160]=asin1_;
  dt1[161]=acos1_;
  dt1[162]=atan1_;
  dt1[163]=sinh1_;
  dt1[164]=cosh1_;
  dt1[165]=tanh1_;
  dt1[166]=erf1_;
  dt1[167]=erfc1_;
  dt1[168]=gamma1_;
  dt1[169]=lgamma1_;
  dt1[170]=rint1_;
  dt1[171]=trunc1_;
  dt1[172]=not1_;
  dt1[173]=kv1_;
  dt1[174]=timer1_;
  dt1[175]=error1_;
  dt1[176]=abort1_;
  dt1[177]=dir1_;
  dt1[178]=val1_;
  dt1[179]=help1_;
  dt1[180]=del1_;
  dt1[181]=bd1_;
  dt1[182]=db1_;
  dt1[183]=vk1_;
  dt1[184]=load1_;
  dt1[185]=md5_;
  dt1[186]=sha1_;
  dt1[187]=sha2_;

  dt2['+']=plus2_;
  dt2['-']=minus2_;
  dt2['*']=times2_;
  dt2['%']=divide2_;
  dt2['&']=minand2_;
  dt2['|']=maxor2_;
  dt2['<']=less2_;
  dt2['>']=more2_;
  dt2['=']=equal2_;
  dt2['^']=power2_;
  dt2['!']=modrot2_;
  dt2['~']=match2_;
  dt2[',']=join2_;
  dt2['#']=take2_;
  dt2['_']=drop2_;
  dt2['$']=form2_;
  dt2['?']=find2_;
  dt2['@']=at2_;
  dt2['.']=dot2_;
  dt2[':']=assign2_;
  dt2[128]=zerocolon2_;
  dt2[129]=onecolon2_;
  dt2[134]=fivecolon2_;
  dt2[135]=sixcolon2_;
  dt2[139]=draw2_;
  dt2[140]=vs2_;
  dt2[141]=sv2_;
  dt2[142]=atan2_;
  dt2[143]=div2_;
  dt2[144]=and2_;
  dt2[145]=or2_;
  dt2[146]=xor2_;
  dt2[147]=euclid2_;
  dt2[148]=rot2_;
  dt2[149]=shift2_;
  dt2[150]=ddot2_;
  dt2[151]=atn2_;
  dt2[152]=sm2_;
  dt2[153]=ss2_;
  dt2[154]=lsq2_;

  /* these are needed because of over dyad */
  dt2avo['+']=plus2_avopt;
  dt2avo['-']=minus2_avopt;
  dt2avo['*']=times2_avopt;
  dt2avo['%']=divide2_avopt;
  dt2avo['&']=minand2_avopt;
  dt2avo['|']=maxor2_avopt;
  dt2avo['<']=less2_avopt;
  dt2avo['>']=more2_avopt;
  dt2avo['=']=equal2_avopt;
  dt2avo['^']=power2_avopt;
  dt2avo['!']=modrot2_avopt;
  dt2avo['~']=match2_avopt;
  dt2avo[',']=join2_avopt;
  dt2avo['#']=take2_avopt;
  dt2avo['_']=drop2_avopt;
  dt2avo['$']=form2_avopt;
  dt2avo['?']=find2_avopt;
  dt2avo['@']=at2_avopt;
  dt2avo['.']=dot2_avopt;
  dt2avo[':']=assign2_avopt;
  dt2avo[133]=fourcolon1_avopt;
  dt2avo[134]=fivecolon1_avopt;
  dt2avo[145]=ci1_avopt;
  dt2avo[146]=ic1_avopt;
  dt2avo[147]=dj1_avopt;
  dt2avo[148]=jd1_avopt;
  dt2avo[149]=lt1_avopt;
  dt2avo[150]=log1_avopt;
  dt2avo[151]=exp1_avopt;
  dt2avo[152]=abs1_avopt;
  dt2avo[153]=sqr1_avopt;
  dt2avo[154]=sqrt1_avopt;
  dt2avo[155]=floor1_avopt;
  dt2avo[156]=ceil1_avopt;
  dt2avo[157]=sin1_avopt;
  dt2avo[158]=cos1_avopt;
  dt2avo[159]=tan1_avopt;
  dt2avo[160]=asin1_avopt;
  dt2avo[161]=acos1_avopt;
  dt2avo[162]=atan1_avopt;
  dt2avo[163]=sinh1_avopt;
  dt2avo[164]=cosh1_avopt;
  dt2avo[165]=tanh1_avopt;
  dt2avo[166]=erf1_avopt;
  dt2avo[167]=erfc1_avopt;
  dt2avo[168]=gamma1_avopt;
  dt2avo[169]=lgamma1_avopt;
  dt2avo[170]=rint1_avopt;
  dt2avo[171]=trunc1_avopt;
  dt2avo[172]=not1_avopt;
  dt2avo[173]=kv1_avopt;
  dt2avo[183]=vk1_avopt;

  dt2avo2['+']=plus2_avopt2;
  dt2avo2['-']=minus2_avopt2;
  dt2avo2['*']=times2_avopt2;
  dt2avo2['%']=divide2_avopt2;
  dt2avo2['&']=minand2_avopt2;
  dt2avo2['|']=maxor2_avopt2;
  dt2avo2['<']=less2_avopt2;
  dt2avo2['>']=more2_avopt2;
  dt2avo2['=']=equal2_avopt2;
  dt2avo2['^']=power2_avopt2;
  dt2avo2['!']=modrot2_avopt2;
  dt2avo2['~']=match2_avopt2;
  dt2avo2[',']=join2_avopt2;
  dt2avo2['#']=take2_avopt2;
  dt2avo2['_']=drop2_avopt2;
  dt2avo2['$']=form2_avopt2;
  dt2avo2['?']=find2_avopt2;
  dt2avo2['@']=at2_avopt2;
  dt2avo2['.']=dot2_avopt2;
  dt2avo2[':']=assign2_avopt2;
  dt2avo2[139]=draw2_avopt2;
  dt2avo2[140]=vs2_avopt2;
  dt2avo2[141]=sv2_avopt2;
  dt2avo2[142]=atan2_avopt2;
  dt2avo2[143]=div2_avopt2;
  dt2avo2[144]=and2_avopt2;
  dt2avo2[145]=or2_avopt2;
  dt2avo2[146]=xor2_avopt2;
  dt2avo2[147]=euclid2_avopt2;
  dt2avo2[148]=rot2_avopt2;
  dt2avo2[149]=shift2_avopt2;
  dt2avo2[150]=ddot2_avopt2;
  dt2avo2[151]=atn2_avopt2;
  dt2avo2[152]=sm2_avopt2;
  dt2avo2[153]=ss2_avopt2;
  dt2avo2[154]=lsq2_avopt2;

  /* builtins */
  dset(C,"in",knew(37,0,fnnew("{(#y)>y?x}"),1,0,-1));
  dset(C,"lin",knew(37,0,fnnew("{(#y)>y?/x}"),1,0,-1));
  dset(C,"dvl",knew(37,0,fnnew("{x@&(#y)=y?/x}"),1,0,-1));
  dset(C,"dv",knew(37,0,fnnew("{x dvl,y}"),1,0,-1));
  dset(C,"di",knew(37,0,fnnew("{$[@x;. di[. x;(!x)?/y];x@&@[(#x)#1;y;:;0]]}"),1,0,-1));
  dset(C,"gtime",knew(37,1,fnnew("{(dj _ x%86400;100 sv 24 60 60 vs x!86400)}"),1,0,-1));
  dset(C,"ltime",knew(37,1,fnnew("{gtime lt x}"),1,0,-1));
  dset(C,"tl",knew(37,1,fnnew("{t+x-lt t:x+x-lt x}"),1,0,-1));
  dset(C,"mul",knew(37,0,fnnew("{x dot\\y}"),1,0,-1));
  dset(C,"inv",knew(37,0,fnnew("{((2##*x)#1,&#*x) lsq x}"),1,0,-1));
  dset(C,"choose",knew(37,0,fnnew("{rint exp lgamma[1+x]-lgamma[1+y]+lgamma[1+x-y]}"),1,0,-1));
  dset(C,"round",knew(37,0,fnnew("{x*rint y%x}"),1,0,-1));
  dset(C,"ssr",knew(37,0,fnnew("{if[nul~x;:nul];i:1+2*!_.5*#x:(0,,/(0,+/~+\\({x<y}'0,\"[\"=y)-{x<y}'(\"]\"=y$:),0)+/x ss $[3=4:y;$y;y])_ x;,/$[7=4:z;@[x;i;z];4:z$:;@[x;i;:;(#i)#,z];@[x;i;:;z]]}"),1,0,-1));

  /* monadic */
  dset(C,"exit",knew(7,1,fnnew("exit"),141,0,-1));
  dset(C,"sleep",knew(7,1,fnnew("sleep"),144,0,-1));
  dset(C,"ci",knew(7,1,fnnew("ci"),145,0,-1));
  dset(C,"ic",knew(7,1,fnnew("ic"),146,0,-1));
  dset(C,"dj",knew(7,1,fnnew("dj"),147,0,-1));
  dset(C,"jd",knew(7,1,fnnew("jd"),148,0,-1));
  dset(C,"lt",knew(7,1,fnnew("lt"),149,0,-1));
  dset(C,"log",knew(7,1,fnnew("log"),150,0,-1));
  dset(C,"exp",knew(7,1,fnnew("exp"),151,0,-1));
  dset(C,"abs",knew(7,1,fnnew("abs"),152,0,-1));
  dset(C,"sqr",knew(7,1,fnnew("sqr"),153,0,-1));
  dset(C,"sqrt",knew(7,1,fnnew("sqrt"),154,0,-1));
  dset(C,"floor",knew(7,1,fnnew("floor"),155,0,-1));
  dset(C,"ceil",knew(7,1,fnnew("ceil"),156,0,-1));
  dset(C,"sin",knew(7,1,fnnew("sin"),157,0,-1));
  dset(C,"cos",knew(7,1,fnnew("cos"),158,0,-1));
  dset(C,"tan",knew(7,1,fnnew("tan"),159,0,-1));
  dset(C,"asin",knew(7,1,fnnew("asin"),160,0,-1));
  dset(C,"acos",knew(7,1,fnnew("acos"),161,0,-1));
  dset(C,"atan",knew(7,1,fnnew("atan"),162,0,-1));
  dset(C,"sinh",knew(7,1,fnnew("sinh"),163,0,-1));
  dset(C,"cosh",knew(7,1,fnnew("cosh"),164,0,-1));
  dset(C,"tanh",knew(7,1,fnnew("tanh"),165,0,-1));
  dset(C,"erf",knew(7,1,fnnew("erf"),166,0,-1));
  dset(C,"erfc",knew(7,1,fnnew("erfc"),167,0,-1));
  dset(C,"gamma",knew(7,1,fnnew("gamma"),168,0,-1));
  dset(C,"lgamma",knew(7,1,fnnew("lgamma"),169,0,-1));
  dset(C,"rint",knew(7,1,fnnew("rint"),170,0,-1));
  dset(C,"trunc",knew(7,1,fnnew("trunc"),171,0,-1));
  dset(C,"not",knew(7,1,fnnew("not"),172,0,-1));
  dset(C,"kv",knew(7,1,fnnew("kv"),173,0,-1));
  dset(C,"time",knew(7,1,fnnew("time"),174,0,-1));
  dset(C,"val",knew(7,1,fnnew("val"),178,0,-1));
  dset(C,"del",knew(7,1,fnnew("del"),180,0,-1));
  dset(C,"bd",knew(7,1,fnnew("bd"),181,0,-1));
  dset(C,"db",knew(7,1,fnnew("db"),182,0,-1));
  dset(C,"vk",knew(7,1,fnnew("vk"),183,0,-1));
  dset(C,"md5",knew(7,1,fnnew("md5"),185,0,-1));
  dset(C,"sha1",knew(7,1,fnnew("sha1"),186,0,-1));
  dset(C,"sha2",knew(7,1,fnnew("sha2"),187,0,-1));

  /* dyadic */
  dset(C,"draw",knew(7,2,fnnew("draw"),139,0,-1));
  dset(C,"vs",knew(7,2,fnnew("vs"),140,0,-1));
  dset(C,"sv",knew(7,2,fnnew("sv"),141,0,-1));
  dset(C,"atan2",knew(7,2,fnnew("atan2"),142,0,-1));
  dset(C,"div",knew(7,2,fnnew("div"),143,0,-1));
  dset(C,"and",knew(7,2,fnnew("and"),144,0,-1));
  dset(C,"or",knew(7,2,fnnew("or"),145,0,-1));
  dset(C,"xor",knew(7,2,fnnew("xor"),146,0,-1));
  dset(C,"euclid",knew(7,2,fnnew("euclid"),147,0,-1));
  dset(C,"rot",knew(7,2,fnnew("rot"),148,0,-1));
  dset(C,"shift",knew(7,2,fnnew("shift"),149,0,-1));
  dset(C,"dot",knew(7,2,fnnew("dot"),150,0,-1));
  dset(C,"at",knew(7,2,fnnew("at"),151,0,-1));
  dset(C,"sm",knew(7,2,fnnew("sm"),152,0,-1));
  dset(C,"ss",knew(7,2,fnnew("ss"),153,0,-1));
  dset(C,"lsq",knew(7,2,fnnew("lsq"),154,0,-1));
}

K* fnd(K *a) {
  K *r=a;
  char *f=0,b[256],*v[256],*g,*h,c;
  int i,j,u,s,n,q,vx,vy,vz;
  fn *ff;
  pgs *pgs;
  SG2(a);
  ff=a->v;
  f=ff->d;

  if(linei==linem) {
    linem<<=1;
    lines=xrealloc(lines,linem*sizeof(char*));
    linef=xrealloc(linef,linem*sizeof(int*));
    linefn=xrealloc(linefn,linem*sizeof(int*));
  }
  g=strchr(f,'\n');
  if(g) { c=*g; *g=0; lines[linei++]=xstrdup(f); *g=c; }
  else lines[linei++]=xstrdup(f);

  s=j=n=q=vx=vy=vz=0;
  if(*f!='{') return r;
  for(;*f;++f) {
    if(*f=='"') f=xeqs(f);
    switch(s) {
    case 0:
      if(*f=='{') s=1;
      else return kerror("parse");
      break;
    case 1:
      while(*f==' '||*f=='\t')++f;
      if(*f=='}') s=2;
      else if(*f=='{') { n++; s=9; }
      else if(*f=='[') s=3;
      else if(*f=='.') s=11;
      else if(*f=='`') s=101;
      else if(isalpha(*f)) { s=7; b[j++]=*f; }
      else s=6;
      break;
    case 101:
      if(isalnum(*f)) s=101;
      else s=1;
      break;
    case 2:
      return kerror("parse");
    case 3:
      if(isalpha(*f)) { s=4; b[j++]=*f; ff->q=1; }
      else if(*f==']') s=5;
      else return kerror("parse");
      break;
    case 4:
      if(isalnum(*f)) { s=4; b[j++]=*f; }
      else if(*f==']') { s=5; b[j]=0; j=0; v[q++]=sp(b); }
      else if(*f==';') { s=3; b[j]=0; j=0; v[q++]=sp(b); }
      break;
    case 5:
      if(*f=='}') s=2;
      else if(*f=='.') s=15;
      else if(*f=='`') s=105;
      else if(isalpha(*f)) { s=7; b[j++]=*f; }
      else s=6;
      break;
    case 105:
      if(isalnum(*f)) s=105;
      else s=5;
      break;
    case 6:
      if(*f=='}') s=2;
      else if(*f=='.') s=16;
      else if(*f=='`') s=106;
      else if(isalpha(*f)) { s=7; b[j++]=*f; }
      else if(*f=='{') { n++; s=8; }
      else s=6;
      break;
    case 106:
      if(isalnum(*f)) s=106;
      else if(*f=='`') s=106;
      else s=6;
      break;
    case 7:
      if(*f=='}') {
        s=2; b[j]=0; j=0;
        if(!ff->q) { /* no implicit xyz if there are formal parameters */
          if(!vx && !strcmp(b,"x")) { vx=1; v[q++]=sp(b); }
          if(!vy && !strcmp(b,"y")) { vy=1; v[q++]=sp(b); }
          if(!vz && !strcmp(b,"z")) { vz=1; v[q++]=sp(b); }
        }
      }
      else if(isalnum(*f)) b[j++]=*f;
      else if(*f=='{') {
        s=10; b[j]=0; j=0;
        if(!ff->q) { /* no implicit xyz if there are formal parameters */
          if(!vx && !strcmp(b,"x")) { vx=1; v[q++]=sp(b); }
          if(!vy && !strcmp(b,"y")) { vy=1; v[q++]=sp(b); }
          if(!vz && !strcmp(b,"z")) { vz=1; v[q++]=sp(b); }
        }
      }
      else { /* no implicit xyz if there are formal parameters */
        s=6; b[j]=0; j=0;
        if(!ff->q) { /* no implicit xyz if there are formal parameters */
          if(!vx && !strcmp(b,"x")) { vx=1; v[q++]=sp(b); }
          if(!vy && !strcmp(b,"y")) { vy=1; v[q++]=sp(b); }
          if(!vz && !strcmp(b,"z")) { vz=1; v[q++]=sp(b); }
        }
      }
      break;
    case 8:
      if(*f=='}') { n--; if(!n) s=6; }
      else if(*f=='{') n++; /* nested function */
      break;
    case 9:
      if(*f=='}') { n--; if(!n) s=5; }
      else if(*f=='{') n++; /* nested function */
      break;
    case 10:
      if(*f=='}') { n--; if(!n) s=7; }
      else if(*f=='{') n++; /* nested function */
      break;
    case 11: /* .z.s */
      if(isalnum(*f)) s=11;
      else if(*f=='.') s=11;
      else s=1;
      break;
    case 15: /* .z.s */
      if(isalnum(*f)) s=15;
      else if(*f=='.') s=15;
      else s=5;
      break;
    case 16: /* .z.s */
      if(isalnum(*f)) s=16;
      else if(*f=='.') s=16;
      else s=6;
      break;
    default: return kerror("parse");
    }
  }

  ff->s_=scope_new(cs);
  if(vz&&q<3) { v[q++] = sp("y"); }
  if(vz&&q<3) { v[q++] = sp("x"); }
  if(vy&&q<2) { v[q++] = sp("x"); }
  if(!ff->l) ff->v=q;
  for(i=0;i<q;i++) { if(kreserved(v[i])) return kerror("reserved"); else EC(scope_set(ff->s_,v[i],null)); }

  /* build ast */
  u=strlen(ff->d);
  g=h=xmalloc(u);
  strncpy(h,&ff->d[1],u-1);
  h[u-2]='\n';
  h[u-1]=0;
  while(*h==' '||*h=='\t')++h;
  if(*h=='[') while(*h++!=']');
  pgs=pgnew();
  pgs->ffli=linei-1;
  pgs->fni=fnamesi-1;
  pgs->p=xstrdup(h);
  ++DEPTH;
  ff->n=pgparse(pgs);
  --DEPTH;
  pgfree(pgs);
  xfree(g);

  kfree(a);
  return r;
}

K* fnprj(K *f, K *a) {
  K *aa,*p=0,*s;
  fn *ff=f->v,*fp;
  unsigned int e=0,n=0;
  if(cs->k) n=1;
  if(at==11) { DO(ac,if(v0(a)[i]->t==16)++e); n=ac; }
  else if(at==10) DO(ac,s=v0(a)[i];DO2(s->c,++n;if(v0(s)[j]->t==16)++e))
  if(ff->v>1&&(ff->v>n||e)) { /* make projection */
    p=knew(7,0,fnnew(""),'p',0,0); p->t=77;
    fp=p->v;
    fp->l=kref(f);
    if(at==11) aa=kref(a);
    else { aa=kv0(1); v0(aa)[0]=kref(a); aa->t=11; }
    fp->a=aa;
    fp->v=ff->v-n+e;
    fp->s_=scope_cp(ff->s_);
  }
  return p;
}

K* fne2(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0,*self=0;
  scope *os=0,*fs=0;
  unsigned int i,j,fa=0,rv;
  int k;
  fn *ff=f->v;
  (void)av;

  if(at==99){fa=1;SG(a);}
  if(a&&!at) DO(ac,SG2(v0(a)[i]);if(v0(a)[i]->t==98)return v0(a)[i])

  ff->s=scope_cp(ff->s_);
  if(!cs->k) ff->s->p=cs; /* function in function */

  if((p=fnprj(f,a))) return p;

  if(at==11&&ac==1) {
    a=v0(a)[0];
    if(a&&!at) DO(ac,SG2(v0(a)[i]);if(v0(a)[i]->t==98)return v0(a)[i])
  }

  if(at==11&&ff->v!=ac) return kerror("valence");
  else if(at==11&&ff->v==1) return kerror("valence");
  fs=ff->s;

  rv=ff->v; /* real valence */
  q=kv0(rv); /* gather parameters */
  i=j=0;
  if(rv>1) {
    if(ac) for(;i<rv;i++) v0(q)[i]=v0(a)[j++];
    else v0(q)[i]=a;
  }
  else if(rv==1) v0(q)[0]=a;

  if(!ff->q) { /* no formal parameters */
    if(rv==3) {
      EC(scope_set(fs,sp("x"),v0(q)[rv-3]));
      EC(scope_set(fs,sp("y"),v0(q)[rv-2]));
      EC(scope_set(fs,sp("z"),v0(q)[rv-1]));
    }
    else if(rv==2) {
      EC(scope_set(fs,sp("x"),v0(q)[rv-2]));
      EC(scope_set(fs,sp("y"),v0(q)[rv-1]));
    }
    else if(rv==1) {
      EC(scope_set(fs,sp("x"),v0(q)[0]));
    }
  }
  else {
    if(rv==1) { EC(scope_set(fs,fs->d->k[0],v0(q)[0])); }
    else DO(rv,EC(scope_set(fs,fs->d->k[i],v0(q)[i])))
  }
  q->c=0;
  kfree(q);

  self=dget(Z,"f");
  dset(Z,"f",f);

  os=cs;cs=fs;
  r=null;
  for(k=ff->n->v-1;k>=0;k--) {
    kfree(r);
    quiet2=0;
    r=node_reduce(ff->n->a[k],0);
    if(r->t==98) { dset(Z,"f",self); kfree(self); return r; }
    if(fret) { fret=0; break; }
  }
  kfree(p);

  dset(Z,"f",self);
  kfree(self);

  SR(r);
  if(!r) r=null;
  cs=os;
  if(quiet2) { kfree(r); r=null; quiet2=0; } /* assignment doesn't return a value */
  scope_free(fs); ff->s=0;
  if(a&&!at) DO(ac,kfree(v0(a)[i]))
  if(fa) kfree(a);
  return r;
}
