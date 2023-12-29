#ifndef P_H
#define P_H

#include "node.h"

#define T000   0 /* $a */
#define T001   1 /* ss */
#define T002   2 /* sz */
#define T003   3 /* s */
#define T004   4 /* se */
#define T005   5 /* e */
#define T006   6 /* ez */
#define T007   7 /* o */
#define T008   8 /* av */
#define T009   9 /* plist */
#define T010  10 /* pz */
#define T011  11 /* pz2 */
#define T012  12 /* klist */
#define T013  13 /* elist */
#define T014  14 /* elistz */
#define T015  15 /* ee */
#define T016  16 /* ';' */
#define T017  17 /* '\n' */
#define T018  18 /* N */
#define T019  19 /* V */
#define T020  20 /* AV */
#define T021  21 /* '[' */
#define T022  22 /* ']' */
#define T023  23 /* '(' */
#define T024  24 /* ')' */
#define T025  25 /* $e */

extern int quiet,quiet2,btime;
extern char **lines;
extern int linei,linem,*linef,*linefn;
extern char **fnames;
extern int fnamesi,fnamesm;

typedef struct {
  char *p;       /* buffer */
  int *S;        /* state */
  int *R;        /* rules */
  node **V;      /* values */
  int si,ri,vi;  /* index */
  int *t,ti,tc;  /* tokens, index, count */
  node **v;      /* token values */
  int lt;        /* last token */
  int ffli;      /* function first line index */
  int fni;       /* filename index */
  int Sm,Rm,Vm;  /* max for S R V */
  int tm,vm;     /* max for t v */
} pgs;

pgs* pgnew();
void pgfree(pgs *s);
node* pgparse(pgs *s);

#endif /* P_H */
