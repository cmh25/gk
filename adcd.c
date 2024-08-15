#include "adcd.h"
#include <string.h>
#include "ops.h"
#include "fn.h"
#include "p.h"
#include "x.h"
#include "av.h"
#include "sym.h"

static node* parse(char *s) {
  int u;
  node *n;
  pgs *p=pgnew();
  u=strlen(s)+2;
  p->p=xmalloc(u);
  strncpy(p->p,s,u-1);
  strcat(p->p,"\n");
  n=pgparse(p);
  pgfree(p);
  return n;
}

K* while1(K *a) {
  K *r=0,*b=0;
  unsigned int i;
  node *n=parse(a->v);
  b=node_reduce(n->a[n->v-1],0); EC(b); SR(b);
  while(b1) {
    kfree(b);
    for(i=1;i<n->v;i++) {
      r=node_reduce(n->a[n->v-1-i],0); EC(r);
      if(fret) break;
      kfree(r);
    }
    if(fret) break;
    b=node_reduce(n->a[n->v-1],0); EC(b); SR(b);
  }
  if(fret) { node_free(n); return r; }
  else { kfree(b); node_free(n); return null; }
}

K* do1(K *a) {
  K *r=0,*b=0;
  unsigned int i;
  int j;
  node *n=parse(a->v);
  b=node_reduce(n->a[n->v-1],0); EC(b); SR(b);
  j=b1;
  while(j-->0) {
    for(i=1;i<n->v;i++) {
      r=node_reduce(n->a[n->v-1-i],0); EC(r);
      if(fret) break;
      kfree(r);
    }
    if(fret) break;
  }
  if(fret) { kfree(b); node_free(n); return r; }
  else { kfree(b); node_free(n); return null; }
}

K* if1(K *a) {
  K *r=0,*b=0;
  unsigned int i;
  node *n=parse(a->v);
  b=node_reduce(n->a[n->v-1],0); EC(b); SR(b);
  if(b1) {
    for(i=1;i<n->v;i++) {
      r=node_reduce(n->a[n->v-1-i],0); EC(r);
      if(fret) break;
      kfree(r);
    }
  }
  kfree(b);
  node_free(n);
  if(fret) return r;
  else return null;
}

K* cond1(K *a) {
  K *r=0,*b=0,*c=0;
  int i;
  node *n = parse(a->v);
  if(n->v==1) { /* format $[5] */
    b=node_reduce(n->a[0],0); EC(b);
    r=format_(b);
    kfree(b);
    node_free(n);
    return r;
  }
  if(n->v==2) { /* form2 $[0;"5"] */
    b=node_reduce(n->a[1],0); EC(b);
    c=node_reduce(n->a[0],0); EC(c);
    r=form2_(b,c);
    kfree(b); kfree(c);
    node_free(n);
    return r;
  }
  for(i=n->v-1;i>=0;i--) {
    b=node_reduce(n->a[i],0); EC(b); SR(b);
    i--;
    if(i<0) { r=b; break; }
    if(b->i) {
      kfree(b);
      r=node_reduce(n->a[i],0); EC(r);
      break;
    }
    else kfree(b);
  }
  if(!r) r=null;
  node_free(n);
  return r;
}

K* amendi3_(K *a, K *b, K *c) {
  K *r=0,*p=0,*q=0;
  int e=E;
  ERR *ee;

  /* error trap */
  if((at==7||at==27||at==37||at==67||at==77)&&ct==7&&((fn*)c->v)->i==':') {
    E=0; r=kv0(2);
    if(at==7) p=apply1(a,b,0);
    else if(at==27&&a7->i) p=apply2(a,a7->l,b,0);
    else if(at==37||at==77) { p=avdo37(a,b,a7->av); if(!p) p=fne2(a,b,0); }
    else if(at==67) p=applyfc1(a,b,0);
    if(p->t==98) { ee=p->v; v0(r)[0]=k1(1); q=kv3(1+strlen(ee->s)); strcpy(v3(q),ee->s); q->c--; v0(r)[1]=q; kfree(p); }
    else { v0(r)[0]=k1(0); v0(r)[1]=p; }
    E=e;
    return rt ? r : knorm(r);
  }

  if(ct!=7&&ct!=37&&ct!=67&&ct!=77&&ct!=87) return kerror("type");

  if(at==4) {
    if(!strlen(v3(a))) { q=k5(ktree); ktree->r++; }
    else { p=scope_get(gs,v3(a)); EC(p); q=p->t<0?kmix(p):kcp(p); kfree(p); }
  }
  else q = at<0 ? kmix(a) : kcp(a);

  if(q->t==5 && at==4 && !strlen(v3(a))) {
    if(bt==4 && strlen(v3(b))==1 && v3(b)[0]!='k') return kerror("reserved");
    if(bt==-4 && strlen(v4(b)[0])==1 && v4(b)[0][0]!='k') return kerror("reserved");
  }

  if(q->t>0 && q->t!=5) return kerror("rank");

  r = amendi3_pd(q,b,c);

  if(at==4) {
    p=rt?r:knorm(r); EC(scope_set(gs,v3(a),p)); kfree(p);
    return kref(a);
  }
  return rt ? r : knorm(r);
}

K* amendi3_pd(K *a, K *b, K *c) {
  K *r=a,*bm=0,*dm=0,*p=0,*q=0;
  unsigned int i,j;
  K *a0=0,*b0=0;

  if(bt==1) {
    j=b1;
    if(j>rc) return kerror("index");
    if(ct==7) p=dt1[((fn*)c->v)->i](v0(r)[j]);
    else p=fne2(c,v0(r)[j],0);
    EC(p);
    kfree(v0(r)[j]);
    v0(r)[j]=p;
  }
  else if(bt==4) {
    q=k4(b->v);
    p=at2_(r,q); EC(p);
    kfree(q);
    if(ct==7) q=dt1[((fn*)c->v)->i](p);
    else q=fne2(c,p,0);
    EC(q);
    dset(r->v,b->v,q);
    kfree(p); kfree(q);
  }
  else if(bt==6) {
    if(rt==5) { p=dkeys(r->v); bm=kmix(p); kfree(p); }
    else { bm=kv0(rc); DO(rc, v0(bm)[i]=k1(i)) }
  }
  else bm = bt<0 ? kmix(b) : kref(b);

  if(bm)  {
    for(i=0;i<bm->c;i++) {
      b0=v0(bm)[i];
      if(b0->t==1) {
        j=b0->i;
        if(ct==7) p=dt1[((fn*)c->v)->i](v0(r)[j]);
        else p=fne2(c,v0(r)[j],0);
        EC(p);
        kfree(v0(r)[j]);
        v0(r)[j]=p;
      }
      else if(b0->t==4) {
        q=k4(b0->v);
        a0=at2_(r,q); EC(a0);
        if(ct==7) p=dt1[((fn*)c->v)->i](a0);
        else p=fne2(c,a0,0);
        EC(p);
        dset(r->v,b0->v,p);
        kfree(a0);
        kfree(p); kfree(q);
      }
      else {
        p=amendi3_(r,b0,c); EC(p);
        kfree(r);
        r=p;
      }
    }
    kfree(bm);
  }
  kfree(dm);

  return r;
}

K* amend3_(K *a, K *b, K *c) {
  K *r,*p,*q,*bm;
  int e=E;
  ERR *ee;

  /* error trap */
  if((at==7||at==27||at==37||at==67||at==77)&&ct==7&&((fn*)c->v)->i==':') {
    bm=bt<0?kmix(b):b;
    E=0; r=kv0(2);
    if(at==7&&bc==1) p=apply1(a,bm,0);
    else if(at==7&&bc==2) p=apply2(a,v0(bm)[0],v0(bm)[1],0);
    else if(at==27) p=apply2(a,a7->l,bm,0);
    else if((at==37||at==77)&&a7->v==bc) { bm->t=11; p=avdo37(a,bm,a7->av); if(!p) p=fne2(a,bm,0); }
    else if(at==67&&bc==1) p=applyfc1(a,bm,0);
    else if(at==67&&bc==2) p=applyfc2(a,v0(bm)[0],v0(bm)[1],0);
    else p=kerror("valence");
    if(p->t==98) { ee=p->v; v0(r)[0]=k1(1); q=kv3(1+strlen(ee->s)); strcpy(v3(q),ee->s); q->c--; v0(r)[1]=q; kfree(p); }
    else { v0(r)[0]=k1(0); v0(r)[1]=p; }
    E=e;
    if(bm!=b) kfree(bm);
    return rt ? r : knorm(r);
  }

  if(ct!=7&&ct!=37&&ct!=67&&ct!=77&&ct!=87) return kerror("type");

  if(at==4) {
    if(!strlen(v3(a))) { q=k5(ktree); ktree->r++; }
    else { p=scope_get(gs,v3(a)); EC(p); q=p->t<0?kmix(p):kcp(p); kfree(p); }
  }
  else q = at<0 ? kmix(a) : kcp(a);

  if(q->t==5 && at==4 && !strlen(v3(a))) {
    if(bt==4 && strlen(v3(b))==1 && v3(b)[0]!='k') return kerror("reserved");
    if(bt==-4 && strlen(v4(b)[0])==1 && v4(b)[0][0]!='k') return kerror("reserved");
  }

  if(q->t==5 && bt!=4 && bt!=-4 && bt!=0 && bt!=6) return kerror("type");
  if(q->t<=0 && bt!=1 && bt!=-1 && bt!=0 && bt!=6) return kerror("type");

  r = amend3_pd(q,b,c);

  if(at==4) {
    p=rt?r:knorm(r); EC(scope_set(gs,v3(a),p)); kfree(p);
    return kref(a);
  }
  return rt ? r : knorm(r);
}

K* amend3_pd(K *a, K *b, K *c) {
  K *r=0,*bm=0,*p=0,*q=0,*s=0,*t=0,*u=0,*v=0,*sm=0,*pm=0;
  unsigned int i,j;
  K **sa=0;
  int si=0,sn=2;
  sa=xmalloc(sizeof(K*)*sn);
  K* (*ff)(K*,K*,K*);

  if(bt==1||bt==4||bt==6) { r=amendi3_(a,b,c); kfree(a); }
  else if(bc==1) { r=amendi3_(a,b,c); kfree(a); }
  else if(at>0&&!bc) {
    if(ct==37) r=fne2(c,a,0);
    else r=dt1[((fn*)c->v)->i](a);
    EC(r);
    kfree(a);
  }
  else {
    r=a;
    bm = bt<0 ? kmix(b) : kref(b);
    if(bm->c==0) {
      sa[si++]=r;
      while(si>0) {
        p=sa[--si];
        for(i=0;i<p->c;i++) {
          q=v0(p)[i];
          if(!q->t) {
            if(si==sn) { sn<<=1; sa=xrealloc(sa,sizeof(K*)*sn); }
            sa[si++]=v0(p)[i];
          }
          else {
            if(ct==37) s=fne2(c,q,0);
            else s=dt1[((fn*)c->v)->i](q);
            EC(s); kfree(q); v0(p)[i]=s;
          }
        }
      }
    }
    else {
      ff = bm->c==2 ? amendi3_ : amend3_;
      p=v0(bm)[0];
      q=drop2_(one,bm);
      if(p->t>0) { pm=kv0(1); v0(pm)[0]=kref(p); }
      else if(p->t<0) pm=kmix(p);
      for(i=0;i<pm->c;i++) {
        v=v0(pm)[i];
        if(v->t==1) {
          s=v0(r)[v->i];
          sm=ff(s,q,c);
          kfree(s);
          v0(r)[v->i]=sm;
        }
        else if(v->t==4) {
          s=dget(r->v,v->v);
          sm=ff(s,q,c);
          kfree(s);
          dset(r->v,v->v,sm);
          kfree(sm);
        }
        else if(v->t==6) {
          if(rt==5) {
            u=dkeys(r->v);
            for(j=0;j<u->c;j++) {
              s=dget(r->v,v4(u)[j]);
              t=ff(s,q,c); EC(t);
              dset(r->v,v4(u)[j],t);
              kfree(s); kfree(t);
            }
            kfree(u);
          }
          else {
            for(j=0;j<r->c;j++) {
              s=ff(v0(r)[j],q,c); EC(s);
              kfree(v0(r)[j]);
              v0(r)[j]=s;
            }
          }
        }
      }
      kfree(pm);
      kfree(q);
    }
    kfree(bm);
  }

  xfree(sa);

  return r;
}

static int conform(K *a, K *b) {
  K *am=0,*bm=0;
  int c=0;
  if(at>0||bt>0) c=1;
  else if(ac==bc) {
    am = at<0?kmix(a) : a;
    bm = bt<0?kmix(b) : b;
    c=1; DO(ac, if(!conform(v0(am)[i],v0(bm)[i])) c=0; break;)
  }
  if(am&&am!=a) kfree(am);
  if(bm&&bm!=b) kfree(bm);
  return c;
}

static K* runfne2(K *f, K *a, K *b, char *av) {
  K *r,*p;
  p=kv0(2); p->t=11;
  v0(p)[0]=a;
  v0(p)[1]=b;
  r=fne2(f,p,av);
  p->c=0; kfree(p);
  return r;
}

K* amendi4_(K *a, K *b, K *c, K *d) {
  K *r,*p,*q;

  if(!conform(b,d)) return kerror("length");

  if(at==4) {
    if(!strlen(v3(a))) { q=k5(ktree); ktree->r++; }
    else { p=scope_get(gs,v3(a)); EC(p); q=p->t<0?kmix(p):kcp(p); kfree(p); }
  }
  else q = at<0 ? kmix(a) : kcp(a);

  if(q->t==5 && at==4 && !strlen(v3(a))) {
    if(bt==4 && strlen(v3(b))==1 && v3(b)[0]!='k') return kerror("reserved");
    if(bt==-4 && strlen(v4(b)[0])==1 && v4(b)[0][0]!='k') return kerror("reserved");
  }

  r = amendi4_pd(q,b,c,d);

  if(at==4) {
    p=rt?r:knorm(r); EC(scope_set(gs,v3(a),p)); kfree(p);
    return kref(a);
  }
  return rt ? r : knorm(r);
}

K* amendi4_pd(K *a, K *b, K *c, K *d) {
  K *r=a,*bm=0,*dm=0,*p=0,*q=0;
  unsigned int i,j;
  K *a0=0,*b0=0,*d0=0;

  if(bt==1) {
    j=b1;
    if(j>rc) return kerror("index");
    if(ct==7) p=((fn*)c->v)->i==':' ? kref(d) : dt2[((fn*)c->v)->i](v0(r)[j],d);
    else p=runfne2(c,v0(r)[j],d,0);
    EC(p);
    kfree(v0(r)[j]);
    v0(r)[j]=p;
  }
  else if(bt==4) {
    q=k4(b->v);
    p=at2_(r,q); EC(p);
    kfree(q);
    if(ct==7) q=((fn*)c->v)->i==':' ? kref(d) : dt2[((fn*)c->v)->i](p,d);
    else q=runfne2(c,p,d,0);
    EC(q);
    dset(r->v,b->v,q);
    kfree(p); kfree(q);
  }
  else if(bt==6) {
    if(rt==5) {
      p=dkeys(r->v);
      bm=kmix(p);
      kfree(p);
    }
    else {
      bm=kv0(rc);
      DO(rc, v0(bm)[i]=k1(i))
    }
  }
  else bm = bt<0 ? kmix(b) : kref(b);

  if(bm)  {
    dm = dt<0 ? kmix(d) : kref(d);
    for(i=0;i<bm->c;i++) {
      b0=v0(bm)[i];
      d0=dm->t?dm:v0(dm)[i];
      if(b0->t==1) {
        j=b0->i;
        if(j>=rc) return kerror("index");
        if(ct==7) p=((fn*)c->v)->i==':' ? kref(d0) : dt2[((fn*)c->v)->i](v0(r)[j],d0);
        else p=runfne2(c,v0(r)[j],d0,0);
        EC(p);
        kfree(v0(r)[j]);
        v0(r)[j]=p;
      }
      else if(b0->t==4) {
        q=k4(b0->v);
        a0=at2_(r,q); EC(a0);
        if(ct==7) p=((fn*)c->v)->i==':' ? kref(d0) : dt2[((fn*)c->v)->i](a0,d0);
        else p=runfne2(c,a0,d0,0);
        EC(p);
        dset(r->v,b0->v,p);
        kfree(a0);
        kfree(p); kfree(q);
      }
      else {
        p=amendi4_(r,b0,c,d0); EC(p);
        kfree(r);
        r=p;
      }
    }
    kfree(bm);
    kfree(dm);
  }

  return r;
}

K* amend4_(K *a, K *b, K *c, K *d) {
  K *r,*p,*q;

  if(at==4) {
    if(!strlen(v3(a))) { q=k5(ktree); ktree->r++; }
    else { p=scope_get(gs,v3(a)); EC(p); q=p->t<0?kmix(p):kref(p); kfree(p); }
  }
  else q = at<0 ? kmix(a) : kcp(a);

  if(q->t==5 && bt!=4 && bt!=-4 && bt!=0 && bt!=6) return kerror("type");
  if(q->t<=0 && bt!=1 && bt!=-1 && bt!=0 && bt!=6) return kerror("type");
  if(q->t==5 && bt==0 && bc && v0(b)[0]->t!=4 && v0(b)[0]->t!=-4 &&v0(b)[0]->t!=6) return kerror("type");

  if(q->t==5 && at==4 && !strlen(v3(a))) {
    if(bt==4 && strlen(v3(b))==1 && v3(b)[0]!='k') return kerror("reserved");
    if(bt==-4 && strlen(v4(b)[0])==1 && v4(b)[0][0]!='k') return kerror("reserved");
  }

  r=amend4_pd(q,b,c,d);

  if(at==4) {
    if(!strlen(v3(a))||v3(a)[0]=='.') {
      kfree(q);
      return kref(a);
    }
    else {
      p=rt?r:knorm(r);
      EC(scope_set(gs,v3(a),p));
      kfree(p);
      return kref(a);
    }
  }
  return rt ? r : knorm(r);
}

K* amend4_pd(K *a, K *b, K *c, K *d) {
  K *r=a,*bm,*dm,*p,*q,*s,*t,*u,*v,*w;
  unsigned int i;
  K* (*ff)(K*,K*,K*,K*);

  bm=bt<0?kmix(b):kref(b);
  dm=d;

  v=shape_(b);

  if(bm->t==1) {
    p=amendi4_pd(r,bm,c,dm); EC(p);
    r=p;
  }
  else if(bm->t==4) {
    p=amendi4_pd(r,bm,c,dm); EC(p);
    r=p;
  }
  else if(bm->t==6) {
    dm=dt<0&&bc<=v->c?kmix(d):kref(d);
    if(rt==5) {
      u=dkeys(r->v);
      for(i=0;i<u->c;i++) {
        t=dm->t?dm:v0(dm)[i];
        s=dget(r->v,v4(u)[i]);
        if(ct==7) p=((fn*)c->v)->i==':' ? kref(t) : dt2[((fn*)c->v)->i](s,t);
        else p=runfne2(c,s,t,0);
        EC(p);
        dset(r->v,v4(u)[i],p);
        kfree(s); kfree(p);
      }
      kfree(u);
    }
    else {
      for(i=0;i<rc;i++) {
        t=dm->t?dm:v0(dm)[i];
        if(ct==7) p=((fn*)c->v)->i==':' ? kref(t) : dt2[((fn*)c->v)->i](v0(r)[i],t);
        else p=runfne2(c,v0(r)[i],t,0);
        EC(p);
        kfree(v0(r)[i]);
        v0(r)[i]=p;
      }
    }
    kfree(dm);
  }
  else if(bm->c==0) {
    if(ct==7) p=((fn*)c->v)->i==':' ? kref(dm) : dt2[((fn*)c->v)->i](r,dm);
    else p=runfne2(c,r,dm,0);
    EC(p);
    kfree(r);
    r=p;
  }
  else {
    dm=dt<0&&bc<=v->c?kmix(d):kref(d);
    ff=bc==2?amendi4_:amend4_;
    p=v0(bm)[0];
    q=bc==2?kref(v0(bm)[1]):drop2_(one,bm);
    if(p->t==1) {
      if(bc==2) t=dm->t?dm:v0(dm)[0];
      else t=dm->t?dm:bc<=v->c?v0(dm)[0]:dm;
      s=ff(v0(r)[p->i],q,c,t); EC(s);
      kfree(v0(r)[p->i]);
      v0(r)[p->i]=s;
    }
    else if(p->t==-1) {
      for(i=0;i<p->c;i++) {
        if(bc==2) t=dm->t?dm:v0(dm)[i];
        else t=dm->t?dm:bc<=v->c?v0(dm)[i]:dm;
        s=ff(v0(r)[v1(p)[i]],q,c,t); EC(s);
        kfree(v0(r)[v1(p)[i]]);
        v0(r)[v1(p)[i]]=s;
      }
    }
    else if(p->t==6||p->t==16) {
      if(rt==5) {
        u=dkeys(r->v);
        for(i=0;i<u->c;i++) {
          if(bc==2) t=dm->t?dm:v0(dm)[i];
          else t=dm->t?dm:bc<=v->c?v0(dm)[i]:dm;
          s=dget(r->v,v4(u)[i]);
          w=ff(s,q,c,t); EC(w);
          dset(r->v,v4(u)[i],w);
          kfree(s); kfree(w);
        }
        kfree(u);
      }
      else {
        for(i=0;i<rc;i++) {
          if(bc==2) t=dm->t?dm:v0(dm)[i];
          else t=dm->t?dm:bc<=v->c?v0(dm)[i]:dm;
          s=ff(v0(r)[i],q,c,t); EC(s);
          kfree(v0(r)[i]);
          v0(r)[i]=s;
        }
      }
    }
    else if(p->t==4) {
      if(bc==2) t=dm->t?dm:v0(dm)[0];
      else t=dm->t?dm:bc<=v->c?v0(dm)[0]:dm;
      u=dget(r->v,p->v);
      s=ff(u,q,c,t); EC(s);
      dset(r->v,p->v,s);
      if(u->t==5) u->v=0;
      kfree(u); kfree(s);
    }
    else if(p->t==-4) {
      for(i=0;i<p->c;i++) {
        if(bc==2) t=dm->t?dm:v0(dm)[i];
        else t=dm->t?dm:bc<=v->c?v0(dm)[i]:dm;
        u=dget(r->v,v4(p)[i]);
        s=ff(u,q,c,t); EC(s);
        dset(r->v,v4(p)[i],s);
        kfree(u); kfree(s);
      }
    }
    kfree(q); kfree(dm);
  }
  kfree(bm); kfree(v);

  return r;
}
