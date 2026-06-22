#ifndef REPL_H
#define REPL_H

#include "k.h"

extern int DEPTH,ecount,REPL,LOADLINE;
K load(char *fn, int load);
K repl(void);

#endif /* REPL_H */
