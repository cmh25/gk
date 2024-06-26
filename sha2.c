#include "sha2.h"
#include <string.h>

#define ROR(n,m) ((n)>>(m)|(n)<<(32-(m)))

typedef struct {
  unsigned int n;
  unsigned int h[8];
} sha2s;

static unsigned int K[64] = {
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
  0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
  0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
  0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
  0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
  0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
  0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
  0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
  0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

static void sha2block(sha2s *s, unsigned char *p) {
  int i;
  unsigned int w[64],v1,t1,v2,t2;
  unsigned int a=s->h[0],b=s->h[1],c=s->h[2],d=s->h[3];
  unsigned int e=s->h[4],f=s->h[5],g=s->h[6],h=s->h[7];

  s->n++;

  for (i = 0; i < 16; i++) w[i] = p[4*i]<<24 | p[4*i+1]<<16 | p[4*i+2]<<8 | p[4*i+3];

  for (; i < 64; i++) {
    v1 = w[i-2];
    t1 = ROR(v1, 17) ^ ROR(v1, 19) ^ (v1 >> 10);
    v2 = w[i-15];
    t2 = ROR(v2, 7) ^ ROR(v2, 18) ^ (v2 >> 3);
    w[i] = t1 + w[i-7] + t2 + w[i-16];
  }

  for (i = 0; i < 64; i++) {
    t1 = h + (ROR(e, 6) ^ ROR(e, 11) ^ ROR(e, 25)) + ((e&f) ^ (~e&g)) + K[i] + w[i];
    t2 = (ROR(a, 2) ^ ROR(a, 13) ^ ROR(a, 22)) + ((a&b) ^ (a&c) ^ (b&c));

    h = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }

  s->h[0] += a;
  s->h[1] += b;
  s->h[2] += c;
  s->h[3] += d;
  s->h[4] += e;
  s->h[5] += f;
  s->h[6] += g;
  s->h[7] += h;
}

static void sha2final(sha2s *s, unsigned char *p, size_t n, unsigned char *d) {
  unsigned int i,hi,len=s->n*64+n,*h=s->h;

  p[n++] = 0x80;
  if(n<=56) memset(p+n,0,64-n-5);
  else {
    memset(p+n,0,64-n);
    sha2block(s,p);
    memset(p,0,56);
  }

  hi = len >> 29; len <<= 3;
  p[56] = p[57] = p[58] = 0;
  p[59] = hi;
  p[60] = len>>24;
  p[61] = len>>16;
  p[62] = len>>8;
  p[63] = len;

  sha2block(s, p);

  for(i=0;i<32>>2;i++) {
    d[i*4]   = h[i]>>24;
    d[i*4+1] = h[i]>>16;
    d[i*4+2] = h[i]>>8;
    d[i*4+3] = h[i];
  }
}

static void hex(char *s, unsigned char *b, size_t n) {
  char h[16]="0123456789abcdef";
  size_t i;
  for(i=0;i<n;i++) {
    s[2*i]=h[b[i]>>4];
    s[2*i+1]=h[b[i]&0xf];
  }
  /* s[2*i]=0; */
}

void sha2(char *h, unsigned char *b, size_t n) {
  unsigned char block[64],d[32];
  sha2s s={0,{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
              0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19}};
  while(n>=64) {
    sha2block(&s,b);
    b+=64;
    n-=64;
  }
  strncpy((char*)block,(char*)b,n);
  sha2final(&s,block,n,d);
  hex(h,d,32);
}
