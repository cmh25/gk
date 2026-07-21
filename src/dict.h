#ifndef DICT_H
#define DICT_H

#include "k.h"

#define nk n(k)
#define nv n(v)
#define DMAX 128

/* Max keys in a dict.  The hash index stores i32 key positions and the capacity
 * slot is read via ik (i32), so past INT32_MAX the dict would silently corrupt;
 * dset/valuecb error with wsfull instead.  Overridable (like BIGV) to exercise
 * the guard in a test build without allocating a 32GB+ dict. */
#ifndef DICTMAX
#define DICTMAX 0x7fffffff
#endif

/* fuzz-build invariant check (dict.c): semantic corruption becomes an abort()
   the fuzzer can see.  No-op outside -DFUZZING. */
#ifdef FUZZING
void dcheck(K d);
#define DCHK(d) dcheck(d)
#else
#define DCHK(d) ((void)0)   /* not empty: `if(c) DCHK(d);` would be -Wempty-body */
#endif

K dnew(void);
void dfree(K d);
K dget(K d, char *key);
K dset(K d, char *key, K val);
K dvals(K d);
K dkeys(K d);
int dcmp(K d0, K d1);
K dcp(K d);

#endif /* DICT_H */
