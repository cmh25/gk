#ifndef WATCH_H
#define WATCH_H

/* Variable watches (triggers).
 *
 * A watch binds a *global* variable to a gk expression string that is
 * evaluated for its side effects every time that variable is assigned
 * (`a:...`, `a+:...`, `a::...`).
 *
 * User surface (single monadic builtin):
 *
 *   watch (`name;"expr")   SET: arm a watch on global `name` in the
 *                          CURRENT namespace.  "expr" runs (in that
 *                          namespace) after each assignment to `name`.
 *                          Re-arming an existing watch replaces its
 *                          expression.  Returns nul.
 *   watch (`name;"")       CLEAR: remove the watch on `name` (an empty
 *                          string or nul as the expression).  Returns nul.
 *   watch x                QUERY (any non-2-list argument: watch[],
 *                          watch nul, watch`, ...).  Returns a list of
 *                          (`fqname;"expr") pairs, one per armed watch,
 *                          where fqname is the fully-qualified name
 *                          (e.g. `.k.a).  Empty list if none armed.
 *
 * Scope:
 *   Watches are keyed by (namespace, name).  Different k-tree
 *   namespaces (`.k`, `.foo`, ...) can hold same-named variables with
 *   independent watches.  A watch fires only for assignments to its own
 *   namespace's variable.  Only simple (non-dotted) assignments to the
 *   current namespace fire; a fully-qualified cross-namespace assign
 *   (`.foo.a:1` from `.k`) does not.
 *
 * Semantics:
 *   - The expression runs AFTER the new value is stored, so it observes
 *     the post-assignment state.
 *   - Re-entrancy: a watch will not re-fire while its own expression is
 *     still running, so a watch that re-assigns its own variable runs
 *     once (not forever).  Distinct watches may chain (a's expr assigns
 *     b, firing b's watch); a cycle terminates when it would re-enter an
 *     already-firing watch.
 *   - Errors in the expression are printed (prefixed "watch:") and do
 *     not abort the triggering assignment.
 */

#include "k.h"

/* Number of armed watches.  Exposed so the global-assignment fast path
 * in scope.c can gate the whole mechanism behind a single integer test
 * (zero watches -> no work, no slowdown to ordinary assignment). */
extern int nwatch;

/* Fire any watch armed on (scope, name).  `name` is an interned name
 * pointer; `scope` is the global scope K the assignment landed in.
 * No-op unless a matching, not-currently-firing watch exists.  Called
 * from scope_set_ only when nwatch>0. */
void watch_fire(K scope, char *name);

/* Fire the watch for a fully-qualified assignment `.ns.var:...`: `path`
 * is the namespace (e.g. ".foo"), `name` the variable.  Used by the
 * dotted-path branch of scope_set_ so qualified and cross-namespace
 * assignments trigger watches too.  Called only when nwatch>0. */
void watch_fire_path(char *path, char *name);

/* Fire the watch for a fully-qualified name (".ns.var"), splitting at the
 * last dot.  Used by scope_set_'s dotted branch and the in-place
 * ktree-amend paths in k.c.  Called only when nwatch>0. */
void watch_fire_fq(char *fq);

/* Free all armed watches (their owned trigger exprs).  Called from
 * exit__ alongside tmr_shutdown so they don't leak at shutdown. */
void watch_shutdown(void);

/* The builtin.  See header comment for surface. */
K watch_(K x);

#endif /* WATCH_H */
