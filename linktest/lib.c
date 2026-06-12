/* Test object for `f 2:(e;t)` link object code (see linktest/test.k).
   Uses the public gk ABI (gk.h): reads arguments with gk_<t>, builds results
   with gk_mk<t> -- which exercises the callback into gk's exported allocators.
   gk.h is copied next to this file by linktest/run.py at build time. */
#include "gk.h"

/* immediate-atom returns */
GK_FN K t_answer(void)         { return gk_mki(42); }                    /* valence 0 */
GK_FN K t_neg(K a)             { return gk_mki(-gk_i(a)); }              /* valence 1 */
GK_FN K t_add(K a, K b)        { return gk_mki(gk_i(a) + gk_i(b)); }     /* valence 2 */
GK_FN K t_sum3(K a, K b, K c)  { return gk_mki(gk_i(a)+gk_i(b)+gk_i(c)); } /* valence 3 */

/* heap returns -- prove the .so resolved & called gk's exported allocators */
GK_FN K t_half(K a)            { return gk_mkf(gk_i(a) / 2.0); }         /* f64 atom */
GK_FN K t_greet(void)          { return gk_mkstr("gk"); }                /* char vector */
GK_FN K t_iota(K a) {                                                    /* i32 vector 0..n-1 */
  int32_t n = gk_i(a), i;
  K r = gk_mkiv(n);
  for(i = 0; i < n; ++i) gk_iv(r)[i] = i;
  return r;
}
