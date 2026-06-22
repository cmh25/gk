#ifndef AES256_H
#define AES256_H

#include <stddef.h>

char* aes256e(const char *s, size_t n, unsigned char *iv, unsigned char *key, size_t *m);
char* aes256d(const char *s, size_t n, unsigned char *iv, unsigned char *key, size_t *m);

#endif /* AES256_H */
