#include "sort.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "k.h"
#include "x.h"

static i32 *L,*M;

i32* csortg(i32 *g, i32 *a, u32 n, i32 min, i32 max, i32 down) {
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

#define LO16(x) (((u32)ik(x)) & 0xffff)
#define HI16S(x) (((u32)ik(x)) >> 16)
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

  /* can get by with just count sort? */
  if(mm1<0x10000000) return csortg(g,a,n,min,max,down);

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

#define MIN(X,Y) (((X)<(Y))?(X):(Y))
#define CMP(x,y) ((x)>(y)?1:(x)<(y)?-1:0)
#define MS(N,T,C) \
static void merge##N(i32 *g, T a, i32 p, i32 q, i32 r, i32 down) { \
  i32 i,j,k; \
  i32 n1 = q-p+1; \
  i32 n2 = r-q; \
  for(i=0;i<n1;i++) L[i] = g[p+i]; \
  for(j=0;j<n2;j++) M[j] = g[q+j+1]; \
  i=j=0; k=p; \
  if(down) { \
    while(i<n1 && j<n2) { \
      if(C(a[L[i]],a[M[j]])>=0) g[k++] = L[i++]; \
      else g[k++] = M[j++]; \
    } \
  } else { \
    while(i<n1 && j<n2) { \
      if(C(a[L[i]],a[M[j]])<=0) g[k++] = L[i++]; \
      else g[k++] = M[j++]; \
    } \
  } \
  while(i<n1) g[k++] = L[i++]; \
  while(j<n2) g[k++] = M[j++]; \
} \
void msortg##N(i32 *g, T a, i32 l, i32 r, i32 down) { \
  i32 c,m,e; \
  L=xmalloc(sizeof(i32)*r); \
  M=xmalloc(sizeof(i32)*r); \
  for(c=1;c<=r;c<<=1) { \
    for(l=0;l<r;l+=2*c) { \
      m = MIN(l+c-1,r); \
      e = MIN(l+2*c-1,r); \
      merge##N(g,a,l,m,e,down); \
    } \
  } \
  if(L){xfree(L);L=0;} \
  if(M){xfree(M);M=0;} \
}
MS(1,i32*,CMP)
MS(2,double*,cmpffx)
MS(3,char*,CMP)
MS(4,char**,strcmp)
MS(0,K*,kcmpr)
