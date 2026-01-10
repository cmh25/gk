#ifndef NT_H
#define NT_H

#include "k.h"

K prime_(K x);      /* primality predicate */
K factor_(K x);     /* prime factorization */
K gcd_(K a, K x);   /* greatest common divisor */
K lcm_(K a, K x);   /* least common multiple */
K modinv_(K a, K x); /* modular multiplicative inverse */

#endif /* NT_H */
