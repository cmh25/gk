#ifndef FN_H
#define FN_H

#include "k.h"
#include "scope.h"
#include "p.h"

extern K fnestack[1024];
extern int fnestacki;

void fninit(void);
K fnnew(char *f);
void fnfree(K f);
K fnd(K f);
K fncp(K f);
K fne_(K f, K x, char *av);
K fne(K f, K x, char *av);
K fne_fast(K f, K x);
K fapply(K f, K x, char *av_outer);

/* closure: if x is a 0xc3 lambda whose scope is s0, replace its scope
   with a captured copy.  Defined here as static inline so callers
   (fne_, fne_fast) can inline it on the hot lambda-return path
   without an indirect call -- making this non-static visibly
   regressed p005 by breaking LTO.  Returns the (possibly copy-
   modified) lambda; returns 3 (KERR_TYPE) for any non-0xc3 input --
   callers gate on 0xc3 to avoid that path. */
static inline K closure(K x, K s0, K closurescope) {
  K r=3; K *_px,_s,*_fs,*_pr;
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
K merge_args(K p, K x);
K wrap_proj(K f, K args);

#endif /* FN_H */
