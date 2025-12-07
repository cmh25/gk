#ifndef V_H
#define V_H

#include "k.h"

#define FDSIZE 20
#define FMSIZE 20
extern K (*FD[FDSIZE])(K,K);
extern K (*FM[FMSIZE])(K);

K plus(K a, K x);
K minus(K a, K x);
K times(K a, K x);
K divide(K a, K x);
K minand(K a, K x);
K maxor(K a, K x);
K less(K a, K x);
K more(K a, K x);
K equal(K a, K x);
K match(K a, K x);
K dot(K a, K x);
K modrot(K a, K x);
K at(K a, K x);
K find(K a, K x);
K take(K a, K x);
K drop(K a, K x);
K power(K a, K x);
K join(K a, K x);
K form(K a, K x);

K flip(K x);
K negate(K x);
K first(K x);
K recip(K x);
K where(K x);
K reverse(K x);
K upgrade(K x);
K downgrade(K x);
K group(K x);
K not_(K x);
K value(K x);
K enumerate(K x);
K atom(K x);
K unique(K x);
K count(K x);
K floor__(K x);
K shape(K x);
K enlist(K x);
K format(K x);

#endif /* V_H */
