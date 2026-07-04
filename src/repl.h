#ifndef REPL_H
#define REPL_H

#include "k.h"

extern int DEPTH,ecount,REPL,LOADLINE;
/* Set by the WASM/browser build (which has no stdin). When set, the interactive
 * error subconsole (repl_) returns immediately instead of printing a prompt and
 * reading stdin -- errors still print (printerror runs first), they just don't drop
 * into the `>` debugger; and load() omits the "in FILE:N" / "load:" context lines so
 * errors read like the interactive REPL. */
extern int wasm;
K load(char *fn, int load);
K repl(void);
/* Suppress the help index that a bare abort fires at the top-level REPL.
 * Called by ipc.c when a remote 'abort arrives on a sync handle: the abort
 * was typed at the SERVER's debug prompt, not here. */
void repl_suppress_help(void);

#endif /* REPL_H */
