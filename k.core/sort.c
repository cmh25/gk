#include "sort.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "k.h"
#include "x.h"

static i32 *M;

static i32* csortg(i32 *g, i32 *a, u32 n, i32 min, i32 max, i32 down) {
  i32 i=0,mm1=(i32)((i64)max-(i64)min+1);
  u32 k;
  i32 *count=xcalloc(mm1,sizeof(i32));
  for(k=0;k<n;k++) count[a[k]-min]++;
  if(down) for(i=mm1-2;i>=0;i--) count[i] += count[i+1];
  else for(i=1;i<mm1;i++) count[i] += count[i-1];
  for(i=n-1;i>=0;i--) g[--count[a[i]-min]] = i;
  xfree(count);
  return g;
}

/* counting-sort grade for 64-bit ints; caller guarantees max-min+1 fits an
 * i32 count array (range < 0x10000000), so a[k]-min is a valid index */
static i32* csortg8(i32 *g, i64 *a, u32 n, i64 min, i64 max, i32 down) {
  i32 i=0,mm1=(i32)((u64)max-(u64)min+1);
  u32 k;
  i32 *count=xcalloc(mm1,sizeof(i32));
  for(k=0;k<n;k++) count[a[k]-min]++;
  if(down) for(i=mm1-2;i>=0;i--) count[i] += count[i+1];
  else for(i=1;i<mm1;i++) count[i] += count[i-1];
  for(i=n-1;i>=0;i--) g[--count[a[i]-min]] = i;
  xfree(count);
  return g;
}

#define LO16(x) (((u32)ik(x)) & 0xffff)
/* high half of the radix key with the sign bit flipped, so signed values order
   correctly as unsigned keys (INT_MIN..-1 before 0..INT_MAX).  The low half is
   unaffected by the flip, so LO16 needs no change. */
#define HI16S(x) (((((u32)ik(x)) >> 16) ^ 0x8000u))
i32* rcsortg(i32 *g, i32 *a, u32 n, i32 down) {
  i32 i=0,mm1=0,as=0;
  u32 k;
  i32 *count=0,*t=0;
  i32 max=INT32_MIN,min=INT32_MAX;
  for(k=0;k<n;k++) {
    max = max < a[k] ? a[k] : max;
    min = min > a[k] ? a[k] : min;
  }
  mm1=(i32)((i64)max-(i64)min+1);

  /* if we have INT32_MIN and INT32_MAX, can't radixsort */
  if(mm1<=0) { i(n,g[i]=i); msortg1(g,a,0,n-1,down); return g; }

  /* can get by with just count sort?  Only when the value range is dense
     relative to n (plus a small absolute floor) — otherwise a short vector with
     a wide range would calloc a ~4*mm1-byte count array (e.g. <0 100000000 ->
     400MB).  The 2-level radix path below is bounded (~256KB) for sparse data. */
  if(mm1<0x10000000 && (i64)mm1 <= (i64)n*8 + 1000000) return csortg(g,a,n,min,max,down);

  t=xcalloc(n,sizeof(i32));
  count=xcalloc(0x10001,sizeof(i32));
  for(k=0;k<n;k++) count[LO16(a[k])]++;
  if(down) for(i=0x10000-1;i>=0;i--) count[i] += count[i+1];
  else for(i=1;i<0x10000;i++) count[i] += count[i-1];
  for(i=n-1;i>=0;i--) t[--count[LO16(a[i])]] = i;
  xfree(count);

  max=INT32_MIN;
  min=INT32_MAX;
  for(k=0;k<n;k++) {
    as=HI16S(a[k]);
    max = max < as ? as : max;
    min = min > as ? as : min;
  }
  mm1=(i32)((i64)max-(i64)min+1);
  count=xcalloc(mm1+1,sizeof(i32));
  for(k=0;k<n;k++) count[HI16S(a[k])-min]++;
  if(down) for(i=mm1-1;i>=0;i--) count[i] += count[i+1];
  else for(i=1;i<mm1;i++) count[i] += count[i-1];
  for(i=n-1;i>=0;i--) g[--count[HI16S(a[t[i]])-min]] = t[i];
  xfree(count);
  xfree(t);
  return g;
}

/* grade for 64-bit ints: counting sort when the value range is small,
 * otherwise fall back to merge sort (no radix tier yet) */
i32* rcsortg8(i32 *g, i64 *a, u32 n, i32 down) {
  u32 k;
  i64 max=INT64_MIN,min=INT64_MAX;
  for(k=0;k<n;k++) {
    max = max < a[k] ? a[k] : max;
    min = min > a[k] ? a[k] : min;
  }
  if(n && (u64)max-(u64)min < 0x10000000u-1
       && (u64)max-(u64)min <= (u64)n*8 + 1000000) return csortg8(g,a,n,min,max,down);
  i(n,g[i]=i);
  msortg8(g,a,0,(i32)n-1,down);
  return g;
}

/* ---- O(n) radix grade for floats (f32 type 9, f64 type 2) -----------------
   Grade (`<`/`>`) on floats uses the EXACT comparator cmpffx (k.h), whose total
   order is: NaN < -inf < negatives < +-0 < positives < +inf, with all NaNs equal.
   That is a total order on bit patterns once we map each float to an unsigned key:

     - flip the sign bit on a non-negative value, or flip ALL bits on a negative
       one.  This makes the IEEE bit pattern compare correctly as an unsigned int
       (negatives, in reverse bit order, sort below positives).
     - NaN: cmpffx ties all NaNs at the very bottom, but the raw transform would
       split them by sign, so force key 0 (below -inf's key) for any NaN.
     - +-0: cmpffx ties -0 and +0 (cmpff uses < / >), so normalize -0 to +0 before
       transforming, otherwise -0's key would land just below +0's.

   The radix is a stable LSD counting sort over 16-bit digits (2 passes for the
   32-bit f32 key, 4 for the 64-bit f64 key) on the index permutation, mirroring
   rcsortg's ascending/descending prefix-sum + backward-fill (so ties keep their
   original, ascending-index order — matching the merge sort it replaces). */

static i32* radixgkey(i32 *g, u64 *key, u32 n, i32 npass, i32 down) {
  i32 *scratch=xmalloc(n*sizeof(i32)),*count=xmalloc(0x10001*sizeof(i32));
  i32 *src=g,*dst=scratch,*tt,i,p,sh;
  u32 k;
  for(k=0;k<n;k++) g[k]=(i32)k;
  for(p=0;p<npass;p++) {
    sh=p*16;
    memset(count,0,0x10001*sizeof(i32));
    for(k=0;k<n;k++) count[(key[src[k]]>>sh)&0xffff]++;
    if(down) for(i=0x10000-1;i>=0;i--) count[i] += count[i+1];
    else for(i=1;i<0x10000;i++) count[i] += count[i-1];
    for(i=(i32)n-1;i>=0;i--) dst[--count[(key[src[i]]>>sh)&0xffff]] = src[i];
    tt=src; src=dst; dst=tt;
  }
  if(src!=g) memcpy(g,src,n*sizeof(i32)); /* odd npass leaves result in scratch */
  xfree(scratch); xfree(count);
  return g;
}

i32* rcsortg2(i32 *g, double *a, u32 n, i32 down) {
  u64 *key=xmalloc(n*sizeof(u64)),u;
  double v; u32 k;
  for(k=0;k<n;k++) {
    v=a[k];
    if(isnan(v)) { key[k]=0; continue; }
    if(v==0.0) u=0; else memcpy(&u,&v,sizeof(u)); /* normalize -0 to +0 */
    key[k] = u ^ (((u64)((i64)u>>63)) | 0x8000000000000000ULL);
  }
  radixgkey(g,key,n,4,down);
  xfree(key);
  return g;
}

i32* rcsortg9(i32 *g, float *a, u32 n, i32 down) {
  u64 *key=xmalloc(n*sizeof(u64)); u32 u,k; float v;
  for(k=0;k<n;k++) {
    v=a[k];
    if(isnan(v)) { key[k]=0; continue; }
    if(v==0.0f) u=0; else memcpy(&u,&v,sizeof(u)); /* normalize -0 to +0 */
    key[k] = (u64)(u ^ (((u32)((i32)u>>31)) | 0x80000000u));
  }
  radixgkey(g,key,n,2,down);
  xfree(key);
  return g;
}

#define MIN(X,Y) (((X)<(Y))?(X):(Y))
#define CMP(x,y) ((x)>(y)?1:(x)<(y)?-1:0)
#define MS(N,T,C) \
static void merge##N(i32 *g, T a, i32 p, i32 q, i32 r, i32 down) { \
  i32 i,j,k; \
  i32 n1 = q-p+1; \
  i32 n2 = r-q; \
  for(i=0;i<n1;i++) M[i] = g[p+i]; \
  for(j=0;j<n2;j++) M[n1+j] = g[q+j+1]; \
  i=0; j=n1; k=p; \
  if(down) { \
    while(i<n1 && j<n1+n2) { \
      if(C(a[M[i]],a[M[j]])>=0) g[k++] = M[i++]; \
      else g[k++] = M[j++]; \
    } \
  } else { \
    while(i<n1 && j<n1+n2) { \
      if(C(a[M[i]],a[M[j]])<=0) g[k++] = M[i++]; \
      else g[k++] = M[j++]; \
    } \
  } \
  while(i<n1) g[k++] = M[i++]; \
  while(j<n1+n2) g[k++] = M[j++]; \
} \
void msortg##N(i32 *g, T a, i32 l, i32 r, i32 down) { \
  i32 c,m,e,p,n=r-l+1; \
  if(n<=1) return; \
  M=xmalloc(sizeof(i32)*n); \
  for(c=1;c<n;c<<=1) { \
    for(p=l;p<=r;p+=2*c) { \
      m = MIN(p+c-1,r); \
      e = MIN(p+2*c-1,r); \
      if(m<e) merge##N(g,a,p,m,e,down); \
    } \
  } \
  xfree(M); M=0; \
}
MS(1,i32*,CMP)
MS(8,i64*,CMP)
MS(3,char*,CMP)
MS(0,K*,kcmpr)
/* sym (-4) grade is handled by the MSD byte-radix below (rsortg4), not a
   string-compare merge -- so no MS(4)/MSJ(4) instantiation. */

/* ===========================================================================
   i64-INDEX GRADE VARIANTS (suffix `j`) -- used only when #x > 2^31, so the
   index permutation and the count buckets must be 64-bit (a bucket can hold
   more than 2^31 items).  These mirror the i32 functions above exactly; the
   i32 versions are left untouched so the common-case path is unchanged.
   ========================================================================= */
static i64 *Mj;

#define MSJ(N,T,C) \
static void mergej##N(i64 *g, T a, i64 p, i64 q, i64 r, i32 down) { \
  i64 i,j,k,n1=q-p+1,n2=r-q; \
  for(i=0;i<n1;i++) Mj[i] = g[p+i]; \
  for(j=0;j<n2;j++) Mj[n1+j] = g[q+j+1]; \
  i=0; j=n1; k=p; \
  if(down) { \
    while(i<n1 && j<n1+n2) { \
      if(C(a[Mj[i]],a[Mj[j]])>=0) g[k++] = Mj[i++]; \
      else g[k++] = Mj[j++]; \
    } \
  } else { \
    while(i<n1 && j<n1+n2) { \
      if(C(a[Mj[i]],a[Mj[j]])<=0) g[k++] = Mj[i++]; \
      else g[k++] = Mj[j++]; \
    } \
  } \
  while(i<n1) g[k++] = Mj[i++]; \
  while(j<n1+n2) g[k++] = Mj[j++]; \
} \
void msortg##N##j(i64 *g, T a, i64 l, i64 r, i32 down) { \
  i64 c,m,e,p,n=r-l+1; \
  if(n<=1) return; \
  Mj=xmalloc(sizeof(i64)*n); \
  for(c=1;c<n;c<<=1) { \
    for(p=l;p<=r;p+=2*c) { \
      m = MIN(p+c-1,r); \
      e = MIN(p+2*c-1,r); \
      if(m<e) mergej##N(g,a,p,m,e,down); \
    } \
  } \
  xfree(Mj); Mj=0; \
}
MSJ(1,i32*,CMP)
MSJ(8,i64*,CMP)
MSJ(3,char*,CMP)
MSJ(0,K*,kcmpr)

/* ===========================================================================
   MSD BYTE-RADIX GRADE -- for collections of byte-strings: sym vectors (-4),
   and (future) homogeneous char-vector lists (case 0, every element a -3).
   Replaces the O(n log n)-string-compare merge sort with an O(sum of
   distinguishing prefix bytes) stable radix.  Stable, unsigned-byte
   order, shorter-prefix-first -- reproduces strcmp (sym) and kcmpr's (u8)
   char-in-list order exactly, so it is a drop-in (validated by diffing the
   grade permutation against the merge sort).

   Buckets (257): 0 = "string ended" at this depth (a NUL for a sym, or
   depth>=count for a list element) -- these are all equal here, kept in stable
   order and NOT recursed; 1..256 = byte+1.  `down` reverses the bucket walk
   (ended last, bytes 0xff..0x00).  Scratch is one index buffer (same size as
   the merge sort's), so the win is time (no string compares), not memory.

   GETBYTE sets `bk` (the 0..256 bucket) for element index `_i` at `depth`.
   ========================================================================= */
#define RSORT(SUF, GT, AT, GETBYTE) \
GT* rsortg##SUF(GT *g, AT a, u64 n, i32 down) { \
  typedef struct { GT lo, hi; i64 d; } rseg; \
  GT *t; rseg *st; \
  i64 cnt[257], start[257], cur[257]; \
  i64 sp, sm, depth, b, acc; \
  GT lo, hi, k, _i, bk; \
  u64 u; \
  for(u=0;u<n;u++) g[u]=(GT)u; \
  if(n<2) return g; \
  t=xmalloc(n*sizeof(GT)); \
  sm=64; st=xmalloc((size_t)sm*sizeof(rseg)); \
  st[0].lo=0; st[0].hi=(GT)n; st[0].d=0; sp=1; \
  while(sp) { \
    --sp; lo=st[sp].lo; hi=st[sp].hi; depth=st[sp].d; \
    if(hi-lo<2) continue; \
    for(b=0;b<257;b++) cnt[b]=0; \
    for(k=lo;k<hi;k++) { _i=g[k]; GETBYTE; cnt[bk]++; } \
    acc=lo; \
    if(down) for(b=256;b>=0;b--) { start[b]=acc; acc+=cnt[b]; } \
    else     for(b=0;b<257;b++) { start[b]=acc; acc+=cnt[b]; } \
    for(b=0;b<257;b++) cur[b]=start[b]; \
    for(k=lo;k<hi;k++) { _i=g[k]; GETBYTE; t[cur[bk]++]=_i; } \
    for(k=lo;k<hi;k++) g[k]=t[k]; \
    for(b=1;b<257;b++) if(cnt[b]>1) { \
      if(sp==sm) { sm*=2; st=xrealloc(st,(size_t)sm*sizeof(rseg)); } \
      st[sp].lo=(GT)start[b]; st[sp].hi=(GT)(start[b]+cnt[b]); st[sp].d=depth+1; sp++; \
    } \
  } \
  xfree(t); xfree(st); return g; \
}

/* sym (-4): NUL-terminated interned string; no embedded NUL, so byte 0 = end */
#define RBYTE4 bk=(unsigned char)a[_i][depth]; if(bk) bk++;
RSORT(4, i32, char**, RBYTE4)
RSORT(4j, i64, char**, RBYTE4)

static i64* csortgj(i64 *g, i32 *a, u64 n, i32 min, i32 max, i32 down) {
  i64 i,mm1=(i64)max-(i64)min+1;
  u64 k;
  i64 *count=xcalloc(mm1,sizeof(i64));
  for(k=0;k<n;k++) count[a[k]-min]++;
  if(down) for(i=mm1-2;i>=0;i--) count[i] += count[i+1];
  else for(i=1;i<mm1;i++) count[i] += count[i-1];
  for(i=(i64)n-1;i>=0;i--) g[--count[a[i]-min]] = i;
  xfree(count);
  return g;
}

static i64* csortg8j(i64 *g, i64 *a, u64 n, i64 min, i64 max, i32 down) {
  i64 i,mm1=(i64)((u64)max-(u64)min+1);
  u64 k;
  i64 *count=xcalloc(mm1,sizeof(i64));
  for(k=0;k<n;k++) count[a[k]-min]++;
  if(down) for(i=mm1-2;i>=0;i--) count[i] += count[i+1];
  else for(i=1;i<mm1;i++) count[i] += count[i-1];
  for(i=(i64)n-1;i>=0;i--) g[--count[a[i]-min]] = i;
  xfree(count);
  return g;
}

/* char grade: 256-bucket counting sort, stable, native-char order (key =
   (i32)c - CHAR_MIN maps the char range to 0..255 ascending, matching the
   merge sort's CMP).  O(n) time, O(256) space -- no O(n) scratch, so it grades
   a >2^31 char vector without the merge sort's 8n-byte temp (the OOM fix). */
i64* csortg3j(i64 *g, char *a, u64 n, i32 down) {
  i64 i,count[256]; u64 k;
  memset(count,0,sizeof(count));
  for(k=0;k<n;k++) count[(i32)a[k]-CHAR_MIN]++;
  if(down) for(i=254;i>=0;i--) count[i] += count[i+1];
  else     for(i=1;i<256;i++)  count[i] += count[i-1];
  for(i=(i64)n-1;i>=0;i--) g[--count[(i32)a[i]-CHAR_MIN]] = i;
  return g;
}
i32* csortg3(i32 *g, char *a, u64 n, i32 down) {
  i64 i,count[256]; u64 k;
  memset(count,0,sizeof(count));
  for(k=0;k<n;k++) count[(i32)a[k]-CHAR_MIN]++;
  if(down) for(i=254;i>=0;i--) count[i] += count[i+1];
  else     for(i=1;i<256;i++)  count[i] += count[i-1];
  for(i=(i64)n-1;i>=0;i--) g[--count[(i32)a[i]-CHAR_MIN]] = (i32)i;
  return g;
}

i64* rcsortgj(i64 *g, i32 *a, u64 n, i32 down) {
  i64 i,mm1=0; u64 k,q;
  i64 *count=0,*t=0;
  i32 max=INT32_MIN,min=INT32_MAX,as=0;
  for(k=0;k<n;k++) { max = max<a[k]?a[k]:max; min = min>a[k]?a[k]:min; }
  mm1=(i64)max-(i64)min+1;
  if(mm1<=0) { for(q=0;q<n;q++) g[q]=(i64)q; msortg1j(g,a,0,(i64)n-1,down); return g; }
  /* big-vector memory budget: counting sort's count array is 8*range bytes,
     so only prefer it when range<=n (count array <= the 8n output) -- past that
     the range-independent 16n radix below is cheaper.  (Tighter than the i32
     path's speed-tuned <=8n, because here n>2^31 and memory dominates.) */
  if(mm1<0x10000000 && mm1 <= (i64)n + 1000000) return csortgj(g,a,n,min,max,down);
  t=xcalloc(n,sizeof(i64));
  count=xcalloc(0x10001,sizeof(i64));
  for(k=0;k<n;k++) count[LO16(a[k])]++;
  if(down) for(i=0x10000-1;i>=0;i--) count[i] += count[i+1];
  else for(i=1;i<0x10000;i++) count[i] += count[i-1];
  for(i=(i64)n-1;i>=0;i--) t[--count[LO16(a[i])]] = i;
  xfree(count);
  max=INT32_MIN; min=INT32_MAX;
  for(k=0;k<n;k++) { as=HI16S(a[k]); max = max<as?as:max; min = min>as?as:min; }
  mm1=(i64)max-(i64)min+1;
  count=xcalloc(mm1+1,sizeof(i64));
  for(k=0;k<n;k++) count[HI16S(a[k])-min]++;
  if(down) for(i=mm1-1;i>=0;i--) count[i] += count[i+1];
  else for(i=1;i<mm1;i++) count[i] += count[i-1];
  for(i=(i64)n-1;i>=0;i--) g[--count[HI16S(a[t[i]])-min]] = t[i];
  xfree(count); xfree(t);
  return g;
}

i64* rcsortg8j(i64 *g, i64 *a, u64 n, i32 down) {
  u64 k,q;
  i64 max=INT64_MIN,min=INT64_MAX;
  for(k=0;k<n;k++) { max = max<a[k]?a[k]:max; min = min>a[k]?a[k]:min; }
  if(n && (u64)max-(u64)min < 0x10000000u-1
       && (u64)max-(u64)min <= (u64)n + 1000000) return csortg8j(g,a,n,min,max,down);
  for(q=0;q<n;q++) g[q]=(i64)q;
  msortg8j(g,a,0,(i64)n-1,down);
  return g;
}

static i64* radixgkeyj(i64 *g, u64 *key, u64 n, i32 npass, i32 down) {
  i64 *scratch=xmalloc(n*sizeof(i64)),*count=xmalloc(0x10001*sizeof(i64));
  i64 *src=g,*dst=scratch,*tt,i; i32 p,sh;
  u64 k;
  for(k=0;k<n;k++) g[k]=(i64)k;
  for(p=0;p<npass;p++) {
    sh=p*16;
    memset(count,0,0x10001*sizeof(i64));
    for(k=0;k<n;k++) count[(key[src[k]]>>sh)&0xffff]++;
    if(down) for(i=0x10000-1;i>=0;i--) count[i] += count[i+1];
    else for(i=1;i<0x10000;i++) count[i] += count[i-1];
    for(i=(i64)n-1;i>=0;i--) dst[--count[(key[src[i]]>>sh)&0xffff]] = src[i];
    tt=src; src=dst; dst=tt;
  }
  if(src!=g) memcpy(g,src,n*sizeof(i64));
  xfree(scratch); xfree(count);
  return g;
}

i64* rcsortg2j(i64 *g, double *a, u64 n, i32 down) {
  u64 *key=xmalloc(n*sizeof(u64)),u;
  double v; u64 k;
  for(k=0;k<n;k++) {
    v=a[k];
    if(isnan(v)) { key[k]=0; continue; }
    if(v==0.0) u=0; else memcpy(&u,&v,sizeof(u));
    key[k] = u ^ (((u64)((i64)u>>63)) | 0x8000000000000000ULL);
  }
  radixgkeyj(g,key,n,4,down);
  xfree(key);
  return g;
}

i64* rcsortg9j(i64 *g, float *a, u64 n, i32 down) {
  u64 *key=xmalloc(n*sizeof(u64)); u32 u; u64 k; float v;
  for(k=0;k<n;k++) {
    v=a[k];
    if(isnan(v)) { key[k]=0; continue; }
    if(v==0.0f) u=0; else memcpy(&u,&v,sizeof(u));
    key[k] = (u64)(u ^ (((u32)((i32)u>>31)) | 0x80000000u));
  }
  radixgkeyj(g,key,n,2,down);
  xfree(key);
  return g;
}
