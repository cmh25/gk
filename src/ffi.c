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
GK_API K gk_mki(int32_t v)     { return t(1, (uint32_t)v); }
GK_API K gk_mkf(double v)      { return t2(v); }
GK_API K gk_mkj(int64_t v)     { return tj(v); }
GK_API K gk_mke(float v)       { return te(v); }
GK_API K gk_mkc(int v)         { return t(3, (uint8_t)v); }
GK_API K gk_mks(const char *s) { return t(4, sp((char*)s)); }
GK_API K gk_mknull(void)       { return null; }

/* ---- make a vector (uninitialized; the author fills it via gk_<t>v) -------
   A negative n is a caller bug (an author computing a length that went below
   zero). tn() takes a 64-bit count, so a negative n sign-extends to a ~2^63
   count and xmalloc tries a multi-exabyte allocation -> wsfull -> the whole
   process exits. Return a wsfull error K instead: that length IS unallocatable,
   so surface it (like the dict readers surface 'type on a non-dict) rather
   than silently handing back an empty vector the author didn't ask for. The
   standard fill loop `for(i=0;i<n;i++) gk_<t>v(r)[i]=..` never runs for n<0,
   so the error propagates back into gk without being dereferenced. */
/* The symmetric POSITIVE bound was missing, and it matters for the same reason:
   tn() multiplies the count by the element size with no overflow check, so a
   large positive n wraps.  gk_mkiv(1<<62) computed 4*2^62 == 0 (mod 2^64) and
   returned a ZERO-byte buffer behind a vector claiming 4611686018427387904
   elements -- `#b` reported it, `b@0` read out of the empty allocation, and
   `b@2000000000j` segfaulted.  Same for mkfv/mkjv/mkev/mksv (8n or 4n), and
   mkkv additionally slipped past libc in the BUDDY build.  VMAX is the
   existing ceiling (element counts above it can't have a byte size), so reuse
   it rather than inventing a per-type bound. */
#define GK_VCHK(n) do { if((n) < 0 || (int64_t)(n) >= (int64_t)VMAX) return KERR_WSFULL; } while(0)
GK_API K gk_mkiv(int64_t n) { GK_VCHK(n); return tn(1, n); }
GK_API K gk_mkfv(int64_t n) { GK_VCHK(n); return tn(2, n); }
GK_API K gk_mkjv(int64_t n) { GK_VCHK(n); return tn(8, n); }
GK_API K gk_mkev(int64_t n) { GK_VCHK(n); return tn(9, n); }
GK_API K gk_mkcv(int64_t n) { GK_VCHK(n); return tn(3, n); }
GK_API K gk_mksv(int64_t n) { GK_VCHK(n); return tn(4, n); }
GK_API K gk_mkkv(int64_t n) { GK_VCHK(n); return tn(0, n); }

/* ---- make a char vector from a C string ----------------------------------- */
GK_API K gk_mkstr(const char *s) {
  size_t n = strlen(s);
  K r = tn(3, (int64_t)n);   /* 64-bit count: don't truncate a huge string */
  if(n) memcpy(px(r), s, n);
  return r;
}
GK_API K gk_mkstrn(const char *s, int64_t n) {
  GK_VCHK(n);              /* negative n -> wsfull (see the constructors above) */
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
GK_API K gk_dnew(void)              { return dnew(); }
GK_API int  gk_isdict(K x)          { return 0x80 == s(x); }
/* dget/dset/dkeys/dvals read a value's slots AS a dict's [keys;vals;cap]; on a
   non-dict those slots are unrelated data, so the raw dict.c routines walk
   garbage pointers -> UB/SEGV. An author who passes the wrong value (there is
   no compile-time K subtype) should get a 'type error surfaced back into gk,
   not a crash or a silent empty result, so each reader checks the 0x80 dict
   subtype first (the internal idiom; gk_isdict is the public form) and returns
   KERR_TYPE otherwise. gk_dset returns void (can't signal), so it consumes v
   per its contract -- no crash, no leak -- and does nothing. */
GK_API void gk_dset(K d, const char *k, K v) {
  if(0x80!=s(d)) { _k(v); return; }
  dset(d, sp((char*)k), v); _k(v);
}
GK_API K    gk_dkeys(K d)           { return 0x80==s(d) ? dkeys(d) : KERR_TYPE; }
GK_API K    gk_dvals(K d)           { return 0x80==s(d) ? knorm(dvals(d)) : KERR_TYPE; }
GK_API K    gk_dget(K d, const char *k) {
  if(0x80!=s(d)) return KERR_TYPE;
  K r = dget(d, sp((char*)k));
  return r ? r : null;
}
GK_API K gk_dict(const char **keys, const K *vals, int64_t n) {
  K d = dnew();
  for(int64_t i = 0; i < n; i++) { dset(d, sp((char*)keys[i]), vals[i]); _k(vals[i]); }
  return d;
}
