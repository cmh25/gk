#ifndef DICT_H
#define DICT_H

#include "k.h"

typedef struct dict {
  int c;
  char **k;
  K **v;
  int r;
} dict;

dict* dnew(void);
void dfree(dict *d);
dict* l2d(K *l);
K* d2l(dict *d);
dict* dcp(dict *d);
K* dget(dict *d, char *key);
void dset(dict *d, char *key, K *v);
K* dvals(dict *d);
K* dkeys(dict *d);
int dcmp(dict *d0, dict *d1);
uint64_t dhash(dict *d);

#endif /* DICT_H */
