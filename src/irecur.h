#ifndef IRECUR_H
#define IRECUR_H

#include "k.h"

K irecur1(K(*ff)(K), K x);
K irecur2(K(*ff)(K,K), K a, K x);

#endif /* IRECUR_H */
