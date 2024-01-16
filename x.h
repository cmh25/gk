#ifndef X_H
#define X_H

#include <stdlib.h>
#include <stdint.h>

extern char esc[256];

void* xmalloc(size_t s);
void xfree(void *p);
void* xcalloc(size_t n, size_t s);
void* xrealloc(void *p, size_t s);
void* xstrdup(const char *s);
void* xstrndup(const char *s, size_t n);
int xatoi(char *s);
double xstrtod(char *s, char **e);
uint64_t xfnv1a(char *v, uint64_t n);
char* xesc(char *s);
char* xunesc(char *s);
char* xeqs(char *p);

#endif /* X_H */
