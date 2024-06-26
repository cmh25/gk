#include "sha1.h"
#include <string.h>

#define K0 0x5A827999
#define K1 0x6ED9EBA1
#define K2 0x8F1BBCDC
#define K3 0xCA62C1D6

typedef struct {
  unsigned int n;
  unsigned int h[5];
} sha1s;

static void sha1block(sha1s *s, unsigned char *p) {
  int i;
  unsigned int a=s->h[0],b=s->h[1],c=s->h[2],d=s->h[3],e=s->h[4];
  unsigned int w[16],f,a5,b30,t,z,j;

  s->n++;
  for(i=0;i<16;i++) {
    j = i * 4;
    w[i] = p[j] << 24 | p[j+1] << 16 | p[j+2] << 8 | p[j+3];
  }

  for(i=0;i<16;i++) {
    f = (b&c) | (~b&d);
    a5 = a<<5 | a>>(32-5);
    b30 = b<<30 | b>>(32-30);
    t = a5 + f + e + w[i&0xf] + K0;
    e = d; d = c; c = b30; b = a; a = t;
  }

  for(;i<20;i++) {
    z = w[(i-3)&0xf] ^ w[(i-8)&0xf] ^ w[(i-14)&0xf] ^ w[i&0xf];
    w[i&0xf] = z<<1 | z>>(32-1);

    f = (b&c) | (~b&d);
    a5 = a<<5 | a>>(32-5);
    b30 = b<<30 | b>>(32-30);
    t = a5 + f + e + w[i&0xf] + K0;
    e = d; d = c; c = b30; b = a; a = t;
  }

  for(;i<40;i++) {
    z = w[(i-3)&0xf] ^ w[(i-8)&0xf] ^ w[(i-14)&0xf] ^ w[i&0xf];
    w[i&0xf] = z<<1 | z>>(32-1);

    f = b ^ c ^ d;
    a5 = a<<5 | a>>(32-5);
    b30 = b<<30 | b>>(32-30);
    t = a5 + f + e + w[i&0xf] + K1;
    e = d; d = c; c = b30; b = a; a = t;
  }

  for(;i<60;i++) {
    z = w[(i-3)&0xf] ^ w[(i-8)&0xf] ^ w[(i-14)&0xf] ^ w[i&0xf];
    w[i&0xf] = z<<1 | z>>(32-1);

    f = ((b|c)&d) | (b&c);
    a5 = a<<5 | a>>(32-5);
    b30 = b<<30 | b>>(32-30);
    t = a5 + f + e + w[i&0xf] + K2;
    e = d; d = c; c = b30; b = a; a = t;
  }

  for(;i<80;i++) {
    z = w[(i-3)&0xf] ^ w[(i-8)&0xf] ^ w[(i-14)&0xf] ^ w[i&0xf];
    w[i&0xf] = z<<1 | z>>(32-1);

    f = b ^ c ^ d;
    a5 = a<<5 | a>>(32-5);
    b30 = b<<30 | b>>(32-30);
    t = a5 + f + e + w[i&0xf] + K3;
    e = d; d = c; c = b30; b = a; a = t;
  }

  s->h[0] += a; s->h[1] += b; s->h[2] += c; s->h[3] += d; s->h[4] += e;
}

static void sha1final(sha1s *s, unsigned char *p, size_t n, unsigned char *d) {
  unsigned int i,hi,len=s->n*64+n,*h=s->h;

  p[n++] = 0x80;
  if(n<=56) memset(p+n,0,64-n-5);
  else {
    memset(p+n,0,64-n);
    sha1block(s,p);
    memset(p,0,56);
  }

  hi = len >> 29; len <<= 3;
  p[56] = p[57] = p[58] = 0;
  p[59] = hi;
  p[60] = len>>24;
  p[61] = len>>16;
  p[62] = len>>8;
  p[63] = len;

  sha1block(s, p);

  for(i=0;i<20>>2;i++) {
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

void sha1(char *h, unsigned char *b, size_t n) {
  unsigned char block[64],d[20];
  sha1s s={0,{0x67452301,0xefcdab89,0x98badcfe,0x10325476,0xc3d2e1f0}};
  while(n>=64) {
    sha1block(&s,b);
    b+=64;
    n-=64;
  }
  strncpy((char*)block,(char*)b,n);
  sha1final(&s,block,n,d);
  hex(h,d,20);
}
