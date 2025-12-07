#ifndef FN_H
#define FN_H

#include "k.h"
#include "scope.h"
#include "p.h"

extern K fnestack[1024];
extern int fnestacki;

void fninit(void);
K fnnew(char *f);
void fnfree(K f);
K fnd(K f);
K fncp(K f);
K fne_(K f, K x, char *av);
K fne(K f, K x, char *av);

#endif /* FN_H */
