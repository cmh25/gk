#include "node.h"
#include <stdio.h>
#include <string.h>
#include "x.h"
#include "ops.h"
#include "sys.h"
#include "sym.h"
#include "scope.h"
#include "fn.h"
#include "av.h"
#include "adcd.h"
#include "timer.h"
#include "p.h"
#include "interp.h"

#ifdef _WIN32
  #define strtok_r strtok_s
#endif

#define DLIMIT 2000

int E=1,A=0;

static node* node_new_(node **a, int v, K *s, int line, int linei) {
  node *n = xmalloc(sizeof(node));
  n->a=a;
  n->v=v;
  n->k=s;
  n->q=0;
  n->t=0;
  n->line=line;
  n->linei=linei;
  return n;
}
node* node_new(node **a, int v, K *s) {
  return node_new_(a,v,s,-1,-1);
}
node* node_newli(node **a, int v, K *s, int line, int linei) {
  return node_new_(a,v,s,line,linei);
}

K* assign3_(K *a, K *b, K *c) {
  K *r=0,*m=0,*n=0,*f=0;
  char *an=a->v,*s=0,*t=0,*u=0,*nn,*rp;
  dict *d=0,*e=0;
  scope *es=cs;

  if(!b) { /* d.a:1 */
    nn=xstrdup(a->v);
    u=t=s=strtok_r(nn,".",&rp);
    if(nn[0]=='.') es=ks;
    m=scope_get(es,sp(s));
    if(m->t==98) { kfree(m); d=dnew(); m=k5(d); }
    else if(m->t==5) d=m->v;
    else { xfree(nn); return kerror("value"); }
    while((s=strtok_r(0,".",&rp))) {
      t=s; e=d;
      n=dget(e,s);
      if(!n) { d=dnew(); n=k5(d); dset(e,s,n); kfree(n); n=0; }
      else if(n->t!=5) { kfree(n); n=0; break; }
      else d=n->v;
    }
    if(strtok_r(0,".",&rp)) { xfree(nn); return kerror("value"); }
    SR(c);
    dset(e,t,c);
    if(n) kfree(n);
    scope_set(es,sp(u),m);
    xfree(nn);
    quiet2=1;
    return m;
  }
  else SG2(a);

  if(at==98) { kfree(a); a=k5(dnew()); }

  if(bt==11&&bc==1) b=v0(b)[0];
  else if(bt==11&&bc>1) bt=0;
  SG2(c);
  f=knew(7,0,fnnew(":"),':',0,0);
  if(bt==0) r=amend4_(a,b,f,c);
  else r=amendi4_(a,b,f,c);
  EC(r);
  scope_set(cs,an,r);
  kfree(r); kfree(f); kfree(a);
  quiet2=1;
  return ct ? c : knorm(c);
}

static K* node_reducepe(node *n, int z) {
  K *r=0,*a=0;
  if(++z>DLIMIT) return kerror("wsfull");
  /* has to be backwards for things like (c:b+1;b:a+1;a:1) */
  if(n->t==11) { /* plist */
    r=kv0(n->v);
    DO(rc,a=node_reduce(n->a[i],z);EC(a);SR(a);v0(r)[rc-i-1]=a)
    rt=11;
  }
  else if(n->t==12) { /* elist */
    if(n->v>1) {
      r=kv0(n->v);
      DO(rc,a=node_reduce(n->a[i],z);EC(a);SR(a);if(a->t==16){kfree(a);a=null;}v0(r)[rc-i-1]=a)
    }
    else {
      r=node_reduce(n->a[0],z); EC(r);
      SR(r);
      if(rt==16) r=kv0(0);
    }
  }
  quiet2=0;
  return rt ? r : knorm(r);
}

static K* reduce17(K* c17, node *n) {
  K *r=0,*a,*c,*p;
  fn *f=c17->v,*g;
  ERR *e;
  a=c17;
  c=f->a;
  if(a7->i>1) { /* a 17 can have a 7 or 37, i will be 0 for 37's, but not primitives or builtins */
    if(ct==16) { r=kref(a); r->t=7; }
    else if(ct==11&&cc==4) r=apply4(a,v0(c)[0],v0(c)[1],v0(c)[2],v0(c)[3]); /* @[!10;1 2 3;+;4 5 6] */
    else if(ct==11&&cc==3) r=apply3(a,v0(c)[0],v0(c)[1],v0(c)[2]); /* @[!10;1 2 3;{-x}] */
    else if(ct==11&&cc==2) r=apply2(a,v0(c)[0],v0(c)[1],0); /* f[1;2] */
    else if(ct==11&&cc==1) r=apply1(a,v0(c)[0],0); /* f[1] */
    else if(ct==7 || ct==27) {
      f=fnnew("");
      f->c=xmalloc(sizeof(K*)*2);
      f->c[0]=kcp(a); f->c[0]->t=7;
      f->c[1]=kref(c);
      f->cn=2;
      r=knew(67,0,f,0,0,0);
    }
    else if(ct==67) {
      f=fnnew("");
      g=c->v;
      f->c=xmalloc(sizeof(K*)*(1+g->cn));
      f->c[0]=kcp(a); f->c[0]->t=7;
      DO(g->cn,f->c[i+1]=kref(g->c[i]))
      f->cn=1+g->cn;
      r=knew(67,0,f,0,0,0);
    }
    else r=apply1(a,c,0);
  }
  else if(a7->cn) { /* 67 */
    r=applyfc1(a,c,0);
  }
  else {
    if(ct==16) { r=kref(a); r->t=37; }
    else {
      //p = ct<0 ? kmix(c): kref(c);
      p = kref(c);
      a->t=37;
      if(!f->n) a=fnd(a);
      r=avdo37(a,p,f->av);
      if(!r) r=fne2(a,p,0);
      kfree(p);
    }
  }
  kfree(c17);
  if(rt==98&&E) {
    e=r->v;
    if(n->line!=-1) { e->i=n->line; e->j=n->linei; }
    if(linef[e->i]!=-1&&e->i!=1+linef[e->i]) fprintf(stderr,"%s ... + %d ",lines[linef[e->i]],e->i-linef[e->i]-1);
    if(linefn[e->i]!=-1) fprintf(stderr,"in %s\n",fnames[linefn[e->i]]); /* TODO: real line by filename */
    kprint(r,"",0,1);
    if(e->i!=1+linef[e->i]||linef[e->i]==-1) {
      fprintf(stderr,"%s\n",lines[e->i]);
      fprintf(stderr,"%*s\n",(int)e->j,"^");
    }
    else {
      fprintf(stderr,"%s\n",lines[linef[e->i]]);
      fprintf(stderr,"%*s\n",(int)strlen(lines[linef[e->i]])-(int)strlen(lines[e->i])-1+(int)e->j,"^");
    }
    kfree(r);
    ecount++;
    r=interp(0);
    if(rt==7&&((fn*)r->v)->i==':') {
      kfree(r);
      if(ct==17) r=kref(((fn*)c->v)->a);
      else r=kref(c);
    }
    //else if(ct==17&&((fn*)c->v)->i==':') {
    //  p=assign2_(ao,r);
    //  kfree(r);
    //  r=p;
    //}
    if(rt!=98) --ecount;
  }
  return r;
}
static K* make17(K *a, K *b, K *c, node *n) {
  K *r;
  fn *f;
  char av[32];
  av[0]=0;
  if(ct==98) return kref(c);
  r=knew(at,a->c,fncp(a->v),a7->i,a->f,0); /* a might be a constant like draw, sv, vs, etc. */
  f=r->v;
  if(at==37) r=fnd(r);
  if(f->av) strcat(av,f->av);
  if(bt==47) strcat(av,b->v);
  if(strlen(av)) { if(f->av) xfree(f->av); f->av=xstrdup(av); }
  if(ct==17) c=reduce17(c,n);
  if(f->a) kfree(f->a);
  f->a=kref(c);
  r->t=17;
  return r;
}
static void appendav(fn *f, char *av) {
  char av2[32];
  av2[0]=0;
  strcat(av2,f->av);
  strcat(av2,av);
  xfree(f->av);
  f->av=xstrdup(av2);
}
static K* make67(K *a, K *c, char *av) {
  fn *f;
  f=fnnew("");
  f->c=xmalloc(sizeof(K*)*2);
  f->c[0]=kcp(a);
  appendav(f->c[0]->v,av);
  f->c[1]=kref(c);
  f->cn=2;
  return knew(67,0,f,0,0,0);
}
static K* append67(K *a, K *c, char *av) {
  fn *f,*g;
  f=fnnew("");
  g=c->v;
  f->c=xmalloc(sizeof(K*)*(1+g->cn));
  f->c[0]=kcp(a);
  appendav(f->c[0]->v,av);
  DO(g->cn,f->c[i+1]=kref(g->c[i]))
  f->cn=1+g->cn;
  return knew(67,0,f,0,0,0);
}
static K* reduce18(K *a) {
  K *r,*p,*q;
  p=v0(a)[0];
  q=v0(a)[1];
  if(q->t==18) {
    q=reduce18(q);
    r=kv0(p->c+q->c); r->t=11;
    DO(p->c,v0(r)[i]=v0(p)[i]);
    DO(q->c,v0(r)[i+p->c]=v0(q)[i]);
  }
  else {
    r=kv0(1+p->c); r->t=11;
    DO(p->c,v0(r)[i]=v0(p)[i]);
    v0(r)[p->c]=q;
  }
  if(q!=v0(a)[1]) { q->c=0; kfree(q); }
  return r;
}
static K* node_reducemd(node *n, int z);
static K* node_reduce_(node *n, int md, int z) {
  K *a,*b,*c,*d,*r=0,*p,*q,*s,*t,*ao=null,*co=null,*pc;
  fn *f=0;
  char av[32];
  ERR *e;
  unsigned int i,j,k;
  if(++z>DLIMIT) return kerror("wsfull");
  av[0]=0;
  if(n->v&&n->a[0]->k&&n->a[0]->k->t==7&&((fn*)n->a[0]->k->v)->i==174) { timer_start(); btime=1; quiet=1; }
  if(fret) { fret=0; return gk; }
  if(!n->v) return kref(n->k);
  if(n->t==11||n->t==12) return node_reducepe(n,z);
  if(n->t==10) { /* list of plist */
    r=kv0(n->v);
    DO(rc,v0(r)[i]=node_reducepe(n->a[i],z))
    r->t=10;
    return r;
  }
  if(n->t==18) {
    b=node_reducemd(n->a[1],z);
    a=node_reduce(n->a[0],z);
    r=kv0(2); r->t=18;
    v0(r)[0]=a;
    v0(r)[1]=b;
    return r;
  }
  else if(n->v==2) { /* assign3 */
    c = node_reducemd(n->a[1]->a[2],z); /* 13 rhs */
    d = node_reducemd(n->a[1]->a[1],z); /* av */
    b = node_reduce(n->a[1]->a[0],z); /* v */
    if(bt==7) {
      f=b->v;
      if(dt==47) {
        strcat(av,f->av); strcat(av,d->v); xfree(f->av); f->av=xstrdup(av);
        kfree(d);
      }
      if(f->i==':') { /* [0]:5 */
        kfree(b);
        a = node_reduce(n->a[0],z);
        r=kv0(2); v0(r)[0]=a; v0(r)[1]=c; rt=13;
      }
      else { /* [0]+5 */
        if(ct==17) c=reduce17(c,n);  /* important to reduce rhs first */
        a = node_reduce(n->a[0],z);  /* v[f:t`]>v` */
        r=kv0(3); v0(r)[0]=a; v0(r)[1]=b; v0(r)[2]=c; rt=15;
      }
    }
    else { /* [1]'[!10;!10] */
      a = node_reduce(n->a[0],z);
      r = kv0(3); v0(r)[0]=a; v0(r)[1]=b; v0(r)[2]=c; rt=14;
    }
    return r;
  }
  /* e > o av ez */
  c=node_reducemd(n->a[2],z); EC(c);
  b=node_reduce(n->a[1],z); EC(b);
  a=node_reduce(n->a[0],z); EC(a);
  if(ct==17) {n->line=n->a[2]->line; n->linei=n->a[2]->linei; }
  if(at==67) {n->line=n->a[0]->a[0]->line; n->linei=n->a[0]->a[0]->linei; }
  if(A) { kfree(a); kfree(b); kfree(c); return null; } /* abort */
  if(md&&at==99&&bt==16&&ct==16) { kfree(b); kfree(c); return a; }
  if(at==7&&(a7->i==133||a7->i==134)&&bt==16&&ct==99) { /* 4:a 5:a */
    SR(c); EC(c);
    r=apply1(a,c,0);
    kfree(a); kfree(b); kfree(c);
    return r;
  }
  if(at==99) {
    ao=kref(a); /* save original for assignment */
    p=a;
    SG(a);
    if(a) { if(at==98) { kfree(a); a=p; } else kfree(p); }
    else a=p;
  }
  co=kref(c);
  SR(b); EC(b);
  SR(c); EC(c);

  if((at==7||at==37||at==67||at==77||at==87)&&bt==47) {
    if(a7->av) strcat(av,a7->av);
    if(b->v) strcat(av,b->v);
  }

  if(at==7) {
    if(a7->i==39&&bt==16&&ct==16) r=apply1(a,null,0);
    else if(ct==11) {
      if(ac&&ac<cc) { /* sm[;"a*b"]"aab" */
        k=0;
        for(i=ac;i<cc;i++) {
          for(j=0;j<ac;j++) {
            if(v0(c)[j]->t==16) { v0(c)[j]=v0(c)[i]; k++; break; }
          }
        }
        cc-=k;
      }
      if(!strcmp(av,"\\")||!strcmp(av,"/")) r=avdom(a,c,av); /* ,\[0;1 2] over scan */
      else if(!strcmp(av,"'")) r=apply1(a,c,av); /* f'[a] */
      else if(ac&&ac>cc) { /* projection */
        r=knew(7,0,fnnew(""),'p',0,0); r->t=87;
        f=r->v;
        f->l=kref(a);
        f->a=kref(c);
        f->v=ac-cc;
      }
      else if(cc==4) r=apply4(a,v0(c)[0],v0(c)[1],v0(c)[2],v0(c)[3]); /* @[!10;1 2 3;+;4 5 6] */
      else if(cc==3) r=apply3(a,v0(c)[0],v0(c)[1],v0(c)[2]); /* @[!10;1 2 3;{-x}] */
      else if(cc==2) r=apply2(a,v0(c)[0],v0(c)[1],av); /* f[1;2] */
      else if(cc==1) r=apply1(a,v0(c)[0],av); /* f[1] */
    }
    else if(md) {
      if(ct==17) {
        c=reduce17(c,n);
        r=make17(a,b,c,n);
      }
      else if(ct==15) { /* f[1]+g[1]+h[1] */
        p=avdom(a,v0(c)[0],av);
        if(!p) p=apply1(a,v0(v0(c)[0])[0],av);
        EC(p);
        r=apply2(v0(c)[1],p,v0(c)[2],0);
        kfree(p);
      }
      else r=make17(a,b,c,n);
    }
    else { /* 7 monadic enabled */
      if(ct==16) { xfree(a7->av); a7->av=xstrdup(av); r=kref(a); }
      else if(ct==17&&((fn*)c->v)->i==':') r=apply2(c,ao,((fn*)c->v)->a,0);
      else if(ct==17&&ao->t==99&&ac!=1) r=apply2(c,ao,((fn*)c->v)->a,0);
      else if(ct==17) {
        c=reduce17(c,n);
        if(ct==7 || ct==27) r=make67(a,c,av);
        else if(ct==67) r=append67(a,c,av);
        else r=apply1(a,c,av);
      }
      else if(ct==15) { /* lgamma[1+y]+lgamma[1+x-y] */
        p=apply1(a,v0(v0(c)[0])[0],0);
        r=apply2(v0(c)[1],p,v0(c)[2],0);
        kfree(p);
      }
      else if(ct==18) {
        p=v0(c)[0];
        q=v0(c)[1];
        r=apply2(a,v0(p)[0],q,av);
      }
      else if(ct==27&&co->t!=99) r=make67(a,c,av);
      else if(ct==67&&co->t!=99) r=append67(a,c,av);
      else r=apply1(a,c,av);
    }
  }
  else if(at==37) {
    if(!a7->s_) a=fnd(a);
    if(md) {
      if(ct==17) {
        if(((fn*)c->v)->i>1&&a7->i!=1&&!strlen(av)) { /* a->i==1 indicates a resolved builtin like in, lin, ssr */
          f=c->v;
          r=apply2(c,a,f->a,0);
        }
        else {
          f=c->v;
          if(strlen(f->d)>1&&strlen(f->av)) { /* {x<3}{x+1}\  store do/while */
            r=kv0(2);
            v0(r)[0]=kref(a);
            v0(r)[1]=kref(c);
            r->t=97;
          }
          else {
            c=reduce17(c,n);
            r=make17(a,b,c,n);
          }
        }
      }
      else if(ct==15) { /* f[1]+g[1]+h[1] */
        p=avdo37(a,v0(c)[0],av);
        if(!p) p=fne2(a,v0(c)[0],0);
        r=apply2(v0(c)[1],p,v0(c)[2],0);
        kfree(p);
      }
      else if(bt==16&&ct==16) r=kref(a);
      else if(ct==11||ct==10) {
        r=avdo37(a,c,av);
        if(!r) r=fne2(a,c,0);
      }
      else r=make17(a,b,c,n);
    }
    else { /* 37 monadic enabled */
      if(ct==16) {
        if(bt==47) { f=a->v; xfree(f->av); f->av=xstrdup(av); }
        r=kref(a);
      }
      else if(ct==17) {
        f=c->v;
        if(f->i==':') {
          if(strchr(ao->v,'.'))  r=assign3_(ao,0,f->a);
          else r=apply2(c,ao,f->a,0);
        }
        else if(strlen(f->d)==1&&bt!=47) r=apply2(c,a,f->a,0); /* {},{} */
        else if(f->cn) { c->t=67; r=applyfc2(c,a,f->a,0); c->t=17; }
        else if(f->v==2) {
          p=kv0(2); v0(p)[0]=a; v0(p)[1]=f->a; p->t=11;
          r=avdo37(c,p,av);
          if(!r) r=fne2(c,p,0);
          p->c=0; kfree(p);
        }
        else if(strlen(f->d)>1&&strlen(f->av)&&f->a->t!=16) { /* do/while */
          p=kv0(2); v0(p)[0]=a; v0(p)[1]=f->a;
          r=avdo37infix(c,p,f->av);
          p->c=0;
          kfree(p);
        }
        else if(((fn*)c->v)->i>1&&a7->i!=1&&!strlen(av)) { /* a->i==1 indicates a resolved builtin like in, lin, ssr */
          r=apply2(c,a,f->a,0);
        }
        else {
          f=c->v;
          if(strlen(f->d)>1&&strlen(f->av)) { /* {x<3}{x+1}\  store do/while */
            r=kv0(2);
            v0(r)[0]=kref(a);
            v0(r)[1]=kref(c);
            r->t=97;
          }
          else {
            c=reduce17(c,n);
            r=avdo37(a,c,av);
            if(!r) r=fne2(a,c,0);
          }
        }
      }
      else if(ct==14) { /* f'[1]'[2]'[3] */
        f=a->v;
        if(bt==47) { xfree(f->av); f->av=xstrdup(av); }
        p=avdo37(a,v0(c)[0],f->av);
        if(!p) p=fne2(a,v0(c)[0],0);
        f=p->v;
        if(v0(c)[1]->t==47) { xfree(f->av); f->av=xstrdup(v0(c)[1]->v); }
        if(v0(c)[2]->t!=14) {
          r=applyprj(p,v0(c)[2]);
          f=r->v;
        }
        else {
          pc=v0(c)[2];
          q=applyprj(p,v0(pc)[0]);
          f=q->v;
          if(v0(pc)[1]->t==47) { xfree(f->av); f->av=xstrdup(v0(pc)[1]->v); }
          if(v0(pc)[2]->t==11) {
            r=applyprj(q,v0(pc)[2]);
            f=r->v;
          }
          kfree(q);
        }
        kfree(p);
      }
      else if(ct==15) { /* f[1]+g[1] */
        p=avdo37(a,v0(c)[0],av);
        if(!p) p=fne2(a,v0(c)[0],0);
        r=apply2(v0(c)[1],p,v0(c)[2],0);
        kfree(p);
      }
      else if(ct==10) { /* f[1][2][3] */
        r=fne2(a,v0(c)[0],0);
        for(i=1;i<c->c;i++) { p=r; r=applyprj(p,v0(c)[i]); kfree(p); }

      }
      else if(ct==18) {
        p=v0(c)[0]; q=v0(c)[1];
        if(q->t==17) {
          s=avdo37(a,p,av);
          if(!s) s=fne2(a,p,0);
          f=q->v;
          if(f&&f->i) { /* f[1]in 1 2 3 */
            t=kv0(2); t->t=11;
            v0(t)[0]=s; v0(t)[1]=f->a;
            r=fne2(q,t,0);
            t->c=0; kfree(t);
          }
          else r=apply2(q,s,f->a,0);
          kfree(s);
        }
        else { /* f[1]2 */
          if(q->t==18) q=reduce18(q);
          if(q->t==11) {
            s=kv0(p->c+q->c); s->t=11;
            DO(p->c,v0(s)[i]=v0(p)[i]);
            DO(q->c,v0(s)[p->c+i]=v0(q)[i]);
          }
          else {
            s=kv0(1+p->c); s->t=11;
            DO(p->c,v0(s)[i]=v0(p)[i]); v0(s)[p->c]=q;
          }
          r=avdo37(a,p,av);
          if(!r) r=fne2(a,s,0);
          s->c=0; kfree(s);
          if(q!=v0(c)[1]) { q->c=0; kfree(q); }
        }
      }
      else {
        if(ct==11&&a7->v<cc) { /* {x,y,z}[;1;2]3 */
          k=0;
          for(i=a7->v;i<cc;i++) {
            for(j=0;j<a7->v;j++) {
              if(v0(c)[j]->t==16) { v0(c)[j]=v0(c)[i]; k++; break; }
            }
          }
          cc-=k;
        }
        r=avdo37(a,c,av);
        if(!r) r=fne2(a,c,0);
      }
    }
  }
  else if(at==57) { /* +: */
    if(md) {
      if(ct==17) c=reduce17(c,n);
      if(a7->a) kfree(a7->a);
      a7->a=kref(c);
      r=kref(a);
    }
  }
  else if(at==67&&ct!=16) {
    if(md) r=make17(a,b,c,n);
    else if(ct==17&&((fn*)c->v)->i==':') r=apply2(c,ao,((fn*)c->v)->a,0);
    else if(ct==11&&cc==2&&(!b->v||!strlen(b->v))) r=applyfc2(a,v0(c)[0],v0(c)[1],0);
    else r=applyfc1(a,c,b->v);
  }
  else if(at==77) {
    if(ct==16) {
      if(bt==47) { f=a->v; xfree(f->av); f->av=xstrdup(av); }
      r=kref(a);
    }
    else if(ct==17) {
      f=c->v;
      if(f->i==':') {
        if(strchr(ao->v,'.'))  r=assign3_(ao,0,f->a);
        else r=apply2(c,ao,f->a,0);
      }
      else if(strlen(f->d)==1&&bt!=47) r=apply2(c,a,f->a,0); /* {},{} */
      else if(f->cn) { c->t=67; r=applyfc2(c,a,f->a,0); c->t=17; }
      else if(f->v==2) {
        p=kv0(2); v0(p)[0]=a; v0(p)[1]=f->a; p->t=11;
        r=avdo37(c,p,av);
        if(!r) r=fne2(c,p,0);
        p->c=0; kfree(p);
      }
      else if(strlen(f->d)>1&&strlen(f->av)) { /* do/while */
        p=kv0(2); v0(p)[0]=a; v0(p)[1]=f->a;
        r=avdo37infix(c,p,f->av);
        p->c=0; kfree(p);
      }
      else if(((fn*)c->v)->i>1&&a7->i!=1&&!strlen(av)) { /* a->i==1 indicates a resolved builtin like in, lin, ssr */
        r=apply2(c,a,f->a,0);
      }
      else {
        c=reduce17(c,n);
        r=applyprj(a,c);
      }
    }
    else r=applyprj(a,c);
  }
  else if(at==87) {
    if(ct==16) { xfree(a7->av); a7->av=xstrdup(av); r=kref(a); }
    else if(ct==17) {
      f=c->v;
      if(f->i==':') {
        if(strchr(ao->v,'.'))  r=assign3_(ao,0,f->a);
        else r=apply2(c,ao,f->a,av);
      }
      else if(f->i=='@') {
        r=apply2(((fn*)a->v)->l,v0(((fn*)a->v)->a)[0],((fn*)c->v)->a,av);
      }
      else if(f->i=='.') {
        r=apply2(((fn*)a->v)->l,v0(((fn*)a->v)->a)[0],((fn*)c->v)->a,av);
      }
    }
    else if(ct==11) {
      f=a->v;
      p=f->a;
      if(f->a->c==2) {
        if(c->c!=1) r=kerror("valence");
        else if(v0(p)[0]->t==16) r=apply2(f->l,v0(c)[0],v0(p)[1],av);
        else r=apply2(f->l,v0(p)[0],v0(c)[0],av);
      }
      else if(f->a->c==1) {
        r=apply2(f->l,v0(p)[0],v0(c)[0],av);
      }
    }
    else {
      f=a->v;
      p=f->a;
      if(f->a->c==2) {
        if(v0(p)[0]->t==16) r=apply2(f->l,c,v0(p)[1],av);
        else r=apply2(f->l,v0(p)[0],c,av);
      }
      else if(f->a->c==1) {
        r=apply2(f->l,v0(p)[0],c,av);
      }
    }
  }
  else if(at==97) { /* do/while */
    if(bt==16&&ct==16) r=kref(a);
    else if(ct==17) {
      f=c->v;
      if(f->i==':') {
        if(strchr(ao->v,'.'))  r=assign3_(ao,0,f->a);
        else r=apply2(c,ao,f->a,av);
      }
      else {
        p=kv0(2); v0(p)[0]=v0(a)[0];
        if(((fn*)c->v)->i=='@') v0(p)[1]=((fn*)c->v)->a;
        else if(((fn*)c->v)->i=='.') v0(p)[1]=((fn*)c->v)->a;
        else { c=reduce17(c,n); v0(p)[1]=c; }
        r=avdo37infix(v0(a)[1],p,((fn*)v0(a)[1]->v)->av);
        p->c=0;
        kfree(p);
      }
    }
    else {
      p=kv0(2); v0(p)[0]=v0(a)[0];
      if(ct==11) v0(p)[1]=v0(c)[0];
      else v0(p)[1]=c;
      r=avdo37infix(v0(a)[1],p,((fn*)v0(a)[1]->v)->av);
      p->c=0;
      kfree(p);
    }
  }
  else if(at==5&&ct==15) { /* a[`b]*3 */
    p=at2_(a,v0(c)[0]); EC(p);
    r=apply2(v0(c)[1],p,v0(c)[2],av);
    kfree(p);
  }
  else if(at==80) r=while1(a);
  else if(at==81) r=do1(a);
  else if(at==82) r=if1(a);
  else if(at==83) r=cond1(a);
  else { /* a is a noun */
    if(bt==16&&ct==16) r=kref(a);
    else if(ct==7) { r=kref(c); ((fn*)r->v)->l=kref(a); r->t=27; }
    else if(ct==37) { r=kref(c); ((fn*)r->v)->l=kref(a); r->t=27; }
    else if(ct==17) {
      f=c->v;
      if(f->i==':') {
        if(ao->t==99&&strchr(ao->v,'.')) r=assign3_(ao,0,f->a);
        else r=apply2(c,ao,f->a,av);
      }
      else if(at==1&&f->a->t==16&&strlen(f->d)>1&&!f->i&&strlen(f->av)&&(!strcmp(f->av,"\\")||!strcmp(f->av,"/"))) { /* 5{x+1}\  store do/while */
        r=kv0(2);
        v0(r)[0]=kref(a);
        v0(r)[1]=kref(c);
        r->t=97;
      }
      else if(f->a->t==16) {
        if(strlen(av)) {
          if(f->av) xfree(f->av);
          if(strlen(av)) f->av=xstrdup(av);
        }
        r=kcp(c); ((fn*)r->v)->l=kref(a); r->t=27;
      }
      else if(f->a->t==7) {
        c=reduce17(c,n);
        r=kref(c);
        f=r->v;
        ((fn*)f->c[0]->v)->l=kref(a);
        f->c[0]->t=27;
      }
      else if(f->a->t==27) {
        c=reduce17(c,n);
        r=kref(c);
        f=r->v;
        ((fn*)f->c[0]->v)->l=kref(a);
        f->c[0]->t=27;
      }
      else if(f->a->t==67) {
        c=reduce17(c,n);
        f=c->v;
        ((fn*)f->c[0]->v)->l=kref(a);
        f->c[0]->t=27;
        r=kref(c);
      }
      else if(f->cn) {
        ct=67;
        r=applyfc2(c,a,f->a,b->v);
        ct=17;
      }
      else if(((fn*)c->v)->i>1) { /* >1 indicates primitive verb */
        r=apply2(c,a,f->a,b->v);
      }
      else {
        p=kv0(2); v0(p)[0]=a; v0(p)[1]=f->a; p->t=11;
        if(!f->n) c=fnd(c);
        if(b->v) strcat(av,b->v);
        if(f->av) strcat(av,f->av);
        r=avdo37infix(c,p,av);
        if(!r) r=fne2(c,p,0);
        p->c=0; kfree(p);
      }
    }
    else if(ct==18) {
      p=knew(7,0,fnnew("@"),'@',0,0);  /* c[1]in"asdf" */
      q=apply2(p,a,v0(v0(c)[0])[0],0);
      s=kv0(2); s->t=11; v0(s)[0]=q; v0(s)[1]=((fn*)v0(c)[1]->v)->a;
      r=avdo37infix(v0(c)[1],s,((fn*)v0(c)[1]->v)->av);
      if(!r) r=fne2(v0(c)[1],s,0);
      s->c=0; kfree(s); kfree(p); kfree(q);
    }
    else if(ct==13) r=assign3_(ao,v0(c)[0],v0(c)[1]);
    else if(ct==14) { /* g[1]'[!10;!10] */
      p=knew(7,0,fnnew("@"),'@',0,0);
      q=apply2(p,a,v0(c)[0],av); /* 37 */
      r=avdo37(q,v0(c)[2],av);
      if(!r) r=fne2(q,v0(c)[2],0);
      kfree(p);
      kfree(q);
    }
    else if(ct==57) {
      fn *f=c->v;
      if(f->i==':') r=assign2g_(ao,f->a); /* global assign */
      else if(ao->v) { /* a+:1 */
        ct=7;
        if(((fn*)c->v)->a->t==16) p=apply1(c,a,0);
        else p=apply2(c,a,((fn*)c->v)->a,0);
        EC(p);
        ct=57;
        q=dget(cs->d,ao->v);
        if(!q) r=assign2g_(ao,p);
        else { r=assign2_(ao,p); kfree(q); }
        kfree(p);
      }
    }
    else if(ct==67) {
      f=c->v;
      ((fn*)f->c[0]->v)->l=kref(a);
      f->c[0]->t=27;
      r=kref(c);
    }
    else { /* a is a noun, c is a noun */
      if(bt==47&&b->v) strcat(av,b->v); /* index converge */
      p=knew(7,0,fnnew("@"),'@',0,0);
      if(!strcmp(av,"\\")||!strcmp(av,"/")) {
        s=kv0(2); v0(s)[0]=a; v0(s)[1]=c;
        r=avdom(p,s,av);
        s->c=0; kfree(s); kfree(p);
      }
      else {
        if(ct==11) {
          if(cc==1) r=apply2(p,a,v0(c)[0],av);
          else { c=knorm(c); r=apply2(p,a,c,av); }
        }
        else if(ct==15) {
          q=apply2(p,a,v0(v0(c)[0])[0],av);
          r=apply2(v0(c)[1],q,v0(c)[2],0);
          kfree(q);
        }
        else r=apply2(p,a,c,av);
        kfree(p);
      }
    }
  }

  if(fret) {
    kfree(a);kfree(b);kfree(c);kfree(ao);kfree(co);
    return gk;
  }
  if(!r) {
    r=kerror("parse");
  }
  if(rt==98&&E) {
    e=r->v;
    if(n->line!=-1) { e->i=n->line; e->j=n->linei; }
    if(linef[e->i]!=-1&&e->i!=1+linef[e->i]) fprintf(stderr,"%s ... + %d ",lines[linef[e->i]],e->i-linef[e->i]-1);
    if(linefn[e->i]!=-1) fprintf(stderr,"in %s\n",fnames[linefn[e->i]]); /* TODO: real line by filename */
    kprint(r,"",0,1);
    if(e->i!=1+linef[e->i]||linef[e->i]==-1) {
      fprintf(stderr,"%s\n",lines[e->i]);
      fprintf(stderr,"%*s\n",(int)e->j,"^");
    }
    else {
      fprintf(stderr,"%s\n",lines[linef[e->i]]);
      fprintf(stderr,"%*s\n",(int)strlen(lines[linef[e->i]])-(int)strlen(lines[e->i])-1+(int)e->j,"^");
    }
    kfree(r);
    ecount++;
    r=interp(0);
    if(rt==7&&((fn*)r->v)->i==':') {
      kfree(r);
      if(ct==17) r=kref(((fn*)c->v)->a);
      else r=kref(c);
    }
    else if(ct==17&&((fn*)c->v)->i==':') {
      p=assign2_(ao,r);
      kfree(r);
      r=p;
    }
    if(rt!=98) --ecount;
  }
  kfree(a);kfree(b);kfree(c);kfree(ao);kfree(co);
  return r ? r : null;
}
K* node_reduce(node *n, int z) {
  return node_reduce_(n,0,z);
}
static K* node_reducemd(node *n, int z) {
  return node_reduce_(n,1,z);
}

void node_free(node *n) {
  if(!n) return;
  DO(n->v,node_free(n->a[i]))
  kfree(n->k);
  xfree(n->a);
  xfree(n);
}

node* node_append(node *n0, node *n1) {
  n1->a = xrealloc(n1->a,sizeof(node*)*(n1->v+1));
  n1->a[n1->v++] = n0;
  return n1;
}

node* node_prepend(node *n0, node *n1) {
  int i;
  n1->a = xrealloc(n1->a,sizeof(node*)*(n1->v+1));
  for(i=n1->v;i>0;i--) n1->a[i]=n1->a[i-1];
  n1->a[0]=n0;
  n1->v++;
  return n1;
}
