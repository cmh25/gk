#include "nt.h"
#include "irecur.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* --- Miller-Rabin primality test (deterministic for 64-bit) --- */

/* modular multiplication avoiding overflow: (a * b) mod m */
static u64 mulmod(u64 a, u64 b, u64 m) {
  u64 r = 0;
  a %= m;
  while(b) {
    if(b & 1) r = (r + a) % m;
    a = (a * 2) % m;
    b >>= 1;
  }
  return r;
}

/* modular exponentiation: (base^exp) mod m */
static u64 powmod(u64 base, u64 exp, u64 m) {
  u64 r = 1;
  base %= m;
  while(exp) {
    if(exp & 1) r = mulmod(r, base, m);
    exp >>= 1;
    base = mulmod(base, base, m);
  }
  return r;
}

/* Miller-Rabin witness test */
static int mr_witness(u64 n, u64 d, int r, u64 a) {
  u64 x = powmod(a, d, n);
  if(x == 1 || x == n - 1) return 1; /* probably prime */
  for(int i = 0; i < r - 1; i++) {
    x = mulmod(x, x, n);
    if(x == n - 1) return 1;
  }
  return 0; /* composite */
}

/* deterministic Miller-Rabin for all 64-bit integers */
static int is_prime(u64 n) {
  if(n < 2) return 0;
  if(n == 2 || n == 3) return 1;
  if(n % 2 == 0) return 0;
  if(n < 9) return 1;
  if(n % 3 == 0) return 0;

  /* write n-1 = 2^r * d */
  u64 d = n - 1;
  int r = 0;
  while((d & 1) == 0) { d >>= 1; r++; }

  /* witnesses that work for all n < 2^64 */
  static u64 witnesses[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
  for(int i = 0; i < 12; i++) {
    u64 a = witnesses[i];
    if(a >= n) continue;
    if(!mr_witness(n, d, r, a)) return 0;
  }
  return 1;
}

/* --- GCD (Euclidean algorithm) --- */

static i64 gcd_i(i64 a, i64 b) {
  if(a < 0) a = -a;
  if(b < 0) b = -b;
  while(b) { i64 t = b; b = a % b; a = t; }
  return a;
}

/* --- Extended Euclidean algorithm for modinv --- */
/* returns x such that (a * x) mod m = 1, or -1 if no inverse */
static i64 modinv_i(i64 a, i64 m) {
  if(m <= 0) return -1;
  a = a % m;
  if(a < 0) a += m;
  if(a == 0) return -1;

  i64 m0 = m, x0 = 0, x1 = 1;
  while(a > 1) {
    if(m == 0) return -1; /* gcd != 1 */
    i64 q = a / m;
    i64 t = m;
    m = a % m;
    a = t;
    t = x0;
    x0 = x1 - q * x0;
    x1 = t;
  }
  if(x1 < 0) x1 += m0;
  return x1;
}

/* --- Sieve of Eratosthenes (odd-only) --- */

/* sieve up to n, returns bitmap where sieve[i] = is_prime(2*i+1) for i>0, sieve[0]=is_prime(2) */
static u8 *make_sieve(u64 n) {
  if(n < 2) return NULL;
  u64 sz = (n / 2) + 1;  /* indices 0..n/2 cover odds 1,3,5,...,n (and 2 at [0]) */
  u8 *sieve = xmalloc(sz);
  memset(sieve, 1, sz);
  sieve[0] = 1;  /* 2 is prime */
  /* sieve[i] for i>0 represents 2*i+1 */
  /* mark 1 as not prime: index for 1 is (1-1)/2 = 0, but we use 0 for 2 */
  /* Actually: let's use sieve[0] for 2, sieve[i] for 2*i+1 when i>0 */
  /* So: sieve[0]=prime(2), sieve[1]=prime(3), sieve[2]=prime(5), etc */
  
  u64 sqrtn = (u64)sqrt((double)n) + 1;
  for(u64 i = 1; 2*i+1 <= sqrtn; i++) {
    if(sieve[i]) {
      u64 p = 2*i + 1;  /* the prime */
      /* mark multiples of p starting at p*p */
      /* p*p = (2i+1)^2 = 4i^2 + 4i + 1, index = (p*p - 1)/2 = 2i^2 + 2i */
      for(u64 j = (p*p - 1) / 2; j < sz; j += p) {
        sieve[j] = 0;
      }
    }
  }
  return sieve;
}

static inline int sieve_lookup(u8 *sieve, i32 v) {
  if(v < 2) return 0;
  if(v == 2) return 1;
  if(v % 2 == 0) return 0;
  return sieve[(v - 1) / 2];
}

/* --- prime_(K x): primality predicate --- */

K prime_(K x) {
  if(s(x)) return kerror("type");

  switch(tx) {
  case 1: {
    i32 v = ik(x);
    return t(1, (u32)(v >= 2 && is_prime((u64)v) ? 1 : 0));
  }
  case -1: {
    u64 len = nx;
    i32 *p = px(x);
    
    /* find max to decide strategy */
    i32 mx = 0;
    for(u64 i = 0; i < len; i++) {
      if(p[i] > mx) mx = p[i];
    }
    
    K r = tn(1, len);
    i32 *pr = px(r);
    
    /* use sieve if max is reasonable and input is dense-ish */
    if(mx >= 2 && mx <= 100000000 && (u64)mx < 8 * len) {
      u8 *sieve = make_sieve((u64)mx);
      for(u64 i = 0; i < len; i++) {
        pr[i] = sieve_lookup(sieve, p[i]);
      }
      xfree(sieve);
    } else {
      /* sparse or huge: per-element Miller-Rabin */
      for(u64 i = 0; i < len; i++) {
        i32 v = p[i];
        pr[i] = (v >= 2 && is_prime((u64)v)) ? 1 : 0;
      }
    }
    return r;
  }
  case 0:
    return irecur1(prime_, x);
  default:
    return kerror("type");
  }
}

/* --- factor_(K x): prime factorization --- */

K factor_(K x) {
  if(s(x)) return kerror("type");

  /* int vector: factor each element */
  if(tx == -1) {
    u64 len = nx;
    i32 *p = px(x);
    K r = tn(0, len);
    K *pr = px(r);
    for(u64 i = 0; i < len; i++) {
      K f = factor_(t(1, (u32)p[i]));
      if(E(f)) { _k(r); return f; }
      pr[i] = f;
    }
    return r;
  }

  /* list: use irecur1 */
  if(tx == 0)
    return irecur1(factor_, x);

  /* scalar int */
  if(tx != 1)
    return kerror("type");

  i64 n = ik(x);

  if(n < 0) n = -n;
  if(n <= 1) return tn(1, 0); /* empty for 0, 1 */

  /* trial division */
  u64 un = (u64)n;
  u64 cap = 16, cnt = 0;
  i32 *factors = xmalloc(cap * sizeof(i32));

  /* factor out 2s */
  while((un & 1) == 0) {
    if(cnt >= cap) { cap *= 2; factors = xrealloc(factors, cap * sizeof(i32)); }
    factors[cnt++] = 2;
    un >>= 1;
  }

  /* odd factors */
  for(u64 f = 3; f * f <= un; f += 2) {
    while(un % f == 0) {
      if(cnt >= cap) { cap *= 2; factors = xrealloc(factors, cap * sizeof(i32)); }
      factors[cnt++] = (i32)f;
      un /= f;
    }
  }

  /* remaining prime > sqrt(n) */
  if(un > 1) {
    if(cnt >= cap) { cap *= 2; factors = xrealloc(factors, cap * sizeof(i32)); }
    factors[cnt++] = (un <= INT32_MAX) ? (i32)un : INT32_MAX;
  }

  K r = tn(1, cnt);
  i32 *pr = px(r);
  for(u64 i = 0; i < cnt; i++) pr[i] = factors[i];
  xfree(factors);
  return r;
}

/* --- gcd_(K a, K x): greatest common divisor --- */

K gcd_(K a, K x) {
  if(s(a) || s(x)) return kerror("type");

  i8 ta_ = ta, tx_ = tx;

  /* list cases: use irecur2 */
  if(ta_ == 0 || tx_ == 0)
    return irecur2(gcd_, a, x);

  /* scalar-scalar */
  if(ta_ == 1 && tx_ == 1) {
    i64 g = gcd_i(ik(a), ik(x));
    return t(1, (u32)(g > INT32_MAX ? INT32_MAX : (i32)g));
  }

  /* scalar-vector */
  if(ta_ == 1 && tx_ == -1) {
    i64 av = ik(a);
    u64 len = nx;
    i32 *px_ = px(x);
    K r = tn(1, len);
    i32 *pr = px(r);
    for(u64 i = 0; i < len; i++) {
      i64 g = gcd_i(av, px_[i]);
      pr[i] = (g > INT32_MAX) ? INT32_MAX : (i32)g;
    }
    return r;
  }

  /* vector-scalar */
  if(ta_ == -1 && tx_ == 1) {
    i64 xv = ik(x);
    u64 len = na;
    i32 *pa_ = px(a);
    K r = tn(1, len);
    i32 *pr = px(r);
    for(u64 i = 0; i < len; i++) {
      i64 g = gcd_i(pa_[i], xv);
      pr[i] = (g > INT32_MAX) ? INT32_MAX : (i32)g;
    }
    return r;
  }

  /* vector-vector */
  if(ta_ == -1 && tx_ == -1) {
    if(na != nx) return kerror("length");
    u64 len = na;
    i32 *pa_ = px(a), *px_ = px(x);
    K r = tn(1, len);
    i32 *pr = px(r);
    for(u64 i = 0; i < len; i++) {
      i64 g = gcd_i(pa_[i], px_[i]);
      pr[i] = (g > INT32_MAX) ? INT32_MAX : (i32)g;
    }
    return r;
  }

  return kerror("type");
}

/* --- lcm_(K a, K x): least common multiple --- */

static i64 lcm_i(i64 a, i64 b) {
  if(a < 0) a = -a;
  if(b < 0) b = -b;
  if(a == 0 || b == 0) return 0;
  return (a / gcd_i(a, b)) * b;
}

K lcm_(K a, K x) {
  if(s(a) || s(x)) return kerror("type");

  i8 ta_ = ta, tx_ = tx;

  /* list cases: use irecur2 */
  if(ta_ == 0 || tx_ == 0)
    return irecur2(lcm_, a, x);

  /* scalar-scalar */
  if(ta_ == 1 && tx_ == 1) {
    i64 l = lcm_i(ik(a), ik(x));
    return t(1, (u32)(l > INT32_MAX ? INT32_MAX : (i32)l));
  }

  /* scalar-vector */
  if(ta_ == 1 && tx_ == -1) {
    i64 av = ik(a);
    u64 len = nx;
    i32 *px_ = px(x);
    K r = tn(1, len);
    i32 *pr = px(r);
    for(u64 i = 0; i < len; i++) {
      i64 l = lcm_i(av, px_[i]);
      pr[i] = (l > INT32_MAX) ? INT32_MAX : (i32)l;
    }
    return r;
  }

  /* vector-scalar */
  if(ta_ == -1 && tx_ == 1) {
    i64 xv = ik(x);
    u64 len = na;
    i32 *pa_ = px(a);
    K r = tn(1, len);
    i32 *pr = px(r);
    for(u64 i = 0; i < len; i++) {
      i64 l = lcm_i(pa_[i], xv);
      pr[i] = (l > INT32_MAX) ? INT32_MAX : (i32)l;
    }
    return r;
  }

  /* vector-vector */
  if(ta_ == -1 && tx_ == -1) {
    if(na != nx) return kerror("length");
    u64 len = na;
    i32 *pa_ = px(a), *px_ = px(x);
    K r = tn(1, len);
    i32 *pr = px(r);
    for(u64 i = 0; i < len; i++) {
      i64 l = lcm_i(pa_[i], px_[i]);
      pr[i] = (l > INT32_MAX) ? INT32_MAX : (i32)l;
    }
    return r;
  }

  return kerror("type");
}

/* --- modinv_(K a, K x): modular multiplicative inverse --- */

K modinv_(K a, K x) {
  if(s(a) || s(x)) return kerror("type");

  i8 ta_ = ta, tx_ = tx;

  /* list cases: use irecur2 */
  if(ta_ == 0 || tx_ == 0)
    return irecur2(modinv_, a, x);

  /* scalar-scalar */
  if(ta_ == 1 && tx_ == 1) {
    i64 inv = modinv_i(ik(a), ik(x));
    if(inv < 0) return kerror("domain");
    return t(1, (u32)(inv > INT32_MAX ? INT32_MAX : (i32)inv));
  }

  /* scalar-vector */
  if(ta_ == 1 && tx_ == -1) {
    i64 av = ik(a);
    u64 len = nx;
    i32 *px_ = px(x);
    K r = tn(1, len);
    i32 *pr = px(r);
    for(u64 i = 0; i < len; i++) {
      i64 inv = modinv_i(av, px_[i]);
      if(inv < 0) { _k(r); return kerror("domain"); }
      pr[i] = (inv > INT32_MAX) ? INT32_MAX : (i32)inv;
    }
    return r;
  }

  /* vector-scalar */
  if(ta_ == -1 && tx_ == 1) {
    i64 mv = ik(x);
    u64 len = na;
    i32 *pa_ = px(a);
    K r = tn(1, len);
    i32 *pr = px(r);
    for(u64 i = 0; i < len; i++) {
      i64 inv = modinv_i(pa_[i], mv);
      if(inv < 0) { _k(r); return kerror("domain"); }
      pr[i] = (inv > INT32_MAX) ? INT32_MAX : (i32)inv;
    }
    return r;
  }

  /* vector-vector */
  if(ta_ == -1 && tx_ == -1) {
    if(na != nx) return kerror("length");
    u64 len = na;
    i32 *pa_ = px(a), *px_ = px(x);
    K r = tn(1, len);
    i32 *pr = px(r);
    for(u64 i = 0; i < len; i++) {
      i64 inv = modinv_i(pa_[i], px_[i]);
      if(inv < 0) { _k(r); return kerror("domain"); }
      pr[i] = (inv > INT32_MAX) ? INT32_MAX : (i32)inv;
    }
    return r;
  }

  return kerror("type");
}
