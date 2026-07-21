#ifndef V_H
#define V_H

#include "k.h"

#define FDSIZE 20
#define FMSIZE 20
extern K (*FD[FDSIZE])(K,K);
extern K (*FM[FMSIZE])(K);

K plus(K a, K x);
K minus(K a, K x);
K times_(K a, K x);
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
K findr(K a, K x);
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

/* one-line scalar helpers shared by v.c, fuse.c, and the k() dispatch
   inline (k.c) -- fast paths must call the SAME definitions the verbs do */
static inline i32 icmp(i32 a,i32 b,i8 op) { return op<0?a<b:op>0?a>b:a==b; }
static inline i32 jcmp(i64 a,i64 b,i8 op) { return op<0?a<b:op>0?a>b:a==b; }
static inline i32 modi(i32 a, i32 b){ i32 r; if(!b) return INT32_MIN; if(b==-1) return 0; r=a%b; if(r&&((r<0)!=(b<0))) r+=b; return r; }

#endif /* V_H */
