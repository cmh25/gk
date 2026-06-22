#include "tmr.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "fn.h"
#include "p.h"
#include "repl.h"
#include "scope.h"

/* persistent state ------------------------------------------------ */

static double timer_interval_s = 0.0;  /* 0 = disabled */
static K      timer_fn          = 0;   /* owned ref, or 0 if never set */
static long long next_tick_ms   = 0;   /* mono clock target */
static int    in_tick           = 0;   /* re-entrancy guard */

static long long mono_ms(void) {
#ifdef _WIN32
  return (long long)GetTickCount64();
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
}

/* lifecycle ------------------------------------------------------- */

void tmr_init(void) {
  timer_interval_s = 0.0;
  if(timer_fn) { _k(timer_fn); timer_fn = 0; }
  next_tick_ms = 0;
  in_tick = 0;
}

void tmr_shutdown(void) {
  if(timer_fn) { _k(timer_fn); timer_fn = 0; }
  timer_interval_s = 0.0;
}

void tmr_fork_clear(void) {
  /* drop both schedule and fn so a child doesn't unexpectedly fire
   * a handler inherited from the parent. */
  if(timer_fn) { _k(timer_fn); timer_fn = 0; }
  timer_interval_s = 0.0;
  next_tick_ms = 0;
  in_tick = 0;
}

/* poll integration ------------------------------------------------ */

int tmr_timeout_ms(void) {
  if(timer_interval_s <= 0.0) return -1;
  if(in_tick)                 return -1;
  if(DEPTH != 1)              return -1;   /* only at top-level prompt */
  long long dt = next_tick_ms - mono_ms();
  if(dt < 0)                  return 0;
  if(dt > 2147483000LL)       return 2147483000;  /* poll() int range */
  return (int)dt;
}

/* --- error reporting --------------------------------------------
 * Print an error per the repl's normal rules and, under `\e 1`,
 * enter a debug sub-repl just like a top-level expression error
 * would (mirrors p.c::pgreduce_'s error path). ipc.c's apply1 can't
 * do this because a sync handler still owes its peer a SYNC_ERR
 * response; the timer has no peer, so blocking in a sub-repl is
 * fine -- and is what the user expects from `\e 1`.
 *
 * `e` is borrowed (the caller still owns and frees it). */
static void tmr_report_error(K e) {
  K p = (e < EMAX) ? kerror(E[e]) : k_(e);
  int is_abort = !strcmp(sk(p), "abort");
  kprint(k_(p), "", "\n", "");           /* clone for kprint to consume */
  if(EFLAG && !SIGNAL && !is_abort) {
    /* DEPTH==1 (gated by tmr_maybe_fire) and in_tick==1, so no
     * re-entrant tick can fire while the user is in the sub-repl. */
    _k(p);
    ++ecount;
    K rr = repl();
    if(rr != null && rr >= EMAX) _k(rr);
    return;
  }
  _k(p);
}

/* --- function invocation ----------------------------------------
 * Like ipc.c::apply1, but error reporting is delegated to
 * tmr_report_error so it picks up \e 1 sub-repl semantics. We
 * save+restore EFLAG so a handler that toggles `\e` itself can't
 * leak that change out. */
static K tmr_apply(K h, K arg) {
  K args = tn(0, 1);
  K *pa  = px(args);
  pa[0]  = arg;
  n(args) = 1;
  int e0 = EFLAG;
  K r    = fne(h, args, "");
  EFLAG  = e0;
  if(E(r)) tmr_report_error(r);
  return r;
}

/* Resolve a stored handler to a callable. If timer_fn is a sym
 * (pass-by-reference, e.g. `timer(3;`f)`), look it up in the global
 * scope at fire time so a later redefinition of `f` is picked up
 * automatically. Otherwise return a fresh ref to the stored fn.
 * Returns an owned K (caller must consume) or an error K. */
static K tmr_resolve(void) {
  if(T(timer_fn) == 4) return scope_get(gs, timer_fn);
  return k_(timer_fn);
}

/* Validate a candidate handler: must be callable with valence 1.
 * Returns 0 on success, an error K on failure. Does not consume fn. */
static K tmr_validate(K fn) {
  K vv = val(fn);
  if(E(vv)) return vv;
  i32 valence = ik(vv);
  _k(vv);
  if(valence != 1) return KERR_VALENCE;
  return 0;
}

void tmr_maybe_fire(void) {
  if(timer_interval_s <= 0.0) return;
  if(in_tick)                 return;
  if(DEPTH != 1)              return;
  if(!timer_fn)               return;
  if(mono_ms() < next_tick_ms) return;

  in_tick = 1;
  K fn = tmr_resolve();
  K r  = null;
  if(E(fn)) {
    /* Sym no longer resolves. Surface the error (and under \e 1
     * drop to a debug sub-repl); keep the timer armed so the user
     * may reinstate the global before the next tick. */
    tmr_report_error(fn);
    if(fn >= EMAX) _k(fn);
  } else if(T(timer_fn) == 4) {
    /* Sym path: the global may have been rebound to something that
     * isn't a 1-valence callable. Validate before invoking, since
     * fne() on a non-callable would otherwise crash. */
    K verr = tmr_validate(fn);
    if(verr) {
      tmr_report_error(verr);
      if(verr >= EMAX) _k(verr);
      _k(fn);
    } else {
      r = tmr_apply(fn, null);
    }
  } else {
    r = tmr_apply(fn, null);
  }
  if(E(r)) {
    if(r >= EMAX) _k(r);
  } else if(r != null && !quiet) {
    /* Echo non-null results, matching the repl's "echo every
     * top-level expression unless it's nul or we're in -q" rule
     * (see repl.c). kprint consumes r. The user can return nul
     * from inside the timer fn if they want a silent tick. */
    kprint(r, "", "\n", "");
  } else {
    _k(r);
  }

  /* Drop missed ticks: schedule next from now, not from the
   * theoretical next slot. Matches q's \t semantics ("if the
   * timer is already running when it's supposed to trigger,
   * that one just gets skipped"). */
  if(timer_interval_s > 0.0)
    next_tick_ms = mono_ms() + (long long)(timer_interval_s * 1000.0);
  in_tick = 0;
}

/* builtin --------------------------------------------------------- */

/* Build the (interval;fn) pair returned by query / set. If no fn
 * has ever been installed, returns (0.0;{}) as a safe placeholder
 * (a silent no-op lambda — calling it returns nul). */
static K tmr_state(void) {
  K r = tn(0, 2);
  K *p = px(r);
  p[0] = t2(timer_interval_s);
  if(timer_fn) p[1] = k_(timer_fn);
  else         p[1] = fnnew("{}");
  return r;
}

K timer_(K x) {
  /* Any 2-element value is a set attempt: mixed list (0.5;{1+2}),
   * but also typed vectors that K may have collapsed to (e.g. (1;5)
   * normalizes to the int vector 1 5). The first element must be a
   * numeric scalar; the second must be a 1-valence callable, or a
   * sym referring to one (pass-by-reference: timer(3;`f) is the
   * timer analog of ``f . `` and resolves at every tick, picking
   * up redefinitions of `f` automatically). Bad shapes raise a type
   * error rather than silently querying.
   *
   * Anything else (scalars, syms, nul, the empty list, longer lists,
   * ...) is a query and returns the current (t;f). */
  if(T(x) <= 0 && n(x) == 2) {
    K iv = xi_(x, 0, T(x));
    K fn = xi_(x, 1, T(x));
    K e  = 0;

    double sec = 0.0;
    if(T(iv) == 1)      sec = (double)ik(iv);
    else if(T(iv) == 2) sec = fk(iv);
    else                e = KERR_TYPE;
    if(!e && (!isfinite(sec) || sec < 0.0)) e = KERR_DOMAIN;

    /* For a sym, resolve it now to validate the target exists and
     * has valence 1, but store the *sym* (not the resolved fn) so
     * later redefinitions of the global pick up automatically. */
    if(!e) {
      if(T(fn) == 4) {
        K resolved = scope_get(gs, fn);
        if(E(resolved)) e = resolved;
        else {
          e = tmr_validate(resolved);
          _k(resolved);
        }
      } else {
        e = tmr_validate(fn);
      }
    }

    if(e) { _k(iv); _k(fn); return e; }

    if(timer_fn) _k(timer_fn);
    timer_fn = fn;        /* xi_ already gave us an owned ref */
    _k(iv);
    timer_interval_s = sec;
    if(sec > 0.0) next_tick_ms = mono_ms() + (long long)(sec * 1000.0);
    else          next_tick_ms = 0;
    return null;
  }

  return tmr_state();
}
