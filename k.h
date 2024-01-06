#ifndef K_H
#define K_H

#include <stddef.h>
#include <math.h>
#include <stdint.h>
#include <limits.h>

extern int precision;

/* 1=i, 2=f, 3=c, 4=s, 5=d, 6=n, 7=f, 0=kv, -1=iv, -2=fv, -3=cv, -4=sv */
/* 16=inull */
/* 20=qse, 21=se */
/* 99=variable, 98=kerror */
/* 10=plistlist, 11=plist, 12=elist, 13=rhs */
/* 14=plist+av, 15=plist+o, 17=vo, 27=ov, 37=lambda, 47=av, 57=+: ,67=fc, 77 87=projection */
typedef struct {
  char t;          /* type */
  unsigned int c;  /* count */
  union {void *v;int i;double f;}; /* data */
  int r;           /* reference count */
} K;

typedef struct {
  char *s;
  int i,j;
} ERR;

extern K *D,*one,*zero,*null,*inull;

/* valid size for vector */
#define VSIZE(x) do{if((x)==INT_MAX||(x)==INT_MIN||(x)==INT_MIN+1) return kerror("wsfull");}while(0);
#define VSIZE2(x) do{if((x)>=INT_MAX) return (char*)-1;}while(0);

/* creators */
#define k1(v)  knew(1,0,0,(v),0,0)
#define k2(v)  knew(2,0,0,0,(v),0)
#define k3(v)  knew(3,0,0,(v),0,0)
#define k4(v)  knew(4,0,(v),0,0,0)
#define k5(v)  knew(5,0,(v),0,0,0)
#define k7(v)  knew(7,0,(v),0,0,0)
#define kv0(c) knew(0,(c),0,0,0,0)
#define kv1(c) knew(-1,(c),0,0,0,0)
#define kv2(c) knew(-2,(c),0,0,0,0)
#define kv3(c) knew(-3,(c),0,0,0,0)
#define kv4(c) knew(-4,(c),0,0,0,0)
/* type-derivable accessors */
#define a1 a->i
#define a2 a->f
#define a3 a->i
#define a4 ((char*)a->v)
#define a5 ((dict*)a->v)
#define a7 ((fn*)(a)->v)
#define b1 b->i
#define b2 b->f
#define b3 b->i
#define b4 ((char*)b->v)
#define b5 ((dict*)b->v)
#define b7 ((fn*)(b)->v)
#define v0(x) ((K**)((x)->v))
#define v1(x) ((int*)((x)->v))
#define v2(x) ((double*)((x)->v))
#define v3(x) ((char*)((x)->v))
#define v4(x) ((char**)((x)->v))
/* count */
#define ac a->c
#define bc b->c
#define cc c->c
#define dc d->c
#define rc r->c
/* type */
#define at a->t
#define bt b->t
#define ct c->t
#define dt d->t
#define rt r->t

#define  DO(n,x) {int i,n_=(n);for(i=0;i<n_;i++){x;}}
#define DO2(n,x) {int j,n_=(n);for(j=0;j<n_;j++){x;}}
#define DO3(n,x) {int k,n_=(n);for(k=0;k<n_;k++){x;}}

#define EC(x) if((x)->t==98)return (x);

/* integer to double, 0I->0i, 0N->0n, -0I->-0i */
#define I2F(a) \
  ((a)==INT_MAX ? INFINITY \
: (((a)==INT_MIN ? NAN \
: (((a)==INT_MIN+1) ? -INFINITY \
: (double)(a)))))

/* floating point comparision */
#define CMPFF(A,B) \
  (isnan(A) && !isnan(B) ? -1 \
 : isnan(B) && !isnan(A) ?  1 \
 : isnan(A) && isnan(B)  ?  0 \
 : (A)<(B) ? -1 \
 : (A)>(B) ?  1 \
 : 0)
/* floating point comparison with tolerance */
#define EPSILON 0.0000000000001
#define CMPFFE(A,B) \
  ((A)-(B) > EPSILON*fabs((A)+(B)) ?  1 \
 : (B)-(A) > EPSILON*fabs((A)+(B)) ? -1 \
 : 0)
#ifndef __linux__
#define CMPFFT(A,B) \
  (isnan(A) && !isnan(B) ? -1 \
 : isnan(B) && !isnan(A) ?  1 \
 : isnan(A) && isnan(B)  ?  0 \
 : isinf(A)&&isinf(B)&&A<0.0&&B>0.0 ? -1 \
 : isinf(B)&&isinf(A)&&B<0.0&&A>0.0 ?  1 \
 : isinf(A)&&!isinf(B)&&A<0.0 ? -1 \
 : isinf(B)&&!isinf(A)&&B<0.0 ?  1 \
 : isinf(A)&&!isinf(B)&&A>0.0 ?  1 \
 : isinf(B)&&!isinf(A)&&B>0.0 ? -1 \
 : CMPFFE(A,B))
#else /* win32 and osx */
#define CMPFFT(A,B) \
  (isnan(A) && !isnan(B) ? -1 \
 : isnan(B) && !isnan(A) ?  1 \
 : isnan(A) &&  isnan(B) ?  0 \
 : isinf(A) <   isinf(B) ? -1 \
 : isinf(B) <   isinf(A) ?  1 \
 : CMPFFE(A,B))
#endif

void kinit();
K* knew(char t, unsigned int c, void *v, int i, double f, int r);
void kfree(K *k);
void kdump(int l);
static inline K* kref(K *k) { if(k->r>=0) k->r++; else if(k->r<=-2) k->r--; return k; }
void kprint(K *p, char *s, unsigned int plevel, int nl);
char* kprint_(K *p, char *s, unsigned int plevel, int nl);
char* kprint5(K *p, char *s, unsigned int plevel, int nl);
K* knorm(K *s);
K* kerror(char *msg);
K* kmix(K *s);
K* kcp(K *s);
int vname(char *s, int len);
int kcmp(K *a, K *b);
uint64_t khash(K *a);
int kreserved(char *s);

#endif /* K_H */
