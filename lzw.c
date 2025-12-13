#include "lzw.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "k.core/x.h"

#define MAX 4096

static char *D[MAX];
static int L[MAX],N=257;

static void bfa(LZW *b, int n, int w) {
  size_t i=b->i;
  unsigned char *p;
  b->i+=w;
  if(b->n<b->i/8+(b->i%8?1:0)) {
    p=xcalloc(b->n<<=1,1);
    memcpy(p,b->b,b->n>>1);
    xfree(b->b);
    b->b=p;
  }
  for(;i<b->i;i++) {
    if(n&1u<<--w) b->b[i/8]|=(1u<<(i%8));
    else b->b[i/8]&=~(1u<<(i%8));
  }
  b->c++;
}

static int bfr(LZW *z, size_t i, int w) {
  int r=0;
  size_t max_bits_from_n = z->n * 8;
  size_t max_bits = (z->i && z->i <= max_bits_from_n) ? z->i : max_bits_from_n;

  if (w <= 0 || i >= max_bits || max_bits - i < (size_t)w) return -1;

  while (w--) {
    size_t byte = i >> 3, bit = i & 7;
    if (byte >= z->n) return -1;      // per-step guard
    if (z->b[byte] & (1u << bit)) r |= 1 << w;
    ++i;
  }
  return r;
}

LZW* lzwnew(void) {
  LZW *r=xcalloc(sizeof(LZW),1);
  r->b=xcalloc(32,1);
  r->n=32;
  r->v=0;
  return r;
}

void lzwfree(LZW *b) {
  xfree(b->b);
  xfree(b);
}

/* ---------- LZW-specific 5-byte hash (old_code[4] + char[1]) ---------- */

typedef struct {
  unsigned char *keys;   /* [m][5] */
  int           *vals;   /* 0 = empty, otherwise code */
  size_t         m;      /* power-of-two size */
  size_t         used;
} HT5;

static inline uint32_t hs5(const unsigned char key[5]) {
  uint32_t h = (uint32_t)key[0]
             | ((uint32_t)key[1] << 8)
             | ((uint32_t)key[2] << 16)
             | ((uint32_t)key[3] << 24);
  h ^= (uint32_t)key[4] * 2654435761u; // cheap mix
  return h;
}

static HT5* ht5new(size_t m) {
  HT5 *ht = xcalloc(1, sizeof(HT5));
  size_t cap = 2;
  while (cap < m) cap <<= 1;
  ht->m = cap;
  ht->keys = xcalloc(ht->m, 5);
  ht->vals = xcalloc(ht->m, sizeof(int));
  return ht;
}

static void ht5clear(HT5 *ht) {
  /* just mark all slots empty; keys can stay */
  memset(ht->vals, 0, ht->m * sizeof(int));
  ht->used = 0;
}

static void ht5free(HT5 *ht) {
  if (!ht) return;
  xfree(ht->keys);
  xfree(ht->vals);
  xfree(ht);
}

static inline int eq5(const unsigned char *a, const unsigned char *b) {
  return a[0]==b[0] &&
         a[1]==b[1] &&
         a[2]==b[2] &&
         a[3]==b[3] &&
         a[4]==b[4];
}

static int ht5get(HT5 *ht, const unsigned char key[5]) {
  size_t mask = ht->m - 1;
  size_t i = hs5(key) & mask;
  while (ht->vals[i]) {
    if (eq5(ht->keys + i*5, key))
      return ht->vals[i];
    i = (i + 1) & mask;
  }
  return -1;
}

static void ht5set(HT5 *ht, const unsigned char key[5], int v) {
  size_t mask = ht->m - 1;
  size_t i = hs5(key) & mask;
  while (ht->vals[i]) {
    if (eq5(ht->keys + i*5, key)) {
      ht->vals[i] = v;
      return;
    }
    i = (i + 1) & mask;
  }
  memcpy(ht->keys + i*5, key, 5);
  ht->vals[i] = v;
  ht->used++;
}

/* ------------------------------- FAST lzwc ------------------------------ */

LZW* lzwc(char *s, size_t n) {
  if (!s) return 0;

  LZW *r = lzwnew();
  if (!r) return 0;
  if(!n) return r;

  /* table sized for low load factor; MAX should be 4096 for 12-bit */
  HT5 *ht = ht5new(MAX * 2);

  int w    = 9;
  int next = 257;

  /* initialize with first byte, but DO NOT emit it yet */
  int old = (unsigned char)*s++;
  size_t rem = n - 1;

  while (rem--) {
    unsigned char c = (unsigned char)*s++;

    unsigned char key[5];
    /* encode (old, c) as 5-byte key safely */
    memcpy(key, &old, 4);
    key[4] = c;

    int code = ht5get(ht, key);
    if (code != -1) {
      /* extend match */
      old = code;
      continue;
    }

    /* no match: output old */
    bfa(r, old, w);

    /* add new entry if room */
    if (next < (int)MAX) {
      ht5set(ht, key, next);
      ++next;
      if (next > (1 << w) && w < 12) ++w;
    } else {
      /* dictionary full -> CLEAR and reset */
      bfa(r, 256, w);
      w = 9; next = 257;
      ht5clear(ht);
      old = (int)c;
      /* continue to next iteration (do not fall through to set) */
      continue;
    }

    /* start new sequence from the single byte c */
    old = c;
  }

  /* flush the last code; DO NOT emit a trailing CLEAR */
  bfa(r, old, w);

  ht5free(ht);
  return r;
}

LZW* lzwd(LZW *z) {
  int j, w=9;
  size_t rn=0,k=0,pl=0;
  char *p=0;
  LZW *r=0;
  N = 257;

  if(!z || !z->b || z->v) goto corrupt;
  if(!(r=lzwnew())) goto corrupt;

  size_t max_bits_from_n = z->n * 8;
  size_t total_bits = (z->i && z->i <= max_bits_from_n) ? z->i : max_bits_from_n;
  
  while(k+(size_t)w<=total_bits) {
    j=bfr(z,k,w);
    if(j<0) goto corrupt;
    k+=w;
    if(j==256) {
      while(N>257) { xfree(D[--N]); D[N]=0; }
      w=9;
      if(p) { xfree(p); p=0; }
    }
    else if(j<N) {
      const char *src;
      size_t len;
      char first;
      if(j < 256) {
        static unsigned char b;
        b = (unsigned char)j;
        src = (const char*)&b;
        len = 1;
        first = b;
      }
      else {
        src = D[j];
        len = L[j];
        first = D[j][0];
      }

      while(r->n < rn + len + 1) {
        size_t nn = r->n << 1;
        unsigned char* nb = xrealloc(r->b, nn);
        if(!nb) goto corrupt;
        r->n = nn; r->b = nb;
      }
      memcpy(r->b + rn, src, len);
      rn += len;

      if(p) {
        if(N >= MAX) goto corrupt;
        p[pl] = first;
        D[N] = p;
        L[N++] = ++pl;
        p = 0;
      }
      if(!(p = xmalloc(len + 2))) goto corrupt;
      memcpy(p, src, len);
      pl = len;
    }
    else {
      if(!p) goto corrupt;
      p[pl++]=p[0];
      char *np=xrealloc(p,pl+2);
      if(!np) goto corrupt;
      p=np;
      if(N >= MAX) goto corrupt;
      if(!(D[N]=xmalloc(pl))) goto corrupt;
      memcpy(D[N],p,pl);
      L[N++]=pl;
      while(r->n < rn + (size_t)pl) {
        size_t nn=r->n<<1;
        unsigned char* nb=xrealloc(r->b,nn);
        if(!nb) goto corrupt;
        r->n=nn; r->b=nb;
      }
      memcpy(r->b+rn,p,pl);
      rn+=pl;
    }
    if ((size_t)N == (1u<<w) && w < 12) ++w;
  }
  while(N>257) { xfree(D[--N]); D[N]=0; };
  if(p) xfree(p);
  r->n=rn;
  return r;
corrupt:
  while(N>257) { xfree(D[--N]); D[N]=0; }
  if(p) { xfree(p); p=0; }
  if(r) { lzwfree(r); r=0; }
  return 0;
}
