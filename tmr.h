#ifndef TMR_H
#define TMR_H

/* Recurring timer.
 *
 * User surface (single monadic builtin):
 *
 *   timer (t;f)   SET: t is seconds (int or float; 0 disables); f is
 *                 a 1-valence fn called as f[nul] each tick. Returns
 *                 nul.
 *   timer x       QUERY (any non-2-list argument: timer[], timer nul,
 *                 timer`, timer 1, ...). Returns the current (t;f).
 *                 Before any set, returns (0.0;{}).
 *
 * Tick handler results follow REPL echo rules: a non-nul result is
 * printed via kprint (suppressed under -q). Return nul from f for a
 * silent tick.
 *
 * Semantics:
 *   - Ticks fire only at the top-level repl prompt (DEPTH==1). Sub-
 *     repls (entered via \e 1 debug, or recursive repl()), script
 *     load(), and ipc sync waits do not fire ticks.
 *   - Re-entrancy guard: a tick is skipped if the previous handler
 *     hasn't returned yet.
 *   - Missed ticks are dropped, not catched up. The next fire is
 *     scheduled `interval` seconds from when the previous handler
 *     returned.
 *   - Cleared on fork: child inherits no timer schedule.
 *   - Errors honor \e: under \e 0 they're printed and continue;
 *     under \e 1 they drop into a debug sub-repl.
 */

#include "k.h"

/* Module init/teardown. tmr_init sets state to "disabled, no fn".
 * tmr_shutdown frees the captured fn ref. Called from main.c /
 * exit__ alongside ipc_init / ipc_shutdown. */
void tmr_init(void);
void tmr_shutdown(void);

/* Drop all timer state in an ipc fork child. The parent's schedule
 * shouldn't keep the child alive past its work; clearing here keeps
 * the child a one-shot worker. */
void tmr_fork_clear(void);

/* Returns the number of ms until the next tick is due, or -1 to mean
 * "wait indefinitely" (timer disabled, not at top-level prompt, or
 * a tick is already running). Suitable for use as the timeout arg
 * to poll() / WaitForMultipleObjects. Negative deltas (overdue) are
 * clamped to 0 so the poll wakes immediately. */
int  tmr_timeout_ms(void);

/* If a tick is due and we're at the top-level prompt and not already
 * inside a tick, invoke the user's fn (as fn[nul]) and reschedule
 * the next fire. No-op otherwise. */
void tmr_maybe_fire(void);

/* The builtin. See header comment for surface. */
K    timer_(K x);

#endif /* TMR_H */
