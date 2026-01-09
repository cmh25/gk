#ifndef LA_H
#define LA_H

#include "k.h"

K lsq_(K a, K x);
K svd_(K x);
K lu_(K x);
K qr_(K x);
K ldu_(K x);
K rref_(K x);

#endif /* LA_H */
