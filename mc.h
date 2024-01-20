#ifndef MC_H
#define MC_H

#include "k.h"

/* math function constructors
 *  F: function
 * F1: function to compute type 1
 * F2: function to compute type 2
 * T1: result type for type 1
 * T2: result type for type 2
 * A1: access macro for result of type -1
 * A2: access macro for result of type -2
*/

#define MC1D(F,F1,F2,T1,T2,A1,A2) \
K* F(K *a) { \
  K *r=0; \
  switch(at) { \
  case  1: r=k##T1(F1(I2F(a1))); break; \
  case  2: r=k##T2(F2(a2)); break; \
  case  0: r=kv0(ac); DO(ac, v0(r)[i]=F((v0(a)[i])); EC(v0(r)[i])) break; \
  case -1: r=kv##T1(ac); DO(ac, A1(r)[i]=F1(I2F(v1(a)[i]))) break; \
  case -2: r=kv##T2(ac); DO(ac, A2(r)[i]=F2(v2(a)[i])) break; \
  default: return kerror("type"); \
  } \
  return r->t ? r : knorm(r); \
}

#define MC1(F,F1,F2,T1,T2,A1,A2) \
K* F(K *a) { \
  K *r=0; \
  switch(at) { \
  case  1: r=k##T1(F1(a1)); break; \
  case  2: r=k##T2(F2(a2)); break; \
  case  0: r=kv0(ac); DO(ac, v0(r)[i]=F((v0(a)[i])); EC(v0(r)[i])) break; \
  case -1: r=kv##T1(ac); DO(ac, A1(r)[i]=F1(v1(a)[i])) break; \
  case -2: r=kv##T2(ac); DO(ac, A2(r)[i]=F2(v2(a)[i])) break; \
  default: return kerror("type"); \
  } \
  return r->t ? r : knorm(r); \
}

#define MC2D(F,F1,F2,T1,T2,A1,A2) \
K* F(K *a, K *b) { \
  K *r=0; \
  if(at <= 0 && bt <= 0 && ac != bc) return kerror("length"); \
  if(at==0 || bt==0) r=each(F,a,b); \
  else { \
  switch(at) { \
  case  1: \
    switch(bt) { \
    case  1: r=k##T1(F1(I2F(a1),I2F(b1))); break; \
    case  2: r=k##T2(F2(I2F(a1),b2)); break; \
    case -1: r=kv##T1(bc); DO(bc, A1(r)[i]=F1(I2F(a1),I2F(v1(b)[i]))) break; \
    case -2: r=kv##T2(bc); DO(bc, A2(r)[i]=F2(I2F(a1),v2(b)[i])) break; \
    } break; \
  case  2: \
    switch(bt) { \
    case  1: r=k##T1(F1(a2,I2F(b1))); break; \
    case  2: r=k##T2(F2(a2,b2)); break; \
    case -1: r=kv##T1(bc); DO(bc, A1(r)[i]=F1(a2,I2F(v1(b)[i]))) break; \
    case -2: r=kv##T2(bc); DO(bc, A2(r)[i]=F2(a2,v2(b)[i])) break; \
    } break; \
  case -1: \
    switch(bt) { \
    case  1: r=kv##T1(ac); DO(ac, A1(r)[i]=F1(I2F(v1(a)[i]),I2F(b1))) break; \
    case  2: r=kv##T2(ac); DO(ac, A2(r)[i]=F2(I2F(v1(a)[i]),b2)) break; \
    case -1: r=kv##T1(ac); DO(ac, A1(r)[i]=F1(I2F(v1(a)[i]),I2F(v1(b)[i]))) break; \
    case -2: r=kv##T2(ac); DO(ac, A2(r)[i]=F2(I2F(v1(a)[i]),v2(b)[i])) break; \
    } break; \
  case -2: \
    switch(bt) { \
    case  1: r=kv##T1(ac); DO(ac, A1(r)[i]=F1(v2(a)[i],I2F(b1))) break; \
    case  2: r=kv##T2(ac); DO(ac, A2(r)[i]=F2(v2(a)[i],b2)) break; \
    case -1: r=kv##T1(ac); DO(ac, A1(r)[i]=F1(v2(a)[i],I2F(v1(b)[i]))) break; \
    case -2: r=kv##T2(ac); DO(ac, A2(r)[i]=F2(v2(a)[i],v2(b)[i])) break; \
    } break; \
  default: return kerror("type"); \
  } \
  } \
  return r->t ? r : knorm(r); \
}

/* integer ops */
#define MC2IO(F,O,N) \
K* F(K *a, K *b) { \
  K *r=0; \
  if((N) && !b1) return kerror("nonce"); \
  if(at <= 0 && bt <= 0 && ac != bc) return kerror("length"); \
  if(at==0 || bt==0) r=each(F,a,b); \
  else { \
  switch(at) { \
  case  1: \
    switch(bt) { \
    case  1: r=k1(a1 O b1); break; \
    case -1: r=kv1(bc); DO(bc, v1(r)[i] = a1 O v1(b)[i]) break; \
    } break; \
  case -1: \
    switch(bt) { \
    case  1: r=kv1(ac); DO(ac, v1(r)[i] = v1(a)[i] O b1) break; \
    case -1: r=kv1(ac); DO(ac, v1(r)[i] = v1(a)[i] O v1(b)[i]) break; \
    } break; \
  default: return kerror("type"); \
  } \
  } \
  return r->t ? r : knorm(r); \
}

/* optimized adverb stubs */
#define MC1A(F) K* F##avopt(K *a, char *av) { (void)a; (void)av; return 0; }
#define MC2A(F) K* F##avopt2(K *a, K *b, char *av) { (void)a; (void)b; (void)av; return 0; }

#endif /* MC_H */
