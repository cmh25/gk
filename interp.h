#ifndef INTERP_H
#define INTERP_H
#include <stdio.h>
#include "k.h"
#include "node.h"
#include "p.h"
extern int ecount,TOP;
K* reduce(node *a, pgs *s, int top);
K* load(char *fnm);
K* interp(int top);
#endif /* INTERP_H */
