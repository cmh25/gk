#include "sort.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "x.h"
#include "k.h"

static int *L,*M;

int* csortg(int *g, int *a, unsigned int n, int min, int max, int down) {
  int i=0,mm1=max-min+1;
  unsigned int k;
  int *count = xcalloc(mm1,sizeof(int));
  for(k=0;k<n;k++) count[a[k]-min]++;
  if(down) for(i=mm1-2;i>=0;i--) count[i] += count[i+1];
  else for(i=1;i<mm1;i++) count[i] += count[i-1];
  for(i=n-1;i>=0;i--) g[--count[a[i]-min]] = i;
  xfree(count);
  return g;
}

int* rcsortg(int *g, int *a, unsigned int n, int down) {
  int i=0,mm1=0,as=0;
  unsigned int k;
  int *count=0,*t=0;
  int max=INT_MIN,min=INT_MAX;
  for(k=0;k<n;k++) {
    max = max < a[k] ? a[k] : max;
    min = min > a[k] ? a[k] : min;
  }
  mm1=max-min+1;

  /* if we have INT_MIN and INT_MAX, can't radixsort */
  if(!mm1) { DO(n,g[i]=i); msortg1(g,a,0,n-1,down); return g; }

  /* can get by with just count sort? */
  if(mm1<0x10000000) return csortg(g,a,n,min,max,down);

  t = xcalloc(n,sizeof(int));
  count = xcalloc(0x10000,sizeof(int));
  for(k=0;k<n;k++) count[a[k]&0xffff]++;
  if(down) for(i=0x10000-1;i>=0;i--) count[i] += count[i+1];
  else for(i=1;i<0x10000;i++) count[i] += count[i-1];
  for(i=n-1;i>=0;i--) t[--count[a[i]&0xffff]] = i;
  xfree(count);

  max=INT_MIN;
  min=INT_MAX;
  for(k=0;k<n;k++) {
    as=(a[k]>>16)&0xffff;
    max = max < as ? as : max;
    min = min > as ? as : min;
  }
  mm1=max-min+1;
  count = xcalloc(mm1,sizeof(int));
  for(k=0;k<n;k++) count[((a[k]>>16)&0xffff)-min]++;
  if(down) for(i=mm1-1;i>=0;i--) count[i] += count[i+1];
  else for(i=1;i<mm1;i++) count[i] += count[i-1];
  for(i=n-1;i>=0;i--) g[--count[((a[t[i]]>>16)&0xffff)-min]] = t[i];
  xfree(count);
  xfree(t);
  return g;
}

#define MIN(X,Y) (((X)<(Y))?(X):(Y))
#define CMP(x,y) ((x)>(y)?1:(x)<(y)?-1:0)
#define MS(N,T,C) \
static void merge##N(int *g, T a, int p, int q, int r, int down) { \
  int i,j,k; \
  int n1 = q-p+1; \
  int n2 = r-q; \
 \
  for(i=0;i<n1;i++) L[i] = g[p+i]; \
  for(j=0;j<n2;j++) M[j] = g[q+j+1]; \
 \
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
 \
  while(i<n1) g[k++] = L[i++]; \
  while(j<n2) g[k++] = M[j++]; \
} \
void msortg##N(int *g, T a, int l, int r, int down) { \
  int c,m,e; \
 \
  L=xmalloc(sizeof(int)*r); \
  M=xmalloc(sizeof(int)*r); \
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

MS(1,int*,CMP)
MS(2,double*,CMPFFT)
MS(3,char*,CMP)
MS(4,char**,strcmp)
MS(0,K**,kcmpr)
