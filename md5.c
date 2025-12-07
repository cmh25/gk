#include "md5.h"
#include <string.h>
#include <stdint.h>

#define ROL(n,m) ((n)<<(m)|(n)>>(32-(m)))

#define LOAD32_LE(p) \
  ((uint32_t)(p)[0]        | \
   (uint32_t)(p)[1] << 8   | \
   (uint32_t)(p)[2] << 16  | \
   (uint32_t)(p)[3] << 24)

#define STORE32_LE(p, v)       \
  do {                         \
    (p)[0] = (uint8_t)((v));   \
    (p)[1] = (uint8_t)((v) >> 8);  \
    (p)[2] = (uint8_t)((v) >> 16); \
    (p)[3] = (uint8_t)((v) >> 24); \
  } while (0)

typedef struct {
  uint32_t n;
  uint32_t h[4];
} md5s;

static const uint8_t S[64] = {
  7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
  5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
  4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
  6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
};

static const uint32_t K[64] = {
  0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
  0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
  0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
  0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
  0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
  0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
  0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
  0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
  0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
  0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
  0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
  0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
  0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
  0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
  0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
  0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

static void md5block(md5s *s, const unsigned char *p) {
  int32_t i,g;
  uint32_t a=s->h[0],b=s->h[1],c=s->h[2],d=s->h[3],f,t,x,y;
  const unsigned char *q;

  s->n++;
  for(i=0;i<64;i++) {
    if(i<16) { f = (b&c) | (~b&d); g = i; }
    else if(i<32) { f = (d&b) | (~d&c); g = 5*i + 1; }
    else if(i<48) { f = b ^ c ^ d; g = 3*i + 5; }
    else { f = c ^ (b | ~d); g = 7*i; }
    t = d;
    d = c;
    c = b;
    q = p + 4*(g & 0xf);
    x = a + f + K[i] + LOAD32_LE(q);

    y = S[i];
    b += ROL(x, y);
    a = t;
  }
  s->h[0] += a;
  s->h[1] += b;
  s->h[2] += c;
  s->h[3] += d;
}

static void md5final(md5s *s, unsigned char *p, size_t n, unsigned char *d) {
  uint32_t i,hi,len=s->n*64+n,*h=s->h;

  p[n++]=0x80;
  if(n>56) {
    memset(p+n,0,64-n);
    md5block(s,p);
    memset(p,0,56);
  }
  else memset(p+n,0,56-n);

  hi = len >> 29; len <<= 3;
  p[56] = len;
  p[57] = len>>8;
  p[58] = len>>16;
  p[59] = len>>24;
  p[60] = hi;
  p[61] = p[62] = p[63] = 0;

  md5block(s, p);
  for(i=0;i<4;i++) STORE32_LE(d + i*4, h[i]);
}

static void hex(char *s, const unsigned char *b, size_t n) {
  const char h[]="0123456789abcdef";
  size_t i;
  for(i=0;i<n;i++) {
    s[2*i]=h[b[i]>>4];
    s[2*i+1]=h[b[i]&0xf];
  }
  s[2*i]=0;
}

void md5(char *h, const unsigned char *b, size_t n) {
  unsigned char block[64],d[16];
  md5s s={0,{0x67452301,0xefcdab89,0x98badcfe,0x10325476}};
  while(n>=64) {
    md5block(&s,b);
    b+=64;
    n-=64;
  }
  memcpy(block,b,n);
  md5final(&s,block,n,d);
  hex(h,d,16);
}
