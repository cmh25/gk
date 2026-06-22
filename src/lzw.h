#ifndef LZW_H
#define LZW_H

#include <stddef.h>

typedef struct {
  char v;   /* version */
  size_t i; /* bit index */
  size_t n; /* bytes */
  size_t c; /* field count */
  unsigned char *b;
} LZW;

LZW* lzwnew(void);
void lzwfree(LZW *b);
LZW* lzwc(char *s, size_t n);
LZW* lzwd(LZW *z);

#endif /* LZW_H */
