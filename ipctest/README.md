# ipctest — manual IPC probes

Two-terminal recipe. Ports used: **15555** (inline) and **15556** (forking).

    # terminal A (server)
    rlwrap ./gk ipctest/srv01.k

    # terminal B (client)
    rlwrap ./gk ipctest/cli01.k

Client scripts end with `\\` so they auto-exit after printing results.
Server scripts fall into the REPL so you can poke state between client runs
(check counters, redefine handlers, etc.).

| pair      | what it exercises                                          |
|-----------|------------------------------------------------------------|
| 01 hello  | basic round-trip via default `.` handler                   |
| 02 async  | async msgs fire server-side side effects                   |
| 03 wrap   | user-defined `.m.g` wrapping default `.`                   |
| 04 close  | `.m.c` fires on peer disconnect                            |
| 05 err    | server raises -> client sees remote error                  |
| 06 fork   | `\m f`: each fresh conn → fresh child → isolated state     |
| 07 dual   | `-i` + `-f` together: shared vs isolated side-by-side      |
| 08 big    | 100k-element vector round-trip                             |
| 09 many   | N parallel clients → shared counter                        |
| 10 loop   | server connects to itself                                  |
| 11 oneshot| `(h;p) N:msg` implicit-open form; dedup and explicit close |
| 12 callback| fork child sync-reports back to parent over inline listener |
| 13 stress | 100 rapid-fire callbacks on one conn; in_dispatch queueing |
| 14 chain  | handler does 3 sync-backs per msg; 60 chained syncs total  |

## conventions
- Server defines whatever state/handlers, then `\m i 15555` and/or `\m f 15556`.
- Client opens, does its steps, closes, `\\`.
- When in doubt about a result, sync it back: `h 4:"varname"`.
- To wait for a burst of asyncs to drain, send a sync on the *same* conn
  right after: messages on one conn are dispatched in order, so the sync's
  response won't come back until every prior async has finished. Cheaper
  and more reliable than `sleep`. See pairs 12, 13, 14.

## default handler
`.m.g` defaults to `{. x}`. Monadic `.` dispatches on `x`:
- `x` a string → execute (`."2+2"` → `4`)
- `x` a `(fn;args)` list → apply (`.(`f;1 2 3)` → `f[1;2;3]`)

So out of the box:

    w 4:"!10"         / -> 0 1 2 3 4 5 6 7 8 9
    w 4:(`f;1 2 3)    / -> f[1;2;3]

## running every pair
`ipctest/run.py` spins up each srvNN, waits for the listener to
bind, runs cliNN against it, and cleans up. Per-run stdout is saved
under the system tempdir as `srvNN.out` / `cliNN.out` for inspection
on failure. Stdlib only, runs on POSIX and Windows.

    python3 ipctest/run.py                 # uses ./gk (or gk.exe on win)
    GK=/path/to/gk python3 ipctest/run.py  # override binary

To run the whole suite under valgrind, point GK at a wrapper:

    cat >/tmp/vgk <<'EOF'
    #!/bin/sh
    exec valgrind --leak-check=full --show-leak-kinds=all \
         --child-silent-after-fork=yes --error-exitcode=1 \
         ./gk "$@"
    EOF
    chmod +x /tmp/vgk
    GK=/tmp/vgk python3 ipctest/run.py

`--child-silent-after-fork=yes` keeps the forking pairs (06, 07, 12-14)
from drowning the log in per-child noise.

## sweeping just the server scripts
Each srv script falls into the REPL, so kill it (Ctrl-D, or `pkill gk`)
between iterations to let the loop advance. Useful when you want to
poke at server state interactively rather than auto-drive with a client:

    for i in ipctest/srv*; do
      echo $i
      valgrind --leak-check=full --show-leak-kinds=all \
               --child-silent-after-fork=yes ./gk $i
    done

Windows (cmd.exe). No valgrind; use Dr. Memory or an ASan build for
leak checking. The fork-mode pairs (srv06, srv07, srv12-14) will just
report `fork: not supported on windows`.

    for %i in (ipctest\srv*) do @echo %i & gk.exe %i

## gotchas
- `log` is the `log()` builtin — don't use it as a variable name. Use e.g. `hist`.
- `.[f;x;:]` / `@[f;x;:]` trap-eval: the third arg must be the literal
  primitive `:`. A lambda like `{x}` gives `rank error` because gk only
  recognizes the bare-`:` form (see `k.c:1823`). Result is `(0;val)` on
  success, `(1;"msg")` on error.
- Semicolon-compound statements like `h 4:"x"; 3:h` only print the last
  value — split them onto separate lines if you want to see each result.
