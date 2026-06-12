#ifndef IO_H
#define IO_H

#include "k.h"

K zerocolon(K a, K x);
K onecolon(K a, K x);
K twocolon(K a, K x);
K threecolon(K a, K x);
K fourcolon(K a, K x);
K fivecolon(K a, K x);
K sixcolon(K a, K x);
K eightcolon(K a, K x);
K del_(K a);
K rename_(K a, K x);
K linkcall(K f, K x); /* apply a 2:-linked C function (subtype 0xdc) to arg list x */

#endif /* IO_H */
