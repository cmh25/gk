#include "fn.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "p.h"
#include "av.h"
#include "repl.h"
#include "fe.h"

K fnestack[EVALDEPTH];   /* per-lambda-call stack; depth-indexed, must match A0/params (k.h) */
int fnestacki=-1;
static char* spf;

void fninit(void) {
  K f;
  f=fnnew("{x dvl,y}"); dset(C,sp("dv"),f); fnfree(f);
  f=fnnew("{$[@x;. di[. x;&|/(!x)=/$[@y;,y;y]];x@&@[(#x)#1;y;:;0]]}"); dset(C,sp("di"),f); fnfree(f);
  f=fnnew("{(dj _ x%86400;100 sv 24 60 60 vs x!86400)}"); dset(C,sp("gtime"),f); fnfree(f);
  f=fnnew("{gtime lt x}"); dset(C,sp("ltime"),f); fnfree(f);
  f=fnnew("{t+x-lt t:x+x-lt x}"); dset(C,sp("tl"),f); fnfree(f);
  f=fnnew("{((2##*x)#1,&#*x) lsq x}"); dset(C,sp("inv"),f); fnfree(f);
  f=fnnew("{rint exp lgamma[1+x]-lgamma[1+y]+lgamma[1+x-y]}"); dset(C,sp("choose"),f); fnfree(f);
  f=fnnew("{x*rint y%x}"); dset(C,sp("round"),f); fnfree(f);
  f=fnnew("{if[nul~x;:nul];i:1+2*!_.5*#x:(0,,/(0,+/~+\\ep[<;0,\"[\"=y]-ep[<;(\"]\"=y$:)],0)+/x ss $[3=4:y;$y;y])_ x;,/$[7=4:z;@[x;i;z];4:z$:;@[x;i;:;(#i)#,z];@[x;i;:;z]]}"); dset(C,sp("ssr"),f); fnfree(f);
  f=fnnew("{_[-1;x;y]}"); dset(C,sp("ep"),f); fnfree(f);
  spf=sp("f");
}

K fnnew(char *s) {
  K f,*pf;
  char *pd;
  if(!s||*s!='{') return null;
  f=tn(0,4);
  pf=px(f);
  pf[0]=tn(3,strlen(s)); /* definition */
  pf[1]=null;            /* parse result */
  pf[2]=null;            /* scope */
  pf[3]=FN_VF(0,0);      /* valence + force-monad FM[] index */
  pd=px(pf[0]);
  i(strlen(s),pd[i]=s[i])
  K p=fnd(f);
  if(p) { _k(f); return p; }
  return st(0xc3,f);
}

void fnfree(K f) {
  _k(f);
}

/* resolve a predefined-fn token/value (0xc9/0xca/0xcb: gtime dv ssr ...) to a
   copy of its k.core lambda, stamping monad valence for the 0xc9 set.  Shared
   by p.c's reduce()/rpd (lexer-emitted tokens) and fe() (the same subtypes
   held as VALUES, e.g. fetched from .r).  Does not consume x. */
K fnpd(K x) {
  K f,*pf;
  if(!(f=dget(C,sk(x)))) return KERR_VALUE;
  K f2=kcp(f); _k(f); if(E(f2)) return f2;
  f=f2;
  pf=px(f);
  if(0xc9==s(x)) pf[3]=FN_VF(1,0);
  return f;
}

static K fncp_(K x) {
  K p;
  K f=tn(0,4);
  K *pf=px(f);
  K *px=px(x);
  pf[0]=kcp(px[0]); /* definition */
  if(E(pf[0])) { K e=pf[0]; _k(f); return e; }
  pf[1]=null;       /* parse result */
  pf[2]=null;       /* scope */
  pf[3]=FN_VF(0,0); /* valence + force-monad FM[] index */
  if((p=fnd(f))) { fnfree(f); return p; }
  /* A lambda scope's slot[0] (parent) is the environment its FREE variables
     resolve against.  fnd just bound the fresh copy's parent to the ambient
     cs -- but a COPY of a lambda must keep the SOURCE's captured environment,
     otherwise free variables rebind to whatever scope did the copy (dynamic
     scope; lost closures -- e.g. a local-capturing lambda passed to another
     function).  The source parent is correct whether it is a live lexical
     enclosing scope or a frozen closure snapshot (slot[3]==1). */
  if(px[2]!=null && pf[2]!=null) {
    K *sps=px(px[2]); K *nps=px(pf[2]);
    _k(nps[0]); nps[0]=k_(sps[0]);
    ((K*)px(nps[0]))[4]=t(1,1);  /* the copy parents there too (scope.c slot 4) */
  }
  return st(0xc3,f);
}
/* fncpproj_ retired in Pass 4 -- 0xc4 replaced by 0xd9. */
K fncp(K x) {
  if(0xc3==s(x)) return fncp_(x);
  return KERR_TYPE;
}

/* ---- the conversion SURFACE ------------------------------------------------
   One definition of "where can a captured lambda hide" is shared by every
   walker in this family:
     0xc3                the lambda itself
     0xd9 projection     slot 0 (the function) AND slot 1 (the held args)
     0xd7 seeded over    slot 0 (the seed -- may be a while-predicate lambda)
                         AND slot 1 (the fn; `3 g/`, `p g/`)
     0xda adverbed       slot 0
     0xc5 composition    slot 0 (the list of composed functions; `,g` etc.)
     plain list          (T==0, subtype 0 or 0x81) every element
     dict 0x80           every value
   Anything else -- vectors, atoms, builtins -- is left alone.
   The walkers: proj_captures (return-path trigger), closure_any (return-path
   conversion), closure_siblings (snapshot re-parent), cc_scan / cc_break
   (cycle collector), fnrestore's relink; and in b.c, fncap's ring test +
   fncap_strip (serialization).  They MUST agree: a lambda one walker
   re-parents is a back-reference every other walker has to see.  Each is
   depth-guarded with maxr and fails toward safety on overflow -- the trigger
   says yes (a wasted snapshot over a missed one), the collector says keep,
   the converters/relinkers stop (degrading to the old stale-frame value
   error, never a crash). */
static int surface_list(K w) { return 0==T(w) && (0==s(w)||0x81==s(w)); }

/* closure: if x is a 0xc3 lambda whose scope parent is s0, replace it with a
   copy whose scope parent is the snapshot.  A COPY because the returned value
   may share the K with the frame's own binding (returned by name); the
   snapshot side (closure_siblings) mutates in place instead, since dcp made
   its structure private.  Static inline on the hot lambda-return path --
   making this non-static visibly regressed p005 by breaking LTO. */
static inline K closure(K x, K s0, K closurescope) {
  K r=KERR_TYPE; K *_px,_s,*_fs,*_pr;
  if(0xc3==s(x)) {
    _px=px(x);
    _s=_px[2];
    if(_s==null) return x;
    _fs=px(_s);
    if(_fs[0]!=s0) return x;
    r=fncp(x);
    if(E(r)) { _k(x); return r; }
    _k(x);
    _pr=px(r);
    _s=_pr[2];
    _fs=px(_s);
    _k(_fs[0]);
    _fs[0]=k_(closurescope);
  }
  return r;
}

/* The walked value graph could be CYCLIC: d.b:@[;].k once stored a
   projection holding the LIVE .k inside the tree, closing .k -> d -> P ->
   .k, and a consume-per-visit walk over a cycle revisits nodes, so an
   error unwind (closure_any's depth guard) over-frees them: UAF.
   proj_own_args below now snapshots dict args at projection formation --
   the only known ring constructor -- but the walks keep this cycle cut as
   defense in depth.  wpath is the container ancestry of the walk in
   progress (proj_captures and closure_any run to completion before the
   other starts, so they can share it); on meeting an ancestor the walk
   stops -- that node is already being handled above.
   wpn <= walk depth <= maxr < EVALDEPTH. */
static K wpath[EVALDEPTH];
static i32 wpn=0;
static int on_wpath(K x) { i32 i; for(i=0;i<wpn;i++) if(wpath[i]==x) return 1; return 0; }

/* proj_captures: does x hold, anywhere on the surface, a lambda whose scope
   parent is s0 -- i.e. would this return capture the current frame?  Lets
   callers skip the scope_cp snapshot for shapes that don't need it: a
   projection of a builtin (`+[3]`), a projection of an already-closed
   lambda, a plain global returned by name.  The held-args walk is what makes
   `{c:1;g:{x y};h:{c+x};g[h]}` trigger -- the function slot `{x y}` captures
   nothing, the ARG h does. */
static int proj_captures(K x, K s0) {
  static int d=0;
  int r=0; u64 j; K *p;
  if(++d>maxr) { --d; return 1; }
#ifdef FUZZING
  /* The wpath/depth guards bound recursion DEPTH and cut cycles, but a shared
     (DAG) surface -- cheap for a converge to build, e.g. f\{x#y}3 -- still
     re-walks each shared subtree along every path: exponential-but-finite, an
     AFL hang.  Charge one unit per node against the per-eval loop budget and
     bail conservatively (like the depth guard: "assume it captures"). */
  if(--gk_budget<0) { --d; return 1; }
#endif
  switch(s(x)) {
  case 0xc3: { K *pf=px(x); if(pf[2]!=null) { K *ps=px(pf[2]); r=ps[0]==s0; } break; }
  case 0xd9: case 0xd7: if(on_wpath(x)) break; wpath[wpn++]=x;
    p=px(x); r=proj_captures(p[0],s0)||proj_captures(p[1],s0); --wpn; break;
  case 0xda: if(on_wpath(x)) break; wpath[wpn++]=x;
    p=px(x); r=proj_captures(p[0],s0); --wpn; break;
  case 0xc5: if(on_wpath(x)) break; wpath[wpn++]=x; /* slot 0 = list of composed fns */
    p=px(x); r=proj_captures(p[0],s0); --wpn; break;
  case 0x80: { if(on_wpath(x)) break; wpath[wpn++]=x;
    p=px(x); K vl=p[1]; K *pv=px(vl);
    for(j=0;j<n(vl)&&!r;++j) r=proj_captures(pv[j],s0);
    --wpn;
    break; }
  default:
    if(surface_list(x) && !on_wpath(x)) { wpath[wpn++]=x;
      p=px(x); for(j=0;j<n(x)&&!r;++j) r=proj_captures(p[j],s0); --wpn; }
  }
  --d;
  return r;
}

/* closure_any: closure-convert a returned value over the whole surface --
   the bare lambda, a projection's function AND its held args, an adverbed
   lambda, and lambdas nested in lists/dicts inside those args.  Containers
   convert their slots in place and return themselves (the value may be
   shared with the frame's own binding -- so was the pre-existing slot-0
   in-place convert); each captured lambda is replaced by a re-parented copy
   (closure()).  Consumes x; on error the partially-converted container is
   freed and the error returned.  A cycle (on_wpath) hands x back untouched
   and unconsumed -- the visit above us owns the conversion. */
static K closure_any(K x, K s0, K closurescope) {
  static int d=0;
  u64 j; K *p, c;
  if(++d>maxr) { --d; _k(x); return KERR_STACK; }
  switch(s(x)) {
  case 0xc3: --d; return closure(x,s0,closurescope);
  case 0xd9: case 0xd7:
    if(on_wpath(x)) break;
    wpath[wpn++]=x;
    p=px(x);
    c=closure_any(p[0],s0,closurescope); if(E(c)) { p[0]=0; --wpn; --d; _k(x); return c; }
    p[0]=c;
    c=closure_any(p[1],s0,closurescope); if(E(c)) { p[1]=0; --wpn; --d; _k(x); return c; }
    p[1]=c;
    --wpn;
    break;
  case 0xda: case 0xc5:  /* 0xc5 slot 0 = list of composed fns */
    if(on_wpath(x)) break;
    wpath[wpn++]=x;
    p=px(x);
    c=closure_any(p[0],s0,closurescope); if(E(c)) { p[0]=0; --wpn; --d; _k(x); return c; }
    p[0]=c;
    --wpn;
    break;
  case 0x80: {
    if(on_wpath(x)) break;
    wpath[wpn++]=x;
    p=px(x); K vl=p[1]; K *pv=px(vl);
    for(j=0;j<n(vl);++j) {
      c=closure_any(pv[j],s0,closurescope);
      if(E(c)) { pv[j]=null; --wpn; --d; _k(x); return c; }
      pv[j]=c;
    }
    --wpn;
    break; }
  default:
    if(surface_list(x) && !on_wpath(x)) {
      wpath[wpn++]=x;
      p=px(x);
      for(j=0;j<n(x);++j) {
        c=closure_any(p[j],s0,closurescope);
        if(E(c)) { p[j]=0; --wpn; --d; _k(x); return c; }
        p[j]=c;
      }
      --wpn;
    }
  }
  --d;
  return x;
}

/* ---- closure-snapshot cycle collector --------------------------------------
   closure_siblings() below re-parents the lambdas INSIDE a closure snapshot to
   the snapshot itself, so a captured lambda can see the frame it lives in --
   that is what makes a captured sibling (`{c:5;f:{c*x};{f x}}`), local
   recursion (`{f:{$[x<1;0;x+f x-1]};{f x}}`) and mutual recursion work.  Each
   such back-reference closes a refcount ring
       S (snapshot) -> dict -> ... -> f -> (f's scope slot 0) -> S
   that refcounting alone can never free -- ~10KB a ring, since the snapshot
   dict is deep-copied and a copied lambda re-parses.  So every snapshot that
   received a back-reference is parked in this registry with ONE owning
   reference, and cc_sweep() frees the rings nothing outside can reach.

   The reachability test is exact, not heuristic, because gk has no non-owning
   references: every reference TO a snapshot is counted in its refcount (gk's
   r field is "extra refs", so owners(x) == rx(x)+1).  S is garbage iff its
   owners are exactly the back-references reachable over its dict values'
   surface plus the registry ref, AND every node on the way to each such
   lambda is owned solely by its holder -- dcp gave the snapshot private
   copies, so ANY extra reference anywhere along a path means something
   outside can still reach the ring (`h:u[]; g:h[]` fishes the sibling out of
   the snapshot: its refcount rises and the ring is kept).  Anything transient
   on the eval stack holds counted refs too, so a mid-eval sweep can only skip
   conservatively, never free early.  A ring that is alive now and dies later
   is picked up by a later sweep, and chains of rings unravel because cc_sweep
   loops until a pass frees nothing. */
static K *ccv=0;                 /* the registry: one owning ref per snapshot */
static u64 ccn=0,ccm=0;
static u64 ccthresh=256;         /* sweep when the registry grows past this */

/* count back-references to S over w's surface into *internal; 0 (with the
   count possibly partial) means a node is shared -- reachable from outside --
   so the caller must keep S.  Every heap node visited, including the dict
   slot value itself, must have exactly one owner. */
static int cc_scan(K w, K S, u64 *internal) {
  static int d=0;
  int r=1; u64 j; K *p;
  if(++d>maxr) { --d; return 0; }
  if(kh(w) && (u64)((ko*)(b(48)&w))->r+1!=1) { --d; return 0; }
  switch(s(w)) {
  case 0xc3: {
    K sc=((K*)px(w))[2];
    if(6!=T(sc) && ((K*)px(sc))[0]==S) ++*internal;
    break; }
  case 0xd9: case 0xd7: p=px(w); r=cc_scan(p[0],S,internal)&&cc_scan(p[1],S,internal); break;
  case 0xda: p=px(w); r=cc_scan(p[0],S,internal); break;
  case 0xc5: p=px(w); r=cc_scan(p[0],S,internal); break;
  case 0x80: { p=px(w); K vl=p[1]; K *pv=px(vl);
    for(j=0;j<n(vl)&&r;++j) r=cc_scan(pv[j],S,internal);
    break; }
  default:
    if(surface_list(w)) { p=px(w); for(j=0;j<n(w)&&r;++j) r=cc_scan(p[j],S,internal); }
  }
  --d;
  return r;
}

static int cc_dead(K S) {
  K *pc=px(S),*pd,vl,*pv;
  u64 j,internal=0;
  if(0x80!=s(pc[1])) return 0;
  pd=px(pc[1]); vl=pd[1]; pv=px(vl);
  for(j=0;j<n(vl);++j) if(!cc_scan(pv[j],S,&internal)) return 0;
  if(!internal) return 0;
  return (u64)((ko*)(b(48)&S))->r+1==internal+1;    /* +1 = the registry's own ref */
}

/* break the back-references on w's surface (cc_free below drops S itself) */
static void cc_break(K w, K S) {
  static int d=0;
  u64 j; K *p;
  if(++d>maxr) { --d; return; }
  switch(s(w)) {
  case 0xc3: {
    K sc=((K*)px(w))[2];
    if(6!=T(sc)) { K *fs=px(sc); if(fs[0]==S) { _k(fs[0]); fs[0]=null; } }
    break; }
  case 0xd9: case 0xd7: p=px(w); cc_break(p[0],S); cc_break(p[1],S); break;
  case 0xda: p=px(w); cc_break(p[0],S); break;
  case 0xc5: p=px(w); cc_break(p[0],S); break;
  case 0x80: { p=px(w); K vl=p[1]; K *pv=px(vl);
    for(j=0;j<n(vl);++j) cc_break(pv[j],S);
    break; }
  default:
    if(surface_list(w)) { p=px(w); for(j=0;j<n(w);++j) cc_break(p[j],S); }
  }
  --d;
}

/* Break S's back-references, then drop the owning ref passed in.  For a dead
   ring that frees the whole snapshot -- dict, lambdas, parse trees.  At
   shutdown it also runs on LIVE snapshots (the registry lets go regardless so
   plain refcounted teardown can free whatever still owns them); a surviving
   lambda is left with a null parent, which nothing can observe because no
   code runs once exit__ starts tearing down. */
static void cc_free(K S) {
  K *pc=px(S),*pd,vl,*pv;
  u64 j;
  if(0x80==s(pc[1])) {
    pd=px(pc[1]); vl=pd[1]; pv=px(vl);
    for(j=0;j<n(vl);++j) cc_break(pv[j],S);   /* S stays alive until the drop below */
  }
  _k(S);
}

static void cc_sweep(void) {
  u64 j; int progress=1;
  while(progress) {
    progress=0;
    for(j=0;j<ccn;) {
      if(cc_dead(ccv[j])) { cc_free(ccv[j]); ccv[j]=ccv[--ccn]; progress=1; }
      else ++j;
    }
  }
  ccthresh=ccn<128?256:2*ccn;  /* a sweep costs O(live): rescale so total cost stays linear */
}

/* Park a snapshot that closure_siblings gave nref back-references.
   nref==0 means no ring exists and refcounting alone frees it -- skip. */
static void cc_register(K S, u64 nref) {
  if(!nref) return;
  if(ccn==ccm) { ccm=ccm?2*ccm:64; ccv=xrealloc(ccv,ccm*sizeof(K)); }
  ccv[ccn++]=k_(S);
  if(ccn>=ccthresh) cc_sweep();
}

void cc_shutdown(void) {
  u64 j;
  for(j=0;j<ccn;++j) cc_free(ccv[j]);
  ccn=ccm=0; xfree(ccv); ccv=0;
  ccthresh=256;
}

/* re-parent the captured lambdas on one snapshot value's surface (see
   closure_siblings below).  In place, no copies: dcp made the snapshot's
   structure private. */
static void sib_(K w, K S, K s0, u64 *nref) {
  static int d=0;
  u64 j; K *p, cs1, e;
  if(++d>maxr) { --d; return; }
  switch(s(w)) {
  case 0xc3:
    if(6==T(((K*)px(w))[2])) {          /* never realized: give it a scope, here */
      cs1=cs; cs=k_(S);
      e=fnd(w);
      _k(cs); cs=cs1;
      if(!e) ++*nref;
    }
    else if(((K*)px(((K*)px(w))[2]))[0]==s0) {   /* realized against the live frame */
      K *fs=px(((K*)px(w))[2]);
      _k(fs[0]); fs[0]=k_(S);
      ++*nref;
    }
    break;
  case 0xd9: case 0xd7: p=px(w); sib_(p[0],S,s0,nref); sib_(p[1],S,s0,nref); break;
  case 0xda: p=px(w); sib_(p[0],S,s0,nref); break;
  case 0xc5: p=px(w); sib_(p[0],S,s0,nref); break;
  case 0x80: { p=px(w); K vl=p[1]; K *pv=px(vl);
    for(j=0;j<n(vl);++j) sib_(pv[j],S,s0,nref);
    break; }
  default:
    if(surface_list(w)) { p=px(w); for(j=0;j<n(w);++j) sib_(p[j],S,s0,nref); }
  }
  --d;
}

/* closure_siblings: make the lambdas captured INSIDE a snapshot resolve names
   against the snapshot itself.  scope_cp just deep-copied the frame; each
   copied lambda still points at the LIVE frame (fncp_ preserves the source
   parent), which the epilogue is about to reset -- so a sibling reached
   through the returned closure, a recursive f, mutually-recursive siblings,
   and a sibling under a wrapper or inside a projection's held args all
   value-errored.  Walk every dict value's surface, re-parenting each lambda
   whose parent is the live frame (s0) to the snapshot and realizing
   never-parsed ones against it, so the frame's names resolve for all of
   them.  Returns the number of back-references created; the caller hands
   that to cc_register, since each one closes a refcount ring only the
   collector can free. */
static u64 closure_siblings(K closurescope, K s0) {
  K *pc=px(closurescope),*pd,vl,*pv;
  u64 j,nref=0;
  if(0x80!=s(pc[1])) return 0;
  pd=px(pc[1]); vl=pd[1];
  pv=px(vl);
  for(j=0;j<n(vl);++j) sib_(pv[j],closurescope,s0,&nref);
  return nref;
}

/* realize the env-less lambdas on one restored binding's surface against sc,
   the frozen captured frame -- ring members ship env-less, bare or nested
   under wrappers / inside held args / in lists and dicts (fncap_strip in
   b.c); this is where they are linked back in.  Returns 0, or the fnd error
   if a shipped lambda's text does not parse. */
static K relink_(K w, K sc, u64 *nref) {
  static int d=0;
  u64 j; K *p, cs0, e=0;
  if(++d>maxr) { --d; return 0; }
  switch(s(w)) {
  case 0xc3:
    if(6==T(((K*)px(w))[2])) {
      cs0=cs; cs=k_(sc);
      e=fnd(w);
      _k(cs); cs=cs0;
      if(!e) ++*nref;
    }
    break;
  case 0xd9: case 0xd7: p=px(w); e=relink_(p[0],sc,nref); if(!e) e=relink_(p[1],sc,nref); break;
  case 0xda: p=px(w); e=relink_(p[0],sc,nref); break;
  case 0xc5: p=px(w); e=relink_(p[0],sc,nref); break;
  case 0x80: { p=px(w); K vl=p[1]; K *pv=px(vl);
    for(j=0;j<n(vl)&&!e;++j) e=relink_(pv[j],sc,nref);
    break; }
  default:
    if(surface_list(w)) { p=px(w); for(j=0;j<n(w)&&!e;++j) e=relink_(p[j],sc,nref); }
  }
  --d;
  return e;
}

/* Rebuild a deserialized closure.  bd ships a closure's captured bindings as a
   dict in the scope slot (see fncap in b.c); every other lambda ships `null`
   there and needs nothing done.  Give the lambda a real scope the ordinary way
   -- fnd() builds the parse result and a fresh scope -- then re-point that
   scope's parent at a frozen frame holding the bindings.  That is the same move
   fncp_ makes for a copy: slot[0] IS the environment free variables resolve
   against, and slot[3]==1 marks the frame a closure snapshot rather than a live
   enclosing scope.  Names the closure did not capture still resolve against the
   receiver's globals, exactly as they did before. */
K fnrestore(K f) {
  K *pf, cap, sc, *psc, *ps, p;
  if(0xc3!=s(f)) return f;
  pf=px(f);
  if(0x80!=s(pf[2])) return f;   /* not a closure blob */
  cap=pf[2];
  pf[2]=null;                    /* let fnd() build the real scope */
  p=fnd(f);
  if(p) { dfree(cap); _k(f); return p; }
  sc=scope_new(gs);              /* the frozen captured frame */
  psc=px(sc);
  dfree(psc[1]); psc[1]=cap;     /* its bindings ARE the captured dict */
  psc[3]=t(1,1);                 /* mark it a closure snapshot */
  ps=px(pf[2]);
  _k(ps[0]); ps[0]=sc;

  /* Realize any captured lambda that arrived WITHOUT an environment of its own,
     now, rather than leaving it for its first call -- because fnd() would then
     parent it to whatever cs happens to be at that moment.  Parent it to sc,
     the frozen captured frame: globals still resolve through it (its own
     parent is gs), and the lambda sees its siblings and itself, so a
     deserialized closure recurses and reaches mutual siblings exactly like a
     live one (closure_siblings gives live snapshots the same shape).  Each
     realization closes the frame -> lambda -> frame refcount ring, so the
     frame is parked with the cycle collector (cc_register above), which frees
     the ring once nothing outside it remains. */
  { K *pd=px(cap), vl=pd[1], *pv=px(vl), e=0; u64 j, nref=0;
    for(j=0;j<n(vl)&&!e;++j) e=relink_(pv[j],sc,&nref);
    cc_register(sc,nref);
    if(e) return e;              /* a shipped lambda's text does not parse */
  }
  return f;
}

#define SB(b,m,l,c) do { \
  if((l)==(m)) { (m)<<=1; (b)=xrealloc((b),(m)*sizeof(*(b))); } \
  (b)[(l)]=(c); \
} while (0)

K fnd(K f) {
  K p,r=0,*pf;
  char *ff=0,*ff0=0,*b,**v,*g,*h;
  int j,s,n,q,vx,vy,vz,ffq=0,first=1,params=1;
  int bm=32,vm=32;
  pf=px(f);
  ff=px(pf[0]); ff0=ff;
  s=j=n=q=vx=vy=vz=0;
  if(!*ff) return r;
  if(*ff!='{') return r;
  b=xcalloc(bm,1); v=xcalloc(vm*sizeof(char*),1);
  for(;*ff;++ff) {

    /* skip comments */
    if(s!=2 && first && *ff=='/') { ++ff; while(*ff&&*ff!='\n') ++ff; --ff; continue; }
    if(s!=2 && *ff== ' ' && ff[1] && ff[1]=='/') { ff+=2; while(*ff&&*ff!='\n') ++ff; --ff; continue; }
    if(s!=2 && ff>ff0 && (ff[-1]=='('||ff[-1]=='['||ff[-1]=='{') && *ff=='/') { ++ff; while(*ff&&*ff!='\n') ++ff; --ff; continue; }
    first=0; if(*ff=='\n') first=1;

    if(*ff=='"') { params=0; while(*ff&&*ff=='"') ff=xeqs(ff); }
    if(!*ff) { r=KERR_PARSE; goto cleanup; }
    switch(s) {
    case 0:
      if(*ff=='{') s=1;
      else { r=KERR_PARSE; goto cleanup; }
      break;
    case 1:
      while(*ff&&isblank((unsigned char)*ff))++ff;
      if(*ff=='"') while(*ff&&*ff=='"') ff=xeqs(ff);
      if(!*ff) { r=KERR_PARSE; goto cleanup; }
      if(*ff=='}') s=2;
      else if(*ff=='{') { n++; s=9; }
      else if(*ff=='['&&params) s=3;
      else if(*ff=='.') s=11;
      else if(*ff=='`') s=101;
      else if(isalpha((unsigned char)*ff)) { s=7; SB(b,bm,j,*ff); j++; }
      else if((i8)*ff<0) { r=KERR_PARSE; goto cleanup; }
      else s=6;
      params=0;
      break;
    case 101:
      /* mirror state 106: continue through `a`b symbol LISTS and catch the
         closing }.  Exiting on the second backtick made its letters scan as
         an identifier, so {`x`y} grew phantom x/y parameters (valence 2). */
      if(isalnum((unsigned char)*ff)) s=101;
      else if(*ff=='`') s=101;
      else if(*ff=='}') s=2;
      else s=6;
      break;
    case 2:
      r=KERR_PARSE; goto cleanup;
    case 3:
      if(isalpha((unsigned char)*ff)) { s=4; SB(b,bm,j,*ff); j++; ffq=1; }
      else if(*ff==']') s=5;
      else { r=KERR_PARSE; goto cleanup; }
      break;
    case 4:
      if(isalnum((unsigned char)*ff)) { s=4; SB(b,bm,j,*ff); j++; }
      else if(*ff==']') {
        s=5; SB(b,bm,j,0); j=0; SB(v,vm,q,sp(b)); q++;
      }
      else if(*ff==';') {
        s=3; SB(b,bm,j,0); j=0; SB(v,vm,q,sp(b)); q++;
      }
      break;
    case 5:
      if(*ff=='}') s=2;
      else if(*ff=='{') { n++; s=8; }
      else if(*ff=='.') s=15;
      else if(*ff=='`') s=105;
      else if(isalpha((unsigned char)*ff)) { s=7; SB(b,bm,j,*ff); j++; }
      else s=6;
      break;
    case 105:
      /* mirror state 106 (see state 101) */
      if(isalnum((unsigned char)*ff)) s=105;
      else if(*ff=='`') s=105;
      else if(*ff=='}') s=2;
      else s=6;
      break;
    case 6:
      if(*ff=='}') s=2;
      else if(*ff=='.') s=16;
      else if(*ff=='`') s=106;
      else if(isalpha((unsigned char)*ff)) { s=7; SB(b,bm,j,*ff); j++; }
      else if(*ff=='{') { n++; s=8; }
      else if((i8)*ff<0) { r=KERR_PARSE; goto cleanup; }
      else s=6;
      break;
    case 106:
      if(isalnum((unsigned char)*ff)) s=106;
      else if(*ff=='`') s=106;
      else if(*ff=='}') s=2;
      else s=6;
      break;
    case 7:
      if(*ff=='}') {
        s=2; SB(b,bm,j,0); j=0;
        if(!ffq) { /* no implicit xyz if there are formal parameters */
          if(!vx && !strcmp(b,"x")) { vx=1; SB(v,vm,q,sp(b)); q++; }
          if(!vy && !strcmp(b,"y")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } vy=1; SB(v,vm,q,sp("y")); q++; }
          if(!vz && !strcmp(b,"z")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } if(!vy) { vy=1; SB(v,vm,q,sp("y")); q++; } vz=1; SB(v,vm,q,sp("z")); q++; }
        }
      }
      else if(isalnum((unsigned char)*ff)) { SB(b,bm,j,*ff); j++; }
      else if(*ff=='{') {
        /* n++ like the other two `{` transitions (cases 5 and 6): state 10
           leaves on `n` reaching 0, so entering it without raising the depth
           drove n negative on the nested `}` and the scan never came back --
           every x/y/z AFTER a nested lambda that directly followed an
           identifier was missed, and the parameter silently became an unbound
           global (`{f{1};x}` -> value error, while `{f {1};x}` was fine). */
        n++; s=10; SB(b,bm,j,0); j=0;
        if(!ffq) { /* no implicit xyz if there are formal parameters */
          if(!vx && !strcmp(b,"x")) { vx=1; SB(v,vm,q,sp(b)); q++; }
          if(!vy && !strcmp(b,"y")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } vy=1; SB(v,vm,q,sp("y")); q++; }
          if(!vz && !strcmp(b,"z")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } if(!vy) { vy=1; SB(v,vm,q,sp("y")); q++; } vz=1; SB(v,vm,q,sp("z")); q++; }
        }
      }
      else if(*ff=='.') {
        /* dotted name: the HEAD may be an implicit x/y/z (x.y reads local
           x's field y), but the components after the dot are path segments,
           never parameters.  Falling into the generic flush sent the tail
           back through the identifier scanner, so {a.y} grew a phantom y
           parameter (and {x.y} registered both x AND y). */
        s=16; SB(b,bm,j,0); j=0;
        if(!ffq) { /* no implicit xyz if there are formal parameters */
          if(!vx && !strcmp(b,"x")) { vx=1; SB(v,vm,q,sp(b)); q++; }
          if(!vy && !strcmp(b,"y")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } vy=1; SB(v,vm,q,sp("y")); q++; }
          if(!vz && !strcmp(b,"z")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } if(!vy) { vy=1; SB(v,vm,q,sp("y")); q++; } vz=1; SB(v,vm,q,sp("z")); q++; }
        }
      }
      else { /* no implicit xyz if there are formal parameters */
        s=6; SB(b,bm,j,0); j=0;
        if(!ffq) { /* no implicit xyz if there are formal parameters */
          if(!vx && !strcmp(b,"x")) { vx=1; SB(v,vm,q,sp(b)); q++; }
          if(!vy && !strcmp(b,"y")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } vy=1; SB(v,vm,q,sp("y")); q++; }
          if(!vz && !strcmp(b,"z")) { if(!vx) { vx=1; SB(v,vm,q,sp("x")); q++; } if(!vy) { vy=1; SB(v,vm,q,sp("y")); q++; } vz=1; SB(v,vm,q,sp("z")); q++; }
        }
      }
      break;
    case 8:
      if(*ff=='}') { n--; if(!n) s=6; }
      else if(*ff=='{') n++; /* nested function */
      break;
    case 9:
      if(*ff=='}') { n--; if(!n) s=5; }
      else if(*ff=='{') n++; /* nested function */
      break;
    case 10:
      if(*ff=='}') { n--; if(!n) s=7; }
      else if(*ff=='{') n++; /* nested function */
      break;
    case 11: /* .z.s */
      if(isalnum((unsigned char)*ff)) s=11;
      else if(*ff=='.') s=11;
      else if(*ff=='}') s=2;
      else s=1;
      break;
    case 15: /* .z.s */
      if(isalnum((unsigned char)*ff)) s=15;
      else if(*ff=='.') s=15;
      else if(*ff=='}') s=2;
      else s=5;
      break;
    case 16: /* .z.s */
      if(isalnum((unsigned char)*ff)) s=16;
      else if(*ff=='.') s=16;
      else if(*ff=='}') s=2;
      else s=6;
      break;
    default: { r=KERR_PARSE; goto cleanup; }
    }
  }

  pf[2]=scope_new(cs);
  ((K*)px(cs))[4]=t(1,1); /* this lambda parents to cs: only a flagged frame's
                             return can capture it (scope.c slot 4).  Set at
                             parse/realize time -- zero per-call cost. */
  if(vz&&q<3) { SB(v,vm,q,sp("y")); q++; }
  if(vz&&q<3) { SB(v,vm,q,sp("x")); q++; }
  if(vy&&q<2) { SB(v,vm,q,sp("x")); q++; }
  pf[3]=FN_VF(q,0);
  i(q,p=scope_set(pf[2],t(4,sp(v[i])),null);if(E(p)) { r=p; goto cleanup; })
  if(q!=(int)n(((K*)px(((K*)((K*)px(pf[2])))[1]))[0])) { r=KERR_PARSE; goto cleanup; }  // {[a;b;b]a,b}
  K locals=tn(4,q); K *plocals=px(locals);
  i(q,plocals[i]=(K)sp(v[i]))

  /* parse */
  h=px(pf[0]);
  ++h;
  while(*h&&isblank((unsigned char)*h))++h;
  if(*h&&*h=='[') while(*h&&*h++!=']');
  g=xcalloc(1,5+strlen(h));
  memcpy(g,h,1+strlen(h));
  if(*g) g[strlen(g)-1]='\n';
  int opencode0=opencode;
  opencode=0;
  K cs0=cs; cs=k_(pf[2]);
  pf[1]=pgparse(g,1,locals);
  _k(cs); cs=cs0;
  opencode=opencode0;
  _k(locals);
  if(E(pf[1])) { r=pf[1]; pf[1]=null; }

  /* Force-monad recognition: a valence-1 body spelled exactly {Vx}, where
     V is a monadic primitive (one of P below, whose index is the FM[] slot),
     collapses at apply time to k(idx,0,arg) -- skipping scope setup + the
     pgreduce() interpreter loop.  We cache idx in pf[3]'s high bits (0 = not a force
     monad); the lambda otherwise stays a normal 0xc3 (prints verbatim,
     dispatches normally everywhere), so only fne_ short-circuits and there
     is no print/round-trip or fne()-handler surface to get wrong.  Requiring
     the canonical 4-char spelling means `{ ,x}`/`{,y}` stay ordinary
     lambdas. */
  pf[3]=FN_VF(q,0);
  if(!r && q==1 && n(pf[0])==4) {
    char *d0=px(pf[0]);
    static const char P[]=":+-*%&|<>=~.!@?#_^,$"; /* index == FM[] slot */
    if(d0[0]=='{' && d0[2]=='x' && d0[3]=='}') {
      const char *vp=strchr(P, d0[1]);
      if(vp && vp!=P) pf[3]=FN_VF(q,(int)(vp-P)); /* exclude ':' (idx 0, no monad) */
    }
  }
cleanup:
  xfree(b); xfree(v);
  return r;
}

// make a copy of r and parent scope
// closure only if parent scope == s0
// can't create a closure like this:
// f:{i+x}
// g:{i:0;f}
// closure() moved to fn.h as a static inline so the hot lambda-return
// path can inline it.  The 0xc4 branch was retired in Pass 4; the
// projection-of-projection closure case is left for a future pass.

K fne_(K f, K x, char *av) {
  K r=0,*pd,*ps,*pf,cs0,*px,t;
  static int d=0;
  u64 n,nn;

  if(++d>maxr || (!(d&7)&&stack_low())) { r=KERR_STACK; --d; goto cleanup; }

  pf=px(f);
  px=px(x);
  n=(u64)FN_VALENCE(pf[3]);
  int fmidx=FN_FMIDX(pf[3]); /* !=0: force monad {Vx}, FM[]/P index of V */

  u64 inc=0; i(nx,if(px[i]==inull)++inc)
  nn=nx-inc; //  if there are inulls, number of inulls must less than or equal to nx
  /* Project condition.  Without av, project on any short-fill.
     With av, project only when av is exactly "'" (each) -- "/"/"\\"
     and friends consume args (over/scan with optional seed take 1 or
     2 args regardless of inner valence) and let avdo handle valence
     itself.  The each case is what enables stacked partial-apply of
     an adverbed lambda, e.g. {x+y*z}'[2]'[3]'[4]. */
  /* Project (lift av to an outer 0xda) for each `'`, and for over/scan
     `/`/`\\` when the inner valence is >=3 -- those have no fold/seeded
     overload at that valence, so fewer args (or a gap) is under-application
     and must yield a projection, matching `'` and plain lambdas. (At
     valence 1/2, `/`/`\\` consume 1-2 args as fold/seed; leave to avdo.) */
  int proj_av = av && *av && av[1]==0 &&
                (av[0]=='\'' || ((av[0]=='/'||av[0]=='\\') && n>=3));
  if((nx<n||nn<n) && (!av||!*av||proj_av)) { /* project */
    /* Issue #2 Pass 3b-5: produce 0xd9(lambda, pargs) projection
       wrapper instead of legacy 0xc4 3-tuple.  Pass 4 extension:
       when av is "'" we lift it to an outer 0xda wrapper, giving
         0xd9(0xda(lambda, "'"), pargs)
       so subsequent applies merge args naturally. */
    K args=kcp(x);
    if(E(args)) { _k(f); _k(x); --d; return args; }
    _k(x);
    if(proj_av) {
      K w=tn(0,2); K *pw=px(w);
      pw[0]=f;  /* ownership transfers into 0xda */
      pw[1]=tnv(3,strlen(av),xmemdup(av,1+strlen(av)));
      K wrapped=st(0xda,w);
      --d;
      return wrap_proj(wrapped, args);
    }
    t=fncp(f);
    if(E(t)) { _k(f); _k(args); --d; return t; }
    _k(f);
    --d;
    return wrap_proj(t, args);
  }
  if(nx!=n && nx!=1 && n!=0 && (!av||!*av)) { r=KERR_VALENCE; --d; goto cleanup; }
  if(av&&*av) {
    if(inc) { r=KERR_VALENCE; --d; goto cleanup; } /* TODO: {y}'[!5;] */
    if(n==0) r=avdo(k_(f),0,k_(x),av);
    else if(n==1) {
      if(nx==1) r=avdo(k_(f),0,k_(px[0]),av);
      else if(0x81==s(x)) r=avdo(k_(f),0,k_(x),av);
      else if(!strcmp(av,"'")) r=avdo(k_(f),0,k_(x),av);
      else { r=KERR_VALENCE; --d; goto cleanup; }
    }
    else if(n==2) {
      /* Issue #2 Pass 9: dyadic avdo on `'` zips two args (eachfe
         dyadic).  For chained-each av (all `'` chars, length>1)
         that's wrong -- we need Cartesian iteration, which only
         happens in the monadic multi-char avdo path on a 0x81
         plist.  Route accordingly.  Other multi-char avs (e.g.
         `'/`, `\\`) keep their dyadic dispatch. */
      int chained_each=0;
      if(av && av[0]) {
        chained_each=1;
        for(char *p=av; *p; ++p) if(*p!='\'') { chained_each=0; break; }
        if(av[0] && !av[1]) chained_each=0; /* single ' is dyadic-eachfe-OK */
      }
      if(nx==2 && !chained_each) r=avdo(k_(f),k_(px[0]),k_(px[1]),av);
      else if(nx==2) r=avdo(k_(f),0,k_(x),av);
      else if(nx==1) r=avdo(k_(f),0,k_(px[0]),av);
      else if(0x81==s(x)) r=avdo(k_(f),0,k_(x),av);
      else { r=KERR_VALENCE; --d; goto cleanup; }
    }
    else if(0x81==s(x)) r=avdo(k_(f),0,k_(x),av);
    else { r=KERR_VALENCE; --d; goto cleanup; }
  }
  else {
    if(6==T(pf[2])) { K p=fnd(f); if(p) { _k(f); _k(x); --d; return p; } fmidx=FN_FMIDX(pf[3]); }
    /* force-monad fast path: {Vx} collapses to the monadic primitive,
       skipping scope setup + pgreduce.  fmidx!=0 holds the FM[] index;
       valence is 1 so the lone arg is px[0]. */
    if(fmidx && nx==1) { r=k(fmidx,0,k_(px[0])); --d; goto cleanup; }
    ps=px(pf[2]);     // scope
    pd=px(ps[1]);     // scope dict
    cs0=cs; cs=k_(pf[2]);
    K pd1save=pd[1];  // save scope dict values
    u64 NX=nx;
    pd[1]=x;
    K ps5save=ps[5];
    ps[5]=t(1,1);
    /* re-entrant activation (recursion): pd[1] is per-activation (borrow +
       scope_set's cow), but pd[0] (the local NAMES) is shared across all
       activations of f.  The callee's first-assigned locals dset their names
       over the caller's slots >= n, so after the call the caller's locals
       resolve to the wrong names (value error, or silently another slot).
       kk0>n means an enclosing activation appended locals: snapshot the tail
       and restore it in the epilogue.  Plain calls pay one compare. */
    u64 kk0=n(pd[0]),kkt=0; char **keysave=0,*keybuf[32];
    if(kk0>n) { kkt=kk0-n; keysave=kkt<=32?keybuf:xmalloc(kkt*sizeof(char*));
      char **pk0=px(pd[0]); i(kkt,keysave[i]=pk0[n+i]) }
    n(pd[0])=n; n(pd[1])=n;
    fnestack[++fnestacki]=f;
    int opencode0=opencode;
    opencode=0;
    RETURN=0;  /* clear early return flag from previous calls */
    r=pgreduce(pf[1],0);
    RETURN=0;  /* a call consumes its own return; RETURN must not leak into the
                  caller's expression.  pgreduce clears it in file mode (!REPL)
                  but left it set in the REPL, so `1+g 5` with g:{:x*2} wrongly
                  yielded 10 not 11.  Only a direct top-level/subconsole `:`
                  (which never runs through fne_) should survive to the REPL. */
    opencode=opencode0;
    --fnestacki;
    /* closure detection: a returned lambda or dict-of-lambdas captures this
       scope's locals.  fne_ reuses f's persistent scope (pf[2]) as the
       activation record, so EVERY exit path must run the epilogue below to
       restore pd[1]/ps[5]/nx -- otherwise a re-entrant activation of the same
       f (e.g. recursion, or a projection/closure result re-applied) sees a
       corrupted scope and frees the still-borrowed merged-args plist (UAF).
       When scope_cp fails (KERR_STACK on a circular/deep scope) we therefore
       drop the closure and goto the epilogue with the error as the result,
       rather than restoring partially and skipping it.  Mirrors fne_fast. */
    /* The gate is the frame's slot-4 flag (a lambda parents to this frame --
       set by fnd/fncp_ at creation, the only two parentage creators), then a
       walk of the WHOLE return value: any shape may carry a captured lambda
       out -- bare, projected/adverbed, a dict value, or nested in a plain
       list (`{c:5;f:{c*x};,f}` finally works).  Unflagged frames -- all code
       that defines no local lambda -- skip everything.  This subsumes the
       old two branches (bare 0xc3/0xd9, and dict-of-ALL-lambdas, which also
       wastefully snapshotted for dicts of non-capturing lambdas).  !E(r)
       first: pgreduce can return an error or the bare-\ abort sentinel (0),
       neither of which is walkable (the old s(r) checks skipped them). */
    if(!E(r) && ik(((K*)px(cs))[4]) && proj_captures(r,cs)) {
      K closurescope=scope_cp(cs);
      if(E(closurescope)) { _k(r); r=closurescope; goto lam_cleanup; }
      K *pc=px(closurescope); pc[3]=t(1,1);
      cc_register(closurescope,closure_siblings(closurescope,cs));
      r=closure_any(r,cs,closurescope);
      _k(closurescope);
    }

lam_cleanup:
    pd=px(ps[1]);   /* ps[1] may have been refreshed during the body (a var read
                       via scope_get_ swaps the path-bound scope dict from the
                       ktree -- _k(ps[1]);ps[1]=q), freeing the dict pd was cached
                       from at entry.  Re-fetch before touching it, as fne_fast's
                       cleanup does, or the writes below are a use-after-free. */
    if(ik(ps[5])==0) _k(pd[1]);
    pd[1]=pd1save;
    ps[5]=ps5save;
    nx=NX;
    if(kkt) { char **pk0=px(pd[0]); i(kkt,pk0[kk0-kkt+i]=keysave[i])
      if(keysave!=keybuf) xfree(keysave); }
    n(pd[0])=n(pd[1]); // locals could have been added, so sync key/val counts
    _k(cs); cs=cs0;
  }
  --d;
cleanup:
  _k(f);
  _k(x);
  return r;
}
/* Fast lambda apply: skips fne/fne_ function-call boundaries and the
   pgreduce() wrapper. Only valid when:
     - f is 0xc3 (not 0xc4 projection)
     - no adverbs on f
     - x is 0x81 plist with size matching valence and no inulls
     - f's scope dict isn't closure-pending (T==6)
   Caller must verify these. Consumes f and x. */
K fne_fast(K f, K x) {
  K *pf=px(f);
  i32 vf=ik(pf[3]);
  u64 nval=(u64)(vf&0xff);
  /* force-monad fast path: {Vx}[a] collapses to the monadic primitive.
     Caller guarantees x is a valence-matched plist with no inulls, so for
     a valence-1 force monad the lone arg is px(x)[0]. */
  if(vf>0xff) { int fmidx=(vf>>8)&0xff;
    if(fmidx) { K r=k(fmidx,0,k_(((K*)px(x))[0])); _k(f); _k(x); return r; } }
  K *ps=px(pf[2]);
  K *pd=px(ps[1]);

  K cs0=cs; cs=k_(pf[2]);
  K pd1save=pd[1];
  u64 NX=nx;
  pd[1]=x;
  K ps5save=ps[5];
  ps[5]=t(1,1);
  /* re-entrant activation: snapshot the shared local-names tail (see fne_) */
  u64 kk0=n(pd[0]),kkt=0; char **keysave=0,*keybuf[32];
  if(kk0>nval) { kkt=kk0-nval; keysave=kkt<=32?keybuf:xmalloc(kkt*sizeof(char*));
    char **pk0=px(pd[0]); i(kkt,keysave[i]=pk0[nval+i]) }
  n(pd[0])=nval; n(pd[1])=nval;
  fnestack[++fnestacki]=f;
  int opencode0=opencode;
  opencode=0;
  RETURN=0;

  /* drive body statements directly; lambda bodies don't have \t/\l/etc.
     mirror pgreduce()'s RETURN handling: clear RETURN on early return
     in non-REPL mode so do_/while_/etc. don't see it leak from the
     lambda. */
  K body=pf[1];
  K *pbody=px(body);
  u64 body_n=n(body);
  K r=null;
  for(u64 i=0;i<body_n;++i) {
    int q;
    K p=pgreduce_(pbody[i],&q);
    if(E(p)) { _k(r); r=p; break; }
    if(EXIT) { _k(p); _k(r); r=null; break; }
    _k(r); r=p;
    if(RETURN) { RETURN=0; break; }  /* a call consumes its own return (see fne_) */
  }

  opencode=opencode0;
  --fnestacki;

  /* closure detection: a returned lambda or dict-of-lambdas captures
     this scope's locals.  NOTE: fne_fast reuses f's persistent scope
     (pf[2]) as the activation record, so every exit path MUST run the
     epilogue below to restore pd[1]/ps[5]/nx -- otherwise a re-entrant
     activation of the same f sees the corrupted scope.  When scope_cp
     fails (e.g. KERR_STACK on a circular/deep scope) we therefore drop
     the closure and goto cleanup with the error as the result, rather
     than returning early and skipping the restore. */
  /* flag-gated any-shape capture (!E(r): error/abort returns) -- see fne_ */
  if(!E(r) && ik(((K*)px(cs))[4]) && proj_captures(r,cs)) {
    K closurescope=scope_cp(cs);
    if(E(closurescope)) { _k(r); r=closurescope; goto cleanup; }
    K *pc=px(closurescope); pc[3]=t(1,1);
    cc_register(closurescope,closure_siblings(closurescope,cs));
    r=closure_any(r,cs,closurescope);
    _k(closurescope);
  }

cleanup:
  pd=px(ps[1]);          /* ps[1] may have been refreshed during the body */
  if(ik(ps[5])==0) _k(pd[1]);
  pd[1]=pd1save;
  ps[5]=ps5save;
  nx=NX;
  if(kkt) { char **pk0=px(pd[0]); i(kkt,pk0[kk0-kkt+i]=keysave[i])
    if(keysave!=keybuf) xfree(keysave); }
  n(pd[0])=n(pd[1]);
  _k(cs); cs=cs0;
  _k(f); _k(x);
  return r;
}

K fne(K f, K x, char *av) {
  if(0xd9==s(f)) return fapply(f,x,av);
  if(0xda==s(f)) return fapply(f,x,av);
  if(0xc3!=s(f)) return fapply(f,x,av);
  return fne_(f, x, (av && *av) ? av : "");
}

static K fapply_impl(K f, K x, char *av_outer);
K fapply(K f, K x, char *av_outer) {
  static int d=0;
  if(++d>maxr || (!(d&7)&&stack_low())) { --d; _k(f); _k(x); return KERR_STACK; }
  K r=fapply_impl(f,x,av_outer);
  --d;
  return r;
}
static K fapply_impl(K f, K x, char *av_outer) {
  if(0xda==s(f)) {
    K *pw=px(f);
    K wf=k_(pw[0]);
    K wav=pw[1];
    char av2[256];
    av2[0]=0;
    if(T(wav)==-3 && n(wav)>0) {
      char *p=px(wav);
      snprintf(av2+strlen(av2),sizeof(av2)-strlen(av2),"%s",p);
    }
    if(av_outer && *av_outer) {
      snprintf(av2+strlen(av2),sizeof(av2)-strlen(av2),"%s",av_outer);
    }
    _k(f);
    return fapply(wf,x,av2);
  }
  if(0xd9==s(f)) {
    /* Issue #2 Pass 3a / 3b-5: (f;args) projection wrapper.
       Two cases:
         1. lx >= val(f): we have enough args to either complete
            this projection or partially apply the inner -- merge
            inline and recurse fapply on the inner.
         2. lx < val(f): not enough new args -> nest as
            0xd9(f, new_args). */
    K vf2=val(f);
    if(E(vf2)) { _k(f); _k(x); return vf2; }
    i32 vn2=ik(vf2); _k(vf2);
    u64 lx=(0x81==s(x))?n(x):1;
    if(vn2>0 && lx<(u64)vn2) {
      /* Nest: build 0xd9(f, new_args).  Issue #2 Pass 5 step 3:
         if av_outer is non-empty (e.g. a chained `'` from
         eachfe/avdo dispatch), preserve it by lifting onto an
         outer 0xda wrapper around the nested projection.
         Without this, `f'[2]'[3]` would lose the outer each
         between partial-apply steps.  Issue #2 Pass 9 fix:
         x may be a borrowed params[] pool plist (c3_apply's
         `xx=params[paramsi++]` pattern -- the K is reused
         across statements with slot[0] overwritten on next
         borrow).  Copy the elements into a fresh 0x81 list so
         the long-lived projection owns its own args storage. */
      K args_new;
      if(0x81==s(x)) {
        K *pxk=px(x);
        u64 ln=n(x);
        K t=tn(0,ln); K *pt=px(t);
        i(ln, pt[i]=k_(pxk[i]))
        _k(x);
        args_new=st(0x81,t);
      }
      else { K t=tn(0,1); ((K*)px(t))[0]=x; args_new=st(0x81,t); }
      K nested=wrap_proj(f, args_new);
      if(av_outer && *av_outer) {
        K w=tn(0,2); K *pw=px(w);
        pw[0]=nested;
        pw[1]=tnv(3,strlen(av_outer),xmemdup(av_outer,1+strlen(av_outer)));
        return st(0xda,w);
      }
      return nested;
    }
    K *pw=px(f);
    K wf=pw[0];
    K wargs=pw[1];
    K merged=merge_args(wargs,x);
    if(E(merged)) { _k(f); return merged; }
    K wf_owned=k_(wf);
    _k(f);
    return fapply(wf_owned,merged,av_outer);
  }
  /* For 0xc3 lambdas: fne() is the canonical entry.
     0xc4 retired in Pass 4 -- replaced by 0xd9 above. */
  if(0xc3==s(f)) return fne(f,x,av_outer);
  /* For everything else (primitive verbs, builtins, etc.) route
     through fe() which already knows how to unpack a 0x81 plist. */
  if(0x81==s(x)) {
    if(n(x)==1) {
      K *pxk=px(x); K x0=k_(pxk[0]); _k(x);
      return fe(f,0,x0,av_outer);
    }
    if(n(x)==2) {
      K *pxk=px(x); K a=k_(pxk[0]); K x1=k_(pxk[1]); _k(x);
      return fe(f,a,x1,av_outer);
    }
    /* 3+ args: leave the plist intact for fe()'s primitive
       3/4-arg dispatch (kamendi3/4 etc.). */
    return fe(f,0,x,av_outer);
  }
  return fe(f,0,x,av_outer);
}

/* Issue #2 Pass 3a.
   Merge new args x into a projection's bound args plist p:
   - If p contains any inull slots, fill them left-to-right from x.
     Excess x elements (beyond the count of inulls in p) are an
     error (KERR_VALENCE).
   - If p has no inull slots, append x's elements (legacy
     projection-collapse semantics: f[1][2] -> f[1;2]).
   x may be either an 0x81 plist or a single K (treated as a
   1-element plist). The result is always an 0x81 plist.
   Takes ownership of x. p remains untouched (caller owns it). */
K merge_args(K p, K x) {
  K xa;
  if(0x81==s(x)) xa=x;
  else { K t=tn(0,1); ((K*)px(t))[0]=x; xa=st(0x81,t); }
  K *pp=px(p); K *pxa=px(xa);
  u64 lp=n(p), lx=n(xa);
  u64 ni=0, nn=0;
  for(u64 i=0;i<lp;++i) if(pp[i]==inull) ++ni; else ++nn;
  /* Legacy projection-collapse semantics (matches fne_ at the
     0xc4 inner-apply path): fill any inulls in p from xa
     left-to-right, then append remaining xa elements past the
     end of p. If lx < ni, return KERR_VALENCE (not enough new
     args to fill all open slots -- legacy 0xc4 behavior).
     A partial-fill projection should be built upstream as a
     fresh wrap_proj instead of trying to merge here. */
  if(lx<ni) { _k(xa); return KERR_VALENCE; }
  u64 lr=nn+lx;
  K r=tn(0,lr); K *pr=px(r);
  u64 xi=0, j=0;
  for(u64 i=0;i<lp;++i) {
    if(pp[i]==inull) pr[j++]=k_(pxa[xi++]);
    else pr[j++]=k_(pp[i]);
  }
  while(xi<lx) pr[j++]=k_(pxa[xi++]);
  _k(xa);
  return st(0x81,r);
}

/* Issue #2 Pass 3a.
   Build an 0xd9(f, args) projection wrapper. f and args are
   incorporated by ownership transfer (caller transfers refs in).
   args must be an 0x81 plist (may contain inulls).

   If f is itself an 0xd9 wrapper, this is a partial-of-partial
   application -- the canonical shape is to flatten by merging
   the new args into f's existing args. We do NOT flatten here;
   that's fapply's job. Producers that need flattening should call
   fapply(f, args, 0) instead. */
/* Projected arguments are captured as independent VALUES -- never a
   reference.  A held arg that is a SHARED dict is deep-copied at
   formation: the live-tree shape @[;].k otherwise closes a refcount ring
   the moment it is stored back into the tree (.k -> d -> P -> .k), which
   refcounting can never reclaim.  Mirrors dset's root rule: only a shared
   (r>0) dict can alias existing structure; a fresh r==0 value is already
   private and skips the copy.  A SHALLOW scan is complete by induction:
   every other construction route (list/dict literals, tree stores)
   already privatizes embedded dicts, and a nested projection was itself
   rule-compliant at ITS formation.  Called at every 0xd9 formation site
   (here and the three direct packs in fe.c).  Returns 0, or the kcp
   error (KERR_STACK on a legacy cyclic dict). */
K proj_own_args(K args) {
  K *p=px(args); u64 j;
  for(j=0;j<n(args);++j)
    if(0x80==s(p[j]) && ((ko*)(b(48)&p[j]))->r>0) {
      K c=kcp(p[j]); if(E(c)) return c;
      _k(p[j]); p[j]=c;
    }
  return 0;
}

K wrap_proj(K f, K args) {
  K e=proj_own_args(args);
  if(e) { _k(f); _k(args); return e; }
  K w=tn(0,2); K *pw=px(w);
  pw[0]=f;
  pw[1]=args;
  return st(0xd9,w);
}
