#include "avopt.h"
#include <stdint.h>
#include <string.h>
#include "x.h"

/* plus minus times */
#define AVOPTPMT(F,O) \
K* F##avopt(K *a, char *av) { \
  K *r=0; \
  int i,s; \
  double f; \
  if(!ac) return 0; \
  if(!strcmp(av,"/")) { \
    if(at==-1) { s=v1(a)[0]; for(i=1;i<ac;i++) s O##= v1(a)[i]; r=k1(s); } \
    if(at==-2) { f=v2(a)[0]; for(i=1;i<ac;i++) f O##= v2(a)[i]; r=k2(f); } \
  } \
  else if(!strcmp(av,"\\")) { \
    if(at==-1) { r=kv1(ac); v1(r)[0]=v1(a)[0]; for(i=1;i<ac;i++) v1(r)[i]=v1(r)[i-1] O v1(a)[i]; } \
    if(at==-2) { r=kv2(ac); v2(r)[0]=v2(a)[0]; for(i=1;i<ac;i++) v2(r)[i]=v2(r)[i-1] O v2(a)[i]; } \
  } \
  return r; \
}
AVOPTPMT(plus2_,+);
AVOPTPMT(minus2_,-);
AVOPTPMT(times2_,*);

K* divide2_avopt(K *a, char *av) {
  K *r=0;
  int i;
  double f;
  if(ac<2) return 0;
  if(!strcmp(av,"/")) {
    if(at==-1) { f=I2F(v1(a)[0]); for(i=1;i<ac;i++) f /= I2F(v1(a)[i]); r=k2(f); }
    if(at==-2) { f=v2(a)[0]; for(i=1;i<ac;i++) f /= v2(a)[i]; r=k2(f); }
  }
  else if(!strcmp(av,"\\")) {
    if(at==-1) { r=kv0(ac); v0(r)[0]=k1(v1(a)[0]); v0(r)[1]=k2(I2F(v1(a)[0])/I2F(v1(a)[1])); for(i=2;i<ac;i++) v0(r)[i]=k2(v0(r)[i-1]->f / I2F(v1(a)[i])); }
    if(at==-2) { r=kv2(ac); v2(r)[0]=v2(a)[0]; for(i=1;i<ac;i++) v2(r)[i]=v2(r)[i-1] / v2(a)[i]; }
  }
  else if(!strcmp(av,"'")) {
    if(at==-1) { r=kv2(ac); DO(ac,v2(r)[i]=1.0/I2F(v1(a)[i])); }
    if(at==-2) { r=kv2(ac); DO(ac,v2(r)[i]=1.0/v2(a)[i]); }
  }
  return r;
}

K* minand2_avopt(K *a, char *av) {
  K *r=0;
  int i,s;
  double f;
  int ht[256];
  char *p;
  if(!ac) return 0;
  if(!strcmp(av,"/")) {
         if(at==-1) { s=v1(a)[0]; for(i=1;i<ac;i++) if(v1(a)[i]<s) s=v1(a)[i]; r=k1(s); }
    else if(at==-2) { f=v2(a)[0]; for(i=1;i<ac;i++) if(CMPFFT(v2(a)[i],f)<0) f=v2(a)[i]; r=k2(f); }
    else if(at==-3) { DO(256,ht[i]=0); DO(ac,ht[v3(a)[i]]=1); for(i=0;i<256;i++) if(ht[i]) break; r=k3(i); }
    else if(at==-4) { p=v4(a)[0]; for(i=1;i<ac;i++) if(strcmp(v4(a)[i],p)<0) p=v4(a)[i]; r=k4(p); }
  }
  else if(!strcmp(av,"\\")) {
         if(at==-1) { r=kv1(ac); v1(r)[0]=v1(a)[0]; for(i=1;i<ac;i++) v1(r)[i]=v1(r)[i-1] < v1(a)[i] ? v1(r)[i-1] : v1(a)[i]; }
    else if(at==-2) { r=kv2(ac); v2(r)[0]=v2(a)[0]; for(i=1;i<ac;i++) v2(r)[i]=CMPFFT(v2(r)[i-1],v2(a)[i])<0 ? v2(r)[i-1] : v2(a)[i]; }
    else if(at==-3) { r=kv3(ac); v3(r)[0]=v3(a)[0]; for(i=1;i<ac;i++) v3(r)[i]=v3(r)[i-1] < v3(a)[i] ? v3(r)[i-1] : v3(a)[i]; }
    else if(at==-4) { r=kv4(ac); v4(r)[0]=v4(a)[0]; for(i=1;i<ac;i++) v4(r)[i]=strcmp(v4(r)[i-1],v4(a)[i])<0 ? v4(r)[i-1] : v4(a)[i]; }
  }
  return r;
}

K* maxor2_avopt(K *a, char *av) {
  K *r=0;
  int i,s;
  double f;
  int ht[256];
  char *p;
  if(!ac) return 0;
  if(!strcmp(av,"/")) {
         if(at==-1) { s=v1(a)[0]; for(i=1;i<ac;i++) if(v1(a)[i]>s) s=v1(a)[i]; r=k1(s); }
    else if(at==-2) { f=v2(a)[0]; for(i=1;i<ac;i++) if(CMPFFT(v2(a)[i],f)>0) f=v2(a)[i]; r=k2(f); }
    else if(at==-3) { DO(256,ht[i]=0); DO(ac,ht[v3(a)[i]]=1); for(i=255;i>=0;i--) if(ht[i]) break; r=k3(i); }
    else if(at==-4) { p=v4(a)[0]; for(i=1;i<ac;i++) if(strcmp(v4(a)[i],p)>0) p=v4(a)[i]; r=k4(p); }
  }
  else if(!strcmp(av,"\\")) {
         if(at==-1) { r=kv1(ac); v1(r)[0]=v1(a)[0]; for(i=1;i<ac;i++) v1(r)[i]=v1(r)[i-1] > v1(a)[i] ? v1(r)[i-1] : v1(a)[i]; }
    else if(at==-2) { r=kv2(ac); v2(r)[0]=v2(a)[0]; for(i=1;i<ac;i++) v2(r)[i]=CMPFFT(v2(r)[i-1],v2(a)[i])>0 ? v2(r)[i-1] : v2(a)[i]; }
    else if(at==-3) { r=kv3(ac); v3(r)[0]=v3(a)[0]; for(i=1;i<ac;i++) v3(r)[i]=v3(r)[i-1] > v3(a)[i] ? v3(r)[i-1] : v3(a)[i]; }
    else if(at==-4) { r=kv4(ac); v4(r)[0]=v4(a)[0]; for(i=1;i<ac;i++) v4(r)[i]=strcmp(v4(r)[i-1],v4(a)[i])>0 ? v4(r)[i-1] : v4(a)[i]; }
  }
  return r;
}

K* less2_avopt(K *a, char *av) { return 0; }
K* more2_avopt(K *a, char *av) { return 0; }
K* equal2_avopt(K *a, char *av) { return 0; }
K* power2_avopt(K *a, char *av) { return 0; }
K* modrot2_avopt(K *a, char *av) { return 0; }
K* match2_avopt(K *a, char *av) { return 0; }
K* join2_avopt(K *a, char *av) {
  K *r=0,*p=0;
  int n=0,t,s=1;
  size_t c=0;
  if(!ac) return 0; \
  if(!strcmp(av,"/")) {
    switch(at) {
    case  1: case 2: case 3: case 4:
      r=kv0(1); v0(r)[0]=kref(a);
      break;
    case  0:
      t=v0(a)[0]->t;
      DO(ac,p=v0(a)[i];c+=p->t>0?1:p->c;if(t!=p->t){s=0;break;})
      if(s) {
        switch(t) {
        case   0: r=kv0(c); DO(ac,p=v0(a)[i];DO2(p->c,v0(r)[n++]=kref(v0(p)[j]))) break;
        case  -1: r=kv1(c); DO(ac,p=v0(a)[i];DO2(p->c,v1(r)[n++]=v1(p)[j])) break;
        case  -2: r=kv2(c); DO(ac,p=v0(a)[i];DO2(p->c,v2(r)[n++]=v2(p)[j])) break;
        case  -3: r=kv3(c); DO(ac,p=v0(a)[i];DO2(p->c,v3(r)[n++]=v3(p)[j])) break;
        case  -4: r=kv4(c); DO(ac,p=v0(a)[i];DO2(p->c,v4(r)[n++]=v4(p)[j])) break;
        }
      }
      break;
    case -1: case -2: case -3: case -4:
      r=kref(a);
      break;
    }
  }
  return r;
}
K* take2_avopt(K *a, char *av) { return 0; }
K* drop2_avopt(K *a, char *av) { return 0; }
K* form2_avopt(K *a, char *av) { return 0; }
K* find2_avopt(K *a, char *av) { return 0; }
K* at2_avopt(K *a, char *av) { return 0; }
K* dot2_avopt(K *a, char *av) { return 0; }
K* assign2_avopt(K *a, char *av) { return 0; }

K* fourcolon1_avopt(K *a, char *av) { return 0; }
K* fivecolon1_avopt(K *a, char *av) { return 0; }

/* plus minus times */
#define AVOPT2PMT(F,O) \
K* F##avopt2(K *a, K *b, char *av) { \
  K *r=0,*p=0,*aa=0,*bb=0; \
  int d,s,v,m,n,*ppi,*pai,*pbi; \
  double f,*ppf,*paf,*pbf; \
  if(!strcmp(av,"/")) { /* eachright */ \
    if(at==-1&&bt==-2) { aa=kv2(ac); DO(ac,v2(aa)[i]=I2F(v1(a)[i])); a=aa; } \
    if(at==-2&&bt==-1) { bb=kv2(bc); DO(bc,v2(bb)[i]=I2F(v1(b)[i])); b=bb; } \
         if(at== 1&&bt==-1) { r=kv1(bc); DO(bc,v1(r)[i]=a1 O v1(b)[i]); } \
    else if(at== 1&&bt==-2) { r=kv2(bc); f=I2F(a1); DO(bc,v2(r)[i]=f O v2(b)[i]); } \
    else if(at== 2&&bt==-1) { r=kv2(bc); DO(bc,v2(r)[i]=a2 O I2F(v1(b)[i])); } \
    else if(at== 2&&bt==-2) { r=kv2(bc); DO(bc,v2(r)[i]=a2 O v2(b)[i]); } \
    else if(at==-1&&bt==-1) { r=kv0(bc); DO(bc,v0(r)[i]=p=kv1(ac);n=v1(b)[i];ppi=v1(p);pai=v1(a);DO2(ac,*ppi++=*pai++ O n)); } \
    else if(at==-2&&bt==-2) { r=kv0(bc); DO(bc,v0(r)[i]=p=kv2(ac);f=v2(b)[i];ppf=v2(p);paf=v2(a);DO2(ac,*ppf++=*paf++ O f)); } \
  } \
  else if(!strcmp(av,"\\")) { /* eachleft */ \
    if(at==-1&&bt==-2) { aa=kv2(ac); paf=v2(aa); pai=v1(a); DO(ac,*paf++=I2F(*pai);pai++); a=aa; } \
    if(at==-2&&bt==-1) { bb=kv2(bc); pbf=v2(bb); pbi=v1(b); DO(bc,*pbf++=I2F(*pbi);pbi++); b=bb; } \
         if(at==-1&&bt== 1) { r=kv1(ac); DO(ac,v1(r)[i]=v1(a)[i] O b1); } \
    else if(at==-1&&bt== 2) { r=kv2(ac); DO(ac,v2(r)[i]=I2F(v1(a)[i]) O b2); } \
    else if(at==-2&&bt== 1) { r=kv2(ac); f=I2F(b1); DO(ac,v2(r)[i]=v2(a)[i] O f); } \
    else if(at==-2&&bt== 2) { r=kv2(ac); DO(ac,v2(r)[i]=v2(a)[i] O b2); } \
    else if(at==-1&&bt==-1) { r=kv0(ac); DO(ac,v0(r)[i]=p=kv1(bc);n=v1(a)[i];ppi=v1(p);pbi=v1(b);DO2(bc,*ppi++=*pbi++ O n)); } \
    else if(at==-2&&bt==-2) { r=kv0(ac); DO(ac,v0(r)[i]=p=kv2(bc);f=v2(a)[i];ppf=v2(p);pbf=v2(b);DO2(bc,*ppf++=*pbf++ O f)); } \
  } \
  else if(!strcmp(av,"'")) { /* slide */ \
    if(at==1) { \
      d=a1; if(!d) return kerror("type"); \
      s=abs(d); \
      v=2; \
      m=(bc-v+s)/s; \
      if((m-1)*s+v!=b->c) return kerror("valence"); \
      if(bt==-1) { \
        r=kv1(m); \
        if(d<0) DO(m,v1(r)[i]=v1(b)[i*s+1] O v1(b)[i*s]) \
        else if(d>0) DO(m,v1(r)[i]=v1(b)[i*s] O v1(b)[i*s+1]) \
      } \
      else if(bt==-2) { \
        r=kv2(m); \
        if(d<0) DO(m,v2(r)[i]=v2(b)[i*s+1] O v2(b)[i*s]) \
        else if(d>0) DO(m,v2(r)[i]=v2(b)[i*s] O v2(b)[i*s+1]) \
      } \
    } \
  } \
  if(aa) kfree(aa); \
  if(bb) kfree(bb); \
  return r; \
}
AVOPT2PMT(plus2_,+);
AVOPT2PMT(minus2_,-);
AVOPT2PMT(times2_,*);

K* divide2_avopt2(K *a, K *b, char *av) {
  K *r=0,*p=0,*aa=0,*bb=0;
  int d,s,v,m;
  double f,*ppf,*paf,*pbf;
  if(!strcmp(av,"/")) { /* eachright */
    if(at==-1) { aa=kv2(ac); DO(ac,v2(aa)[i]=I2F(v1(a)[i])); a=aa; }
    if(bt==-1) { bb=kv2(bc); DO(bc,v2(bb)[i]=I2F(v1(b)[i])); b=bb; }
         if(at== 1&&bt==-2) { r=kv2(bc); f=I2F(a1); DO(bc,v2(r)[i]=f / v2(b)[i]); }
    else if(at== 2&&bt==-2) { r=kv2(bc); DO(bc,v2(r)[i]=a2 / v2(b)[i]); }
    else if(at==-2&&bt==-2) { r=kv0(bc); DO(bc,v0(r)[i]=p=kv2(ac);f=v2(b)[i];ppf=v2(p);paf=v2(a);DO2(ac,*ppf++=*paf++ / f)); }
  }
  else if(!strcmp(av,"\\")) { /* eachleft */
    if(at==-1) { aa=kv2(ac); DO(ac,v2(aa)[i]=I2F(v1(a)[i])); a=aa; }
    if(bt==-1) { bb=kv2(bc); DO(bc,v2(bb)[i]=I2F(v1(b)[i])); b=bb; }
    else if(at==-2&&bt== 1) { r=kv2(ac); f=I2F(b1); DO(ac,v2(r)[i]=v2(a)[i] / f); }
    else if(at==-2&&bt== 2) { r=kv2(ac); DO(ac,v2(r)[i]=v2(a)[i] / b2); }
    else if(at==-2&&bt==-2) { r=kv0(ac); DO(ac,v0(r)[i]=p=kv2(bc);f=v2(a)[i];ppf=v2(p);pbf=v2(b);DO2(bc,*ppf++=*pbf++ / f)); }
  }
  else if(!strcmp(av,"'")) { /* slide */
    if(at==1) {
      d=a1; if(!d) return kerror("type");
      s=abs(d);
      v=2;
      m=(bc-v+s)/s;
      if((m-1)*s+v!=b->c) return kerror("valence");
      if(bt==-1) {
        r=kv2(m);
        if(d<0) DO(m,v1(r)[i]=I2F(v1(b)[i*s+1]) / v1(b)[i*s])
        else if(d>0) DO(m,v1(r)[i]=I2F(v1(b)[i*s]) / v1(b)[i*s+1])
      }
      else if(bt==-2) {
        r=kv2(m);
        if(d<0) DO(m,v2(r)[i]=v2(b)[i*s+1] / v2(b)[i*s])
        else if(d>0) DO(m,v2(r)[i]=v2(b)[i*s] / v2(b)[i*s+1])
      }
    }
  }
  if(aa) kfree(aa);
  if(bb) kfree(bb);
  return r;
}
K* minand2_avopt2(K *a, K *b, char *av) { return 0; }
K* maxor2_avopt2(K *a, K *b, char *av) { return 0; }
K* less2_avopt2(K *a, K *b, char *av) { return 0; }
K* more2_avopt2(K *a, K *b, char *av) { return 0; }
K* equal2_avopt2(K *a, K *b, char *av) { return 0; }
K* power2_avopt2(K *a, K *b, char *av) { return 0; }
K* modrot2_avopt2(K *a, K *b, char *av) { return 0; }
K* match2_avopt2(K *a, K *b, char *av) { return 0; }
K* join2_avopt2(K *a, K *b, char *av) { return 0; }
K* take2_avopt2(K *a, K *b, char *av) { return 0; }
K* drop2_avopt2(K *a, K *b, char *av) { return 0; }
K* form2_avopt2(K *a, K *b, char *av) { return 0; }
K* find2_avopt2(K *a, K *b, char *av) {
  K *r=0,**hk,**kp;
  uint64_t m,w,h,t=0,q;
  int i,*ht,*hi,*n,z=0,zi,min=INT_MAX,max=INT_MIN;
  unsigned char hc[256],*da,*db;
  double *f,*hd;
  char **s,**hs;
  if(at!=bt) return 0;
  if(!strcmp(av,"/")) {
    switch(at) {
    case  0:
      r=kv1(bc);
      m=ac;
      w=1; while(w<=m) w<<=1; q=w-1;
      hk=xcalloc(w,sizeof(K*));
      hi=xmalloc(sizeof(int)*w);
      kp=v0(a);kp--;
      for(i=0;i<ac;i++) {
        kp++;
        h=khash(*kp)&q;
        while(hk[h]!=0 && kcmp(hk[h],*kp)) h=(h+1)&q;
        if(!hk[h]) hk[h]=*kp;hi[h]=i;;
      }
      kp=v0(b);kp--;
      for(i=0;i<bc;i++) {
        kp++;
        h=khash(*kp)&q;
        while(hk[h]!=0 && kcmp(hk[h],*kp)) h=(h+1)&q;
        v1(r)[i]=hk[h]?hi[h]:ac;
      }
      xfree(hi);
      xfree(hk);
      break;
    case -1:
      r=kv1(bc);
      DO(ac, if(max<v1(a)[i])max=v1(a)[i];if(min>v1(a)[i])min=v1(a)[i])
      if(min>=0) { /* optimize for all positive n */
        hi=xmalloc(sizeof(int)*++max);
        DO(max,hi[i]=ac)
        n=v1(a);n--;
        DO(ac,n++;if(hi[*n]==ac)hi[*n]=i)
        n=v1(b);n--;
        DO(bc,n++;v1(r)[i]=*n<max?hi[*n]:ac)
        xfree(hi);
      }
      else {
        m=max-min+1;
        if(!m) m=2; /* handle ?0I 0N */
        if(ac<m) m=ac;
        w=1; while(w<=m) w<<=1; q=w-1;
        ht=xcalloc(w,sizeof(int));
        hi=xmalloc(sizeof(int)*w);
        zi=ac;
        n=v1(a);n--;
        for(i=0;i<ac;i++) {
          n++;
          if(!*n){ if(!z) { z=1; zi=i; }; continue; }
          h=(*n*2654435761)&q;
          while(ht[h] && ht[h]!=*n) h=(h+1)&q;
          if(!ht[h]){ht[h]=*n;hi[h]=i;}
        }
        n=v1(b);n--;
        for(i=0;i<bc;i++) {
          n++;
          if(!*n) { v1(r)[i]=zi; continue; }
          h=(*n*2654435761)&q;
          while(ht[h] && ht[h]!=*n) h=(h+1)&q;
          v1(r)[i]=ht[h]?hi[h]:ac;
        }
        xfree(hi);
        xfree(ht);
      }
      break;
    case -2:
      r=kv1(bc);
      m=ac;
      w=1; while(w<=m) w<<=1; q=w-1;
      hd=xcalloc(w,sizeof(double));
      hi=xmalloc(sizeof(int)*w);
      zi=ac;
      f=v2(a);f--;
      for(i=0;i<ac;i++) {
        f++;
        if(!*f){ if(!z) { z=1; zi=i; }; continue; }
        h=((uint64_t)*f*2654435761)&q;
        while(hd[h]!=0 && CMPFF(hd[h],*f)) h=(h+1)&q;
        if(!hd[h]){hd[h]=*f;hi[h]=i;}
      }
      f=v2(b);f--;
      for(i=0;i<bc;i++) {
        f++;
        if(!*f) { v2(r)[i]=zi; continue; }
        h=((uint64_t)*f*2654435761)&q;
        while(hd[h]!=0 && CMPFF(hd[h],*f)) h=(h+1)&q;
        v1(r)[i]=hd[h]?hi[h]:ac;
      }
      xfree(hi);
      xfree(hd);
      break;
    case -3:
      DO(256,hc[i]=ac)
      da=(unsigned char*)v3(a);da--;
      DO(ac, da++;if(hc[*da]==ac){hc[*da]=i;if(++t==256)break;})
      r=kv1(bc);
      db=(unsigned char*)v3(b);db--;
      DO(bc, db++;v1(r)[i]=hc[*db];)
      break;
    case -4:
      r=kv1(bc);
      m=ac;
      w=1; while(w<=m) w<<=1; q=w-1;
      hs=xcalloc(w,sizeof(char*));
      hi=xmalloc(sizeof(int)*w);
      s=v4(a);s--;
      for(i=0;i<ac;i++) {
        s++;
        h=xfnv1a((char*)*s, strlen(*s))&q;
        while(hs[h]!=0 && strcmp(hs[h],*s)) h=(h+1)&q;
        if(!hs[h]) hs[h]=*s;hi[h]=i;;
      }
      s=v4(b);s--;
      for(i=0;i<bc;i++) {
        s++;
        h=xfnv1a((char*)*s, strlen(*s))&q;
        while(hs[h]!=0 && strcmp(hs[h],*s)) h=(h+1)&q;
        v1(r)[i]=hs[h]?hi[h]:ac;
      }
      xfree(hi);
      xfree(hs);
      break;
    }
  }
  return r;
}
K* at2_avopt2(K *a, K *b, char *av) { return 0; }
K* dot2_avopt2(K *a, K *b, char *av) { return 0; }
K* assign2_avopt2(K *a, K *b, char *av) { return 0; }

K* flip_avopt(K *a, char *av) { return 0; }
K* negate_avopt(K *a, char *av) { return 0; }
K* first_avopt(K *a, char *av) { return 0; }
K* reciprocal_avopt(K *a, char *av) { return 0; }
K* where_avopt(K *a, char *av) { return 0; }
K* reverse_avopt(K *a, char *av) { return 0; }
K* upgrade_avopt(K *a, char *av) { return 0; }
K* downgrade_avopt(K *a, char *av) { return 0; }
K* group_avopt(K *a, char *av) { return 0; }
K* shape_avopt(K *a, char *av) { return 0; }
K* enumerate_avopt(K *a, char *av) { return 0; }
K* not_avopt(K *a, char *av) { return 0; }
K* enlist_avopt(K *a, char *av) { return 0; }
K* count_avopt(K *a, char *av) { return 0; }
K* flr_avopt(K *a, char *av) { return 0; }
K* format_avopt(K *a, char *av) { return 0; }
K* unique_avopt(K *a, char *av) { return 0; }
K* atom_avopt(K *a, char *av) { return 0; }
K* value_avopt(K *a, char *av) { return 0; }
