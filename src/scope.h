#ifndef SCOPE_H
#define SCOPE_H

#include "dict.h"
#include "k.h"

extern K ks,gs,cs;
extern K ktree,C,Z,D;
#define LOCALSMAX 2048
extern K locals[LOCALSMAX];

void scope_init(char **args, int nargs);

/* Refresh .z.P with the current process pid. .z.P is captured once during
 * scope_init, so anyone who forks (e.g. ipc.c's forking server) needs to
 * call this in the child or the pid will lie. */
void scope_refresh_pid(void);

/* Set .z.w to `fd` for the duration of a handler dispatch. Call with
 * `fd` set before invoking .m.s / .m.g / .m.c, and call with 0 after,
 * so outside-handler reads of .z.w see 0. */
void scope_set_z_w(int fd);

/* .z.filepath = the path of the script currently being loaded.
 * load() sets it on * entry and restores the prior value on exit, so nested
 * loads each see their own path. A loader script reads .z.filepath to find
 * its own directory and 2:-link a sibling .so without a collision-prone bare-name search.
 * _set returns the previous value (a retained ref, or 0 if the key was unset)
 * for the caller to hand back to _restore at the end of the load. */
K scope_set_z_filepath(char *path);
void scope_restore_z_filepath(K old);

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

/* Global-variable cache (storage in scope.c). The cache check is exposed
   here as an inline so name-resolution fast paths can short-circuit a cached
   global WITHOUT the scope_get -> scope_get_ call chain (scope_get_ only
   checks the cache at the bottom of that chain). Valid only when the active
   scope is gs and `n` is a simple (non-dotted) interned name -- the same
   guard scope_get_ uses. Invalidation is unchanged: gcache_clear() (called on
   every global assignment and dict modification) zeroes both arrays, so a
   hit here is exactly as fresh as a hit inside scope_get_. Returns a +1 ref
   on hit (caller owns it), or 0 on miss. */
#define GCACHEN 8
extern char *gcachek[GCACHEN];
extern K gcachev[GCACHEN];
static inline K gcache_get(char *n) {
  i(GCACHEN, if(gcachek[i]==n) return k_(gcachev[i]));
  return 0;
}

#endif /* SCOPE_H */
