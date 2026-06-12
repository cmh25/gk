# linktest

End-to-end test for `f 2:(e;t)` — link object code (dynamic FFI).

- `lib.c` — a tiny shared-object source that `#include "gk.h"` (the public FFI
  header) and exercises the ABI: immediate-atom returns across valences 0–3
  (`t_answer`/`t_neg`/`t_add`/`t_sum3`) plus heap returns built by calling back
  into gk's exported allocators (`t_half`→f64, `t_greet`→char vector,
  `t_iota`→i32 vector via `gk_mkiv`/`gk_iv`).
- `test.k` — links those functions and exercises every apply form (bracket,
  `.`, juxtaposition), heap returns, value semantics, projection, adverbs, and
  the error paths. Reads the compiled library's path from the `GKLINKLIB`
  environment variable.
- `run.py` — copies `gk.h` + `lib.c` into a temp dir, compiles the shared
  object there, sets `GKLINKLIB`, runs `test.k`, and reports pass/fail. Part of
  `make test` / `make.bat test`. (gk must be built with the `gk_*` ABI exported,
  which `make`/`make.bat` do.)

Unlike the other suites this one needs a C compiler at run time:

- POSIX: `cc` / `gcc` / `clang` (`-shared`, `-fPIC`).
- Windows: MSVC `cl` (or `clang-cl`), using `/LD` to build the `.dll` —
  available once `vcvars64.bat` has been run, which `make.bat` already
  requires. No mingw/clang needed. `lib.c` marks its functions
  `__declspec(dllexport)` so `GetProcAddress` can find them.

If no compiler is found (`$CC` unset and none on PATH) the test **skips**
rather than failing, so `make test` stays green on toolchain-less machines.

```
python3 linktest/run.py          # uses ./gk and the first compiler found
GK=/path/to/gk python3 linktest/run.py
CC=clang python3 linktest/run.py
```
