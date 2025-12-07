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
  K k,v,m,*pd,*pv;
  char **pk;
  u32 i;
  u64 n;
  if(d==ktree && strlen(key)==1 && (*key!='k'||0x80!=s(val))) return kerror("reserved");
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
