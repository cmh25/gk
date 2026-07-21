/* Cross-verb fusion kernels.  A fusion runs two primitives as one pass so
   the intermediate value is never built.  Every kernel follows the DECLINE
   PROTOCOL: return 0 when the shapes/types don't apply and the caller falls
   back to the unfused path -- a kernel never errors where the unfused
   pipeline wouldn't, and its result matches the unfused result exactly.

   Registry of ALL fusions, including ones whose code lives elsewhere:
     wherecmp    &a<x / &a>x / &a=x      kernel here; parser hook in p.c
                                         (compare dyad + following monadic &)
     raze_       ,/x on a general list   kernel here; driven from overd (av.c)
     seeded fold +/a,x -> seeded over    parse-time rewrite in p.c (0xd7
                                         seeded-fold channel); no kernel
     int-atom dyad inline                k() dispatch fast path (k.c):
                                         + - * & | < > = ~ ! on two plain
                                         int atoms, mirrors PMT/mamo/lme/
                                         match/modi exactly
   New kernels: add here, follow the decline protocol, extend this list. */
#ifndef FUSE_H
#define FUSE_H

#include "k.h"

int wherecmp(K a, K x, i8 op, K *out);
K raze_(K x);

#endif /* FUSE_H */
