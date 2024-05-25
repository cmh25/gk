#include "rand.h"

#ifdef _WIN32
  static unsigned long long x=123456789, y=362436069, z=521288629, t;
#else
  static unsigned long x=123456789, y=362436069, z=521288629, t;
#endif

static int rr(unsigned int *s) {
  unsigned int x;
  unsigned int a = 0x9d2c5680;
  unsigned int b = 0xefc60000;
  *s *= 1103515245 + 12345;
  x = *s;
  x = x ^ (x >> 11);
  x = x ^ (x << 7 & a);
  x = x ^ (x << 15 & b);
  x = x ^ (x >> 18);
  return x>>1;
}

static void shuffle(int *a, int n) {
  static unsigned int p=1;
  int i,j,t;
  int rm=2147483647; /* instead of RAND_MAX */
  if(n>1) {
    for(i=0;i<n-1;i++) {
      j = i + rr(&p) / (rm / (n - i) + 1);
      t = a[j];
      a[j] = a[i];
      a[i] = t;
    }
  }
}

void drawi(K *s, int m) {
  unsigned int i;

  for(i=0;i<s->c;i++) {
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;
    t = x; x = y; y = z;
    z = t ^ x ^ y;
    v1(s)[i] = z%m;
  }
}

void drawf(K *s, double m) {
  unsigned int i;
  int rm=2147483647; /* instead of RAND_MAX */
  double rmi=m/rm;

  for(i=0;i<s->c;i++) {
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;
    t = x; x = y; y = z;
    z = t ^ x ^ y;
    v2(s)[i] = (double)(z%rm)*rmi;
  }
}

void deal(K *s, int m) {
  int *deck=xmalloc(sizeof(int)*m);
  DO(m,deck[i]=i)
  shuffle(deck,m);
  DO(s->c,v1(s)[i]=deck[i])
  xfree(deck);
}
