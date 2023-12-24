#ifndef INTERP_H
#define INTERP_H
#include <stdio.h>
#include "k.h"
extern int ecount,TOP;
K* interp(char *fn, int interactive, int top);
#endif /* INTERP_H */
