# timer

gk has a built-in recurring timer. You set an interval and a function;
the function fires at the top-level REPL prompt every `interval`
seconds until you turn it off.

## quick start

```
  timer(1;{`0:"tick\n"})    / fire every 1s
  ...
  timer(0;{x})              / interval 0 disables
```

## surface

The timer is a single monadic builtin, `timer`. It has two forms:

```
timer (t;f)        SET. t is seconds (int or float, 0 disables);
                   f is a 1-valence fn called as f[nul] each tick,
                   OR a sym `g referring to a global g (resolved at
                   every tick). Returns nul.
timer x            QUERY (anything that isn't a 2-element value:
                   timer[], timer nul, timer`, timer 1, ...).
                   Returns the current (t;f). Before any set,
                   returns (0.0;{}).
```

Examples:

```
  timer[]
(0.0;{})
  timer(0.5;{!10})
  timer[]
(0.5;{!10})
  timer 0                   / 0 by itself is a query, not a set
(0.5;{!10})
  timer(0;{})               / disable
  timer[]
(0.0;{})
```

## set-form validation

A 2-element argument is *always* interpreted as a set attempt, even
when k normalizes it to a typed vector (e.g. `(1;5)` is the int
vector `1 5`, not a mixed list). Bad shapes raise an error rather
than silently falling back to the query path:

```
  timer("a";5)              / interval not numeric
type error
  timer(-1;{x})             / interval negative
domain error
  timer(1;5)                / fn not callable
type error
  timer(1;`undefined)       / sym doesn't resolve
value error
  timer(1;{x+y})            / valence != 1
valence error
  timer 1 2                 / same: 2-element int vector
type error
```

Valence is checked strictly: the fn must have valence exactly 1.
`{x+y}` would just project on `nul` rather than fire, which is never
what you want.

## pass by reference (sym)

The fn can be a sym referring to a global function, mirroring
``` `f . ` ``` (apply by reference). The sym is resolved at *every*
tick, so redefinitions are picked up live:

```
  f:{!3}
  timer(0.5;`f)             / fires {!3}
0 1 2
0 1 2
  f:{!7}                    / next tick fires the new f
0 1 2 3 4 5 6
0 1 2 3 4 5 6
  timer[]
(0.5;`f)                    / query returns the sym, not the fn
```

If the sym stops resolving (deleted, rebound to a non-callable, or
rebound to a non-1-valence fn), the corresponding error prints at
the next tick and the timer stays armed -- restoring the global
resumes ticking on the following interval.

## tick output

Tick handler results follow the REPL's echo rules: a non-nul result
is printed via `kprint`, suppressed under `-q`. Return `nul`
explicitly when you want a silent tick:

```
  timer(1;{!5})
0 1 2 3 4
0 1 2 3 4
...

  timer(1;{!5;nul})         / compute, then return nul
  ...                       / no output
```

## semantics

- **top-level only.** Ticks fire only at the top-level REPL
  prompt. Sub-REPLs (entered via `\` after an error under `\e 1`,
  or any other recursive REPL), script `load()`, and the wait
  inside an IPC sync call (`h 4:msg`) all suppress firing.
- **re-entrancy guard.** A tick is skipped if the previous handler
  hasn't returned yet. Long-running handlers won't stack.
- **missed ticks are dropped.** The next fire is scheduled
  `interval` seconds from when the previous handler *returned*,
  not from when it was supposed to fire. There is no catch-up.
- **cleared on fork.** An IPC fork child inherits no timer
  schedule. The parent's timer continues unaffected.
- **persists across script load.** A script can install a timer
  and return; the timer keeps firing once the REPL is reached.
  Subsequent `load()` calls do not clear it.
- **errors honor `\e`.** Under `\e 0` the error is printed and the
  timer keeps running. Under `\e 1` the handler error drops you
  into a debug sub-REPL, with the timer suspended until you exit
  it with `\`.

## why a builtin and not `\t`?

k3 uses `\t expr` for both timing an expression and setting a
recurring timer (the form is overloaded by argument type). gk
splits these: `\t expr` is the one-shot timer, and `timer (t;f)` is
the recurring one. A builtin keeps the recurring form
introspectable (`timer[]` returns the current state) and lets it
participate in normal k expressions, including passing the
`(t;f)` pair around as data.
