#include "aes256.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define BLOCKLEN 16
#define KEYLEN 32
#define KEYLENE 240
#define NB 4
#define NK 8
#define NR 14

typedef struct {
  unsigned char rk[KEYLENE];
  unsigned char iv[BLOCKLEN];
} ctx;

typedef unsigned char state[4][4];

static unsigned char sbox[256] = {
  0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
  0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
  0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
  0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
  0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
  0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
  0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
  0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
  0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
  0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
  0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
  0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
  0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
  0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
  0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
  0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16 };

static unsigned char rcon[11] = { 0x8d,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36 };

#define SBV(num) (sbox[(num)])

static void keyexpand(unsigned char* rk, unsigned char* Key) {
  unsigned i,j,k;
  unsigned char t[4];

  // The first round key is the key itself.
  for(i=0;i<NK;++i) {
    rk[(i<<2)+0] = Key[(i<<2)+0];
    rk[(i<<2)+1] = Key[(i<<2)+1];
    rk[(i<<2)+2] = Key[(i<<2)+2];
    rk[(i<<2)+3] = Key[(i<<2)+3];
  }

  // All other round keys are found from the previous round keys.
  for(i=NK;i<NB*(NR+1);++i) {
    {
      k=(i-1)*4;
      t[0]=rk[k+0];
      t[1]=rk[k+1];
      t[2]=rk[k+2];
      t[3]=rk[k+3];
    }

    if(i%NK==0) {
      // This function shifts the 4 bytes in a word to the left once.
      // [a0,a1,a2,a3] becomes [a1,a2,a3,a0]

      // Function RotWord()
      {
        unsigned char u=t[0];
        t[0]=t[1];
        t[1]=t[2];
        t[2]=t[3];
        t[3]=u;
      }

      // Function Subword()
      {
        t[0]=SBV(t[0]);
        t[1]=SBV(t[1]);
        t[2]=SBV(t[2]);
        t[3]=SBV(t[3]);
      }

      t[0]^=rcon[i/NK];
    }
    if (i%NK==4) {
      // Function Subword()
      {
        t[0]=SBV(t[0]);
        t[1]=SBV(t[1]);
        t[2]=SBV(t[2]);
        t[3]=SBV(t[3]);
      }
    }
    j = i*4; k=(i-NK)*4;
    rk[j+0]=rk[k+0]^t[0];
    rk[j+1]=rk[k+1]^t[1];
    rk[j+2]=rk[k+2]^t[2];
    rk[j+3]=rk[k+3]^t[3];
  }
}

// This function adds the round key to state.
// The round key is added to the state by an XOR function.
static void addrk(unsigned char r, state *s, unsigned char *rk) {
  unsigned char i,j;
  for(i=0;i<4;++i)
    for(j=0;j<4;++j)
      (*s)[i][j] ^= rk[(r*NB*4)+(i*NB)+j];
}

// The subbytes Function Substitutes the values in the
// state matrix with values in an S-box.
static void subbytes(state *s) {
  unsigned char i, j;
  for(i=0;i<4;++i)
    for(j=0;j<4;++j)
      (*s)[j][i] = SBV((*s)[j][i]);
}

// The shiftrows() function shifts the rows in the state to the left.
// Each row is shifted with different offset.
// Offset = Row number. So the first row is not shifted.
static void shiftrows(state *s) {
  unsigned char t;

  // Rotate first row 1 columns to left
  t          = (*s)[0][1];
  (*s)[0][1] = (*s)[1][1];
  (*s)[1][1] = (*s)[2][1];
  (*s)[2][1] = (*s)[3][1];
  (*s)[3][1] = t;

  // Rotate second row 2 columns to left
  t          = (*s)[0][2];
  (*s)[0][2] = (*s)[2][2];
  (*s)[2][2] = t;

  t          = (*s)[1][2];
  (*s)[1][2] = (*s)[3][2];
  (*s)[3][2] = t;

  // Rotate third row 3 columns to left
  t          = (*s)[0][3];
  (*s)[0][3] = (*s)[3][3];
  (*s)[3][3] = (*s)[2][3];
  (*s)[2][3] = (*s)[1][3];
  (*s)[1][3] = t;
}

static unsigned char xtime(unsigned char x) {
  return ((x<<1)^(((x>>7)&1)*0x1b));
}

// mixcols function mixes the columns of the state matrix
static void mixcols(state *s) {
  unsigned char i,t,u,v;
  for(i=0;i<4;++i) {
    t  = (*s)[i][0];
    u  = (*s)[i][0] ^ (*s)[i][1] ^ (*s)[i][2] ^ (*s)[i][3] ;
    v  = (*s)[i][0] ^ (*s)[i][1] ; v = xtime(v);  (*s)[i][0] ^= v ^ u ;
    v  = (*s)[i][1] ^ (*s)[i][2] ; v = xtime(v);  (*s)[i][1] ^= v ^ u ;
    v  = (*s)[i][2] ^ (*s)[i][3] ; v = xtime(v);  (*s)[i][2] ^= v ^ u ;
    v  = (*s)[i][3] ^ t          ; v = xtime(v);  (*s)[i][3] ^= v ^ u ;
  }
}

// Multiply is used to multiply numbers in the field GF(2^8)
// Note: The last call to xtime() is unneeded, but often ends up generating a smaller binary
//       The compiler seems to be able to vectorize the operation better this way.
//       See https://github.com/kokke/tiny-AES-c/pull/34
#define Multiply(x, y)                                \
      (  ((y & 1) * x) ^                              \
      ((y>>1 & 1) * xtime(x)) ^                       \
      ((y>>2 & 1) * xtime(xtime(x))) ^                \
      ((y>>3 & 1) * xtime(xtime(xtime(x)))) ^         \
      ((y>>4 & 1) * xtime(xtime(xtime(xtime(x))))))   \

// cipher is the main function that encrypts the PlainText.
static void cipher(state *s, unsigned char* rk) {
  unsigned char r=0;

  // Add the First round key to the state before starting the rounds.
  addrk(0,s,rk);

  // There will be NR rounds.
  // The first NR-1 rounds are identical.
  // These NR rounds are executed in the loop below.
  // Last one without mixcols()
  for(r=1;;++r) {
    subbytes(s);
    shiftrows(s);
    if(r==NR) break;
    mixcols(s);
    addrk(r,s,rk);
  }
  // Add round key to last round
  addrk(NR,s,rk);
}

static unsigned char* hex(unsigned char *b, size_t n) {
  char h[16]="0123456789abcdef";
  size_t i;
  unsigned char *r=malloc(1+n*2);
  for(i=0;i<n;i++) {
    r[2*i]=h[b[i]>>4];
    r[2*i+1]=h[b[i]&0xf];
  }
  r[2*i]=0;
  return r;
}

static unsigned char* unhex(unsigned char *h) {
  size_t i,n = strlen((char*)h)/2;
  unsigned char *r=malloc(n);
  for(i=0;i<n;i++) {
    sscanf((char*)h,"%2hhx",&r[i]);
    h+=2;
  }
  return r;
}

static void aesinit(ctx *c, unsigned char *iv, unsigned char *key) {
  keyexpand(c->rk,key);
  memcpy(c->iv,iv,BLOCKLEN);
}

static void aescrypt(ctx *c, unsigned char *b, size_t n) {
  unsigned char buffer[BLOCKLEN];
  size_t i;
  int j;
  for(i=0,j=BLOCKLEN;i<n;++i,++j) {
    if(j==BLOCKLEN) {
      /* regen xor compliment in buffer */
      memcpy(buffer, c->iv, BLOCKLEN);
      cipher((state*)buffer,c->rk);

      /* Increment iv and handle overflow */
      for(j=(BLOCKLEN-1);j>=0;--j) {
        /* inc will overflow */
        if(c->iv[j]==255) { c->iv[j]=0; continue; }
        c->iv[j] += 1;
        break;
      }
      j=0;
    }
    b[i] = (b[i] ^ buffer[j]);
  }
}

/*  echo -n "charles" | openssl enc --aes-256-ctr -iv "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff" -K "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4" | xxd -p */
char* aes256e(char *s, unsigned char *iv, unsigned char *key) {
  ctx c;
  size_t i,n=strlen(s);
  int m=16-n%16;
  unsigned char *b=malloc(n+m);
  char *r;
  memcpy(b,s,n);
  for(i=n;i<n+m;i++) b[i]=m;
  aesinit(&c,iv,key);
  aescrypt(&c,b,n+m);
  r=(char*)hex(b,n+m);
  free(b);
  r[n*2]=0;
  return r;
}

char* aes256d(char *s, unsigned char *iv, unsigned char *key) {
  ctx c;
  size_t i,n=strlen(s);
  int m=32-n%32;
  unsigned char *b=malloc(1+n+m),*r;
  memcpy(b,s,n);
  for(i=n;i<n+m;i++) b[i]=48;
  b[i]=0;
  r=unhex(b);
  free(b);
  aesinit(&c,iv,key);
  aescrypt(&c,r,(n+m)/2);
  r[n/2]=0;
  return (char*)r;
}
