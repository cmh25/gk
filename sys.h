#ifndef SYS_H
#define SYS_H

#include "k.h"
#include "node.h"

K* sleep1_(K *a, char *av);
K* exit1_(K *a, char *av);
K* t0(K *a, char *av);
K* tt0(K *a, char *av);

K* vs2_(K *a, K *b, char *av);      K* vs2_avopt2(K *a, K *b, char *av);
K* sv2_(K *a, K *b, char *av);      K* sv2_avopt2(K *a, K *b, char *av);
K* draw2_(K *a, K *b, char *av);    K* draw2_avopt2(K *a, K *b, char *av);
K* ci1_(K *a, char *av);            K* ci1_avopt(K *a, char *av);
K* ic1_(K *a, char *av);            K* ic1_avopt(K *a, char *av);
K* dj1_(K *a, char *av);            K* dj1_avopt(K *a, char *av);
K* jd1_(K *a, char *av);            K* jd1_avopt(K *a, char *av);
K* lt1_(K *a, char *av);            K* lt1_avopt(K *a, char *av);

K* log1_(K *a, char *av);           K* log1_avopt(K *a, char *av);
K* exp1_(K *a, char *av);           K* exp1_avopt(K *a, char *av);
K* abs1_(K *a, char *av);           K* abs1_avopt(K *a, char *av);
K* sqr1_(K *a, char *av);           K* sqr1_avopt(K *a, char *av);
K* sqrt1_(K *a, char *av);          K* sqrt1_avopt(K *a, char *av);
K* floor1_(K *a, char *av);         K* floor1_avopt(K *a, char *av);
K* ceil1_(K *a, char *av);          K* ceil1_avopt(K *a, char *av);
K* ddot2_(K *a, K *b, char *av);    K* ddot2_avopt2(K *a, K *b, char *av);
K* sin1_(K *a, char *av);           K* sin1_avopt(K *a, char *av);
K* cos1_(K *a, char *av);           K* cos1_avopt(K *a, char *av);
K* tan1_(K *a, char *av);           K* tan1_avopt(K *a, char *av);
K* asin1_(K *a, char *av);          K* asin1_avopt(K *a, char *av);
K* acos1_(K *a, char *av);          K* acos1_avopt(K *a, char *av);
K* atan1_(K *a, char *av);          K* atan1_avopt(K *a, char *av);
K* sinh1_(K *a, char *av);          K* sinh1_avopt(K *a, char *av);
K* cosh1_(K *a, char *av);          K* cosh1_avopt(K *a, char *av);
K* tanh1_(K *a, char *av);          K* tanh1_avopt(K *a, char *av);
K* atan2_(K *a, K *b, char *av);    K* atan2_avopt2(K *a, K *b, char *av);
K* euclid2_(K *a, K *b, char *av);  K* euclid2_avopt2(K *a, K *b, char *av);
K* erf1_(K *a, char *av);           K* erf1_avopt(K *a, char *av);
K* erfc1_(K *a, char *av);          K* erfc1_avopt(K *a, char *av);
K* gamma1_(K *a, char *av);         K* gamma1_avopt(K *a, char *av);
K* lgamma1_(K *a, char *av);        K* lgamma1_avopt(K *a, char *av);
K* rint1_(K *a, char *av);          K* rint1_avopt(K *a, char *av);
K* trunc1_(K *a, char *av);         K* trunc1_avopt(K *a, char *av);
K* div2_(K *a, K *b, char *av);     K* div2_avopt2(K *a, K *b, char *av);
K* and2_(K *a, K *b, char *av);     K* and2_avopt2(K *a, K *b, char *av);
K* or2_(K *a, K *b, char *av);      K* or2_avopt2(K *a, K *b, char *av);
K* xor2_(K *a, K *b, char *av);     K* xor2_avopt2(K *a, K *b, char *av);
K* not1_(K *a, char *av);           K* not1_avopt(K *a, char *av);
K* rot2_(K *a, K *b, char *av);     K* rot2_avopt2(K *a, K *b, char *av);
K* shift2_(K *a, K *b, char *av);   K* shift2_avopt2(K *a, K *b, char *av);
K* lsq2_(K *a, K *b, char *av);     K* lsq2_avopt2(K *a, K *b, char *av);
K* atn2_(K *a, K *b, char *av);     K* atn2_avopt2(K *a, K *b, char *av);
K* sm2_(K *a, K *b, char *av);      K* sm2_avopt2(K *a, K *b, char *av);
K* ss2_(K *a, K *b, char *av);      K* ss2_avopt2(K *a, K *b, char *av);
K* kv1_(K *a, char *av);            K* kv1_avopt(K *a, char *av);
K* timer1_(K *a, char *av);         K* timer1_avopt(K *a, char *av);
K* error1_(K *a, char *av);         K* error1_avopt(K *a, char *av);

#endif /* SYS_H */
