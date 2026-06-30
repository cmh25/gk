/* WASM/browser entry points (compiled only for the Emscripten target).
 *
 * The browser REPL has no stdin loop: instead of main()/repl(), JS calls gk_init()
 * once, then drives evaluation per input. We route every eval through load() -- the
 * exact tested script path (multi-statement, comments, prints each top-level
 * expression) -- by writing the input to a MEMFS file and loading it. This reuses
 * 100% of the file-eval semantics, identical to `./gk script.k`.
 *
 * Output needs no capture code: kprint() -> fwrite/fputs to stdout, which Emscripten
 * routes to Module.print automatically. Errors render through kprint as well.
 */
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <stdio.h>
#include <stdlib.h>   /* exit (\\ / exit[x] -> session reset via worker.js) */
#include <string.h>
#include "k.h"       /* K, kinit, kprint, E(), null */
#include "repl.h"    /* load */
#include "scope.h"   /* scope_init */
#include "fn.h"      /* fninit */
#include "p.h"       /* pinit */
#include "ipc.h"     /* ipc_init_ns (pure K state; socket inits are skipped) */
#include "h.h"       /* help (lone "\" -> top-level menu) */

/* Mirror main()'s init sequence minus the socket/stdin/timer setup that has no
 * meaning in a tab. ipc_init_ns only mutates K state (the .m messaging handlers),
 * so it's safe and kept for parity. */
EMSCRIPTEN_KEEPALIVE void gk_init(void) {
  static char *noargs[1] = {0};
  kinit();
  scope_init(noargs, 0);
  fninit();
  pinit();
  ipc_init_ns();
  wasm = 1;   /* no interactive error subconsole in a browser tab (no stdin) */
  /* The startup banner (same as main.c). To stdout so it shows in the REPL output. */
  printf("gk-v3.0.0 Copyright (c) 2023-2026 Charles Hall\n\n");
}

/* An eval error opens gk's interactive `>` debug subconsole IN PLACE (p.c:312-317:
 * ++ecount; repl()), which reads stdin. A browser tab has no stdin, so that read
 * hits EOF and latches EXIT (and leaves ecount/DEPTH elevated), silencing every
 * later eval. We don't want the interactive debugger in a one-shot REPL anyway, so
 * reset these latches to a clean state before each run. The error itself is
 * still printed (printerror fires before the subconsole opens). */
static void reset_latches(void) {
  if(EXIT) { _k(EXIT); EXIT = 0; }
  ecount = 0;
  DEPTH = 0;
  SIGNAL = 0;
  RETURN = 0;
}

/* Evaluate gk source. Writes it to a MEMFS file and runs load() exactly as the CLI
 * does for a script arg: results print themselves; only an error K is printed here
 * (matching main.c:118-119). */
EMSCRIPTEN_KEEPALIVE void gk_eval(const char *src) {
  /* Leading "\" forms that load() would otherwise swallow: a lone "\" is the top-level
   * help menu, and "\\" means exit (== exit[0]). A browser tab can't really quit, so
   * exit boots a fresh session -- exit() unwinds the wasm runtime and worker.js catches
   * it and restarts us. Topic forms (\0, \+, ...) pass through to eval as usual. */
  { const char *p = src; while(*p==' '||*p=='\t'||*p=='\n'||*p=='\r') ++p;
    if(*p=='\\') {
      const char *q=p+1; while(*q==' '||*q=='\t'||*q=='\n'||*q=='\r') ++q;
      if(!*q) { help(0); return; }                                 /* "\"  -> help        */
      if(*q=='\\') { const char *r=q+1; while(*r==' '||*r=='\t'||*r=='\n'||*r=='\r') ++r;
        if(!*r) exit(0); }                                         /* "\\" -> exit (reset)*/
    } }
  /* Scratch file kept under /tmp so it doesn't clutter the MEMFS root that scripts
   * see via !"." . The filename never surfaces in errors -- the WASM build blanks
   * pfile (repl.c), so error context reads like the interactive REPL. */
  const char *path = "/tmp/gk_in";
  FILE *f = fopen(path, "wb");
  if(!f) { fprintf(stderr, "gk_eval: cannot open %s\n", path); return; }
  fwrite(src, 1, strlen(src), f);
  fputc('\n', f);   /* load() reads line-terminated statements */
  fclose(f);
  reset_latches();
  load((char*)path, 1);   /* results + errors print themselves (load/printerror) */
  /* exit[x] / exit x -> boot a fresh session. EXIT is also set by repl_'s wasm
   * error-subconsole unwind (which leaves ecount>0), so require ecount==0 to avoid
   * treating an ordinary error (e.g. a value error) as an exit request. */
  if(EXIT && !ecount) exit(0);
}

/* Run a preloaded demo file directly by MEMFS path (e.g. "demos/mortgage.k"). */
EMSCRIPTEN_KEEPALIVE void gk_run_file(const char *path) {
  reset_latches();
  load((char*)path, 1);
}
#endif
