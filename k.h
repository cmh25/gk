#ifndef K__H
#define K__H

#include "k.core/k.h"

#define st(t,z) ((K)(t)<<48|((K)(z)&b(48)))
#define set_sx(x,s) (((x) & ~ (b(56) & ~b(48))) | ((K)(s) << 48))

#define VST(x) (!s(x) || s(x) & 0x80)
#define ISF(x) ((x)<256 || (VST(x) && s(x)&0x40))

#ifdef _WIN32
#define MAXR 100
#else
#define MAXR 1000
#endif
#define EMAX 15

extern i32 maxr;
extern char *E[EMAX];
extern i32 zeroclamp;
extern char *R_NUL,*R_DRAW,*R_DOT,*R_VS,*R_SV,*R_ATAN2,*R_DIV,*R_AND,*R_OR,*R_SHIFT,*R_ROT,*R_XOR,*R_HYPOT,*R_ENCRYPT,*R_DECRYPT,*R_SETENV,*R_RENAME,*R_SQR,*R_ABS,*R_SLEEP,*R_IC,*R_CI,*R_DJ,*R_JD,*R_LT,*R_LOG,*R_EXP,*R_SQRT,*R_FLOOR,*R_CEIL,*R_SIN,*R_COS,*R_TAN,*R_ASIN,*R_ACOS,*R_ATAN,*R_AT,*R_SS,*R_SM,*R_LSQ,*R_SINH,*R_COSH,*R_TANH,*R_ERF,*R_ERFC,*R_GAMMA,*R_LGAMMA,*R_RINT,*R_TRUNC,*R_NOT,*R_KV,*R_VK,*R_VAL,*R_BD,*R_DB,*R_HB,*R_BH,*R_ZB,*R_BZ,*R_MD5,*R_SHA1,*R_SHA2,*R_GETENV,*R_SVD,*R_LU,*R_QR,*R_LDU,*R_RREF,*R_DET,*R_MAG,*R_PRIME,*R_FACTOR,*R_GCD,*R_LCM,*R_MODINV,*R_EXIT,*R_DEL,*R_DO,*R_WHILE,*R_IF,*R_IN,*R_DVL,*R_LIN,*R_DVL,*R_DV,*R_DI,*R_GTIME,*R_LTIME,*R_TL,*R_MUL,*R_INV,*R_CHOOSE,*R_ROUND,*R_SSR,*R_EP;

K kcpcb(K x);
static inline K kcp2(K x) { return s(x)?kcpcb(x):k_(x); }
static inline K kerror(char *e) { return t(4,st(0x84,sp(e))); }

void kinit(void);
const char* kprint_(K x, char *s, char *e, char *s0);
void kprint(K x, char *s, char *e, char *s0);
K kerror(char *e);
i32 kreserved(char *p);
K val(K x);
K kamendi3(K d, K i, K f);
K kamendi4(K d, K i, K f, K y);
K kamend3(K d, K i, K f);
K kamend4(K d, K i, K f, K y);
K kslide(K f, K a, K x, char *av);
void kdump(i32 l);
void kdp(K x);

#endif /* K__H */
