#ifndef OPS_H
#define OPS_H

#include "k.h"

/* function return */
extern K *gk;
extern int fret;

/* dyadic */
K* plus2_(K *a, K *b);
K* minus2_(K *a, K *b);
K* times2_(K *a, K *b);
K* divide2_(K *a, K *b);
K* minand2_(K *a, K *b);
K* maxor2_(K *a, K *b);
K* less2_(K *a, K *b);
K* more2_(K *a, K *b);
K* equal2_(K *a, K *b);
K* power2_(K *a, K *b);
K* modrot2_(K *a, K *b);
K* match2_(K *a, K *b);
K* join2_(K *a, K *b);
K* take2_(K *a, K *b);
K* drop2_(K *a, K *b);
K* form2_(K *a, K *b);
K* find2_(K *a, K *b);
K* at2_(K *a, K *b);
K* dot2_(K *a, K *b);
K* assign2_(K *a, K *b);
K* assign2g_(K *a, K *b);

/* monadic */
K* flip_(K *a);
K* negate_(K *a);
K* first_(K *a);
K* reciprocal_(K *a);
K* where_(K *a);
K* reverse_(K *a);
K* upgrade_(K *a);
K* downgrade_(K *a);
K* group_(K *a);
K* shape_(K *a);
K* enumerate_(K *a);
K* not_(K *a);
K* enlist_(K *a);
K* count_(K *a);
K* flr_(K *a);
K* format_(K *a);
K* unique_(K *a);
K* atom_(K *a);
K* value_(K *a);
K* return_(K *a);

K* fourcolon1_(K *a);
K* fivecolon1_(K *a);
K* precision1_(K *a);
K* kdump1_(K *a);

K* apply4(K *f, K *a, K *b, K *c, K *d);
K* apply3(K *f, K *a, K *b, K *c);
K* apply2(K *f, K *a, K *b, char *av);
K* apply1(K *f, K *a, char *av);
K* applyfc2_(K *f, K *a, K *b);
K* applyfc2(K *f, K *a, K *b, char *av);
K* applyfc1_(K *f, K *a);
K* applyfc1(K *f, K *a, char *av);
K* applyprj(K *f, K *a);

K* signal_(K* a);
K* abort1_(K* a);
K* dir1_(K* a);
K* val1_(K *a);
K* help1_(K *a);
K* bd1_(K *a);
K* db1_(K *a);

K* load1_(K *a);

#endif /* OPS_H */
