/* ffi.c -- exported gk-ABI wrappers for 2: link object code.
 *
 * These are the only gk symbols a linked .so/.dll resolves at load time.
 *
 * The public surface is declared in gk.h. */
#include "k.h"
#include "dict.h"
#include <string.h>

#define GK_NO_K_TYPEDEF   /* use k.h's K, not gk.h's, to avoid a typedef clash */
#define GK_BUILD          /* we are gk: GK_API exports (dllexport on Windows) */
#include "gk.h"

/* ---- make an atom --------------------------------------------------------- */
GK_API K gk_mki(int32_t v)     { return t(1, (u32)v); }
GK_API K gk_mkf(double v)      { return t2(v); }
GK_API K gk_mkj(int64_t v)     { return tj(v); }
GK_API K gk_mke(float v)       { return te(v); }
GK_API K gk_mkc(int v)         { return t(3, (u8)v); }
GK_API K gk_mks(const char *s) { return t(4, sp((char*)s)); }
GK_API K gk_mknull(void)       { return null; }

/* ---- make a vector (uninitialized; the author fills it via gk_<t>v) ------- */
GK_API K gk_mkiv(int32_t n) { return tn(1, n); }
GK_API K gk_mkfv(int32_t n) { return tn(2, n); }
GK_API K gk_mkjv(int32_t n) { return tn(8, n); }
GK_API K gk_mkev(int32_t n) { return tn(9, n); }
GK_API K gk_mkcv(int32_t n) { return tn(3, n); }
GK_API K gk_mksv(int32_t n) { return tn(4, n); }
GK_API K gk_mkkv(int32_t n) { return tn(0, n); }

/* ---- make a char vector from a C string ----------------------------------- */
GK_API K gk_mkstr(const char *s) {
  size_t n = strlen(s);
  K r = tn(3, (i32)n);
  if(n) memcpy(px(r), s, n);
  return r;
}
GK_API K gk_mkstrn(const char *s, int32_t n) {
  K r = tn(3, n);
  if(n > 0) memcpy(px(r), s, (size_t)n);
  return r;
}

/* ---- utilities ------------------------------------------------------------ */
GK_API K    gk_err(const char *msg) { return kerror((char*)msg); }
GK_API K    gk_ref(K x)             { return k_(x); }
GK_API void gk_unref(K x)           { _k(x); }
GK_API K    gk_norm(K x)            { return knorm(x); }

/* ---- dictionaries --------------------------------------------------------- */
/* A gk dict is a 0x80-subtyped 3-list [keys(sym vec); vals(K list); m(cap)].
   These wrappers build/read it with direct C -- no interpreter round-trip.
   gk_dnew + gk_dset are the streaming core (build any dict, incl. a parse tree
   of unknown size, recursing inner-first); gk_dict is column sugar. */
GK_API K gk_dnew(void)                       { return dnew(); }
GK_API void gk_dset(K d, const char *k, K v) { dset(d, sp((char*)k), v); _k(v); }
GK_API int  gk_isdict(K x)                   { return 0x80 == s(x); }
GK_API K    gk_dkeys(K d)                    { return dkeys(d); }
GK_API K    gk_dvals(K d)                    { return knorm(dvals(d)); }
                          /* normalize the copy: a homogeneous column comes back
                             as a typed vector (knorm no-ops a mixed list) */
GK_API K    gk_dget(K d, const char *k) {
  K r = dget(d, sp((char*)k));
  return r ? r : null;
}
GK_API K gk_dict(const char **keys, const K *vals, int32_t n) {
  K d = dnew();
  for(int32_t i = 0; i < n; i++) { dset(d, sp((char*)keys[i]), vals[i]); _k(vals[i]); }
  return d;
}
