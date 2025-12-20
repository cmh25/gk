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
} pgs;

extern int quiet,RETURN;
extern int STOP,EFLAG,SIGNAL;
extern K EXIT;
extern int opencode;
extern char *pfile;
extern int gline,glinei,gline0,gline0i,fileline;
extern char *glinep,*gline0p;

pgs* pgnew(void);
void pgfree(pgs *s);
pn* pnnew(pgs *s, int t, K v, K n, int m, int f);
pn* pnnewi(pgs *s, int t, K v, K n, int m, int f, int i, int line);
void pnfree(pn *n);
pn* pnappend(pn *n0, pn *n1);
K pgparse(char *q, int load, K locals);
K pgreduce_(K x, int *quiet);
K pgreduce(K x, int p);
K prnew(int n);
void prfree(K x);
void pinit(void);
void pexit(void);

#endif /* P_H */
