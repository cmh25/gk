#ifndef NODE_H
#define NODE_H

#include "k.h"

extern int E,A,S;

typedef struct node {
  struct node **a;  /* children */
  unsigned int v;   /* valence */
  K *k;             /* value */
  char q;           /* quiet */
  char t;           /* type */
  int line;         /* source line */
  int linei;        /* source line index */
} node;

node* node_new(node **a, int v, K *s);
node* node_newli(node **a, int v, K *s, int line, int linei);
void node_free(node *n);
K* node_reduce(node *n, int z);
node* node_append(node *n0, node *n1);
node* node_prepend(node *n0, node *n1);

#endif /* NODE_H */
