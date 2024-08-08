#ifndef HT_H
#define HT_H

#include <stddef.h>

typedef struct {
  char **kh;  /* keys */
  int *nh;    /* keys lengths */
  int *vh;    /* values */
  size_t i;   /* index */
  size_t m;   /* max */
} HT;

HT* htnew(size_t m);
void htfree(HT *ht);
void htset(HT *ht, char *k, int n, int v);
int htget(HT *ht, char *k, int n);

#endif /* HT_H */
