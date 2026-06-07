#ifndef SORT_H
#define SORT_H

#include <stdlib.h>
#include "k.h"

int* rcsortg(int *g, int *a, unsigned int n, int down);
int* rcsortg8(int *g, i64 *a, unsigned int n, int down);
int* rcsortg2(int *g, double *a, unsigned int n, int down);
int* rcsortg9(int *g, float *a, unsigned int n, int down);
void msortg1(int *g, int *a, int l, int r, int down);
void msortg8(int *g, i64 *a, int l, int r, int down);
void msortg3(int *g, char *a, int l, int r, int down);
void msortg4(int *g, char **a, int l, int r, int down);
void msortg0(int *g, K *a, int l, int r, int down);

#endif /* SORT_H */
