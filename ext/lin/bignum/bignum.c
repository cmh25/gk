/* bignum.c -- arbitrary-precision integers for gk via 2: link object code,
 * backed by GNU MP (libgmp).
 *
 *   badd:"bignum.so" 2:("badd";2)
 *   badd["12345678901234567890";"98765432109876543210"]
 *       -> "111111111011111111100"
 *
 * The ABI is just gk char vectors (type -3): each big integer is its base-10
 * string.
 *
 * Build:  make   (gcc -O2 -shared -fPIC -I<gkdir> bignum.c -o bignum.so -lgmp) */
#include <gmp.h>
#include <stdlib.h>
#include <string.h>
#include "gk.h"

/* Parse a gk string as a base-10 integer into a freshly-init'd z. Accepts a
   char vector (-3) AND a char atom (3): gk collapses a 1-char literal like "1"
   to an atom, so a single-digit argument arrives as type 3, not -3.
   Returns 1 on success; on failure z is already cleared and 0 is returned. */
static int getz(K x, mpz_t z) {
  mpz_init(z);
  int t = gk_t(x);
  char *s;
  if (t == GK_CHARV) {                 /* char vector: copy its n bytes */
    size_t n = gk_n(x);
    s = (char *)malloc(n + 1);
    if (!s) { mpz_clear(z); return 0; }
    memcpy(s, gk_cv(x), n);
    s[n] = 0;
  } else if (t == GK_CHAR) {           /* char atom (e.g. "1"): one byte */
    s = (char *)malloc(2);
    if (!s) { mpz_clear(z); return 0; }
    s[0] = (char)gk_c(x);
    s[1] = 0;
  } else { mpz_clear(z); return 0; }
  int ok = (mpz_set_str(z, s, 10) == 0);   /* GMP: 0 == valid */
  free(s);
  if (!ok) mpz_clear(z);
  return ok;
}

/* Render z as a base-10 gk char vector (-3). */
static K mkz(const mpz_t z) {
  size_t sz = mpz_sizeinbase(z, 10) + 2;   /* digits + sign + NUL */
  char *buf = (char *)malloc(sz);
  if (!buf) return gk_err("wsfull");
  mpz_get_str(buf, 10, z);
  K r = gk_mkstrn(buf, (int32_t)strlen(buf));
  free(buf);
  return r;
}

#define BIGNUM_DYAD(name, op)                                   \
  GK_FN K name(K a, K b) {                                      \
    mpz_t x, y;                                                 \
    if (!getz(a, x))            return gk_err("domain");        \
    if (!getz(b, y)) { mpz_clear(x); return gk_err("domain"); } \
    op;                                                         \
    K r = mkz(x);                                               \
    mpz_clear(x); mpz_clear(y);                                 \
    return r;                                                   \
  }
BIGNUM_DYAD(badd, mpz_add(x, x, y))
BIGNUM_DYAD(bsub, mpz_sub(x, x, y))
BIGNUM_DYAD(bmul, mpz_mul(x, x, y))

/* division / remainder: guard divide-by-zero. We match gk's primitives, which
   are themselves inconsistent for negative operands: `div` truncates toward
   zero (-17 div 5 = -3, GMP tdiv_q) while `!` floors / takes the sign of the
   divisor (-17!5 = 3, GMP fdiv_r). So bdiv uses tdiv, bmod uses fdiv. */
GK_FN K bdiv(K a, K b) {
  mpz_t x, y;
  if (!getz(a, x)) return gk_err("domain");
  if (!getz(b, y)) { mpz_clear(x); return gk_err("domain"); }
  if (mpz_sgn(y) == 0) { mpz_clear(x); mpz_clear(y); return gk_err("domain"); }
  mpz_tdiv_q(x, x, y);
  K r = mkz(x); mpz_clear(x); mpz_clear(y); return r;
}
GK_FN K bmod(K a, K b) {
  mpz_t x, y;
  if (!getz(a, x)) return gk_err("domain");
  if (!getz(b, y)) { mpz_clear(x); return gk_err("domain"); }
  if (mpz_sgn(y) == 0) { mpz_clear(x); mpz_clear(y); return gk_err("domain"); }
  mpz_fdiv_r(x, x, y);   /* floored: result sign follows divisor, matches gk `!` */
  K r = mkz(x); mpz_clear(x); mpz_clear(y); return r;
}

/* base ^ exp; exp must be a non-negative integer fitting an unsigned long. */
GK_FN K bpow(K a, K b) {
  mpz_t x, e;
  if (!getz(a, x)) return gk_err("domain");
  if (!getz(b, e)) { mpz_clear(x); return gk_err("domain"); }
  if (mpz_sgn(e) < 0 || !mpz_fits_ulong_p(e)) {
    mpz_clear(x); mpz_clear(e); return gk_err("domain");
  }
  mpz_pow_ui(x, x, mpz_get_ui(e));
  K r = mkz(x); mpz_clear(x); mpz_clear(e); return r;
}

/* ---- monadic: n! ---------------------------------------------------------- */
GK_FN K bfac(K a) {
  mpz_t n;
  if (!getz(a, n)) return gk_err("domain");
  if (mpz_sgn(n) < 0 || !mpz_fits_ulong_p(n)) { mpz_clear(n); return gk_err("domain"); }
  unsigned long ni = mpz_get_ui(n);
  mpz_t r; mpz_init(r);
  mpz_fac_ui(r, ni);
  K k = mkz(r); mpz_clear(n); mpz_clear(r); return k;
}
