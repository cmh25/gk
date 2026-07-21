#include "scope.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#ifdef _WIN32
#include "systime.h"
#include "win_unistd.h"
#define getpid() _getpid()
#else
#include "sys/time.h"
#include <unistd.h>
  static int __gettimeofday(struct timeval * tp, struct timezone * tzp) {
    return gettimeofday(tp,tzp);
  }
#endif
#include "fn.h"
#include "watch.h"

/* scopea[] slots.  Only NAMESPACE scopes live here now (see scope_new_), so this
   bounds how many namespaces a workspace may have, not how many closures or
   calls it may make -- which is what it used to bound, at 1000, by hard-exiting.
   8 bytes a slot, so 16384 costs 128KB and is far past any real ktree. */
#define SM 16384

#ifdef _WIN32
#define strtok_r strtok_s
#endif

static K scopea[SM];
static char *spt,*spT;
K ks,gs,cs;
K ktree,C,Z,D;
K locals[LOCALSMAX];

/* global variable cache (GCACHEN + the inline gcache_get fast path live in
   scope.h so name-resolution callers can short-circuit before the call chain) */
char *gcachek[GCACHEN];
K gcachev[GCACHEN];

void gcache_clear(void) {
  i(GCACHEN,gcachek[i]=0;gcachev[i]=0)
}

void scope_init(char **args, int nargs) {
  char hn[256];
  K a;
  ks=scope_newk(null,t(4,sp("")));
  gs=scope_newk(ks,t(4,sp(".k")));
  cs=k_(gs); /* cs starts out in `.k */
  K *pks=px(ks);
  K *pgs=px(gs);
  ktree=pks[1];
  Z=dnew();
  K *pk=px(ktree); char **pkk=px(pk[0]); K *pkv=px(pk[1]);
  pkk[0]=sp("k"); pkv[0]=k_(pgs[1]);
  pkk[1]=sp("z"); pkv[1]=k_(Z);
  /* .r: every reserved name, mapped to the value the lexer emits for it (the
     builtin itself), so `!.r` lists the reserved words and `.r.sin` is sin.
     Written into the ktree directly like .k/.z above; the top-level
     single-letter guards (scope_set_ / dset) keep users from rebinding it. */
  { K R=dnew();
    i32 j,nn=kreserved_n();
    for(j=0;j<nn;j++) {
      char *rn=kreserved_i(j);
      K rv=kreserved_val(rn); if(!rv) rv=null;
      (void)dset(R,rn,rv); _k(rv);
    }
    pkk[2]=sp("r"); pkv[2]=R;
  }
  n(pk[0])=3; n(pk[1])=3;
  C=dnew();
  D=t(4,sp(".k"));
#ifdef _WIN32
  DWORD size=256;
  GetComputerNameA(hn,&size);
#else
  gethostname(hn,256);
#endif
  /* .z.i (classic k _i): the user arguments after the script -- interpreter name,
   * flags, and the script path itself are all excluded (main() collects them,
   * so flag position is irrelevant).  Empty list when no args were given. */
  if(nargs>0) {
    a=tn(0,nargs); K *pa=px(a);
    i(nargs,char *a_=args[i];pa[i]=tnv(3,strlen(a_),xmemdup(a_,1+strlen(a_))))
  }
  else a=tn(0,0);
  spt=sp("t");
  spT=sp("T");
  K *pz=px(Z); char **pzk=px(pz[0]); K *pzv=px(pz[1]);
  pzk[0]=sp("h"); pzv[0]=t(4,sp(hn));
  pzk[1]=sp("P"); pzv[1]=t(1,(u32)getpid());
  pzk[2]=sp("i"); pzv[2]=a;
  pzk[3]=spt; pzv[3]=null;
  pzk[4]=spT; pzv[4]=null;
  pzk[5]=sp("f"); pzv[5]=null;
  pzk[6]=sp("w"); pzv[6]=t(1,0);
  n(pz[0])=7; n(pz[1])=7; pz[2]=t(1,7);
  i(LOCALSMAX,locals[i]=tn(0,DMAX))
}

/* Slot 1 of .z is the "P" (pid) entry; this matches the layout written by
 * scope_init() above. After fork() the child inherited the parent's value,
 * so callers (currently only ipc.c's forking-listener child path) must
 * refresh it. We overwrite the int K in-place; if the runtime allocates
 * a fresh K for t(1,...) we leak ~16 bytes per fork, which is negligible
 * for a per-connection cost. */
void scope_refresh_pid(void) {
  K *pz = px(Z);
  K *pzv = px(pz[1]);
  pzv[1] = t(1, (u32)getpid());
}

/* .z.w = fd of the peer whose message is currently being dispatched to
 * .m.s / .m.g / .m.c. Zero outside handler context. Stored as a scalar
 * int K (t(1,...)), which is pointer-tagged and never heap-allocated,
 * so overwriting in place is leak-free. */
void scope_set_z_w(int fd) {
  K *pz = px(Z);
  K *pzv = px(pz[1]);
  pzv[6] = t(1, (u32)fd);
}

/* See scope.h. dset retains its value, so for a freshly built path vector we
 * drop our creating ref afterward (the dict then owns the lone ref); the
 * displaced previous value is freed by dset's update path. _set returns the
 * old value retained so _restore can put it back and drop the held ref. */
K scope_set_z_filepath(char *path) {
  K old = dget(Z, sp("filepath"));   /* retained ref, or 0 if key absent */
  K v = path ? tnv(3,strlen(path),xmemdup(path,1+strlen(path))) : null;
  (void)dset(Z, sp("filepath"), v);
  if(v!=null) _k(v);
  return old;
}
void scope_restore_z_filepath(K old) {
  (void)dset(Z, sp("filepath"), old?old:null);
  if(old) _k(old);
}

static K scope_new_(K p, K k) {
  K r,*prk;
  int i;
  PRK(6);
  prk[0]=k_(p);
  prk[1]=dnew();
  prk[2]=k?k:null;
  prk[3]=t(1,0); /* closure? */
  prk[4]=t(1,0); /* a lambda parents to this frame (fnd/fncp_ set it -- the
                    only two places parentage is created).  fne_'s epilogue
                    checks it to decide whether a return can capture the
                    frame; frames that never define a local lambda skip the
                    conversion walk entirely.  Set at parse time, so it also
                    covers literals never assigned to a local ({,{c*x}}). */
  prk[5]=t(1,0); /* borrowed */
  /* Only a NAMESPACE scope goes in scopea[] -- the table is how they get torn
     down at exit, and how scope_find() looks one up by path.  A lambda frame is
     owned by its lambda and needs neither, so it does not scan the table at all
     (it used to, on every scope creation, and then hit the full-table check even
     though it was never going to register).  Running out is a `limit` error the
     caller can trap, not a bare printf and exit(1) -- which was not a k error,
     could not be trapped, and exited 0. */
  if(k) {
    for(i=0;i<SM;i++) if(!scopea[i]) break;
    if(i==SM) { _k(r); return kerror("limit"); }
    scopea[i]=k_(r);
  }
  return r;
}
K scope_new(K p) { return scope_new_(p,0); }
K scope_newk(K p, K k) { return scope_new_(p,k); }

void scope_free(K s) {
  /* note: variable dict has to be freed first since scopes can have circular references */
  K *psu=px(s);
  dfree(psu[1]);
  psu[1]=null;
  _k(s);
}

void scope_free_all(void) {
  i(SM,if(!scopea[i]) break; scope_free(scopea[i]))
  i(LOCALSMAX,n(locals[i])=0;_k(locals[i]))
}

int scope_vktp(char *x) {
  int v=1,s=0;
  while(*x) {
    if(!v) break;
    switch(s) {
    case 0:
      if(*x=='.') s=1;
      else if(isalpha((unsigned char)*x)) s=2;
      else v=0;
      break;
    case 1:
      if(isalpha((unsigned char)*x)) s=2;
      else v=0;
      break;
    case 2:
      if(*x=='.') s=1;
      else if(!isalnum((unsigned char)*x)) v=0;
      break;
    }
    ++x;
  }
  if(s==1) v=0; // .k.
  return v;
}

/* n is a global reference. ex: .k.a.b.c */
static K ktree_get(char *n) {
  K r=0,q;
  char s[256],*p,*t;
  if(!*n) return k_(ktree);
  if(!scope_vktp(n)) return KERR_DOMAIN;
  memcpy(s,n,1+strlen(n));
  p=strtok_r(s,".",&t);
  r=dget(ktree,sp(p));
  while((p=strtok_r(0,".",&t))) {
    if(!r) return KERR_VALUE;
    if(0x80!=s(r)) { _k(r); return KERR_VALUE; }
    q=dget(r,sp(p));
    _k(r); r=q;
  }
  return r?r:KERR_VALUE;
}

static K t0(void) { /* ts */
  return t(1,(u32)(time(0)-2051222400l)); /* 2035-1970 */
}

static K tt0(void) { /* TS */
  struct timeval tv;
  __gettimeofday(&tv,0);
  double d = tv.tv_sec - 2051222400l; /* 2035-1970 */
  return t2(d/60/60/24);
}

static K scope_get_local(K s, int n) {
  K *ps=px(s);
  K *pd=px(ps[1]);
  K *pv=px(pd[1]);
  return k_(pv[n]);
}
static void setz(void) {
  K t;
  t=t0(); (void)dset(Z,spt,t); _k(t);
  t=tt0(); (void)dset(Z,spT,t); _k(t);
  if(fnestacki>=0) (void)dset(Z,sp("f"),fnestack[fnestacki]);
  else (void)dset(Z,sp("f"),null);
}
static K scope_get_(K s, char *n) {
  K r=0,t=0,*ps=px(s),q;
  char *p,*rp;
  char nn[256];

  /* check global cache first (for global scope lookups of simple names) */
  if(s==gs && n[0] != '.')
    i(GCACHEN,if(gcachek[i]==n) return k_(gcachev[i]));

  if(!*n) { setz(); return k_(ktree); }
  if(strlen(n)>255) return KERR_LENGTH;
  if(n[0]=='.') {
    if(n[1]=='z' && (n[2]==0 || n[2]=='.')) setz();
    return ktree_get(n);
  }
  if(ps[2]!=null) { /* refresh scope dict from ktree */
    q=ktree_get(sk(ps[2]));
    if(E(q)) return q;
    if(0x80!=s(q)) { _k(q); return KERR_VALUE; }
    _k(ps[1]); ps[1]=q;
  }
  if(strchr(n,'.')) {
    memcpy(nn,n,1+strlen(n));
    p=strtok_r(nn,".",&rp);
    if(p) {
      r=scope_get_(s,sp(p));
      while((p=strtok_r(0,".",&rp))) {
        if(!p) return KERR_VALUE;
        if(0x80!=s(r)) { _k(r); return KERR_VALUE; }
        t=dget(r,sp(p));
        _k(r); r=t;
      }
    }
    return r?r:KERR_VALUE;
  }
  else {
    // this can happen like this:
    // .[`;nul;,;"0"]
    // .[`;nul;,]
    // pretty sure this can't happen anymore
    //if(0x80!=s(ps[1])) return KERR_VALUE;
    r=dget(ps[1],n);
    if(!r && ps[0]!=null && s!=gs) r=scope_get_(ps[0],n);
    if(!r && s!=gs) r=scope_get_(gs,n);
    if(!r) r=dget(C,n);

    /* add to global cache */
    if(s==gs && r && !E(r)) {
      memmove(&gcachek[1],&gcachek[0],(GCACHEN-1)*sizeof(char*));
      memmove(&gcachev[1],&gcachev[0],(GCACHEN-1)*sizeof(K));
      gcachek[0]=n;
      gcachev[0]=r;
    }

    return r?r:KERR_VALUE;
  }
}

K scope_get(K s, K n) {
  if(4==T(n)) return scope_get_(s,sk(n));
  else return scope_get_local(s,ik(n));
}
static K scope_set_local(K s, int n, K v) {
  K *ps,*pd,*pv;
  char **pk;
  if(s==gs) {
    /* handle a situation like {x::1} */
    ps=px(cs);
    pd=px(ps[1]);
    pk=px(pd[0]);
    return scope_set(s,t(4,pk[n]),v);
  }
  ps=px(s);
  pd=px(ps[1]);
  pv=px(pd[1]);
  _k(pv[n]);
  pv[n]=k_(v);
  return v;
}

/*
 - consumes v (caller transfers ownership)
 - returns p (the stored value, caller owns one reference)
 - on error, v is freed and error is returned
 */
static K scope_set_(K s, char *n, K v) {
  K d,e=0,m,*psu,w;
  char *rp,nn[256],*t,*ss,*u;
  K es=s,*pes;
  ko *kd;
  int copy=0, gcopy=0;
  if(strlen(n)>255) { _k(v); return KERR_LENGTH; }

  /* clear global cache on any global set */
  if(s == gs) gcache_clear();
  psu=px(s);

  if(psu[2]!=null) { /* refresh scope dict from ktree */
    K q=ktree_get(sk(psu[2]));
    if(E(q)) { _k(v); return q; } /* every error exit consumes v; this one
                                     (namespace erased out from under \d,
                                     e.g. via .[`;...]) leaked the value */
    if(0x80!=s(q)) { _k(q); _k(v); return KERR_VALUE; }
    _k(psu[1]); psu[1]=q;
  }

  if(0x80==s(v)&&(s==gs||s==ks)) { // d.c:.k - copy shared dicts when setting to global
    kd=(ko*)(b(48)&v); if(kd->r>0) gcopy=1;
  }
  else if(0x80==s(v)&&s!=gs&&s!=ks) { // copy shared dicts from global when setting to local scope
    kd=(ko*)(b(48)&v); if(kd->r>1) gcopy=1;  // only if refcount > 1 (shared)
  }
  if(s==ks && strlen(n)==1 && *n!='k' && *n!='m') { // top level, single letters reserved, except k and m (.m is the ipc namespace)
    _k(v); return KERR_RESERVED;
  }
  if(kreserved(sp(n))) { // reserved names
    _k(v); return KERR_RESERVED;
  }
  if(strchr(n,'.')) {
    if(n[0]=='.') {
      // cannot change current directory path
      char *skd=sk(D);
      if(skd==strstr(skd,n)) { _k(v); return KERR_DOMAIN; }
      // .m is a reserved top-level namespace (ipc); .m.x can still be set
      if(n[1]=='m' && n[2]==0) {
        _k(v); return KERR_RESERVED;
      }
      es=ks;
    }
    if(!scope_vktp(n)) { _k(v); return KERR_DOMAIN; }
    memcpy(nn,n,1+strlen(n));
    u=t=ss=strtok_r(nn,".",&rp);

    pes=px(es);
    d=dget(pes[1],sp(ss));
    if(!d) d=dnew();
    else if(0x80!=s(d)) { _k(d); _k(v); return KERR_VALUE; }
    kd=(ko*)(b(48)&d);
    if(kd->r>1) { --kd->r; d=kcp(d); if(E(d)) { _k(v); return d; } } /* copy on write */
    e=m=k_(d);
    K par=0; char *pkey=0;  /* parent node of e and the key linking it, for COW relink */
    while((ss=strtok_r(0,".",&rp))) {
      t=ss; e=d; _k(d);
      if(v==e) copy=1;  /* v in traversal path - need to copy */
      d=dget(e,sp(ss));
      if(d) { if(0x80!=s(d)) break; }
      else d=dnew();
      kd=(ko*)(b(48)&e);
      /* copy-on-write: copy the shared node BEFORE releasing its refcount.
         If kcp fails (e.g. KERR_STACK while deep-copying a circular value),
         leaving --kd->r already applied would double-decrement e: the manual
         drop plus the _k(m) below (m still owns e through the traversal tree)
         would underflow e's refcount and double-free it at teardown.
         The copy must also be relinked into its parent: the parent still
         physically points at the original e (set by the prior iteration's
         dset below), so dset(par,pkey,cp) swaps in the copy and drops that
         reference properly. Without it the original is under-counted (the bare
         --kd->r assumes a transfer that never happens) and double-freed, and
         the copy is orphaned. The top node (par==0) is owned via m and written
         back by scope_set_ below, so it just needs the bare drop. */
      if(kd->r>1 && v!=e) {
        K cp=kcp(e); if(E(cp)) { _k(d); _k(m); _k(v); return cp; }
        if(par) { (void)dset(par,pkey,cp); _k(cp); } else --kd->r;
        e=cp;
      }
      (void)dset(e,sp(ss),d);
      par=e; pkey=t;
    }
    _k(d);
    if(!strtok_r(0,".",&rp)) {
      if(copy||gcopy) { w=kcp(v); if(E(w)) { _k(m); _k(v); return w; } (void)dset(e,sp(t),w); _k(v); }
      else { (void)dset(e,sp(t),v); w=v; }
      K m2=scope_set_(es,sp(u),m);  /* consumes m */
      if(E(m2)) { _k(w); return m2; }
      _k(m2);  /* free the returned tree ref - we only need w */
      /* A fully-qualified absolute assign (`.ns.var:...`) writes the leaf
         into namespace `.ns`; fire that namespace's watch.  Relative dotted
         assigns (`a.b:...`) need no handling here: their writeback recursion
         above already hit the simple-name hook for the top variable in the
         current scope. */
      if(nwatch && n[0]=='.') watch_fire_fq(n);
      return w;
    }
    else  { _k(m); _k(v); return KERR_VALUE; }
  }
  else {
    // this can happen like this:
    // .[`;nul;,;"0"]
    // .[`;nul;,]
    if(0x80!=s(psu[1])) { _k(v); return KERR_VALUE; }
    /* in-place amend writeback: @[`d;ky;:;y] amends the stored dict and
       assigns the SAME object back (kamendi4).  The shared-dict copy rule
       exists to stop a fetched dict from aliasing a SECOND slot; storing a
       slot's own value back creates no new alias, and copying here is what
       made every by-name dict amend O(n) -- the whole map, per key. */
    if(gcopy) { w=kcp(v); if(E(w)) { _k(v); return w; } (void)dset(psu[1],sp(n),w); _k(v); }
    else { (void)dset(psu[1],sp(n),v); w=v; }
    /* Fire for a write into ANY namespace scope (slot 2 non-null), not just the
       current one.  A `::` inside a fn defined under `\d foo` writes foo's
       globals even when called from elsewhere (scope_home), so gating on
       s==gs skipped the trigger for exactly those writes -- the variable
       changed and its watch stayed silent.  A lambda frame has a null slot 2,
       so locals still never fire. */
    if(nwatch && null!=psu[2]) watch_fire(s,sp(n));
    return w;
  }
}

K scope_home(void) { /* see scope.h */
  K sc=cs,*ps;
  while(sc!=null && sc!=ks) {
    ps=px(sc);
    if(null!=ps[2]) return sc;
    sc=ps[0];
  }
  return gs;
}

K scope_set(K s, K n, K v) {
  K *ps=px(s);
  if(1==ik(ps[5])) { /* scope dict vals are borrowed, make copy */
    K d=ps[1]; K *pd=px(d);
    /* cv (not v) so we don't shadow the param: on copy failure (e.g. a stack
     * error mid deep-copy of a value holding a lambda) the value-to-assign v is
     * owned here and must be freed, else it leaks. */
    K cv=kcp(pd[1]); if(E(cv)) { _k(v); return cv; }
    cv=kresize(cv,ik(pd[2]));
    n(cv)=n(pd[1]);
    pd[1]=cv;
    ps[5]=t(1,0);
  }
  if(4==T(n)) return scope_set_(s,sk(n),v);
  else return scope_set_local(s,ik(n),v);
}

K scope_cp(K s) {
  K e, s2, *ps2;
  K *ps=px(s);
  /* A namespace scope is SHARED, never copied.  This walks up the parent chain
     to snapshot the lambda frames a closure captured; the namespace scopes above
     them are global, and a closure must see a global's CURRENT value, not one
     frozen when it was created (`g:1; u:{c:x;{c+g}}; v:u 10; g:2; v[]` is 12).
     Copying them was pure waste -- scope_get_ refreshes a namespace scope's dict
     from the ktree on every lookup, so the copied dicts were thrown away on use.
     It was also the source of two real bugs: every closure dragged in a copy of
     the whole global scope AND THE KTREE, whose lambdas' scopes pointed back
     into the copy, so refcounting could never free it (t254: "extra copy of
     ktree was getting created and never freed").  That is what the scopea[]
     registration below used to paper over -- at the cost of a permanent extra
     reference and a slot per copy, which killed the interpreter outright once
     the table filled. */
  if(s==ks || null!=ps[2]) return k_(s);
  s2=tn(0,6); ps2=px(s2);
  ps2[0]=scope_cp(ps[0]); if(E(ps2[0])) { e=ps2[0]; _k(s2); return e; }
  ps2[1]=dcp(ps[1]); if(E(ps2[1])) { e=ps2[1]; _k(s2); return e; }
  ps2[2]=ps[2];
  ps2[3]=ps[3];
  ps2[4]=t(1,0); /* snapshots never exit-convert; the flag stays down */
  ps2[5]=t(1,0);

  /* Do NOT register the copy in scopea[].  That table exists so the NAMESPACE
     scopes -- which nothing else owns, and which scope_find() has to be able to
     look up by path -- can be torn down at exit; scope_newk puts them there and
     scope_free_all frees them.  A scope_cp copy is a closure snapshot: it is
     owned by the closure that captured it and dies with it, by refcount.
     Registering it here did two things, both bad.  It pinned a permanent extra
     reference (`k_(s2)`), so no closure snapshot was EVER freed while the
     process ran.  And it burned a slot per copy -- several, since this function
     recurses up the parent chain -- out of a fixed table of SM, at which point
     gk printed "error: scope_new() i==SM" and called exit(1): untrappable, no k
     error, and it even exits 0.  Creating a few hundred closures killed the
     interpreter (`u:{c:x;{c*x}}; do[400;u 3]` on v4). */
  return s2;
}

K scope_find(char *x) {
  int i;
  for(i=0;i<SM;i++) {
    if(!scopea[i]) break;
    K *ps=px(scopea[i]);
    if(ps[2]==t(4,sp(x))) break;
  }
  if(i==SM) {
    printf("error: scope_find() i==SM\n");
    exit(1);
  }
  return scopea[i];
}
