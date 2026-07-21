#include "nt.h"
#include "irecur.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef FUZZING
/* factor of a large reshape (or a column of many distinct hard 63-bit values
   that outrun the repeat cache) is a legitimately long job -- minutes -- which
   AFL flags as a hang.  Charge the shared per-eval budget per element so it
   converges to a fast 'limit like converge/while do. */
extern long gk_budget;
#endif

/* --- Miller-Rabin primality test (deterministic for 64-bit) --- */

/* Modular products here run in MONTGOMERY form (R = 2^64): one montmul is
   ~4 machine multiplies, where the portable overflow-safe alternative (the
   64-step shift-add "peasant" loop) is ~3000 cycles.  That difference is
   what makes Miller-Rabin and rho on a hard 63-bit value microseconds
   instead of milliseconds -- `factor` of a nested reshape holding ~500
   copies of one such value was a 19s fuzz hang.  All moduli are odd and
   < 2^63 (inputs are |i64| with the 2s divided out), which keeps every
   intermediate sum below 2^64.  Plain C: the 128-bit product high half
   comes from 32-bit splits. */

static u64 mulhi64(u64 a, u64 b) {
  u64 a0 = (u32)a, a1 = a >> 32, b0 = (u32)b, b1 = b >> 32;
  u64 t = a0 * b0;
  u64 u = a1 * b0 + (t >> 32);
  u64 v = a0 * b1 + (u32)u;
  return a1 * b1 + (u >> 32) + (v >> 32);
}

/* nneg = -(n^-1) mod 2^64 by Newton doubling (odd n: n*n = 1 mod 8, and
   each x *= 2-n*x doubles the correct low bits: 3 -> 6 -> ... -> 96) */
static u64 mont_nneg(u64 n) {
  u64 x = n;
  for(int i = 0; i < 6; i++) x *= 2 - n * x;
  return (u64)0 - x;
}

/* (a*b)/R mod n: a,b in Montgomery form.  lo+m*n = 0 mod 2^64, so that
   addition's carry is exactly (lo != 0). */
static u64 montmul(u64 a, u64 b, u64 n, u64 nneg) {
  u64 lo = a * b, hi = mulhi64(a, b);
  u64 m = lo * nneg;
  u64 t = hi + mulhi64(m, n) + (lo != 0);
  return t >= n ? t - n : t;
}

static u64 addmod_(u64 a, u64 b, u64 n) { /* a,b < n < 2^63 */
  u64 t = a + b;
  return t >= n ? t - n : t;
}

/* one = R mod n; r2 = R^2 mod n (to_mont(a) = montmul(a, r2)) */
static void mont_init(u64 n, u64 *nneg, u64 *one, u64 *r2) {
  *nneg = mont_nneg(n);
  u64 o = ((u64)0 - n) % n;
  u64 r = o;
  for(int i = 0; i < 64; i++) r = addmod_(r, r, n);
  *one = o; *r2 = r;
}

/* deterministic Miller-Rabin for all odd n < 2^63 (and trivial cases) */
static int is_prime(u64 n) {
  static const u64 witnesses[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
  if(n < 2) return 0;
  for(int i = 0; i < 12; i++) {
    if(n % witnesses[i] == 0) return n == witnesses[i];
  }
  if(n < 41 * 41) return 1;

  /* write n-1 = 2^r * d */
  u64 d = n - 1;
  int r = 0;
  while((d & 1) == 0) { d >>= 1; r++; }

  u64 nneg, one, r2;
  mont_init(n, &nneg, &one, &r2);
  u64 nm1 = n - one;                       /* Montgomery form of n-1 */
  for(int i = 0; i < 12; i++) {
    u64 b = montmul(witnesses[i], r2, n, nneg);   /* to_mont(witness) */
    u64 x = one, e = d;
    while(e) {                             /* x = witness^d, Montgomery */
      if(e & 1) x = montmul(x, b, n, nneg);
      b = montmul(b, b, n, nneg);
      e >>= 1;
    }
    if(x == one || x == nm1) continue;
    int ok = 0;
    for(int j = 0; j < r - 1; j++) {
      x = montmul(x, x, n, nneg);
      if(x == nm1) { ok = 1; break; }
    }
    if(!ok) return 0;
  }
  return 1;
}

/* --- Pollard rho (Floyd cycle) for 64-bit factoring --- */

static u64 gcd_u(u64 a, u64 b) {
  while(b) { u64 t = b; b = a % b; a = t; }
  return a;
}

/* one nontrivial factor of an odd composite n < 2^63.  Brent's cycle
   finding over Montgomery products, gcds batched 128 steps at a time (the
   accumulated q picks up extra R factors, but gcd(R,n)=1 for odd n, so
   gcd(q,n) is unchanged).  Deterministic: x0 = 2 and the polynomial
   constant c retries 1,2,3,... so a given n always splits the same way
   (gk results must be reproducible; no rand()). */
static u64 rho_pollard(u64 n) {
  u64 nneg, one, r2;
  mont_init(n, &nneg, &one, &r2);
  for(u64 c = 1;; c++) {
    u64 cm = montmul(c % n, r2, n, nneg);
    u64 y = montmul(2, r2, n, nneg), x = y, ys = y, q = one, g = 1, r = 1;
    while(g == 1) {
      x = y;
      for(u64 i = 0; i < r; i++) y = addmod_(montmul(y, y, n, nneg), cm, n);
      for(u64 k = 0; k < r && g == 1; k += 128) {
        ys = y;
        u64 lim = 128 < r - k ? 128 : r - k;
        for(u64 i = 0; i < lim; i++) {
          y = addmod_(montmul(y, y, n, nneg), cm, n);
          q = montmul(q, x > y ? x - y : y - x, n, nneg);
        }
        g = gcd_u(q, n);
      }
      r <<= 1;
    }
    if(g == n) {           /* batch overshot the factor: redo from ys,
                              one gcd per step */
      do {
        ys = addmod_(montmul(ys, ys, n, nneg), cm, n);
        g = gcd_u(x > ys ? x - ys : ys - x, n);
      } while(g == 1);
    }
    if(g != n) return g;   /* g == n even stepwise: next c */
  }
}

/* --- GCD (Euclidean algorithm) --- */

/* unsigned core: |INT64_MIN| = 2^63 doesn't fit i64, so negating it in i64
   was UB (wrapped on -O3: gcd[0Nj;6j] gave -2j).  0Nj is treated as its
   numeric value 2^63, like the int path treats 0N as 2^31; a gcd of
   exactly 2^63 saturates to J_INF (0Ij), mirroring the lcm clamp. */
static u64 absu(i64 a) { u64 u = (u64)a; return a < 0 ? 0 - u : u; }

static i64 gcd_i(i64 a, i64 b) {  /* gcd_u: the Pollard-rho helper above */
  u64 g = gcd_u(absu(a), absu(b));
  return g > (u64)INT64_MAX ? J_INF : (i64)g;
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
  case 8: {
    i64 v = jk(x);
    return t(1, (u32)(v >= 2 && is_prime((u64)v) ? 1 : 0));
  }
  case -8: {
    u64 len = nx;
    i64 *p = px(x);
    K r = tn(1, len);
    i32 *pr = px(r);
    /* Miller-Rabin on a hard 63-bit value is ~1ms; a vector of repeats
       (data columns, fuzz reshapes) shouldn't re-test each copy.  Same
       8-slot cache as factor's vector paths. */
    i64 cv[8]; i32 cp[8]; u8 ch[8] = {0};
    for(u64 i = 0; i < len; i++) {
      i64 v = p[i];
      int h = (u32)v & 7;
      if(ch[h] && cv[h] == v) { pr[i] = cp[h]; continue; }
      pr[i] = (v >= 2 && is_prime((u64)v)) ? 1 : 0;
      cv[h] = v; cp[h] = pr[i]; ch[h] = 1;
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

  /* Vector paths keep a tiny per-call result cache: repeated values are
     the norm (data columns, and fuzz inputs built by reshape), and a hard
     63-bit element costs ~45ms to factor -- a reshape holding ~500 copies
     of one such value was a fuzz hang.  8 direct-mapped slots; the cached
     K is an alias of the last stored result for that value (no extra ref
     held, so the error path needs no cleanup). */

  /* int vector: factor each element */
  if(tx == -1) {
    u64 len = nx;
    i32 *p = px(x);
    K r = tn(0, len);
    K *pr = px(r);
    i32 cv[8]; K cr[8] = {0};
    for(u64 i = 0; i < len; i++) {
#ifdef FUZZING
      if(--gk_budget < 0) { _k(r); return kerror("limit"); }
#endif
      int h = (int)(((u64)(u32)p[i] * 0x9E3779B97F4A7C15ULL) >> 61) & 7;  /* Fibonacci hash (see -8 path) */
      if(cr[h] && cv[h] == p[i]) { pr[i] = k_(cr[h]); continue; }
      K a = t(1, (u32)p[i]);
      K f = factor_(a);
      _k(a);                     /* the temp atom leaked, 24B per element */
      if(E(f)) { _k(r); return f; }
      pr[i] = f;
      cv[h] = p[i]; cr[h] = f;
    }
    return r;
  }

  /* long vector: factor each element */
  if(tx == -8) {
    u64 len = nx;
    i64 *p = px(x);
    K r = tn(0, len);
    K *pr = px(r);
    i64 cv[8]; K cr[8] = {0};
    for(u64 i = 0; i < len; i++) {
#ifdef FUZZING
      if(--gk_budget < 0) { _k(r); return kerror("limit"); }
#endif
      int h = (int)(((u64)p[i] * 0x9E3779B97F4A7C15ULL) >> 61) & 7;  /* Fibonacci
        hash: every input bit affects the slot.  A low-bits-only (v&7) cache put
        2 and INT64_MIN+2 (same low 3 bits) in one slot; a vector alternating
        them thrashed it into O(n) refactorings. */
      if(cr[h] && cv[h] == p[i]) { pr[i] = k_(cr[h]); continue; }
      K a = tj(p[i]);
      K f = factor_(a);
      _k(a);                     /* the temp atom leaked, 24B per element */
      if(E(f)) { _k(r); return f; }
      pr[i] = f;
      cv[h] = p[i]; cr[h] = f;
    }
    return r;
  }

  /* list: irecur1's iterative walk plus the vector paths' repeat cache
     (irecur1 alone re-factors every copy -- a 92k mixed reshape holding
     one hard 63-bit long was an AFL hang) */
  if(tx == 0) {
    typedef struct { K r,x; size_t i; } sf;
    K r;
    i32 sm=32,sp=0;
    i32 cvi[8]; K cri[8]={0};
    i64 cvj[8]; K crj[8]={0};
    sf *stack=xmalloc(sizeof(sf)*sm);
    stack[sp++]=(sf){tn(0,nx),x,0};
    while(sp) {
      sf *f=&stack[sp-1];
      if(f->i==n(f->x)) {
        r=f->r;
        --sp;
        if(sp) ((K*)px(stack[sp-1].r))[stack[sp-1].i++]=knorm(r);
      }
      else {
        K x_=((K*)px(f->x))[f->i];
        if(!s(x_)&&!T(x_)) {
          if(sp==sm) stack=xrealloc(stack,sizeof(sf)*(sm*=2));
          stack[sp++]=(sf){tn(0,n(x_)),x_,0};
        }
        else {
          K r_;
#ifdef FUZZING
          if(--gk_budget<0) { while(sp--) _k(stack[sp].r); xfree(stack); return kerror("limit"); }
#endif
          if(T(x_)==1&&!s(x_)) {
            i32 v=ik(x_); int h=(int)(((u64)(u32)v*0x9E3779B97F4A7C15ULL)>>61)&7;  /* Fibonacci hash (see -8 path) */
            if(cri[h]&&cvi[h]==v) r_=k_(cri[h]);
            else { r_=factor_(x_); if(!E(r_)) { cvi[h]=v; cri[h]=r_; } }
          }
          else if(T(x_)==8&&!s(x_)) {
            i64 v=jk(x_); int h=(int)(((u64)v*0x9E3779B97F4A7C15ULL)>>61)&7;  /* Fibonacci hash (see -8 path) */
            if(crj[h]&&cvj[h]==v) r_=k_(crj[h]);
            else { r_=factor_(x_); if(!E(r_)) { cvj[h]=v; crj[h]=r_; } }
          }
          else r_=factor_(x_);
          if(E(r_)) { while(sp--) _k(stack[sp].r); xfree(stack); return r_; }
          ((K*)px(f->r))[f->i++]=r_;
        }
      }
    }
    xfree(stack);
    return knorm(r);
  }

  /* long scalar: trial division for small factors, then Miller-Rabin +
     Pollard rho to split what remains.  Pure trial division was a fuzz
     hang: a ~2^63 prime (or a semiprime of two ~2^31.5 primes) needs
     sqrt(n)/2 ~ 1.5e9 iterations; rho splits the same input in ~ms. */
  if(tx == 8) {
    i64 n = jk(x);
    if(n == J_NULL) return tn(8, 0);      /* null -> empty, like 0 and 1 */
    if(n < 0) n = -n;
    if(n <= 1) return tn(8, 0);
#ifdef FUZZING
    /* work-proportional charge: a hard 63-bit factorization is ~0.4ms, so cap
       the count (~1950 before 'limit) so a column of many DISTINCT hard values
       -- which the repeat cache can't dedup -- can't run for minutes. */
    if((gk_budget-=512)<0) return kerror("limit");
#endif
    u64 un = (u64)n, cap = 16, cnt = 0;
    i64 *factors = xmalloc(cap * sizeof(i64));
    while((un & 1) == 0) {
      if(cnt >= cap) { cap *= 2; factors = xrealloc(factors, cap * sizeof(i64)); }
      factors[cnt++] = 2; un >>= 1;
    }
    /* primality FIRST: a Miller-Rabin is ~us, so a prime never pays the
       trial loop (a general list of repeated ~2^31 primes was a fuzz hang
       at ~50us each: 23k wasted divisions per element before the old
       post-trial check) */
    if(un > 1 && is_prime(un)) {
      if(cnt >= cap) { cap *= 2; factors = xrealloc(factors, cap * sizeof(i64)); }
      factors[cnt++] = (i64)un;
      un = 1;
    }
    for(u64 f = 3; f <= 8191 && f * f <= un; f += 2) {
      while(un % f == 0) {
        if(cnt >= cap) { cap *= 2; factors = xrealloc(factors, cap * sizeof(i64)); }
        factors[cnt++] = (i64)f; un /= f;
      }
    }
    if(un > 1) {
      /* every remaining prime factor is > 8191; split composites with rho.
         A u64 has < 64 prime factors, so the work stack is bounded. */
      u64 stk[64]; int sp = 0;
      stk[sp++] = un;
      while(sp) {
        u64 v = stk[--sp];
        if(is_prime(v)) {
          if(cnt >= cap) { cap *= 2; factors = xrealloc(factors, cap * sizeof(i64)); }
          factors[cnt++] = (i64)v;
          continue;
        }
        u64 d = rho_pollard(v);
        stk[sp++] = d; stk[sp++] = v / d;
      }
    }
    /* rho emits factors in arbitrary order (and the early prime check can
       front-run smaller trial factors); restore ascending */
    for(u64 i = 1; i < cnt; i++) {
      i64 key = factors[i]; u64 j = i;
      while(j > 0 && factors[j-1] > key) { factors[j] = factors[j-1]; j--; }
      factors[j] = key;
    }
    K r = tn(8, cnt);
    i64 *pr = px(r);
    for(u64 i = 0; i < cnt; i++) pr[i] = factors[i];
    xfree(factors);
    return r;
  }

  /* scalar int */
  if(tx != 1)
    return kerror("type");

  i64 n = ik(x);

  if(n == INT32_MIN) return tn(1, 0);  /* 0N -> empty like the long null
    (negating INT32_MIN read it as 2^31 and returned thirty-one 2s) */
  if(n < 0) n = -n;
  if(n <= 1) return tn(1, 0); /* empty for 0, 1 */
#ifdef FUZZING
  if((gk_budget-=32)<0) return kerror("limit");  /* ~16x cheaper than the 63-bit
    path (see tx==8); charge proportionally */
#endif

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

  /* primality first, as in the long path: a prime ~2^31 otherwise pays
     the full 23k-division trial loop */
  if(un > 1 && is_prime(un)) {
    if(cnt >= cap) { cap *= 2; factors = xrealloc(factors, cap * sizeof(i32)); }
    factors[cnt++] = (un <= INT32_MAX) ? (i32)un : INT32_MAX;
    un = 1;
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

/* read element i of an int/long numeric K (atom ignores i) as i64 */
static int isintlong(i8 t) { return t==1||t==8||t==-1||t==-8; }
static i64 readj(K x, u64 i) {
  switch(tx) {
  case  1: return (i64)ik(x);
  case  8: return jk(x);
  case -1: return (i64)((i32*)px(x))[i];
  case -8: return ((i64*)px(x))[i];
  }
  return 0;
}

/* generic long-result number-theory dyad: applies f elementwise over
   int/long operands (atoms broadcast), producing a long atom/vector. If
   domain!=0, a negative f result raises a domain error (for modinv). */
static K ntdyad_j(K a, K x, i64(*f)(i64,i64), int domain) {
  if(!isintlong(ta) || !isintlong(tx)) return kerror("type");
  int av=ta<0, xv=tx<0;
  if(av && xv && na!=nx) return kerror("length");
  if(!av && !xv) { /* scalar-scalar */
    i64 v=f(readj(a,0),readj(x,0));
    if(domain && v<0) return kerror("domain");
    return tj(v);
  }
  u64 n = av?na:nx;
  K r=tn(8,n); i64 *pr=px(r);
  for(u64 i=0;i<n;i++) {
    i64 v=f(readj(a,i),readj(x,i));
    if(domain && v<0) { _k(r); return kerror("domain"); }
    pr[i]=v;
  }
  return r;
}

/* --- gcd_(K a, K x): greatest common divisor --- */

K gcd_(K a, K x) {
  if(s(a) || s(x)) return kerror("type");

  i8 ta_ = ta, tx_ = tx;

  /* list cases: use irecur2 */
  if(ta_ == 0 || tx_ == 0)
    return irecur2(gcd_, a, x);

  /* any long operand: long result (no INT32_MAX clamp) */
  if(ta_==8 || ta_==-8 || tx_==8 || tx_==-8)
    return ntdyad_j(a, x, gcd_i, 0);

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
  u64 ua = absu(a), ub = absu(b);  /* unsigned: |INT64_MIN| is not UB here */
  if(ua == 0 || ub == 0) return 0;
  u64 x = ua / gcd_u(ua, ub);
  /* saturate to J_INF (0Ij) on i64 overflow, mirroring the int path's clamp
     to 0I (INT32_MAX): a wrapped product is a plausible-looking wrong number.
     Pre-multiply check for positive operands: x*b overflows iff x > MAX/b. */
  if(x > (u64)INT64_MAX / ub) return J_INF;
  return (i64)(x * ub);
}

K lcm_(K a, K x) {
  if(s(a) || s(x)) return kerror("type");

  i8 ta_ = ta, tx_ = tx;

  /* list cases: use irecur2 */
  if(ta_ == 0 || tx_ == 0)
    return irecur2(lcm_, a, x);

  /* any long operand: long result (no INT32_MAX clamp) */
  if(ta_==8 || ta_==-8 || tx_==8 || tx_==-8)
    return ntdyad_j(a, x, lcm_i, 0);

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

  /* any long operand: long result (no INT32_MAX clamp) */
  if(ta_==8 || ta_==-8 || tx_==8 || tx_==-8)
    return ntdyad_j(a, x, modinv_i, 1);

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
