#ifndef SORT_H
#define SORT_H

#include <stdlib.h>
#include "k.h"

int* csortg(int *g, int *a, unsigned int n, int min, int max, int down);
int* rcsortg(int *g, int *a, unsigned int n, int down);
void msortg1(int *g, int *a, int l, int r, int down);
void msortg2(int *g, double *a, int l, int r, int down);
void msortg3(int *g, char *a, int l, int r, int down);
void msortg4(int *g, char **a, int l, int r, int down);
void msortg0(int *g, K **a, int l, int r, int down);

#endif /* SORT_H */

