#include "dict.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "scope.h"

K dnew(void) {
  K d,k,v,m,*pdu;
  d=tn(0,4); pdu=px(d);
  k=tn(4,DMAX);
  v=tn(0,DMAX);
  m=t(1,DMAX);
  pdu[0]=k; pdu[1]=v; pdu[2]=m; pdu[3]=null;
  nk=0; nv=0;
  return st(0x80,d);
}

void dfree(K d) {
  _k(d);
}

/* ---- key hash index (payload slot 3) ----
 * Lookup used to be a linear scan of the key vector -- O(n) per dget/dset,
 * O(n^2) to build or query a big map.  Slot 3 now caches an open-addressing
 * table over the interned key pointers: an int vector whose [0] is the key
 * COUNT the table was built for and [1..] is a power-of-two table of
 * key-slot+1 (0 = empty).  Hash and equality are on the interned pointer.
 *
 * The count stamp makes the table self-invalidating.  All ordinary key
 * appends go through dset, which maintains a valid table; but fn.c's
 * activation machinery (fne_/fne_fast) truncates n(keys) on a lambda-frame
 * dict and later restores the exact prior names, and a stale table must not
 * be consulted in between.  "stamp == n(keys)" is precisely "table matches
 * the key vector": appends move the count away, and the restore that brings
 * it back also brings back the same interned names.
 *
 * Duplicate keys (constructible via `.+(`a`a;1 2)`) keep their first-wins
 * lookup semantics: build and insert never displace an existing entry for
 * the same pointer, so the table always maps a key to its FIRST slot,
 * exactly like the linear scan.  Small dicts (lambda frames, most
 * namespaces) never build a table at all.
 *
 * The slot itself is a runtime cache: bd ships 3 payload slots and db
 * re-adds a null slot 3 (b.c), and the print/unmake transposes hide it
 * (k.c/v.c) just as they already hide the capacity in slot 2. */
#define DHTHRESH 32

static inline u64 dhash_(char *p) {
  u64 h=(u64)p;
  h^=h>>33; h*=0xff51afd7ed558ccd; h^=h>>29;
  return h;
}

/* (re)build the table for d's current keys; returns its payload, 0 on
 * failure (index vector unset).  Sized to <=25% load so appends stay cheap. */
static i32 *dixbuild(K d) {
  K *pd=px(d);
  u64 j,s,nn=n(pd[0]),h=64;
  char **pk=px(pd[0]);
  while(h<4*nn) h<<=1;
  K ix=tn(1,(i64)(h+1));
  i32 *pi=px(ix);
  memset(pi,0,(h+1)*sizeof(i32));
  pi[0]=(i32)nn;
  i32 *tab=pi+1;
  for(j=0;j<nn;j++) {
    s=dhash_(pk[j])&(h-1);
    for(;;) {
      i32 e=tab[s];
      if(!e) { tab[s]=(i32)j+1; break; }
      if(pk[e-1]==pk[j]) break; /* duplicate key: first slot wins */
      s=(s+1)&(h-1);
    }
  }
  _k(pd[3]); pd[3]=ix;
  DCHK(d);
  return pi;
}

/* the valid table for d, building/rebuilding when worthwhile; 0 when d
 * stays linear (small, legacy 3-slot payload, count too big for i32, or a
 * slot 3 that is not a table). */
static i32 *densure(K d) {
  K *pd=px(d);
  u64 nn=n(pd[0]);
  if(n(b(48)&d)<4 || nn<DHTHRESH || nn>=(u64)INT32_MAX) return 0;
  if(pd[3]!=null) {
    if(-1!=T(pd[3]) || n(pd[3])<2) return 0;
    i32 *pi=px(pd[3]);
    if((u64)pi[0]==nn) return pi;
  }
  return dixbuild(d);
}

K dget(K d, char *key) {
  K *pd=px(d),*pv;
  char **pk;
  u64 nn=n(pd[0]);
  pk=px(pd[0]); pv=px(pd[1]);
  if(nn>=DHTHRESH) {
    i32 *pi=densure(d);
    if(pi) {
      u64 h=n(pd[3])-1,s=dhash_(key)&(h-1);
      i32 *tab=pi+1;
      i32 e;
      while((e=tab[s])) {
        if((u64)e<=nn && pk[e-1]==key) return k_(pv[e-1]);
        s=(s+1)&(h-1);
      }
      return 0;
    }
  }
  i(nn,if(key==*pk++) return k_(pv[i]))
  return 0;
}

K dset(K d, char *key, K val) {
  K k,v,m,*pd,*pv,cp=0;
  char **pk;
  u32 i;
  u64 n;
  gcache_clear();  /* conservative: invalidate cache on any dict modification */
  /* the empty symbol is not a valid dictionary key.  The index-amend paths
   * (kamendi*) already reject it, and `d[`]:x` errors, but the named-amend
   * paths (.[`ns;`;...]) dset it directly -- guard at the single chokepoint
   * where keys enter a dict so every path is covered. */
  if(!*key) return kerror("domain");
  /* a key is one name segment: a letter, then letters/digits/underscores.
   * Dotted paths are split before dset, so anything else here is a malformed
   * key (e.g. `$"a-b") that name lookup could never reach. */
  { char *vp=key; if(!isalpha((unsigned char)*vp)) return kerror("domain");
    while(*++vp) if(!isalnum((unsigned char)*vp)&&*vp!='_') return kerror("domain"); }
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
  /* indexed lookup for big dicts; the linear scan stays for small ones and
   * for a stale table (see the slot-3 comment above dixbuild) */
  i32 *pi=0,*tab=0;
  u64 h=0,hs=0;
  if(nk>=DHTHRESH && (pi=densure(d))) {
    i32 e;
    h=n(pd[3])-1; tab=pi+1; hs=dhash_(key)&(h-1);
    while((e=tab[hs])) {
      if((u64)e<=nk && pk[e-1]==key) break;
      hs=(hs+1)&(h-1);
    }
    i = e ? (u64)e-1 : nk;   /* hs is the insert slot when absent */
    if(i<nk) { _k(pv[i]); pv[i]=k_(val); }
  }
  else for(i=0;i<nk;i++) if(key==pk[i]) { _k(pv[i]); pv[i]=k_(val); break; }
  if(i==nk) {
    /* A dict holds at most INT32_MAX keys: the hash index stores key positions
     * as i32 (dixbuild) and the capacity slot is read back via ik (i32).  Past
     * this the positions/capacity would silently wrap and corrupt the heap, so
     * error cleanly instead.  Supporting more would mean an i64 index whose
     * hash table alone (~1.5x the dict's memory) isn't a good trade for a
     * >2^31-key dict (a 32GB+ structure). */
    if(nk>=DICTMAX) { if(cp) _k(cp); return KERR_WSFULL; }
    if(nk==m) {
      n=nk;
      m=m?m<<1:DMAX; /* m==0 happens: db resets a dict's capacity to its key
                        count (b.c), which for an EMPTY dict is 0, and 0<<1
                        stays 0 -- the append below then wrote past a
                        zero-element resize (AFL sig 11, 2026-07-14) */
      if(m>DICTMAX) m=DICTMAX; /* keep capacity in i32 range even while count still grows toward the cap */
      k=kresize(pd[0],m);
      v=kresize(pd[1],m);
      pd[2]=t(1,(u32)m);
      pk=px(k);
      pv=px(v);
      nk=n; nv=n;
    }
    pk[nk++]=key;
    pv[nv++]=k_(val);
    if(pi) {
      /* maintain the table: over half full rebuilds (dixbuild reads the new
       * count), else insert and move the stamp with the count */
      if(2*(nk+1)>h) dixbuild(d);
      else { tab[hs]=(i32)nk; pi[0]=(i32)nk; }
    }
  }
  if(cp) _k(cp); /* drop our local ref to the copy; the dict now owns it */
  if(!(nk&63)) DCHK(d);
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
    int c;
    if(i==n(k0) && i==n(k1)) break;
    else if(i==n(k0) && i<n(k1)) {r=r?r:-1;break;}    /* count */
    else if(i<n(k0) && i==n(k1)) {r=r?r:1;break;}     /* count */
    else if((c=strcmp(pk0[i],pk1[i]))) {r=c<0?-1:1;break;}     /* key */
    else if((c=kcmpr(pv0[i],pv1[i]))) {r=c<0?-1:1;break;}      /* value */
    ++i;
  }
  return r;
}

#ifdef FUZZING
#include <stdlib.h>  /* abort */
/* Invariant check for the fuzz build: a violated dict invariant aborts, so
   AFL perceives semantic corruption -- not just memory errors -- as a crash.
   Checks: key/value counts agree; a slot-3 table is an int vector of pow2+1
   length; and when the table is stamped valid, every key is reachable
   through it (the property dget relies on to skip the linear scan). */
void dcheck(K d) {
  K *pd=px(d);
  u64 nn=n(pd[0]);
  if(n(pd[1])!=nn) abort();
  if(n(b(48)&d)>3 && pd[3]!=null) {
    u64 h=n(pd[3])-1;
    if(-1!=T(pd[3])||n(pd[3])<2||(h&(h-1))) abort();
    i32 *pi=px(pd[3]);
    if((u64)pi[0]==nn) {
      char **pk=px(pd[0]);
      u64 j,s;
      i32 e;
      for(j=0;j<nn;j++) {
        s=dhash_(pk[j])&(h-1);
        while((e=pi[1+s])) { if((u64)e<=nn&&pk[e-1]==pk[j]) break; s=(s+1)&(h-1); }
        if(!e) abort();
      }
    }
  }
}
#endif
