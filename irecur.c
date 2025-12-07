#include "irecur.h"

#define IS0(x) (!s(x)&&!T(x))
K irecur1(K(*ff)(K), K x) {
  typedef struct { K r,x; size_t i; } sf;
  K r;
  i32 sm=32,sp=0;
  sf *stack=xmalloc(sizeof(sf)*sm);
  stack[sp++]=(sf){tn(0,nx),x,0};
  while(sp) {
    sf *f=&stack[sp-1];
    if(f->i==n(f->x)) {
      r=f->r;
      --sp;
      if(sp) ((K*)px(stack[sp-1].r))[stack[sp-1].i++]=knorm(r);
    }
    else {
      K x_=((K*)px(f->x))[f->i];
      if(IS0(x_)) {
        if(sp==sm) stack=xrealloc(stack,sizeof(sf)*(sm*=2));
        stack[sp++]=(sf){tn(0,n(x_)),x_,0};
      }
      else {
        K r_=ff(x_);
        if(E(r_)) { while(sp--) _k(stack[sp].r); xfree(stack); return r_; }
        ((K*)px(f->r))[f->i++]=r_;
      }
    }
  }
  xfree(stack);
  return knorm(r);
}
K irecur2(K(*ff)(K,K), K a, K x) {
  typedef struct { K r,a,x; size_t i; } sf;
  K r;
  i32 sm=32,sp=0;
  if(!ta&&!tx&&!s(a)&&!s(x)&&na!=nx) return KERR_LENGTH;
  sf *stack=xmalloc(sizeof(sf)*sm);
  stack[sp++]=(sf){tn(0,IS0(a)?na:nx),a,x,0};
  while(sp) {
    sf *f=&stack[sp-1];
    i32 a0=IS0(f->a), x0=IS0(f->x);
    size_t n=x0?n(f->x):a0?n(f->a):1;
    if(f->i==n) {
      r=f->r;
      --sp;
      if(sp) ((K*)px(stack[sp-1].r))[stack[sp-1].i++]=knorm(r);
    }
    else {
      K a_=a0?((K*)px(f->a))[f->i]:a;
      K x_=x0?((K*)px(f->x))[f->i]:x;
      i32 a0_=IS0(a_), x0_=IS0(x_);
      if(a0_||x0_) {
        if(a0_&&x0_&&(n(a_)!=n(x_))) { while(sp--) _k(stack[sp].r); xfree(stack); return KERR_LENGTH; }
        if(sp==sm) stack=xrealloc(stack,sizeof(sf)*(sm*=2));
        n=a0_?n(a_):n(x_);
        stack[sp++]=(sf){tn(0,n),a_,x_,0};
      }
      else {
        K r_=ff(a_,x_);
        if(E(r_)) { while(sp--) _k(stack[sp].r); xfree(stack); return r_; }
        ((K*)px(f->r))[f->i++]=r_;
      }
    }
  }
  xfree(stack);
  return knorm(r);
}
