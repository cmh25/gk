#include "av.h"
#include <string.h>
#include "fn.h"
#include "ops.h"
#include "x.h"

#define ffi (((fn*)f->v)->i)
K* avdom(K *f, K *a, char *av) {
  K *r=0;
  int n;
  char av2[32];
  av2[0]=0;
  if(av) strncpy(av2,av,32);
  if(!(n=av?strlen(av):0)) return r;
  if((r=dt2avo[ffi](a,av))) return r;
  av2[n-1]=0;
  if(av[n-1]=='\'') {
    if(at==11) {
      if(ac==1) r=eachm(f,v0(a)[0],av2); /* f'[a] */
      else r=eachparam(f,a,av2);
    }
    else r=eachm(f,a,av2);        /* f'a */
  }
  else if(av[n-1]=='/') {
    if(at==11) r=over(f,a,av2);   /* f/[a;b] */
    else if(ffi=='@'&&at<=0&&ac) r=overm(f,a,av2); /* a/1 (index converge) */
    else if(f->c==1) r=kerror("type"); /* ic/"asdf" ??? */
    else r=overd(f,a,av2);        /* f/a */
  }
  else if(av[n-1]=='\\') {
    if(at==11) r=scan(f,a,av2);   /* f\[a;b] */
    else if(ffi=='@'&&at<=0&&ac) r=scanm(f,a,av2); /* a\1 (index converge) */
    else r=scand(f,a,av2);        /* f\a */
  }
  return r;
}

K* eachm(K *f, K *a, char *av) {
  K *r=0,*am=0;
  int n;
  K*(*ff)(K*,char*)=dt1[ffi];
  if(at<0) am=kmix(a); else am=a;
  n=strlen(av);
  if(at>0) {
    if(n) r=avdom(f,am,av);
    else r=ff(am,0);
  }
  else {
    r = kv0(ac);
    if(n) DO(ac,v0(r)[i]=avdom(f,v0(am)[i],av); EC(v0(r)[i]))
    else DO(ac,v0(r)[i]=ff(v0(am)[i],0); EC(v0(r)[i]))
  }
  if(am!=a) kfree(am);
  return r->t ? r : knorm(r);
}

K* overd(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0,*am=0;
  int i,n;
  K*(*ff)(K*,K*,char*)=dt2[ffi];

  n=strlen(av);
  if(n) {
    q=kref(a); /* first */
    p=avdom(f,q,av); EC(p); /* previous */
    if(kcmp(p,q)) {
      r=avdom(f,p,av); EC(r);
      while(kcmp(r,q)&&kcmp(r,p)) {
        kfree(p);
        p=r;
        r=avdom(f,r,av); EC(r);
      }
      kfree(r);
      r=kref(p);
    } else r=kref(p);
    kfree(p);
    kfree(q);
    return r->t ? r : knorm(r);
  }

  if(!ac&&at<=0) {
    if(strchr("+*&|",ffi)&&!ac&&at<=0) {
      switch(at) {
      case  0:
      case -1:
        if(ffi=='+') return k1(0);
        else if(ffi=='*') return k1(1);
        else if(ffi=='&') return k1(1);
        else if(ffi=='|') return k1(0);
        else if(at==1&&ffi=='=') return k1(1);
        break;
      case -2:
        if(ffi=='+') return k2(0);
        else if(ffi=='*') return k2(1);
        else if(ffi=='&') return k2(INFINITY);
        else if(ffi=='|') return k2(-INFINITY);
        else if(at==1&&ffi=='=') return k1(1);
        break;
      case -3:
        if(ffi=='&') return k3(0);
        else if(ffi=='|') return k3(0);
        else return kerror("type");
        break;
      default: return kerror("type");
      }
    }
    else if(ffi==',') return kref(a);
    else return kerror("length");
  }

  if(at<0) am=kmix(a);
  else am=a;
  r = am->c ? kref(v0(am)[0]) : kref(am);
  for(i=1;i<am->c;i++) {
    p=ff(r,v0(am)[i],0); EC(p);
    kfree(r);
    r=p;
  }
  if(am!=a) kfree(am);
  return r->t ? r : knorm(r);
}

K* scand(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0,*am=0,*sr=0;
  int i,j=0,n;
  K*(*ff)(K*,K*,char*)=dt2[ffi];

  n=strlen(av);
  if(n) {
    q=kref(a); /* first */
    p=avdom(f,q,av); EC(p); /* previous */
    sr=kv0(32);
    v0(sr)[j++]=q;
    if(kcmp(p,q)) {
      v0(sr)[j++]=p;
      r=avdom(f,p,av); EC(r);
      while(kcmp(r,q)&&kcmp(r,p)) {
        if(j==sr->c) { sr->c<<=1; sr->v=xrealloc(sr->v,sizeof(K*)*sr->c); }
        v0(sr)[j++]=r;
        p=r;
        r=avdom(f,p,av); EC(r);
      }
    }
    kfree(r);
    sr->c=j;
    r=sr;
    return r->t ? r : knorm(r);
  }

  if(strchr("+*&|",ffi)&&at<=0&&!ac) {
    switch(at) {
    case  0: r=kv0(0); break;
    case -1: r=kv1(0); break;
    case -2: r=kv2(0); break;
    case -3: r=kv3(0); break;
    case -4: r=kv4(0); break;
    }
    return r;
  }
  if(at==-1&&ac==0&&ffi=='=') return kv1(0);

  if(at>0) return kref(a);
  if(!ac) return kv0(0);
  if(at<0) am=kmix(a);
  else am=a;
  r=kv0(ac);
  v0(r)[0]=kref(v0(am)[0]);
  for(i=1;i<am->c;i++) { v0(r)[i]=ff(v0(r)[i-1],v0(am)[i],0); EC(v0(r)[i]); }
  if(am!=a) kfree(am);
  return r->t ? r : knorm(r);
}

K* over(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0;
  int i,m=1;
  K*(*ff)(K*,K*,char*)=dt2[ffi];

  if(at!=11) return kerror("type");
  if(ac!=2) return kerror("valence");

  q=kv0(ac); /* list of sets of parameters */
  v0(q)[0]=kref(v0(a)[0]);
  for(i=1;i<ac;i++) {
    p=v0(a)[i];
    if(m==1&&p->t<=0&&m<p->c) m=p->c;
    else if(p->t<=0&&m!=p->c) return kerror("valence");
    else if(p->t==16) return kerror("valence"); /* TODO: projection */
    if(p->t<0) v0(q)[i]=kmix(p);
    else v0(q)[i]=kref(p);
  }
  if(m==1) r=ff(v0(a)[0],v0(a)[1],av);
  else {
    r=kref(v0(a)[0]);
    for(i=0;i<m;i++) {
      p=r;
      if(v0(q)[1]->t>0) r=ff(p,v0(q)[1],av);
      else r=ff(p,v0(v0(q)[1])[i],av);
      kfree(p);
      EC(r);
    }
  }
  kfree(q);

  return r;
}

K* scan(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0;
  int i,m=1;
  K*(*ff)(K*,K*,char*)=dt2[ffi];

  if(at!=11) return kerror("type");
  if(ac!=2) return kerror("valence");

  q=kv0(ac); /* list of sets of parameters */
  v0(q)[0]=kref(v0(a)[0]);
  for(i=1;i<ac;i++) {
    p=v0(a)[i];
    if(m==1&&p->t<=0&&m<p->c) m=p->c;
    else if(p->t<=0&&m!=p->c) return kerror("valence");
    else if(p->t==16) return kerror("valence"); /* TODO: projection */
    if(p->t<0) v0(q)[i]=kmix(p);
    else v0(q)[i]=kref(p);
  }
  if(m==1) r=ff(v0(a)[0],v0(a)[1],av);
  else {
    r=kv0(m+1);
    v0(r)[0]=kref(v0(a)[0]);
    for(i=0;i<m;i++) {
      if(v0(q)[1]->t>0) v0(r)[i+1]=ff(v0(r)[i],v0(q)[1],av);
      else v0(r)[i+1]=ff(v0(r)[i],v0(v0(q)[1])[i],av);
      EC(v0(r)[i+1]);
    }
  }
  kfree(q);

  return r;
}

/* only for index converge */
K* overm(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0;
  K*(*ff)(K*,K*,char*)=dt2[ffi];

  q=v0(a)[1];                 /* first */
  p=ff(v0(a)[0],v0(a)[1],av); /* previous */
  if(kcmp(p,q)) {
    r=ff(v0(a)[0],p,av); EC(r);
    while(kcmp(r,q)&&kcmp(r,p)) {
      kfree(p);
      p=r;
      r=ff(v0(a)[0],p,av); EC(r);
    }
  } else r=kref(p);
  kfree(r);
  r=p;
  return r->t ? r : knorm(r);
}

/* only for index converge */
K* scanm(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0,*sr=0;
  int j=0;
  K*(*ff)(K*,K*,char*)=dt2[ffi];

  q=v0(a)[1];                 /* first */
  p=ff(v0(a)[0],v0(a)[1],av); EC(p); /* previous */
  if(p==q) kfree(q);
  sr=kv0(32);
  v0(sr)[j++]=kref(q);
  if(kcmp(p,q)) {
    v0(sr)[j++]=p;
    r=ff(v0(a)[0],p,av); EC(r);
    while(kcmp(r,q)&&kcmp(r,p)) {
      if(j==sr->c) { sr->c<<=1; sr->v=xrealloc(sr->v,sizeof(K*)*sr->c); }
      v0(sr)[j++]=r;
      p=r;
      r=ff(v0(a)[0],p,av); EC(r);
    }
  }
  kfree(r);
  sr->c=j;
  r=sr;

  return r->t ? r : knorm(r);
}

K* eachparam(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0;
  int i,m;
  if(at!=11) return kerror("type");
  if(ac!=2) return kerror("valence");
  p = v0(a)[0]->t<0 ? kmix(v0(a)[0]) : v0(a)[0];
  q = v0(a)[1]->t<0 ? kmix(v0(a)[1]) : v0(a)[1];
  if(p->t==16) return kerror("valence");  /* TODO: projection */
  if(q->t==16) return kerror("valence");  /* TODO: projection */
  m=p->c<q->c?q->c:p->c;
  r=kv0(m);
  for(i=0;i<m;i++) { v0(r)[i]=dt2[ffi](p->t>0?p:v0(p)[i],q->t>0?q:v0(q)[i],av); EC(v0(r)[i]); }
  if(p!=v0(a)[0]) kfree(p);
  if(q!=v0(a)[1]) kfree(q);
  return r->t ? r : knorm(r);
}

K* avdo(K *f, K *a, K *b, char *av) {
  K *r=0;
  int n;
  char av2[32];
  av2[0]=0;
  if(av) strncpy(av2,av,32);
  if(!(n=av?strlen(av):0)) return r;
  if((r=dt2avo2[ffi](a,b,av))) return r;
  av2[n-1]=0;
  //if(av[n-1]=='\'') r=eachprior(f,a,b,av2);     /* a f'b */
  if(av[n-1]=='\'') r=slide(f,a,b,av2);     /* a f'b */
  else if(av[n-1]=='/') r=eachright(f,a,b,av2); /* a f/b */
  else if(av[n-1]=='\\') r=eachleft(f,a,b,av2); /* a f\b */
  return r;
}

K* eachprior(K *f, K *a, K *b, char *av) {
  K *r=0,*bm=0;
  int n;
  K*(*ff)(K*,K*,char*)=dt2[ffi];
  if(bt<=0&&!bc) return kv0(0);
  if(at<=0&&!ac) return kv0(0);
  if(bt<0) bm=kmix(b); else bm=b;

  n=strlen(av);
  if(bt>0) {
    if(n) r=avdo(f,b,a,av);
    else r=ff(b,a,av);
  }
  else {
    r = kv0(bc);
    if(n) {
      v0(r)[0]=avdo(f,v0(bm)[0],a,av); EC(v0(r)[0]);
      DO(bc-1, v0(r)[i+1]=avdo(f,v0(bm)[i+1],v0(bm)[i],av); EC(v0(r)[i+1]))
    }
    else {
      v0(r)[0]=ff(v0(bm)[0],a,0); EC(v0(r)[0]);
      DO(bc-1, v0(r)[i+1]=ff(v0(bm)[i+1],v0(bm)[i],0); EC(v0(r)[i+1]))
    }
  }

  if(bm!=b) kfree(bm);
  return r->t ? r : knorm(r);
}

K* slide(K *f, K *a, K *b, char *av) {
  K *r=0,*bm=0;
  int n,d,s,v,m;;
  K*(*ff)(K*,K*,char*)=dt2[ffi];
  if(at!=1) return kerror("type");
  if(bt<=0&&!bc) return kv0(0);
  if(at<=0&&!ac) return kv0(0);
  if(bt<0) bm=kmix(b); else bm=b;

  d=a1;
  s=abs(d);
  v=2;
  m=(bc-v+s)/s;
  if((m-1)*s+v!=bm->c) return kerror("valence");

  n=strlen(av);
  if(bt>0) {
    if(n) r=avdo(f,b,a,av);
    else r=ff(b,a,av);
  }
  else {
    r = kv0(m);
    if(n) {
      v0(r)[0]=avdo(f,v0(bm)[0],a,av); EC(v0(r)[0]);
      DO(bc-1, v0(r)[i+1]=avdo(f,v0(bm)[i+1],v0(bm)[i],av); EC(v0(r)[i+1]))
    }
    else if(d<0) {
      DO(m, v0(r)[i]=ff(v0(bm)[i*s+1],v0(bm)[i*s],0); EC(v0(r)[i]))
    }
    else if(d>0) {
      DO(m, v0(r)[i]=ff(v0(bm)[i*s],v0(bm)[i*s+1],0); EC(v0(r)[i]))
    }
  }

  if(bm!=b) kfree(bm);
  return r->t ? r : knorm(r);
}

K* eachright(K *f, K *a, K *b, char *av) {
  K *r=0,*bm=0;
  int n;
  K*(*ff)(K*,K*,char*)=dt2[ffi];
  if(bt<0) bm=kmix(b); else bm=b;
  n=strlen(av);
  if(bt>0) {
    if(n) r=avdo(f,a,bm,av);
    else r=ff(a,bm,0);
  }
  else {
    r = kv0(bc);
    if(n) DO(bc,v0(r)[i]=avdo(f,a,v0(bm)[i],av);EC(v0(r)[i]))
    else DO(bc,v0(r)[i]=ff(a,v0(bm)[i],0);EC(v0(r)[i]))
  }
  if(bm!=b) kfree(bm);
  return r->t ? r : knorm(r);
}

K* eachleft(K *f, K *a, K *b, char *av) {
  K *r=0,*am=0;
  int n;
  K*(*ff)(K*,K*,char*)=dt2[ffi];
  if(at<0) am=kmix(a); else am=a;
  n=strlen(av);
  if(at>0) {
    if(n) r=avdo(f,am,b,av);
    else r=ff(am,b,0);
  }
  else {
    r = kv0(ac);
    if(n) DO(ac,v0(r)[i]=avdo(f,v0(am)[i],b,av);EC(v0(r)[i]))
    else DO(ac,v0(r)[i]=ff(v0(am)[i],b,0);EC(v0(r)[i]))
  }
  if(am!=a) kfree(am);
  return r->t ? r : knorm(r);
}

K* avdo37(K *f, K *a, char *av) {
  K *r=0;
  int n;
  char av2[32];
  fn *ff=f->v;
  if(!ff->s_) f=fnd(f);
  av2[0]=0;
  if(av) strncpy(av2,av,32);
  if(!(n=av?strlen(av):0)) return r;
  av2[n-1]=0;
  SG2(a); if(a->t==98)return a;
  if(ff->v==1) {
    if(av[n-1]=='\'') {
      if(at==11) r=eachm37(f,v0(a)[0],av2);    /* {x}'[a] */
      else r=eachm37(f,a,av2);                 /* {x}'a */
    }
    else if(av[n-1]=='/') r=overm37(f,a,av2);  /* {x}/a */
    else if(av[n-1]=='\\') r=scanm37(f,a,av2); /* {x}\a */
  }
  else if(ff->v==2) {
    if(av[n-1]=='\'') {
      if(at==11) r=eachparam37(f,a,av2); /* {x+y}'[a;b] */
      else r=eachprior37(f,a,av2);       /* {x+y}'a */
    }
    else if(av[n-1]=='/') {
      if(at==11) r=over37(f,a,av2);      /* {x+y}/[a;b] */
      else r=overd37(f,a,av2);           /* {x+y}/a */
    }
    else if(av[n-1]=='\\') {
      if(at==11) r=scan37(f,a,av2);      /* {x+y}\[a;b] */
      else r=scand37(f,a,av2);           /* {x+y}\a */
    }
  }
  else {
    if(av[n-1]=='\'') {
      if(at==11) r=eachparam37(f,a,av2); /* {x+y+z}'[a;b;c] */
      else r=kerror("type");
    }
    else if(av[n-1]=='/') {
      if(at==11) r=over37(f,a,av2);      /* {x+y+z}/[a;b;c] */
      else r=kerror("type");
    }
    else if(av[n-1]=='\\') {
      if(at==11) r=scan37(f,a,av2);      /* {x+y+z}\[a;b;c] */
      else r=kerror("type");
    }
  }
  kfree(a);
  return r;
}

K* avdo37infix(K *f, K *a, char *av) {
  K *r=0;
  int n;
  char av2[32];
  fn *ff=f->v;
  if(!ff->s_) f=fnd(f);
  av2[0]=0;
  if(av) strncpy(av2,av,32);
  if(!(n=av?strlen(av):0)) return r;
  av2[n-1]=0;
  if(a&&!at) DO(ac,SG2(v0(a)[i]);if(v0(a)[i]->t==98)return v0(a)[i])
  SG2(a); if(a->t==98)return a;
  if(ff->v==1) {
    if(av[n-1]=='\'') r=slide37infix(f,a,av2);       /* a {x}' b */
    if(av[n-1]=='/') r=overm37infix(f,a,av2);       /* a {x}/ b */
    else if(av[n-1]=='\\') r=scanm37infix(f,a,av2); /* a {x}\ b */
  }
  else {
    if(av[n-1]=='\'') r=slide37infix(f,a,av2);       /* a {x+y}' b */
    //if(av[n-1]=='\'') r=eachprior37(f,a,av2);       /* a {x+y}' b */
    else if(av[n-1]=='/') r=eachright37(f,a,av2);   /* a {x+y}/ b */
    else if(av[n-1]=='\\') r=eachleft37(f,a,av2);   /* a {x+y}\ b */
  }
  kfree(a);
  if(a&&!at) DO(ac,kfree(v0(a)[i]))
  return r;
}

K* eachright37(K *f, K *a, char *av) {
  K *r=0,*am=0,*bm=0,*q=0;
  int n;
  K* (*ff)(K*,K*,char*);
  am = v0(a)[0]->t<0 ? kmix(v0(a)[0]) : kref(v0(a)[0]);
  bm = v0(a)[1]->t<0 ? kmix(v0(a)[1]) : kref(v0(a)[1]);
  n = strlen(av);
  ff = n ? avdo37infix : fne2;
  q = kv0(2); q->t=11;
  if(bm->t>0) {
    v0(q)[0]=am;
    v0(q)[1]=bm;
    r=ff(f,q,av); EC(r);
  }
  else {
    r=kv0(bm->c);
    v0(q)[0]=am;
    DO(bm->c,v0(q)[1]=v0(bm)[i];v0(r)[i]=ff(f,q,av);EC(v0(r)[i]))
  }
  q->c=0;
  kfree(q);
  kfree(am);
  kfree(bm);
  return r->t ? r : knorm(r);
}

K* eachleft37(K *f, K *a, char *av) {
  K *r=0,*am=0,*bm=0,*q=0;
  int n;
  K* (*ff)(K*,K*,char*);
  am = v0(a)[0]->t<0 ? kmix(v0(a)[0]) : kref(v0(a)[0]);
  bm = v0(a)[1]->t<0 ? kmix(v0(a)[1]) : kref(v0(a)[1]);
  n = strlen(av);
  ff = n ? avdo37infix : fne2;
  q = kv0(2); q->t=11;
  if(am->t>0) {
    v0(q)[0]=am;
    v0(q)[1]=bm;
    r=ff(f,q,av);
  }
  else {
    r=kv0(am->c);
    v0(q)[1]=bm;
    DO(am->c,v0(q)[0]=v0(am)[i];v0(r)[i]=ff(f,q,av);EC(v0(r)[i]))
  }
  q->c=0;
  kfree(q);
  kfree(am);
  kfree(bm);
  return r->t ? r : knorm(r);
}

K* eachparam37(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0;
  int m=1;
  if(ac!=((fn*)f->v)->v) return kerror("valence");
  q=kv0(ac); /* list of sets of parameters */
  DO(ac, p=v0(a)[i];
         if(m==1&&p->t<=0&&m<p->c) m=p->c;
         else if(p->t<=0&&m!=p->c) return kerror("valence");
         else if(p->t==6) return kerror("valence"); /* TODO: projection */
         if(p->t<0) v0(q)[i]=kmix(p);
         else v0(q)[i]=kref(p))
  r=kv0(m);
  p=kv0(ac); p->t=11;
  DO(rc,DO2(p->c,v0(p)[j]= v0(q)[j]->t ? v0(q)[j] : v0(v0(q)[j])[i]);
                 v0(r)[i]=fne2(f,p,0);EC(v0(r)[i]))
  p->c=0; kfree(p);
  kfree(q);
  if(m==1) { p=v0(r)[0]; r->c=0; kfree(r); r=p; }
  return r->t ? r : knorm(r);
}

K* eachm37(K *f, K *a, char *av) {
  K *r=0,*am=0;
  int n;
  K* (*ff)(K*,K*,char*);
  if(at<0) am=kmix(a); else am=a;
  n=strlen(av);
  ff = n ? avdo37 : fne2;
  if(at>0) r=ff(f,am,av);
  else {
    r = kv0(ac);
    DO(ac,v0(r)[i]=ff(f,v0(am)[i],av);EC(v0(r)[i]))
  }
  if(am!=a) kfree(am);
  return r->t ? r : knorm(r);
}

K* over37(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0;
  int i,j,m=1;

  if(at!=11) return kerror("type");
  if(ac!=((fn*)f->v)->v) return kerror("valence");

  q=kv0(ac); /* list of sets of parameters */
  v0(q)[0]=kref(v0(a)[0]);
  for(i=1;i<ac;i++) {
    p=v0(a)[i];
    if(m==1&&p->t<=0&&m<p->c) m=p->c;
    else if(p->t<=0&&m!=p->c) return kerror("valence");
    else if(p->t==16) return kerror("valence"); /* TODO: projection */
    if(p->t<0) v0(q)[i]=kmix(p);
    else v0(q)[i]=kref(p);
  }
  if(m==1) r=fne2(f,a,av);
  else {
    p=kv0(ac); p->t=11;
    r=kref(v0(a)[0]);
    v0(p)[0]=null;
    for(i=0;i<m;i++) {
      kfree(v0(p)[0]);
      v0(p)[0]=kref(r);
      for(j=1;j<ac;j++)
        v0(p)[j]=v0(q)[j]->t>0?v0(q)[j]:v0(v0(q)[j])[i];
      kfree(r);
      r=fne2(f,p,av); EC(r);
    }
    kfree(v0(p)[0]);
    p->c=0; kfree(p);
  }
  kfree(q);

  return r->t ? r : knorm(r);
}

K* scan37(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0;
  int i,j,m=1;

  if(at!=11) return kerror("type");
  if(ac!=((fn*)f->v)->v) return kerror("valence");

  q=kv0(ac); /* list of sets of parameters */
  v0(q)[0]=kref(v0(a)[0]);
  for(i=1;i<ac;i++) {
    p=v0(a)[i];
    if(m==1&&p->t<=0&&m<p->c) m=p->c;
    else if(p->t<=0&&m!=p->c) return kerror("valence");
    else if(p->t==16) return kerror("valence"); /* TODO: projection */
    if(p->t<0) v0(q)[i]=kmix(p);
    else v0(q)[i]=kref(p);
  }
  if(m==1) r=fne2(f,a,av);
  else {
    r=kv0(m+1);
    p=kv0(ac); p->t=11;
    v0(r)[0]=kref(v0(a)[0]);
    for(i=0;i<m;i++) {
      v0(p)[0]=v0(r)[i];
      for(j=1;j<ac;j++)
        v0(p)[j]=v0(q)[j]->t>0?v0(q)[j]:v0(v0(q)[j])[i];
      v0(r)[i+1]=fne2(f,p,av);EC(v0(r)[i+1]);
    }
    p->c=0; kfree(p);
  }
  kfree(q);

  return r->t ? r : knorm(r);
}

K* overm37(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0;

  q=a;            /* first */
  p=fne2(f,q,av); EC(p); /* previous */
  if(kcmp(p,q)) {
    r=fne2(f,p,av); EC(r);
    while(kcmp(r,q)&&kcmp(r,p)) {
      kfree(p);
      p=r;
      r=fne2(f,r,av); EC(r);
    }
  } else r=kref(p);
  kfree(r);
  r=p;
  return r->t ? r : knorm(r);
}

K* scanm37(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0,*sr=0;
  int j=0;

  q=a;            /* first */
  p=fne2(f,q,av); EC(p); /* previous */
  if(p==q) kfree(q);
  sr=kv0(32);
  v0(sr)[j++]=kref(q);
  if(kcmp(p,q)) {
    v0(sr)[j++]=p;
    r=fne2(f,p,av); EC(r);
    while(kcmp(r,q)&&kcmp(r,p)) {
      if(j==sr->c) { sr->c<<=1; sr->v=xrealloc(sr->v,sizeof(K*)*sr->c); }
      v0(sr)[j++]=r;
      p=r;
      r=fne2(f,p,av); EC(r);
    }
  }
  kfree(r);
  sr->c=j;
  r=sr;

  return r->t ? r : knorm(r);
}

K* overm37infix(K *f, K *a, char *av) {
  K *r=0,*p=0,*b=0;
  int i;

  if(v0(a)[0]->t==1) { /* do */
    r=kref(v0(a)[1]);
    for(i=0;i<v0(a)[0]->i;i++) {
      p=fne2(f,r,av); EC(p);
      kfree(r);
      r=p;
    }
  }
  else if(v0(a)[0]->t==37) { /* while */
    r=kref(v0(a)[1]);
    b=fne2(v0(a)[0],r,av); EC(b);
    for(i=0;bt==1&&b->i;i++) {
      kfree(b);
      p=fne2(f,r,av); EC(p);
      b=fne2(v0(a)[0],p,av); EC(b);
      kfree(r);
      r=p;
    }
    kfree(b);
  }
  else return kerror("type");

  return r->t ? r : knorm(r);
}

K* scanm37infix(K *f, K *a, char *av) {
  K *r=0,*sr=0,*b=0;
  int i,j=0;

  if(v0(a)[0]->t==1) { /* do */
    r=kv0(v0(a)[0]->i+1);
    v0(r)[0]=kref(v0(a)[1]);
    DO(v0(a)[0]->i, v0(r)[i+1]=fne2(f,v0(r)[i],av);EC(v0(r)[i+1]))
  }
  else if(v0(a)[0]->t==37) { /* while */
    sr=kv0(32);
    v0(sr)[j++]=kref(v0(a)[1]);
    b=fne2(v0(a)[0],v0(sr)[0],av);
    for(i=0;bt==1&&b->i;i++) {
      kfree(b);
      if(j==sr->c) { sr->c<<=1; sr->v=xrealloc(sr->v,sizeof(K*)*sr->c); }
      v0(sr)[i+1]=fne2(f,v0(sr)[i],av);EC(v0(sr)[i+1]);
      b=fne2(v0(a)[0],v0(sr)[i+1],av); EC(b);
    }
    kfree(b);
    sr->c=i+1;
    r=sr;
  }
  else return kerror("type");

  return r->t ? r : knorm(r);
}

K* eachprior37(K *f, K *a, char *av) {
  K *r=0,*am=0,*p=0,*q=0;
  int i,n;
  K* (*ff)(K*,K*,char*);

  if(!at&&!ac) return kv0(0); /* {x,y}'() */

  if(at>0&&at!=11) {
    r=kv0(2);
    v0(r)[0]=kref(a);
    v0(r)[1]=kref(a);
    return knorm(r);
  }
  else if((!at||at==11)&&ac==2) {
    if(v0(a)[0]->t>0) {
      p=join2_(v0(a)[0],v0(a)[1],0);
      if(p->t<0) am=kmix(p); else am=p;
    }
    else { /* 1 2 {x,y}' 3 4 */
      p=enlist_(v0(a)[0],0);
      q=v0(a)[1]->t<0?kmix(v0(a)[1]):kref(v0(a)[1]);
      am=join2_(p,q,0);
      kfree(p);
      kfree(q);
      p=0;
    }
  }
  else if(at<0) am=kmix(a); else am=a;

  n=strlen(av);
  ff=n?avdo37:fne2;
  r=kv0(am->c-1);
  q=kv0(2); q->t=11;
  for(i=0;i<am->c-1;i++) {
    v0(q)[0]=v0(am)[i+1];
    v0(q)[1]=v0(am)[i];
    v0(r)[i]=ff(f,q,av); EC(v0(r)[i]);
  }
  q->c=0; kfree(q);

  if(am->c==2) { /* if there were only two args, not a list */
    q=r;
    r=kref(v0(r)[0]);
    kfree(q);
  }

  if(am!=a) kfree(am);
  if(p&&p!=am) kfree(p);
  return r->t ? r : knorm(r);
}

K* slide37infix(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0,*qm=0,*t=0;
  int i,j,n,d,m,v,s;
  K* (*ff)(K*,K*,char*);

  n=strlen(av);
  ff=n?avdo37:fne2;

  p=v0(a)[0];
  q=v0(a)[1];
  if(p->t!=1) return kerror("type");

  if(q->t<0) qm=kmix(q); else qm=q;

  d=p->i;
  s=abs(d);
  v=((fn*)f->v)->v;
  m=(qm->c-v+s)/s;
  if(m<1) return kerror("length");
  if(qm->t&&(m-1)*s+v!=1) return kerror("valence");   /* atom */
  else if((m-1)*s+v!=qm->c) return kerror("valence"); /* list */

  r = kv0(m);
  if(d<0) {
    for(i=0;i<m;i++) {
      t=kv0(v); t->t=11;
      for(j=0;j<v;j++) v0(t)[j]=v0(qm)[i*s+v-j-1];
      v0(r)[i]=ff(f,t,av);EC(v0(r)[i]);
      t->c=0;
      kfree(t);
    }
  }
  else if(d>0) {
    for(i=0;i<m;i++) {
      t=kv0(v); t->t=11;
      for(j=0;j<v;j++) v0(t)[j]=v0(qm)[i*s+j];
      v0(r)[i]=ff(f,t,av); EC(v0(r)[i]);
      t->c=0;
      kfree(t);
    }
  }

  if(qm!=q) kfree(qm);
  return r->t ? r : knorm(r);
}

K* overd37(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0,*am=0;
  int i,n;

  n=strlen(av);
  if(n) {
    q=kref(a); /* first */
    p=avdo37(f,q,av); EC(p); /* previous */
    if(kcmp(p,q)) {
      r=avdo37(f,p,av); EC(r);
      while(kcmp(r,q)&&kcmp(r,p)) {
        kfree(p);
        p=r;
        r=avdo37(f,r,av); EC(r);
      }
      kfree(r);
      r=kref(p);
    } else r=kref(p);
    kfree(p);
    kfree(q);
    return r->t ? r : knorm(r);
  }

  if(at<0) am=kmix(a);
  else am=a;
  r = am->c ? kref(v0(am)[0]) : kref(am);
  for(i=1;i<am->c;i++) {
    q=kv0(2); v0(q)[0]=kref(r); v0(q)[1]=kref(v0(am)[i]); q->t=11;
    p=fne2(f,q,0); EC(p);
    kfree(q);
    kfree(r);
    r=p;
  }
  if(am!=a) kfree(am);
  return r->t ? r : knorm(r);
}

K* scand37(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0,*am=0,*sr=0;
  int i,j=0,n;

  n=strlen(av);
  if(n) {
    q=kref(a); /* first */
    p=avdo37(f,q,av); EC(p); /* previous */
    sr=kv0(32);
    v0(sr)[j++]=q;
    if(kcmp(p,q)) {
      v0(sr)[j++]=p;
      r=avdo37(f,p,av); EC(r);
      while(kcmp(r,q)&&kcmp(r,p)) {
        if(j==sr->c) { sr->c<<=1; sr->v=xrealloc(sr->v,sizeof(K*)*sr->c); }
        v0(sr)[j++]=r;
        p=r;
        r=avdo37(f,p,av); EC(r);
      }
    }
    kfree(r);
    sr->c=j;
    r=sr;
    return r->t ? r : knorm(r);
  }

  if(!ac) return kref(a);
  if(at<0) am=kmix(a);
  else am=a;
  r=kv0(ac);
  v0(r)[0]=kref(v0(am)[0]);
  for(i=1;i<am->c;i++) {
    q=kv0(2); q->t=11;
    v0(q)[0]=kref(v0(r)[i-1]);
    v0(q)[1]=kref(v0(am)[i]);
    v0(r)[i]=fne2(f,q,0); EC(v0(r)[i]);
    kfree(q);
  }
  if(am!=a) kfree(am);
  return r->t ? r : knorm(r);
}

K* each(K*(*f)(K*,K*,char*), K *a, K *b, char *av) {
  K *r=0,*am=0,*bm=0;
  if(a) {
    if(!b) return kerror("valence");
    if(at<=0 && bt<=0 && ac!=bc) return kerror("length");

    if(at<0) am=kmix(a); else am=a;
    if(bt<0) bm=kmix(b); else bm=b;

    if(at<=0 && bt<=0) {
      r=kv0(ac);
      if(bt<=0) DO(ac, v0(r)[i]=f(v0(am)[i],v0(bm)[i],av);EC(v0(r)[i]))
    }
    else if(at<=0 && bt>0) {
      r=kv0(ac);
      DO(ac, v0(r)[i]=f(v0(am)[i],bm,av);EC(v0(r)[i]))
    }
    else if(at>0 && bt<=0) {
      r=kv0(bc);
      DO(bc, v0(r)[i]=f(am,v0(bm)[i],av);EC(v0(r)[i]))
    }
    else r=f(am,bm,av);

    if(a!=am) kfree(am);
    if(b!=bm) kfree(bm);
  }
  else {
    r=kv0(bc);
    if(bt<0) bm=kmix(b); else bm=b;
    DO(bc, v0(r)[i]=f(0,v0(bm)[i],av);EC(v0(r)[i]))
    if(b!=bm) kfree(bm);
  }
  return r->t ? r : knorm(r);
}

K* avdomfc(K *f, K *a, char *av) {
  K *r=0;
  int n;
  char av2[32];
  av2[0]=0;
  if(av) strncpy(av2,av,32);
  if(!(n=av?strlen(av):0)) return r;
  av2[n-1]=0;
  if(av[n-1]=='\'') {
    if(at==11) {
      if(ac==1) r=eachmfc(f,v0(a)[0],av2); /* f'[a] */
      else r=eachparamfc(f,a,av2);
    }
    else r=eachmfc(f,a,av2);        /* f'a */
  }
  else if(av[n-1]=='/') {
    if(at==11) r=overfc(f,a,av2);   /* f/[a;b] */
    else if(f->c==1) r=kerror("type"); /* ic/"asdf" ??? */
    else r=overdfc(f,a,av2);        /* f/a */
  }
  else if(av[n-1]=='\\') {
    if(at==11) r=scanfc(f,a,av2);   /* f\[a;b] */
    else r=scandfc(f,a,av2);        /* f\a */
  }
  return r;
}

K* eachmfc(K *f, K *a, char *av) {
  K *r=0,*am=0;
  int n;
  if(at<0) am=kmix(a); else am=a;
  n=strlen(av);
  if(at>0) {
    if(n) r=avdomfc(f,am,av);
    else r=applyfc1_(f,am,0);
  }
  else {
    r = kv0(ac);
    if(n) DO(ac,v0(r)[i]=avdomfc(f,v0(am)[i],av);EC(v0(r)[i]))
    else DO(ac,v0(r)[i]=applyfc1_(f,v0(am)[i],0);EC(v0(r)[i]))
  }
  if(am!=a) kfree(am);
  return r->t ? r : knorm(r);
}

K* overdfc(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0,*am=0;
  int i,n;

  n=strlen(av);
  if(n) {
    q=kref(a); /* first */
    p=avdomfc(f,q,av); EC(p); /* previous */
    if(kcmp(p,q)) {
      r=avdomfc(f,p,av); EC(r);
      while(kcmp(r,q)&&kcmp(r,p)) {
        kfree(p);
        p=r;
        r=avdomfc(f,r,av); EC(r);
      }
      kfree(r);
      r=kref(p);
    } else r=kref(p);
    kfree(p);
    kfree(q);
    return r->t ? r : knorm(r);
  }

  if(at<0) am=kmix(a);
  else am=a;
  r = am->c ? kref(v0(am)[0]) : kref(am);
  for(i=1;i<am->c;i++) {
    p=applyfc2_(f,r,v0(am)[i],0); EC(p);
    kfree(r);
    r=p;
  }
  if(am!=a) kfree(am);
  return r->t ? r : knorm(r);
}

K* scandfc(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0,*am=0,*sr=0;
  int i,j=0,n;

  n=strlen(av);
  if(n) {
    q=kref(a); /* first */
    p=avdomfc(f,q,av); EC(p); /* previous */
    sr=kv0(32);
    v0(sr)[j++]=q;
    if(kcmp(p,q)) {
      v0(sr)[j++]=p;
      r=avdomfc(f,p,av); EC(r);
      while(kcmp(r,q)&&kcmp(r,p)) {
        if(j==sr->c) { sr->c<<=1; sr->v=xrealloc(sr->v,sizeof(K*)*sr->c); }
        v0(sr)[j++]=r;
        p=r;
        r=avdomfc(f,p,av); EC(r);
      }
    }
    kfree(r);
    sr->c=j;
    r=sr;
    return r->t ? r : knorm(r);
  }

  if(!ac) return kref(a);
  if(at<0) am=kmix(a);
  else am=a;
  r=kv0(ac);
  v0(r)[0]=kref(v0(am)[0]);
  for(i=1;i<am->c;i++) { v0(r)[i]=applyfc2_(f,v0(r)[i-1],v0(am)[i],0); EC(v0(r)[i]); }
  if(am!=a) kfree(am);
  return r->t ? r : knorm(r);
}

K* overfc(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0;
  int i,m=1;

  if(at!=11) return kerror("type");
  if(ac!=2) return kerror("valence");

  q=kv0(ac); /* list of sets of parameters */
  v0(q)[0]=kref(v0(a)[0]);
  for(i=1;i<ac;i++) {
    p=v0(a)[i];
    if(m==1&&p->t<=0&&m<p->c) m=p->c;
    else if(p->t<=0&&m!=p->c) return kerror("valence");
    else if(p->t==16) return kerror("valence"); /* TODO: projection */
    if(p->t<0) v0(q)[i]=kmix(p);
    else v0(q)[i]=kref(p);
  }
  if(m==1) r=applyfc2_(f,v0(a)[0],v0(a)[1],0);
  else {
    r=kref(v0(a)[0]);
    for(i=0;i<m;i++) {
      p=r;
      if(v0(q)[1]->t>0) r=applyfc2_(f,p,v0(q)[1],0);
      else r=applyfc2_(f,p,v0(v0(q)[1])[i],0);
      EC(r);
      kfree(p);
    }
  }
  kfree(q);

  return r;
}

K* scanfc(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0;
  int i,m=1;

  if(at!=11) return kerror("type");
  if(ac!=2) return kerror("valence");

  q=kv0(ac); /* list of sets of parameters */
  v0(q)[0]=kref(v0(a)[0]);
  for(i=1;i<ac;i++) {
    p=v0(a)[i];
    if(m==1&&p->t<=0&&m<p->c) m=p->c;
    else if(p->t<=0&&m!=p->c) return kerror("valence");
    else if(p->t==16) return kerror("valence"); /* TODO: projection */
    if(p->t<0) v0(q)[i]=kmix(p);
    else v0(q)[i]=kref(p);
  }
  if(m==1) r=applyfc2_(f,v0(a)[0],v0(a)[1],0);
  else {
    r=kv0(m+1);
    v0(r)[0]=kref(v0(a)[0]);
    for(i=0;i<m;i++) {
      if(v0(q)[1]->t>0) v0(r)[i+1]=applyfc2_(f,v0(r)[i],v0(q)[1],0);
      else v0(r)[i+1]=applyfc2_(f,v0(r)[i],v0(v0(q)[1])[i],0);
      EC(v0(r)[i+1]);
    }
  }
  kfree(q);

  return r;
}

K* eachparamfc(K *f, K *a, char *av) {
  K *r=0,*p=0,*q=0;
  int m;
  if(at!=11) return kerror("type");
  if(ac!=2) return kerror("valence");
  p = v0(a)[0]->t<0 ? kmix(v0(a)[0]) : v0(a)[0];
  q = v0(a)[1]->t<0 ? kmix(v0(a)[1]) : v0(a)[1];
  if(p->t==16) return kerror("valence");  /* TODO: projection */
  if(q->t==16) return kerror("valence");  /* TODO: projection */
  m=p->c<q->c?q->c:p->c;
  r=kv0(m);
  DO(m, v0(r)[i]=applyfc2_(f,p->t>0?p:v0(p)[i],q->t>0?q:v0(q)[i],0); EC(v0(r)[i]));
  if(p!=v0(a)[0]) kfree(p);
  if(q!=v0(a)[1]) kfree(q);
  return r->t ? r : knorm(r);
}

K* avdofc(K *f, K *a, K *b, char *av) {
  K *r=0;
  int n;
  char av2[32];
  av2[0]=0;
  if(av) strncpy(av2,av,32);
  if(!(n=av?strlen(av):0)) return r;
  av2[n-1]=0;
  //if(av[n-1]=='\'') r=eachpriorfc(f,a,b,av2);     /* a f'b */
  if(av[n-1]=='\'') r=slidefc(f,a,b,av2);     /* a f'b */
  else if(av[n-1]=='/') r=eachrightfc(f,a,b,av2); /* a f/b */
  else if(av[n-1]=='\\') r=eachleftfc(f,a,b,av2); /* a f\b */
  return r;
}

K* eachpriorfc(K *f, K *a, K *b, char *av) {
  K *r=0,*bm=0;
  int n;
  if(bt<=0&&!bc) return kv0(0);
  if(at<=0&&!ac) return kv0(0);
  if(bt<0) bm=kmix(b); else bm=b;

  n=strlen(av);
  if(bt>0) {
    if(n) r=avdofc(f,b,a,av);
    else r=applyfc2_(f,b,a,0);
  }
  else {
    r = kv0(bc);
    if(n) {
      v0(r)[0]=avdofc(f,v0(bm)[0],a,av); EC(v0(r)[0]);
      DO(bc-1, v0(r)[i+1]=avdofc(f,v0(bm)[i+1],v0(bm)[i],av); EC(v0(r)[i+1]))
    }
    else {
      v0(r)[0]=applyfc2_(f,v0(bm)[0],a,0); EC(v0(r)[0]);
      DO(bc-1, v0(r)[i+1]=applyfc2_(f,v0(bm)[i+1],v0(bm)[i],0); EC(v0(r)[i+1]))
    }
  }

  if(bm!=b) kfree(bm);
  return r->t ? r : knorm(r);
}

K* slidefc(K *f, K *a, K *b, char *av) {
  K *r=0,*bm=0;
  int n,d,s,v,m;;
  if(at!=1) return kerror("type");
  if(bt<=0&&!bc) return kv0(0);
  if(at<=0&&!ac) return kv0(0);
  if(bt<0) bm=kmix(b); else bm=b;

  d=a1;
  s=abs(d);
  v=2;
  m=(bc-v+s)/s;
  if((m-1)*s+v!=bm->c) return kerror("valence");

  n=strlen(av);
  if(bt>0) {
    if(n) r=avdofc(f,b,a,av);
    else r=applyfc2_(f,b,a,0);
  }
  else {
    r=kv0(m);
    if(n) {
      v0(r)[0]=avdofc(f,v0(bm)[0],a,av); EC(v0(r)[0]);
      DO(bc-1, v0(r)[i+1]=avdofc(f,v0(bm)[i+1],v0(bm)[i],av); EC(v0(r)[i+1]))
    }
    else if(d<0) DO(m,v0(r)[i]=applyfc2_(f,v0(bm)[i*s+1],v0(bm)[i*s],0);EC(v0(r)[i]))
    else if(d>0) DO(m,v0(r)[i]=applyfc2_(f,v0(bm)[i*s],v0(bm)[i*s+1],0);EC(v0(r)[i]))
  }

  if(bm!=b) kfree(bm);
  return r->t ? r : knorm(r);
}

K* eachrightfc(K *f, K *a, K *b, char *av) {
  K *r=0,*bm=0;
  int n;
  if(bt<0) bm=kmix(b); else bm=b;
  n=strlen(av);
  if(bt>0) {
    if(n) r=avdofc(f,a,bm,av);
    else r=applyfc2_(f,a,bm,0);
  }
  else {
    r = kv0(bc);
    if(n) DO(bc,v0(r)[i]=avdofc(f,a,v0(bm)[i],av);EC(v0(r)[i]))
    else DO(bc,v0(r)[i]=applyfc2_(f,a,v0(bm)[i],0);EC(v0(r)[i]))
  }
  if(bm!=b) kfree(bm);
  return r->t ? r : knorm(r);
}

K* eachleftfc(K *f, K *a, K *b, char *av) {
  K *r=0,*am=0;
  int n;
  if(at<0) am=kmix(a); else am=a;
  n=strlen(av);
  if(at>0) {
    if(n) r=avdofc(f,am,b,av);
    else r=applyfc2_(f,am,b,0);
  }
  else {
    r=kv0(ac);
    if(n) DO(ac,v0(r)[i]=avdofc(f,v0(am)[i],b,av);EC(v0(r)[i]))
    else DO(ac,v0(r)[i]=applyfc2_(f,v0(am)[i],b,0);EC(v0(r)[i]))
  }
  if(am!=a) kfree(am);
  return r->t ? r : knorm(r);
}
