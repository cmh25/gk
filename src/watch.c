#include "watch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "p.h"
#include "scope.h"

/* persistent state ------------------------------------------------
 *
 * Parallel arrays keyed by (scope path, name).  Both keys are interned
 * char* (symbol storage / scope path symbols), so matching at fire time
 * is a pair of pointer compares -- no strcmp.  WATCHMAX is small and the
 * table is scanned linearly; watches are expected to number in the few. */

#define WATCHMAX 256

static char *watch_scope[WATCHMAX]; /* interned namespace path, e.g. ".k"   */
static char *watch_name [WATCHMAX]; /* interned variable name,  e.g. "a"    */
static K     watch_expr [WATCHMAX]; /* owned char-vector: the trigger expr  */
static char  watch_busy [WATCHMAX]; /* 1 while this watch's expr is running */
int          nwatch = 0;

/* path symbol of a global scope (".k", ".foo", ...), or 0 if none */
static char *scope_path(K s) {
  K *ps = px(s);
  return ps[2] != null ? sk(ps[2]) : 0;
}

static int watch_find(char *scope, char *name) {
  for(int i = 0; i < nwatch; i++)
    if(watch_name[i] == name && watch_scope[i] == scope) return i;
  return -1;
}

/* Evaluate the trigger expression `e` (a char atom or vector) in scope
 * `sc`, discarding its value.  The trigger runs through the ordinary
 * eval path, so errors are printed and honor `\e` (the debug sub-repl)
 * exactly like any other top-level expression -- nothing special.
 * Mirrors the char-vector eval in v.c::valuecb (pgparse takes ownership
 * of the heap buffer; we must not free it). */
static void watch_run(K e, K sc) {
  size_t en = (T(e) == 3) ? 1 : (size_t)n(e);
  char *h = xmalloc(en + 2);
  if(T(e) == 3) h[0] = ck(e);
  else { char *pe = px(e); for(size_t i = 0; i < en; i++) h[i] = pe[i]; }
  h[en] = '\n'; h[en + 1] = 0;

  /* run the expression in the watched namespace, not whatever scope the
   * triggering assignment happened to be made from (e.g. a `::` inside a
   * function). */
  K cs0 = cs; cs = k_(sc);
  int oc = opencode; opencode = 1;
  K q = pgparse(h, 0, 0);
  opencode = oc;
  if(!E(q) && q) {
    K r = pgreduce(q, 0);
    prfree(q);
    if(E(r)) { if(0x84 == s(r)) _k(r); } else _k(r);
  } else if(E(q) && 0x84 == s(q)) _k(q);
  _k(cs); cs = cs0;
}

/* Fire the watch keyed by (path, name), running its trigger in `eval`.
 * path/name must be interned.  `eval` is the scope the trigger runs in. */
static void watch_fire_(char *path, char *name, K eval) {
  int i = watch_find(path, name);
  if(i < 0 || watch_busy[i]) return;
  watch_busy[i] = 1;
  watch_run(watch_expr[i], eval);
  /* The trigger may have called watch() to clear or re-arm this watch,
   * which swap-compacts the table -- so re-locate by key before clearing
   * the busy flag rather than trusting the old index. */
  i = watch_find(path, name);
  if(i >= 0) watch_busy[i] = 0;
}

/* Simple (non-dotted) assignment to global scope `scope`. */
void watch_fire(K scope, char *name) {
  char *p = scope_path(scope);
  if(p) watch_fire_(p, name, scope);
}

/* Fully-qualified assignment `.ns.var:...`: `path` is the namespace
 * (".ns"), `name` the variable.  Resolves the namespace to its scope
 * for the trigger eval.  Both args are raw (not necessarily interned). */
void watch_fire_path(char *path, char *name) {
  char *p = sp(path), *nm = sp(name);
  if(watch_find(p, nm) < 0) return;   /* skip scope_find unless armed */
  K sc = scope_find(p);
  if(sc) watch_fire_(p, nm, sc);
}

/* Fire watches touched by a fully-qualified assignment (".ns.var...").
 * Used by the dotted-path branch of scope_set_ and the in-place ktree-amend
 * (kt) paths in k.c, which mutate a qualified variable without going through
 * scope_set.
 *
 * An assignment like `.k.a.b:3` mutates the dict variable `.k.a` (it gains
 * member `b`) AND, if `.k.a` is itself a namespace, assigns var `b` within
 * it.  So every interior dot is a candidate (namespace, var) boundary:
 * (".k","a") and (".k.a","b").  We fire any watch armed at each -- splitting
 * only at the last dot (the old behaviour) misses the watch on `a`. */
void watch_fire_fq(char *fq) {
  char nb[256];
  size_t L = strlen(fq);
  if(L == 0 || L >= sizeof nb) return;
  memcpy(nb, fq, L + 1);
  /* skip the leading dot of an absolute path; each later dot splits the
   * path into (namespace = text before it, var = the single token after). */
  for(char *dot = strchr(nb + 1, '.'); dot; dot = strchr(dot + 1, '.')) {
    char *var = dot + 1, *nd = strchr(var, '.');
    char vb[256]; size_t vl = nd ? (size_t)(nd - var) : strlen(var);
    memcpy(vb, var, vl); vb[vl] = 0;
    *dot = 0;                       /* nb = namespace path for this boundary */
    watch_fire_path(nb, vb);        /* no-op unless a watch is armed there   */
    *dot = '.';                     /* restore for the next boundary         */
  }
}

/* Free all armed watches.  Called from exit__ so the stored trigger
 * exprs (owned refs) don't leak at shutdown -- mirror of tmr_shutdown. */
void watch_shutdown(void) {
  for(int i = 0; i < nwatch; i++) _k(watch_expr[i]);
  nwatch = 0;
}

/* SET / CLEAR a watch on `name` in the current namespace. `expr` is the
 * trigger; an empty string or nul clears.  Consumes neither argument
 * (they are borrowed from the caller's list). */
static K watch_set(K name, K expr) {
  char *sc = scope_path(gs);
  if(!sc) return KERR_DOMAIN;
  char *nm = sk(name);
  /* the empty symbol (`) is not a variable name; reject it for both set and
   * clear rather than storing the nonsensical key ".k." */
  if(!*nm) return KERR_DOMAIN;

  /* nul or "" clears; a non-empty char atom/vector sets; anything else
   * is a type error. */
  int clear;
  if(expr == null)                          clear = 1;
  else if(T(expr) == -3 && n(expr) == 0)    clear = 1;
  else if(T(expr) == 3 || T(expr) == -3)    clear = 0;
  else                                       return KERR_TYPE;

  int i = watch_find(sc, nm);
  if(clear) {
    if(i >= 0) {
      _k(watch_expr[i]);
      int last = --nwatch;
      watch_scope[i] = watch_scope[last];
      watch_name [i] = watch_name [last];
      watch_expr [i] = watch_expr [last];
      watch_busy [i] = watch_busy [last];
    }
    return null;
  }
  if(i >= 0) { _k(watch_expr[i]); watch_expr[i] = k_(expr); return null; }
  if(nwatch == WATCHMAX) return KERR_LENGTH;
  watch_scope[nwatch] = sc;
  watch_name [nwatch] = nm;
  watch_expr [nwatch] = k_(expr);
  watch_busy [nwatch] = 0;
  nwatch++;
  return null;
}

/* QUERY: a list of (`fqname;"expr") pairs, fqname = path + "." + name. */
static K watch_list(void) {
  K r = tn(0, nwatch); K *pr = px(r);
  for(int i = 0; i < nwatch; i++) {
    char fq[256];
    snprintf(fq, sizeof fq, "%s.%s", watch_scope[i], watch_name[i]);
    K pair = tn(0, 2); K *pp = px(pair);
    pp[0] = t(4, sp(fq));
    pp[1] = k_(watch_expr[i]);
    pr[i] = pair;
  }
  return r;
}

K watch_(K x) {
  /* A 2-element list is the set/clear form (`name;"expr"); its first
   * element must be a symbol -- anything else is a type error, not a
   * query.  A query is any argument that isn't a 2-element list
   * (watch[], watch`, watch nul, ...). */
  if(T(x) <= 0 && n(x) == 2) {
    K name = xi_(x, 0, T(x));
    if(T(name) != 4) { _k(name); return KERR_TYPE; }
    K expr = xi_(x, 1, T(x));
    K r = watch_set(name, expr);
    _k(name); _k(expr);
    return r;
  }
  return watch_list();
}
