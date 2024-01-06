#ifndef FN_H
#define FN_H

#include "k.h"
#include "node.h"
#include "scope.h"

typedef struct fn {
  char *f;        /* definition */
  unsigned int v; /* valence */
  node *n;        /* ast */
  scope *s_;      /* initial scope */
  scope *s;       /* active scope */
  char *av;       /* adverbs */
  int q;          /* explicit formal params */
  K *l;           /* left arg, fixed dyad, projection */
  K *r;           /* right arg, intermediate parse */
  K **a;          /* 7 list, function composition */
  int an;         /* count, function composition */
  int i;          /* index into dt */
} fn;

extern int DEPTH;
extern K* (*dt2[256])(K *a, K *b, char *av);
extern K* (*dt1[256])(K *a, char *av);
extern K* (*dt2avo[256])(K *a, char *av);
extern K* (*dt2avo2[256])(K *a, K *b, char *av);

fn* fnnew(char *f);
void fnfree(fn *fn);
fn* fncp(fn *fn);
void fninit();
K* fnd(K *f);
K* fne2(K *f, K *a, char *av);

#endif /* FN_H */
