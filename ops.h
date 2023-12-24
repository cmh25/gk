#ifndef OPS_H
#define OPS_H

#include "k.h"

/* function return */
extern K *gk;
extern int fret;

/* dyadic */
K* plus2_(K *a, K *b, char *av);
K* minus2_(K *a, K *b, char *av);
K* times2_(K *a, K *b, char *av);
K* divide2_(K *a, K *b, char *av);
K* minand2_(K *a, K *b, char *av);
K* maxor2_(K *a, K *b, char *av);
K* less2_(K *a, K *b, char *av);
K* more2_(K *a, K *b, char *av);
K* equal2_(K *a, K *b, char *av);
K* power2_(K *a, K *b, char *av);
K* modrot2_(K *a, K *b, char *av);
K* match2_(K *a, K *b, char *av);
K* join2_(K *a, K *b, char *av);
K* take2_(K *a, K *b, char *av);
K* drop2_(K *a, K *b, char *av);
K* form2_(K *a, K *b, char *av);
K* find2_(K *a, K *b, char *av);
K* at2_(K *a, K *b, char *av);
K* dot2_(K *a, K *b, char *av);
K* assign2_(K *a, K *b, char *av);

/* monadic */
K* flip_(K *a, char *av);
K* negate_(K *a, char *av);
K* first_(K *a, char *av);
K* reciprocal_(K *a, char *av);
K* where_(K *a, char *av);
K* reverse_(K *a, char *av);
K* upgrade_(K *a, char *av);
K* downgrade_(K *a, char *av);
K* group_(K *a, char *av);
K* shape_(K *a, char *av);
K* enumerate_(K *a, char *av);
K* not_(K *a, char *av);
K* enlist_(K *a, char *av);
K* count_(K *a, char *av);
K* flr_(K *a, char *av);
K* format_(K *a, char *av);
K* unique_(K *a, char *av);
K* atom_(K *a, char *av);
K* value_(K *a, char *av);
K* return_(K *a, char *av);

K* fourcolon1_(K *a, char *av);
K* fivecolon1_(K *a, char *av);
K* precision1_(K *a, char *av);
K* kdump1_(K *a, char *av);

K* apply4(K *f, K *a, K *b, K *c, K *d, char *av);
K* apply3(K *f, K *a, K *b, K *c, char *av);
K* apply2(K *f, K *a, K *b, char *av);
K* apply1(K *f, K *a, char *av);
K* applyfc2_(K *f, K *a, K *b, char *av);
K* applyfc2(K *f, K *a, K *b, char *av);
K* applyfc1_(K *f, K *a, char *av);
K* applyfc1(K *f, K *a, char *av);
K* applyprj(K *f, K *a, char *av);

K* signal_(K* a, char *av);
K* abort1_(K* a, char *av);
K* dir1_(K* a, char *av);
K* val1_(K *a, char *av);
K* help1_(K *a, char *av);
K* bd1_(K *a, char *av);
K* db1_(K *a, char *av);

#endif /* OPS_H */
