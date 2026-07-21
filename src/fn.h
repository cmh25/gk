#ifndef FN_H
#define FN_H

#include "k.h"
#include "scope.h"
#include "p.h"

extern K fnestack[EVALDEPTH];
extern int fnestacki;

#define FN_VALENCE(v) (ik(v)&0xff)
#define FN_FMIDX(v) ((ik(v)>>8)&0xff)
#define FN_VF(val,fm) t(1,((val)&0xff)|(((fm)&0xff)<<8))

void fninit(void);
K fnnew(char *f);
void fnfree(K f);
K fnpd(K x);  /* resolve a 0xc9/0xca/0xcb predefined-fn token/value to its k.core lambda */
K fnd(K f);
K fncp(K f);
/* Rebuild a deserialized closure (see fn.c): db hands the captured bindings
   back as a dict in the lambda's scope slot; this turns them into a frozen
   parent frame.  A no-op for any other value. */
K fnrestore(K f);
K fne_(K f, K x, char *av);
K fne(K f, K x, char *av);
K fne_fast(K f, K x);
K fapply(K f, K x, char *av_outer);
/* closure-snapshot cycle collector (fn.c): break every parked snapshot's
   back-references and release the registry, so refcounted teardown can free
   the rings.  Call once, from exit__, before the K state is torn down. */
void cc_shutdown(void);

/* fn_inner: reach through 0xd9 (projection) / 0xda (adverbed) wrapper chains
   to the function they apply -- slot 0, recursively.  The inner 0xc3 is what
   carries a scope, so everything that re-parents, collects, or ships a
   snapshot's lambdas (closure_siblings, cc_dead/cc_free, fncap, fnrestore)
   looks through wrappers with this; a bare value passes through unchanged. */
static inline K fn_inner(K w) {
  while(0xd9==s(w)||0xda==s(w)) w=((K*)px(w))[0];
  return w;
}

/* closure()/closure_any()/proj_captures() live in fn.c now: their only
   callers (fne_, fne_fast) are there, and they grew a deep walk over the
   conversion surface -- see the surface comment in fn.c. */
K merge_args(K p, K x);
K proj_own_args(K args);
K wrap_proj(K f, K args);

#endif /* FN_H */
