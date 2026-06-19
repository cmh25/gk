#ifndef RAND_H
#define RAND_H

#include <stdint.h>

void drawi(int *s, int64_t n, int m);
void drawj(int64_t *s, int64_t n, int64_t m);
void drawf(double *s, int64_t n, double m);
void drawe(float *s, int64_t n, float m);
void deal(int *s, int n, int m);
void dealj(int64_t *s, int n, int m);
void rand_reseed(unsigned long s);

#endif /* RAND_H */
