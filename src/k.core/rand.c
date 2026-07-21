#include "rand.h"
#include <stdlib.h>
#include "x.h"

/* xorshift state word: unsigned long, EXCEPT Windows where long is 32 bits
   (LLP64) and the stream would diverge from the documented sequences.
   BOTH streams (draw x/y/z and shuffle sx/sy/sz) must be this width -- the
   shuffle stream first shipped as bare unsigned long and t343/t498/t576/
   t609 failed on Windows. */
#ifdef _WIN32
typedef unsigned long long uxs;
#else
typedef unsigned long uxs;
#endif

static uxs x=123456789, y=362436069, z=521288629, t;

/* shuffle's own xorshift generator, kept SEPARATE from the draw stream
   (x/y/z) so fixing deal leaves documented roll sequences untouched -- the
   original code likewise gave shuffle its own state, it was just BROKEN:
   `*s *= 1103515245 + 12345` folded to a multiply by the EVEN constant
   1103527590 (the intended `*s = *s*1103515245 + 12345` lost its `+`), so
   the state gained a factor of 2 per call and hit 0 (mod 2^32) after 32
   calls -- every deal after that returned the identity permutation,
   forever, process-wide.  rand_reseed perturbs sx/sy/sz too so forked
   children get distinct deals. */
static uxs sx=88172645, sy=987654321, sz=43219876;

static uxs snext(void) {
  uxs st;
  sx ^= sx << 16;
  sx ^= sx >> 5;
  sx ^= sx << 1;
  st = sx; sx = sy; sy = sz;
  sz = st ^ sx ^ sy;
  return sz;
}

static void shuffle(int *a, int n) {
  int i,j,t;
  if(n>1) {
    for(i=0;i<n-1;i++) {
      j = i + (int)(snext() % (uxs)(n - i));   /* j in [i, n-1] */
      t = a[j];
      a[j] = a[i];
      a[i] = t;
    }
  }
}

void drawi(int *s, int64_t n, int m) {
  int64_t i;

  for(i=0;i<n;i++) {
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;
    t = x; x = y; y = z;
    z = t ^ x ^ y;
    s[i] = z%m;
  }
}

void drawj(int64_t *s, int64_t n, int64_t m) {
  int64_t i;

  for(i=0;i<n;i++) {
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;
    t = x; x = y; y = z;
    z = t ^ x ^ y;
    s[i] = (int64_t)((unsigned long long)z % (unsigned long long)m);
  }
}

void drawf(double *s, int64_t n, double m) {
  int64_t i;
  int rm=2147483647; /* instead of RAND_MAX */
  double rmi=m/rm;

  for(i=0;i<n;i++) {
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;
    t = x; x = y; y = z;
    z = t ^ x ^ y;
    s[i] = (double)(z%rm)*rmi;
  }
}

void drawe(float *s, int64_t n, float m) {
  int64_t i;
  int rm=2147483647; /* instead of RAND_MAX */
  double rmi=(double)m/rm;

  for(i=0;i<n;i++) {
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;
    t = x; x = y; y = z;
    z = t ^ x ^ y;
    s[i] = (float)((double)(z%rm)*rmi);
  }
}

void deal(int *s, int n, int m) {
  int i,*deck=xmalloc(sizeof(int)*m);
  for(i=0;i<m;i++) deck[i]=i;
  shuffle(deck,m);
  for(i=0;i<n;i++) s[i]=deck[i];
  xfree(deck);
}

void dealj(int64_t *s, int n, int m) {
  int i,*deck=xmalloc(sizeof(int)*m);
  for(i=0;i<m;i++) deck[i]=i;
  shuffle(deck,m);
  for(i=0;i<n;i++) s[i]=deck[i];
  xfree(deck);
}

/* Re-seed the xorshift state. Used by ipc.c after fork() so each child
 * gets a distinct stream; without this every forked child would produce
 * the same "random" sequence. Mixes the caller-supplied value into all
 * three state words and ensures none end up zero (xorshift would lock
 * to zero forever). */
void rand_reseed(unsigned long s) {
  x = 123456789UL ^ s;
  y = 362436069UL ^ (s * 2654435761UL);
  z = 521288629UL ^ (s + 0x9e3779b9UL);
  if(!x) x = 1;
  if(!y) y = 1;
  if(!z) z = 1;
  sx = 88172645UL  ^ (s * 40503UL);        /* shuffle's stream too, so a */
  sy = 987654321UL ^ (s + 0x7f4a7c15UL);   /* forked child's deals differ */
  sz = 43219876UL  ^ (s * 2246822519UL);
  if(!sx) sx = 1;
  if(!sy) sy = 1;
  if(!sz) sz = 1;
}
