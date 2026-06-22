#ifndef SORT_H
#define SORT_H

#include <stdlib.h>
#include "k.h"

int* rcsortg(int *g, int *a, unsigned int n, int down);
int* rcsortg8(int *g, i64 *a, unsigned int n, int down);
int* rcsortg2(int *g, double *a, unsigned int n, int down);
int* rcsortg9(int *g, float *a, unsigned int n, int down);
int* csortg3(int *g, char *a, u64 n, int down);          /* char counting sort */
void msortg1(int *g, int *a, int l, int r, int down);
void msortg8(int *g, i64 *a, int l, int r, int down);
void msortg3(int *g, char *a, int l, int r, int down);
void msortg0(int *g, K *a, int l, int r, int down);

/* i64-index variants (suffix j): for grading vectors with >2^31 elements */
i64* rcsortgj(i64 *g, int *a, u64 n, int down);
i64* rcsortg8j(i64 *g, i64 *a, u64 n, int down);
i64* rcsortg2j(i64 *g, double *a, u64 n, int down);
i64* rcsortg9j(i64 *g, float *a, u64 n, int down);
i64* csortg3j(i64 *g, char *a, u64 n, int down);         /* char counting sort */
void msortg1j(i64 *g, int *a, i64 l, i64 r, int down);
void msortg8j(i64 *g, i64 *a, i64 l, i64 r, int down);
void msortg3j(i64 *g, char *a, i64 l, i64 r, int down);
void msortg0j(i64 *g, K *a, i64 l, i64 r, int down);

/* MSD byte-radix grade (sym -4): replaces the string-compare merge sort, fills
   g with the grade permutation itself (no enumerate prefill needed). */
i32* rsortg4(i32 *g, char **a, u64 n, int down);
i64* rsortg4j(i64 *g, char **a, u64 n, int down);

#endif /* SORT_H */
