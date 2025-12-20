#ifndef DICT_H
#define DICT_H

#include "k.h"

#define nk n(k)
#define nv n(v)
#define DMAX 100

K dnew(void);
void dfree(K d);
K dget(K d, char *key);
K dset(K d, char *key, K val);
K dvals(K d);
K dkeys(K d);
int dcmp(K d0, K d1);
K dcp(K d);

#endif /* DICT_H */
