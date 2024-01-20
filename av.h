#ifndef AV_H
#define AV_H

#include "k.h"

K* avdom(K *f, K *a, char *av);
K* eachm(K *f, K *a, char *av);
K* overd(K *f, K *a, char *av);
K* scand(K *f, K *a, char *av);
K* over(K *f, K *a);
K* scan(K *f, K *a);
K* overm(K *f, K *a);
K* scanm(K *f, K *a);
K* eachparam(K *f, K *a);

K* avdo(K *f, K *a, K *b, char *av);
//K* eachprior(K *f, K *a, K *b, char *av);
K* slide(K *f, K *a, K *b, char *av);
K* eachright(K *f, K *a, K *b, char *av);
K* eachleft(K *f, K *a, K *b, char *av);

K* avdo37(K *f, K *a, char *av);
K* avdo37infix(K *f, K *a, char *av);
K* eachright37(K *f, K *a, char *av);
K* eachleft37(K *f, K *a, char *av);
K* eachparam37(K *f, K *a);
K* eachm37(K *f, K *a, char *av);
K* over37(K *f, K *a, char *av);
K* overm37(K *f, K *a, char *av);
K* overm37infix(K *f, K *a, char *av);
K* scan37(K *f, K *a, char *av);
K* scanm37(K *f, K *a, char *av);
K* scanm37infix(K *f, K *a, char *av);
K* eachprior37(K *f, K *a, char *av);
K* slide37infix(K *f, K *a, char *av);
K* overd37(K *f, K *a, char *av);
K* scand37(K *f, K *a, char *av);

K* each(K*(*f)(K*,K*), K *a, K *b);

K* avdomfc(K *f, K *a, char *av);
K* eachmfc(K *f, K *a, char *av);
K* overdfc(K *f, K *a, char *av);
K* scandfc(K *f, K *a, char *av);
K* overfc(K *f, K *a);
K* scanfc(K *f, K *a);
K* eachparamfc(K *f, K *a);

K* avdofc(K *f, K *a, K *b, char *av);
//K* eachpriorfc(K *f, K *a, K *b, char *av);
K* slidefc(K *f, K *a, K *b, char *av);
K* eachrightfc(K *f, K *a, K *b, char *av);
K* eachleftfc(K *f, K *a, K *b, char *av);

#endif /* AV_H */
