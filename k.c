#include "k.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>
#ifdef _WIN32
  #include "unistd.h"
#else
  #include <sys/mman.h>
  #include <unistd.h>
#endif
#include "x.h"
#include "sym.h"
#include "dict.h"
#include "scope.h"
#include "fn.h"

static char ds[256];
int precision=7;
K *D,*one,*zero,*null,*inull;

static K **KS;
static int ksi=-1,km=1024;

void kinit(int argc, char **argv) {
  K *p,*a;
  char hn[256];
  #ifdef _WIN32
  int size=256;
  GetComputerNameA(hn,&size);
  #else
  gethostname(hn,256);
  #endif
  Z = dnew();
  KS=xmalloc(sizeof(K*)*km);
  one = knew(1,0,0,1,0,-1);
  zero = knew(1,0,0,0,0,-1);
  null = knew(6,0,0,0,0,-1);
  inull = knew(16,0,0,0,0,-1);
  ks=gs=scope_new(0);
  cs=scope_new(gs);
  ktree=ks->d;
  ks->k=sp(".");
  cs->k=sp(".k");
  p=k5(cs->d); dset(ktree,"k",p); kfree(p);
  p=k5(Z); dset(ktree,"z",p); kfree(p);
  gs=cs; /* gs starts out in `.k */
  fninit();
  dset(C,"nul",null);
  D=k4(".k"); D->r=-1;
  if(argc>1) {
    a=kv0(argc-2);
    DO(argc-2,v0(a)[i]=knew(-3,strlen(argv[i+2]),argv[i+2],0,0,0))
  }
  else a=kv0(0);
  dset(Z,"h",k4(hn));
  dset(Z,"P",k1(getpid()));
  dset(Z,"i",a);
  dset(Z,"T",null);
  dset(Z,"t",null);
  dset(Z,"f",null);
  kfree(a);
}

K* knew(char t, unsigned int c, void *v, int i, double f, int r) {
  K *s;
  if(ksi>=0) s=KS[ksi--];
  else s=xmalloc(sizeof(K));

  s->t = t;
  s->c = c;
  s->v = 0;
  s->r = r;

       if(t ==  0) s->v = xmalloc(sizeof(K*)*c);
  else if(t == 17) s->v = xmalloc(sizeof(K*)*c);
  else if(t == -1) s->v = xmalloc(sizeof(int)*c);
  else if(t == -2) s->v = xmalloc(sizeof(double)*c);
  else if(t == -3) s->v = xmalloc(sizeof(char)*c);
  else if(t == -4) s->v = xmalloc(sizeof(char*)*c);
  else if(t ==  1) s->i = i;
  else if(t ==  2) s->f = f;
  else if(t ==  3) s->i = i;

  if(v) {
         if(t ==  4) s->v = sp(v);
    else if(t ==  5) s->v = v;
    else if(t ==  7) {s->v = v; ((fn*)s->v)->i=i;}
    else if(t == 17) s->v = v; /* +1 */
    else if(t == 27) s->v = v; /* 1+ */
    else if(t == 37) {s->v = v; ((fn*)s->v)->i=i;}
    else if(t == 47) s->v = v; /* av */
    else if(t == 57) {s->v = v; ((fn*)s->v)->i=i;} /* +: */
    else if(t == 67) s->v = v; /* composition */
    else if(t ==  0) memcpy(s->v,v,sizeof(K*)*c);
    else if(t == -1) memcpy(s->v,v,sizeof(int)*c);
    else if(t == -2) memcpy(s->v,v,sizeof(double)*c);
    else if(t == -3) memcpy(s->v,v,sizeof(char)*c);
    else if(t == -4) memcpy(s->v,v,sizeof(char*)*c);
    else if(t == 80) s->v = xstrdup(v); /* while */
    else if(t == 81) s->v = xstrdup(v); /* do */
    else if(t == 82) s->v = xstrdup(v); /* if */
    else if(t == 83) s->v = xstrdup(v); /* cond */
    else if(t == 98) {s->v = xmalloc(sizeof(ERR));((ERR*)s->v)->s=sp(v);((ERR*)s->v)->i=0;((ERR*)s->v)->j=0;} /* kerror */
    else if(t == 99) s->v = sp(v); /* variable */
  }

  return s;
}

void kfree(K *s) {
  size_t len;
  if(!s) return;
  if(s->r==-1) return; /* constant */
#ifndef _WIN32
  if(s->r==-2) {
    if(s->t==-1) len=12+s->c*sizeof(int);
    else if(s->t==-2) len=12+s->c*sizeof(double);
    else if(s->t==-3) len=12+s->c*sizeof(char);
    else { fprintf(stderr,"unexpected mmap'd type\n"); exit(1); }
    munmap((char*)s->v-12,len);
    s->v=0;
    if(1+ksi<km) KS[++ksi]=s;
    else xfree(s);
    return;
   }
#endif
  if(!s->r) {
    if(s->t==0||s->t==10||s->t==11||s->t==12||s->t==13||s->t==14||s->t==15||s->t==18||s->t==97)
      DO(s->c, kfree(v0(s)[i]))
    else if(s->t== 5) { dfree(s->v); s->v=0; }
    else if(s->t==7||s->t==17||s->t==27||s->t==37||s->t==57||s->t==67||s->t==77||s->t==87) { fnfree(s->v); s->v=0; }
    if(s->v && s->t!=1 && s->t!=2 && s->t!=3 && s->t!=4 && s->t!=99 ) xfree(s->v);
    if(1+ksi<km) KS[++ksi]=s;
    else xfree(s);
  }
  else if(s->r<-2) s->r++;
  else s->r--;
}

void kdump(int l) {
  int t,c=0,m=4,n;;
  unsigned int i;
  K *v;
  dict *d=cs->d;
  K *keys=dkeys(d);
  char *s,*p;
  if(l) {
    DO(keys->c,n=strlen(d->k[i]);if(m<n)m=n);
    for(i=0;i<keys->c;i++) {
      v=d->v[i];
      t=v->t; if(t==27||t==37||t==67||t==77||t==87) t=7;
      if(v->c>100) { c=v->c; v->c=100; }
      s=kprint5(v,"","",0);
      if(c) v->c=c;
      s=xrealloc(s,strlen(s)+3);
      if(strlen(s)>30) { s[30]=0; strcat(s,"..."); }
      if(t==5||t==0) DO(strlen(s),if(s[i]=='\n')s[i]=';');
      if((p=strchr(s,'\n'))) { *p=0; strcat(s," ..."); }
      printf("%-*s t:[%2d] c:[%10d] r:[%2d] %s\n",m,d->k[i],t,v->c,v->r,s);
      xfree(s);
    }
  }
  else {
    if(keys->c) printf("%s",d->k[0]);
    for(i=1;i<keys->c;i++) printf(" %s",d->k[i]);
    printf("\n");
  }
  kfree(keys);
}


static char *o=0;
static size_t oc=0;
static size_t bs0=10000000;
static size_t bs=10000000;

void kprint(K *p, char *s, char *s0, int nl) {
  if(o){xfree(o);o=0;oc=0;}
  char *r=kprint_(p,s,s0,nl);
  printf("%s",r);
  xfree(r);
  if(o){xfree(o);o=0;oc=0;}
}

char* kprint5(K *p, char *s, char *s0, int nl) {
  if(o){xfree(o);o=0;oc=0;}
  char *r=kprint_(p,s,s0,nl);
  if(o){xfree(o);o=0;oc=0;}
  return r;
}

static void pi(char *s, char *c, int i) {
  if(i==INT_MAX) oc+=sprintf(o+oc,"%s%s0I",s,c);
  else if(i==INT_MIN) oc+=sprintf(o+oc,"%s%s0N",s,c);
  else if(i==INT_MIN+1) oc+=sprintf(o+oc,"%s%s-0I",s,c);
  else oc+=sprintf(o+oc,"%s%s%d",s,c,i);
}
static void pf(char *s, char *c, double f) {
  char ds[256];
  if(isinf(f)&&f>0.0) oc+=sprintf(o+oc,"%s%s0i",s,c);
  else if(isnan(f)) oc+=sprintf(o+oc,"%s%s0n",s,c);
  else if(isinf(f)&&f<0.0) oc+=sprintf(o+oc,"%s%s-0i",s,c);
  else {
    sprintf(ds,"%0.*g",precision,f);
    if(!strchr(ds,'.')&&!strchr(ds,'e')) strcat(ds, ".0");
    oc+=sprintf(o+oc,"%s%s%s",s,c,ds);
  }
}
static void ps(char *s, char *p) {
  int v; char *e;
  v=vname(p,strlen(p));
  oc+=sprintf(o+oc,"%s`",s);
  if(!v) oc+=sprintf(o+oc,"\"");
  e=xesc(p);
  oc+=sprintf(o+oc,"%s",e);
  xfree(e);
  if(!v) oc+=sprintf(o+oc,"\"");
}
static void pc(unsigned char c) {
  if((c<32||c>126)) {
    if(c==8) oc+=sprintf(o+oc,"\\b");
    else if(c==9) oc+=sprintf(o+oc,"\\t");
    else if(c==10) oc+=sprintf(o+oc,"\\n");
    else if(c==13) oc+=sprintf(o+oc,"\\r");
    else oc+=sprintf(o+oc,"\\%03o",c);
  }
  else {
    if(c==34) oc+=sprintf(o+oc,"\\\"");
    else if(c==92) oc+=sprintf(o+oc,"\\\\");
    else oc+=sprintf(o+oc,"%c",c);
  }
}
char* kprint_(K *p, char *s, char *s0, int nl) {
  int add0=1;
  unsigned int i;
  char s2[128],s3[128];
  int z=0;
  K *p2=0;
  dict *d=0;
  fn *f=0;
  char *ee[11]={"domain","index","int","length","nonce","parse","rank","reserved","type","valence","value"};
  int en=11;

  if(!o){ bs=bs0; o=xcalloc(bs,1);oc=0; }
  if(oc<<1 > bs) { bs+=bs0; o=xrealloc(o,bs); }

  SG2(p);

  switch(p->t) {
  case 1: pi(s,"",p->i); break;
  case 2: pf(s,"",p->f); break;
  case 3: oc+=sprintf(o+oc,"%s\"",s); pc(p->i); oc+=sprintf(o+oc,"\""); break;
  case 4: ps(s,v3(p)); break;
  case 5:
    d = p->v;
    if(d->c == 0) oc+=sprintf(o+oc,"%s.()",s);
    else if(d->c == 1) {
      p2 = d->v[0];
      if((((p2->t > 0 || !p2->c) && p2->t != 5) || p2->t == -3)
         || (p2->t==5 && !((dict*)p2->v)->c)) {
        oc+=sprintf(o+oc,"%s.,(`%s;",s,d->k[0]);
        xfree(kprint_(p2, "", s0, 0));
        oc+=sprintf(o+oc,";)");
      } else {
        oc+=sprintf(o+oc,"%s.,(`%s\n",s,d->k[0]);
        sprintf(s2, "%s%s   ", s, s0);
        xfree(kprint_(p2, s2, s0, 0));
        oc+=sprintf(o+oc,"\n%s   )",s);
      }
    } else {
      oc+=sprintf(o+oc,"%s.(",s);
      p2 = d->v[0];
      if((((p2->t > 0 || !p2->c) && p2->t != 5) || p2->t == -3)
         || (p2->t==5 && !((dict*)p2->v)->c)) {
        oc+=sprintf(o+oc,"(`%s;",d->k[0]);
        xfree(kprint_(p2, "", s0, 0));
        oc+=sprintf(o+oc,";)\n");
        sprintf(s2, "%s%s  ", s, s0);
      } else {
        oc+=sprintf(o+oc,"(`%s\n",d->k[0]);
        sprintf(s2, "%s%s   ", s, s0);
        xfree(kprint_(p2, s2, s0, 0));
        oc+=sprintf(o+oc,"\n%s)\n",s2);
        sprintf(s2, "%s%s  ", s, s0);
      }
      for(i=1;(int)i<d->c;i++) {
        if(oc<<1 > bs) { bs+=bs0; o=xrealloc(o,bs); }
        p2 = d->v[i];
        if((((p2->t > 0 || !p2->c) && p2->t != 5) || p2->t == -3)
           || (p2->t==5 && !((dict*)p2->v)->c)) {
          oc+=sprintf(o+oc,"%s(`%s;",s2,d->k[i]);
          xfree(kprint_(p2, "", s0, 0));
          if((int)i==d->c-1)oc+=sprintf(o+oc,";))");
          else oc+=sprintf(o+oc,";)\n");
        } else {
          oc+=sprintf(o+oc,"%s(`%s\n",s2,d->k[i]);
          sprintf(s3, "%s%s   ", s, s0);
          xfree(kprint_(p2, s3, s0, 0));
          if((int)i==d->c-1)oc+=sprintf(o+oc,"\n%s))",s3);
          else oc+=sprintf(o+oc,"\n%s)\n",s3);
        }
      }
    }
    break;
  case 6: case 16: if(strlen(s)) oc+=sprintf(o+oc,"%s",s); else nl=0; break;
  case 7:
  case 37:
    oc+=sprintf(o+oc,"%s%s",s,((fn*)p->v)->d);
    if(((fn*)p->v)->av) oc+=sprintf(o+oc,"%s",((fn*)p->v)->av);
    break;
  case 27:
    xfree(kprint_(((fn*)p->v)->l,s,s0,0));
    p->t=7;
    xfree(kprint_(p,"",s0,0));
    p->t=27;
    break;
  case 67:
    f=p->v;
    xfree(kprint_(f->c[0],s,s0,0));
    DO(f->cn-1,xfree(kprint_(f->c[i+1],"",s0,0)))
    break;
  case 77: case 87:
    f=p->v;
    xfree(kprint_(f->l,s,s0,0));
    xfree(kprint_(f->a,s,s0,0));
    if(f->av) oc+=sprintf(o+oc,"%s",f->av);
    break;
  case 97:
    f=v0(p)[1]->v;
    xfree(kprint_(v0(p)[0],s,s0,0));
    v0(p)[1]->t=37;
    xfree(kprint_(v0(p)[1],s,s0,0));
    v0(p)[1]->t=17;
    break;
  case 11:
    oc+=sprintf(o+oc,"[");
    if(p->c) {
      DO(p->c-1,xfree(kprint_(v0(p)[i],"",s0,0));oc+=sprintf(o+oc,";"))
      xfree(kprint_(v0(p)[p->c-1],"",s0,0));
    }
    else xfree(kprint_(p,"",s0,0));
    oc+=sprintf(o+oc,"]");
    break;
  case  0:
    if(p->c == 1) {
      oc+=sprintf(o+oc,"%s,",s);
      sprintf(s3,"%s%s ",s,s0);
      xfree(kprint_(v0(p)[0],"",s3,0));
      break;
    }

    /* any elements with count>0 (except cv and fixed dyad) */
    for(i=0;i<p->c;i++) {
      if(v0(p)[i]->c && v0(p)[i]->t != -3 && v0(p)[i]->t != 27 && v0(p)[i]->t != 97) {
        z=1;
        break;
      }
    }
    /* more than one and all cv? */
    if(!z && p->c>0) {
      z=1;
      for(i=0;i<p->c;i++) if(v0(p)[i]->t != -3) { z=0; break; }
    }
    /* any dictionaries? */
    if(!z) for(i=0;i<p->c;i++) if(v0(p)[i]->t == 5 && ((dict*)v0(p)[i]->v)->c) { z=1; break; }

    if(z) {
      oc+=sprintf(o+oc,"%s(", s);
      sprintf(s3,"%s%s ",s,s0);
      xfree(kprint_(v0(p)[0],"",s3,1));
      if(v0(p)[0]->t==6||v0(p)[0]->t==16) oc+=sprintf(o+oc,"\n");
      sprintf(s2, "%s%s ", s, s0);
      for(i=1;i<p->c-1;i++) xfree(kprint_(v0(p)[i],s2,"",1));
      xfree(kprint_(v0(p)[i],s2,"",0));
      oc+=sprintf(o+oc,")");
      break;
    } else {
      oc+=sprintf(o+oc,"%s(", s);
      for(i=0;i<p->c;i++) {
        if(oc<<1 > bs) { bs+=bs0; o=xrealloc(o,bs); }
        p2=v0(p)[i];
        switch(p2->t) {
        case  1: pi("","",p2->i); break;
        case  2: pf("","",p2->f); break;
        case  3: xfree(kprint_(p2,"","",0)); break;
        case  4: oc+=sprintf(o+oc,"`%s", v3(p2)); break;
        case  5: oc+=sprintf(o+oc,".()"); break;
        case  7:
        case 37:
          oc+=sprintf(o+oc,"%s",((fn*)p2->v)->d);
          if(((fn*)p2->v)->av) oc+=sprintf(o+oc,"%s",((fn*)p2->v)->av);
          break;
        case 27:
          xfree(kprint_(((fn*)p2->v)->l,"","",0));
          p2->t=7;
          xfree(kprint_(p2,"","",0));
          p2->t=27;
          break;
        case 67:
          f=p2->v;
          DO(f->cn,xfree(kprint_(f->c[i],"","",0)))
          break;
        case 77: case 87:
          f=p2->v;
          xfree(kprint_(f->l,"","",0));
          xfree(kprint_(f->a,"","",0));
          if(f->av) oc+=sprintf(o+oc,"%s",f->av);
          break;
        case 97:
          f=v0(p2)[1]->v;
          xfree(kprint_(v0(p2)[0],"","",0));
          v0(p2)[1]->t=37;
          xfree(kprint_(v0(p2)[1],"","",0));
          v0(p2)[1]->t=17;
          break;
        case  0: oc+=sprintf(o+oc,"()"); break;
        case -1: oc+=sprintf(o+oc,"!0"); break;
        case -2: oc+=sprintf(o+oc,"0#0.0"); break;
        case -3: xfree(kprint_(p2,"","",0)); break;
        case -4: oc+=sprintf(o+oc,"0#`"); break;
        default: break;
        }
        if(i != p->c-1) oc+=sprintf(o+oc,";");
      }
      oc+=sprintf(o+oc,")");
      break;
    }
  case -1:
    if(p->c == 0) oc+=sprintf(o+oc,"%s!0", s);
    else if(p->c == 1) pi(s,",",v1(p)[0]);
    else {
      pi(s,"",v1(p)[0]);
      for(i=1;i<p->c;i++) {
        if(oc<<1 > bs) { bs+=bs0; o=xrealloc(o,bs); }
        pi(" ","",v1(p)[i]);
        VSIZE2(oc)
      }
    }
    break;
  case -2:
    if(p->c==0) { sprintf(ds, "0#0.0"); oc+=sprintf(o+oc,"%s%s", s, ds); }
    else if(p->c==1) pf(s,",",v2(p)[0]);
    else {
      sprintf(ds, "%0.*g", precision, v2(p)[0]);
      if(isinf(v2(p)[0])&&v2(p)[0]>0.0) { sprintf(ds, "0i"); add0=0; }
      else if(isnan(v2(p)[0])) { sprintf(ds, "0n"); add0=0; }
      else if(isinf(v2(p)[0])&&v2(p)[0]<0.0) { sprintf(ds, "-0i"); add0=0; }
      oc+=sprintf(o+oc,"%s%s", s, ds);
      if(strchr(ds, '.')||strchr(ds,'e')) add0=0;
      for(i=1;i<p->c;i++) {
        if(oc<<1 > bs) { bs+=bs0; o=xrealloc(o,bs); }
        sprintf(ds, "%0.*g", precision, v2(p)[i]);
        if(strchr(ds, '.')||strchr(ds,'e')) add0=0;
        if(p->c == 1+i && add0) strcat(ds, ".0");
        if(isinf(v2(p)[i])&&v2(p)[i]>0.0) { sprintf(ds, "0i"); add0=0; }
        else if(isnan(v2(p)[i])) { sprintf(ds, "0n"); add0=0; }
        else if(isinf(v2(p)[i])&&v2(p)[i]<0.0) { sprintf(ds, "-0i"); add0=0; }
        oc+=sprintf(o+oc," %s", ds);
        VSIZE2(oc)
      }
    }
    break;
  case -3:
    oc+=sprintf(o+oc,"%s",s);
    if(p->c == 1) oc+=sprintf(o+oc,",");
    oc+=sprintf(o+oc,"\"");
    for(i=0;i<p->c;i++) {
      if(oc<<1 > bs) { bs+=bs0; o=xrealloc(o,bs); }
      pc(v3(p)[i]);
    }
    oc+=sprintf(o+oc,"\"");
    break;
  case -4:
    if(p->c==0) oc+=sprintf(o+oc,"%s0#`",s);
    else if(p->c==1) oc+=sprintf(o+oc,"%s,`%s", s, v4(p)[0]);
    else {
      ps(s,v4(p)[0]);
      for(i=1;i<p->c;i++) {
        if(oc<<1 > bs) { bs+=bs0; o=xrealloc(o,bs); }
        ps(" ",v4(p)[i]);
        VSIZE2(oc)
      }
    }
    break;
  case 98:
    fprintf(stderr,"%s",((ERR*)p->v)->s);
    while(--en>=0) if(!strcmp(((ERR*)p->v)->s,ee[en])) { fprintf(stderr," error"); break; }
    fprintf(stderr,"\n");
    break;
  case 99:
    oc+=sprintf(o+oc,"error: [%s] is unassigned",(char*)p->v);
    break;
  default:
    oc+=sprintf(o+oc,"error: [%d] is unexpected",p->t);
  }
  if(nl&&p->t!=98) oc+=sprintf(o+oc,"\n");
  kfree(p);
  return xstrdup(o);
}

/* normalize a mixed list if all elements are the same type */
K* knorm(K *s) {
  unsigned int i;
  char t;
  K *p;

  if(!s) return 0;
  if(!s->c) return s;
  if(s->t==11) return s; /* plist */
  t = v0(s)[0]->t;
  if(t<=0) return s;
  for(i=1;i<s->c;i++) if(t != v0(s)[i]->t) return s;

  if(t==5||t==6||t==16||t==7||t==17||t==27||t==37||t==67||t==77||t==87||t==97) return s; /* no -5 -6 -7 */

  /* convert to type t */
  p = knew(-t,s->c,0,0,0,0);
  switch(t) {
  case 1: DO(s->c, v1(p)[i]=v0(s)[i]->i) break;
  case 2: DO(s->c, v2(p)[i]=v0(s)[i]->f) break;
  case 3: DO(s->c, v3(p)[i]=v0(s)[i]->i) break;
  case 4: DO(s->c, v4(p)[i]=v0(s)[i]->v) break;
  default: printf("default in knorm:\n"); break;
  }

  kfree(s);

  return p;
}

K* kerror(char *msg) {
  return knew(98,0,msg,0,0,0);
}

/* convert to mixed list */
K* kmix(K *s) {
  K *p=0;

  if(!s->t) return kcp(s);
  else p = knew(0,s->c,0,0,0,0);

  switch(s->t) {
  case -1: DO(s->c, v0(p)[i] = k1(v1(s)[i])) break;
  case -2: DO(s->c, v0(p)[i] = k2(v2(s)[i])) break;
  case -3: DO(s->c, v0(p)[i] = k3(v3(s)[i])) break;
  case -4: DO(s->c, v0(p)[i] = k4(v4(s)[i])) break;
  default: printf("unhandled case in kmix [%d]\n", s->t); break;
  }
  return p;
}

K* kcp(K *s) {
  K *p=0;

  if(!s) return 0;
  if(s->r==-1) return s; /* constant */

  p = xmalloc(sizeof(K));
  p->t = s->t;
  p->c = s->c;
  p->v = 0;
  p->i = s->i;
  p->f = s->f;
  p->r = 0;

  switch(p->t) {
  case -1: p->v = memcpy(xmalloc(sizeof(int)*p->c), s->v, sizeof(int)*p->c); break;
  case -2: p->v = memcpy(xmalloc(sizeof(double)*p->c), s->v, sizeof(double)*p->c); break;
  case -3: p->v = xmalloc(s->c); memcpy(p->v,s->v,s->c); break;
  case -4: p->v = xmalloc(sizeof(char*)*s->c); DO(s->c, v4(p)[i] = v4(s)[i]) break;
  case 0: case 10: case 11: case 97: p->v = xmalloc(sizeof(K*)*p->c); DO(p->c, v0(p)[i] = kcp(v0(s)[i])) break;
  case 3: p->v = s->v; break;
  case 4: p->v = s->v; break;
  case 5: p->v = dcp(s->v); break;
  case 7: p->v = fncp(s->v); break;
  case 17: case 27: case 37: case 67: case 77: case 87: p->v = fncp(s->v); break;
  default: break;
  }

  return p;
}

int vname(char *s, int len) {
  int i,a=1,v=1;
  if(s[0]=='.'&&len>1) a=0;
  for(i=0;i<len;i++) {
    if(a && !isalpha(s[i])) v=0;
    else if(!isalnum(s[i]) && s[i]!='.') v=0;
    if(s[i]=='.') a=1;
    else a=0;
  }
  return v;
}

int kcmp(K *a, K *b) {
  unsigned int i,r=0,m;
  char *sa,*sb;

  if(at<bt) r=-1;
  else if(at>bt) r=1;
  else if(at==1 && a1<b1) r=-1;
  else if(at==1 && a1>b1) r=1;
  else if(at==2 && (r=CMPFFT(a2,b2)));
  else if(at==3 && a3<b3) r=-1;
  else if(at==3 && a3>b3) r=1;
  else if(at==4) r=strcmp(a4,b4);
  else if(at==5) r=dcmp(a5,b5);
  else if(at==7||at==27||at==37||at==67||at==77||at==87) {
    sa=kprint5(a,"",0,0);
    sb=kprint5(b,"",0,0);
    r=strcmp(sa,sb);
    xfree(sa); xfree(sb);
  }
  else if(at==-1) {
    for(i=0;;i++) {
      if(i==ac && i<bc) {r=-1;break;}
      else if(i==bc && i<ac) {r=1;break;}
      else if(i==ac && i==bc) break;
      else if(v1(a)[i]<v1(b)[i]){r=-1;break;}
      else if(v1(a)[i]>v1(b)[i]){r=1;break;}
    }
  }
  else if(at==-2) {
    for(i=0;;i++) {
      if(i==ac && i<bc) {r=-1;break;}
      else if(i==bc && i<ac) {r=1;break;}
      else if(i==ac && i==bc) break;
      else if((r=CMPFFT(v2(a)[i],v2(b)[i]))) break;
    }
  }
  else if(at==-3) {
    m=ac<bc?ac:bc;
    for(i=0;i<m;i++) {
      if(v3(a)[i]<v3(b)[i]){r=-1;break;}
      if(v3(a)[i]>v3(b)[i]){r=1;break;}
    }
    if(!r) {
      if(ac<bc) r=-1;
      else if(bc<ac) r=1;
    }
  }
  else if(at==-4) {
    for(i=0;;i++) {
      if(i==ac && i<bc) {r=-1;break;}
      else if(i==bc && i<ac) {r=1;break;}
      else if(i==ac && i==bc) break;
      else if(strcmp(v4(a)[i],v4(b)[i])<0){r=-1;break;}
      else if(strcmp(v4(a)[i],v4(b)[i])>0){r=1;break;}
    }
  }
  else if(at==0||at==97) {
    for(i=0;;i++) {
      if(i==ac && i<bc) {r=-1;break;}
      else if(i==bc && i<ac) {r=1;break;}
      else if(i==ac && i==bc) break;
      else if(kcmp(v0(a)[i],v0(b)[i])<0){r=-1;break;}
      else if(kcmp(v0(a)[i],v0(b)[i])>0){r=1;break;}
    }
  }

  return r;
}

uint64_t khash(K *a) {
  uint64_t r=0;
  char *s;
  switch(at) {
  case  1: r=(uint64_t)a1*2654435761; break;
  case  2: r=(uint64_t)a2*2654435761; break;
  case  3: r=(uint64_t)a3*2654435761; break;
  case  4: r=xfnv1a(a4,strlen(a4)); break;
  case  5: r=dhash(a5); break;
  case  6: case 16: r=0; break;
  case  7: case 27: case 37: case 67: case 77: case 87: s=kprint5(a,"",0,0); r=xfnv1a(s,strlen(s)); xfree(s); break;
  case  0: case 97: DO(ac,r^=khash(v0(a)[i])) break;
  case -1: DO(ac,r^=(uint64_t)v1(a)[i]*2654435761) break;
  case -2: DO(ac,r^=(uint64_t)v2(a)[i]*2654435761) break;
  case -3: DO(ac,r^=(uint64_t)v3(a)[i]*2654435761) break;
  case -4: DO(ac,r^=xfnv1a(v4(a)[i],strlen(v4(a)[i]))) break;
  default:
    fprintf(stderr,"error: unsupported type in khash()\n");
    exit(1);
  }
  return r;
}

int kreserved(char *s) {
  K *c=dkeys(C);
  DO(cc,if(!strcmp(s,v4(c)[i])){kfree(c);return 1;})
  kfree(c);
  return 0;
}
