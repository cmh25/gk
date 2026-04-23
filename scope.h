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

/* Refresh .z.P with the current process pid. .z.P is captured once during
 * scope_init, so anyone who forks (e.g. ipc.c's forking server) needs to
 * call this in the child or the pid will lie. */
void scope_refresh_pid(void);

/* Set .z.w to `fd` for the duration of a handler dispatch. Call with
 * `fd` set before invoking .m.s / .m.g / .m.c, and call with 0 after,
 * so outside-handler reads of .z.w see 0. */
void scope_set_z_w(int fd);
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
