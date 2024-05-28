#ifndef SCOPE_H
#define SCOPE_H

#include "dict.h"
#include "k.h"

#define  SG(x) do{if((x)->t==99){(x)=scope_get(cs,(x)->v);}}while(0)
#define SG2(x) do{if((x)->t==99){(x)=scope_get(cs,(x)->v);}else kref((x));}while(0)
#define  SR(x) do{K*x_;if((x)->t==99){x_=(x);(x)=scope_get(cs,(x)->v);kfree(x_);}}while(0)

typedef struct scope {
  struct scope *p; /* parent */
  dict *d;         /* function local vars */
  char *k;         /* ktree path */
} scope;

extern scope *ks,*gs,*cs;
extern dict *ktree,*C,*Z;

scope* scope_new(scope *p);
scope* scope_newk(scope *p, char *k);
void scope_free(scope *s);
K* scope_get(scope *s, char *n);
K* scope_set(scope *s, char *n, K *v);
scope* scope_cp(scope *s);
scope* scope_find(dict *d);

#endif /* SCOPE_H */
