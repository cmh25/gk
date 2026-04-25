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

/* --- function invocation, mirroring ipc.c::apply1 ----------------
 * We keep this local rather than sharing with ipc.c so the two
 * subsystems can evolve independently; both wrappers are small. */
static K tmr_apply(K h, K arg) {
  K args = tn(0, 1);
  K *pa  = px(args);
  pa[0]  = arg;
  n(args) = 1;
  int e0 = EFLAG;
  K r    = fne(h, args, "");
  EFLAG  = e0;
  if(!e0 && E(r)) {
    K p = (r < EMAX) ? kerror(E[r]) : r;
    kprint(p, "", "\n", "");
    if(p != r) _k(p);
  }
  return r;
}

void tmr_maybe_fire(void) {
  if(timer_interval_s <= 0.0) return;
  if(in_tick)                 return;
  if(DEPTH != 1)              return;
  if(!timer_fn)               return;
  if(mono_ms() < next_tick_ms) return;

  in_tick = 1;
  K r = tmr_apply(k_(timer_fn), null);
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
   * numeric scalar; the second must be a 1-valence callable. Bad
   * shapes raise a type error rather than silently querying.
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

    /* fn must be callable (val() errors for ints, floats, syms,
     * strings, ...) and have valence exactly 1, since we'll invoke
     * it as fn[nul]. {x+y} would just project rather than fire,
     * which is never what the user wants. */
    if(!e) {
      K vv = val(fn);
      if(E(vv)) e = vv;
      else {
        i32 valence = ik(vv);
        _k(vv);
        if(valence != 1) e = KERR_VALENCE;
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
