#include "sha1.h"
#include <string.h>
#include <stdint.h>

#define K0 0x5A827999
#define K1 0x6ED9EBA1
#define K2 0x8F1BBCDC
#define K3 0xCA62C1D6
#define LOAD32_BE(p) \
  ((uint32_t)(p)[3]        | \
   (uint32_t)(p)[2] << 8   | \
   (uint32_t)(p)[1] << 16  | \
   (uint32_t)(p)[0] << 24)
#define STORE32_BE(p, v)       \
  do {                         \
    (p)[3] = (uint8_t)((v));   \
    (p)[2] = (uint8_t)((v) >> 8);  \
    (p)[1] = (uint8_t)((v) >> 16); \
    (p)[0] = (uint8_t)((v) >> 24); \
  } while (0)
#define STORE64_BE(p, v)       \
  do {                         \
    (p)[7] = (uint8_t)((v));   \
    (p)[6] = (uint8_t)((v) >> 8);  \
    (p)[5] = (uint8_t)((v) >> 16); \
    (p)[4] = (uint8_t)((v) >> 24); \
    (p)[3] = (uint8_t)((v) >> 32); \
    (p)[2] = (uint8_t)((v) >> 40); \
    (p)[1] = (uint8_t)((v) >> 48); \
    (p)[0] = (uint8_t)((v) >> 56); \
  } while (0)

typedef struct {
  uint64_t n;
  uint32_t h[5];
} sha1s;

static inline uint32_t ROTL(uint32_t x, uint32_t n) {
    n &= 31; // reduce mod 32
    return (x << n) | (x >> ((32 - n) & 31));
}

static void sha1block(sha1s *s, const unsigned char *p) {
  int32_t i;
  uint32_t a=s->h[0],b=s->h[1],c=s->h[2],d=s->h[3],e=s->h[4];
  uint32_t w[16],f,a5,b30,t,z;

  s->n++;
  for(i=0;i<16;i++) w[i] = LOAD32_BE(p+4*i);

  for(i=0;i<16;i++) {
    f = (b&c) | (~b&d);
    a5 = ROTL(a,5);
    b30 = ROTL(b,30);
    t = a5 + f + e + w[i&0xf] + K0;
    e = d; d = c; c = b30; b = a; a = t;
  }

  for(;i<20;i++) {
    z = w[(i-3)&0xf] ^ w[(i-8)&0xf] ^ w[(i-14)&0xf] ^ w[i&0xf];
    w[i&0xf] = z<<1 | z>>(32-1);

    f = (b&c) | (~b&d);
    a5 = ROTL(a,5);
    b30 = ROTL(b,30);
    t = a5 + f + e + w[i&0xf] + K0;
    e = d; d = c; c = b30; b = a; a = t;
  }

  for(;i<40;i++) {
    z = w[(i-3)&0xf] ^ w[(i-8)&0xf] ^ w[(i-14)&0xf] ^ w[i&0xf];
    w[i&0xf] = z<<1 | z>>(32-1);

    f = b ^ c ^ d;
    a5 = ROTL(a,5);
    b30 = ROTL(b,30);
    t = a5 + f + e + w[i&0xf] + K1;
    e = d; d = c; c = b30; b = a; a = t;
  }

  for(;i<60;i++) {
    z = w[(i-3)&0xf] ^ w[(i-8)&0xf] ^ w[(i-14)&0xf] ^ w[i&0xf];
    w[i&0xf] = z<<1 | z>>(32-1);

    f = ((b|c)&d) | (b&c);
    a5 = ROTL(a,5);
    b30 = ROTL(b,30);
    t = a5 + f + e + w[i&0xf] + K2;
    e = d; d = c; c = b30; b = a; a = t;
  }

  for(;i<80;i++) {
    z = w[(i-3)&0xf] ^ w[(i-8)&0xf] ^ w[(i-14)&0xf] ^ w[i&0xf];
    w[i&0xf] = z<<1 | z>>(32-1);

    f = b ^ c ^ d;
    a5 = ROTL(a,5);
    b30 = ROTL(b,30);
    t = a5 + f + e + w[i&0xf] + K3;
    e = d; d = c; c = b30; b = a; a = t;
  }

  s->h[0] += a; s->h[1] += b; s->h[2] += c; s->h[3] += d; s->h[4] += e;
}

static void sha1final(sha1s *s, const unsigned char *msg, size_t n, unsigned char *d) {
  unsigned char block[64];
  uint32_t i;
  uint64_t len = (s->n*64ULL+n)*8ULL;

  memset(block, 0, sizeof(block));  // always clear all 64 bytes
  memcpy(block, msg, n);            // copy leftover message

  block[n++] = 0x80;

  if(n>56) {
    sha1block(s, block);
    memset(block, 0, sizeof(block));
  }

  STORE64_BE(block+56,len);
  sha1block(s,block);
  for(i=0;i<5;i++) STORE32_BE(d+4*i, s->h[i]);
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

void sha1(char *h, const unsigned char *b, size_t n) {
  unsigned char d[20];
  sha1s s = {0,{0x67452301,0xefcdab89,0x98badcfe,0x10325476,0xc3d2e1f0}};
  while(n>=64) { sha1block(&s,b); b += 64; n -= 64; }

  sha1final(&s, b, n, d);
  hex(h,d,20);
}
