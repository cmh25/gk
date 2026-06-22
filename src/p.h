#ifndef P_H
#define P_H

#include "k.h"

#define T000   0 /* $a */
#define T001   1 /* s */
#define T002   2 /* e */
#define T003   3 /* se */
#define T004   4 /* o */
#define T005   5 /* ez */
#define T006   6 /* klist */
#define T007   7 /* plist */
#define T008   8 /* pz */
#define T009   9 /* elist */
#define T010  10 /* elistz */
#define T011  11 /* ee */
#define T012  12 /* ';' */
#define T013  13 /* '\n' */
#define T014  14 /* N */
#define T015  15 /* V */
#define T016  16 /* '(' */
#define T017  17 /* ')' */
#define T018  18 /* '[' */
#define T019  19 /* ']' */
#define T020  20 /* $e */

typedef struct pn {
  int t;         /* type */
  K v;           /* value verb */
  K n;           /* value noun */
  struct pn **a; /* children */
  int m;         /* children count */
  int f;         /* free? */
  int i;         /* index */
  int line;
  char *file;
  int gline;     /* global line in lambda */
  int lv;        /* 1 if this t==19 is in lvalue context (assignment LHS) */
  int call_n;    /* cached can_inline_call result for t==19 nodes; -2 = not
                    yet computed, -1 = not inlinable, >=0 = arity */
} pn;

typedef struct {
  char *p;       /* buffer */
  int *S;        /* state */
  int *R;        /* rules */
  pn **V;        /* values */
  int si,ri,vi;  /* index */
  int *t,ti,tc;  /* tokens,index,count */
  pn **v;        /* token values */
  int lt;        /* last token */
  int ll;        /* last was lambda */
  int Sm,Rm,Vm;  /* max for S R V */
  int tm,vm;     /* max for t v */
  K values;      /* token values */
  K index;       /* token index in line of q */
  K line;        /* line of q */
  int valuesmax;
  int q;
  pn **all_nodes;  /* array of all allocated parse nodes */
  int node_count;  /* number of nodes in the array */
  int node_max;    /* maximum capacity of nodes array */
  char *file;      /* filename */
  K locals;
  int overflow;    /* set by mark_lvalues when a statement's parse tree exceeds
                      maxr depth; pgparse turns it into a "stack" error */
} pgs;

extern int quiet,RETURN;
extern int STOP,EFLAG,SIGNAL;
extern K EXIT;
extern int opencode;
extern char *pfile;

#ifdef FUZZING
/* Iteration budget for the unbounded loop constructs (converge f/x, f\x and
 * while-test f/x, f\x).  gk legitimately allows non-terminating programs
 * (e.g. sin/1 converges only in the float limit), so under fuzzing we cap the
 * total converge/while iterations per top-level eval and raise 'limit.  This
 * turns language-legal non-termination into a fast clean error instead of an
 * AFL hang, and never touches the shipping binary (no FUZZING -> no counter). */
extern long gk_budget;
#define GK_BUDGET 1000000L
/* Companion allocation budget: the iteration cap above only counts loop turns,
 * so it can't catch a SINGLE primitive that allocates/copies a huge structure
 * (e.g. 88888888#x, big take/drop/arith) -- those march toward the OS memory
 * cap for seconds, which AFL flags as a hang.  Cap total bytes xmalloc'd per
 * top-level eval; on overrun take xmalloc's existing OOM path (printf+exit(1)),
 * just reached in ms instead of after seconds of thrashing.  FUZZING-only. */
extern long gk_alloc_budget;
#define GK_ALLOC_BUDGET (64L*1024*1024)
#endif
extern int gline,glinei,gline0,gline0i,fileline;
extern char *glinep,*gline0p;
extern K params[];
extern int paramsi;

pgs* pgnew(void);
void pgfree(pgs *s);
pn* pnnew(pgs *s, int t, K v, K n, int m, int f);
pn* pnnewi(pgs *s, int t, K v, K n, int m, int f, int i, int line);
void pnfree(pn *n);
K pgparse(char *q, int load, K locals);
K pgreduce_(K x, int *quiet);
K pgreduce(K x, int p);
K prnew(int n);
void prfree(K x);
void pinit(void);
void pexit(void);

#endif /* P_H */
