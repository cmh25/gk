#include "dict.h"
#include <stdio.h>
#include <string.h>
#include "scope.h"

K dnew(void) {
  K d,k,v,m,*pdu;
  d=tn(0,3); pdu=px(d);
  k=tn(4,DMAX);
  v=tn(0,DMAX);
  m=t(1,DMAX);
  pdu[0]=k; pdu[1]=v; pdu[2]=m;
  nk=0; nv=0;
  return st(0x80,d);
}

void dfree(K d) {
  _k(d);
}

K dget(K d, char *key) {
  K *pd=px(d),*pv;
  char **pk;
  pk=px(pd[0]); pv=px(pd[1]);
  i(n(pd[0]),if(key==*pk++) return k_(pv[i]))
  return 0;
}

K dset(K d, char *key, K val) {
  K k,v,m,*pd,*pv,cp=0;
  char **pk;
  u32 i;
  u64 n;
  gcache_clear();  /* conservative: invalidate cache on any dict modification */
  /* top-level single-letter keys are reserved, except 'k' (.k namespace)
   * and 'm' (.m ipc namespace), each of which must be a dict. */
  if(d==ktree && strlen(key)==1
     && ((*key!='k' && *key!='m') || 0x80!=s(val))) return kerror("reserved");
  /* A dictionary reference must never be stored on the namespace root by
   * reference: f reading the tree (e.g. `.`) and the result being stored back
   * (as in `.[`;`k;{.*...}]`) would form a reference cycle the refcount GC can
   * never reclaim.  This mirrors scope_set_'s gcopy rule but covers the amend
   * paths (kamend3/kamend4) that write the tree directly via dset.  Only a
   * SHARED dict (r>0) can alias existing tree structure; a fresh r==0 value
   * (e.g. scope_set_'s own gcopy result) is already private, so we skip it and
   * avoid double-copying. */
  if(d==ktree && 0x80==s(val) && ((ko*)(b(48)&val))->r>0) {
    cp=kcp(val); if(E(cp)) return cp; val=cp;
  }
  pd=px(d);
  k=pd[0]; v=pd[1]; m=ik(pd[2]);
  pk=px(k); pv=px(v);
  for(i=0;i<nk;i++) if(key==pk[i]) { _k(pv[i]); pv[i]=k_(val); break; }
  if(i==nk) {
    if(nk==m) {
      n=nk;
      m<<=1;
      k=kresize(pd[0],m);
      v=kresize(pd[1],m);
      pd[2]=t(1,(u32)m);
      pk=px(k);
      pv=px(v);
      nk=n; nv=n;
    }
    pk[nk++]=key;
    pv[nv++]=k_(val);
  }
  if(cp) _k(cp); /* drop our local ref to the copy; the dict now owns it */
  return null;
}

K dvals(K d) {
  K *pd=px(d);
  return kcp(pd[1]);
}

K dkeys(K d) {
  K *pd=px(d);
  return k_(pd[0]);
}

K dcp(K d) {
  K *pd=px(d);
  if(T(pd[2])!=1) return kerror("type");
  int m=ik(pd[2]);
  K t=pd[2];
  K d2=kcp(b(48)&d); if(E(d2)) return d2;
  K r=st(0x80,d2);
  K *pr=px(r);
  pd[2]=pr[2]=t;
  K k=pr[0]; K v=pr[1];
  u64 n=nk;
  k=kresize(k,m); v=kresize(v,m);
  nk=n; nv=n;
  return r;
}

int dcmp(K d0, K d1) {
  int r=0;
  K i=0,*pd0,k0,v0,*pv0,*pd1,k1,v1,*pv1;
  char **pk0,**pk1;
  pd0=px(d0); k0=pd0[0]; v0=pd0[1]; pk0=px(k0); pv0=px(v0);
  pd1=px(d1); k1=pd1[0]; v1=pd1[1]; pk1=px(k1); pv1=px(v1);
  for(;;) {
    if(i==n(k0) && i==n(k1)) break;
    else if(i==n(k0) && i<n(k1)) {r=r?r:-1;break;}    /* count */
    else if(i<n(k0) && i==n(k1)) {r=r?r:1;break;}     /* count */
    else if(strcmp(pk0[i],pk1[i])<0) {r=-1;break;}    /* key */
    else if(strcmp(pk0[i],pk1[i])>0) {r=1;break;}     /* key */
    else if(kcmpr(pv0[i],pv1[i])<0) {r=-1;break;}     /* value */
    else if(kcmpr(pv0[i],pv1[i])>0) {r=1;break;}      /* value */
    ++i;
  }
  return r;
}
