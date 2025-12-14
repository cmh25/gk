#ifndef SCOPE_H
#define SCOPE_H

#include "dict.h"
#include "k.h"

extern K ks,gs,cs;
extern K ktree,C,Z,D;
#define LOCALSMAX 2048
extern K locals[LOCALSMAX];
extern int localsi;

void scope_init(int argc, char **argv);
K scope_new(K p);
K scope_newk(K p, K k);
void scope_free(K s);
void scope_free_all(void);
K scope_get(K s, K n);
K scope_set(K s, K n, K v);
K scope_cp(K s);
K scope_find(char *x);
int scope_vktp(char *x);
void gcache_clear(void);

#endif /* SCOPE_H */
