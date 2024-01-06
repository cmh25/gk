#include "sort.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "x.h"
#include "k.h"

#define MIN(X,Y) (((X)<(Y))?(X):(Y))

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

static void merge1(int *g, int *a, int p, int q, int r, int down) {
  int i,j,k;
  int n1 = q-p+1;
  int n2 = r-q;

  for(i=0;i<n1;i++) L[i] = g[p+i];
  for(j=0;j<n2;j++) M[j] = g[q+j+1];

  i=j=0; k=p;
  if(down) {
    while(i<n1 && j<n2) {
      if(a[L[i]] >= a[M[j]]) g[k++] = L[i++];
      else g[k++] = M[j++];
    }
  } else {
    while(i<n1 && j<n2) {
      if(a[L[i]] <= a[M[j]]) g[k++] = L[i++];
      else g[k++] = M[j++];
    }
  }

  while(i<n1) g[k++] = L[i++];
  while(j<n2) g[k++] = M[j++];
}

void msortg1(int *g, int *a, int l, int r, int down) {
  int c,m,e;

  L=xmalloc(sizeof(int)*r);
  M=xmalloc(sizeof(int)*r);
  for(c=1;c<=r;c<<=1) {
    for(l=0;l<r;l+=2*c) {
      m = MIN(l+c-1,r);
      e = MIN(l+2*c-1,r);
      merge1(g,a,l,m,e,down);
    }
  }
  if(L){xfree(L);L=0;}
  if(M){xfree(M);M=0;}
}

static void merge2(int *g, double *a, int p, int q, int r, int down) {
  int i,j,k;
  int n1 = q-p+1;
  int n2 = r-q;

  for(i=0;i<n1;i++) L[i] = g[p+i];
  for(j=0;j<n2;j++) M[j] = g[q+j+1];

  i=j=0; k=p;
  if(down) {
    while(i<n1 && j<n2) {
      if(CMPFFT(a[L[i]],a[M[j]])>=0) g[k++] = L[i++];
      else g[k++] = M[j++];
    }
  } else {
    while(i<n1 && j<n2) {
      if(CMPFFT(a[L[i]],a[M[j]])<=0) g[k++] = L[i++];
      else g[k++] = M[j++];
    }
  }

  while(i<n1) g[k++] = L[i++];
  while(j<n2) g[k++] = M[j++];
}

void msortg2(int *g, double *a, int l, int r, int down) {
  int c,m,e;
  L=xmalloc(sizeof(int)*r);
  M=xmalloc(sizeof(int)*r);
  for(c=1;c<=r;c<<=1) {
    for(l=0;l<r;l+=2*c) {
      m = MIN(l+c-1,r);
      e = MIN(l+2*c-1,r);
      merge2(g,a,l,m,e,down);
    }
  }
  if(L){xfree(L);L=0;}
  if(M){xfree(M);M=0;}
}

static void merge3(int *g, char *a, int p, int q, int r, int down) {
  int i,j,k;
  int n1 = q-p+1;
  int n2 = r-q;
  for(i=0;i<n1;i++) L[i] = g[p+i];
  for(j=0;j<n2;j++) M[j] = g[q+j+1];

  i=j=0; k=p;
  if(down) {
    while(i<n1 && j<n2) {
      if(a[L[i]] >= a[M[j]]) g[k++] = L[i++];
      else g[k++] = M[j++];
    }
  } else {
    while(i<n1 && j<n2) {
      if(a[L[i]] <= a[M[j]]) g[k++] = L[i++];
      else g[k++] = M[j++];
    }
  }

  while(i<n1) g[k++] = L[i++];
  while(j<n2) g[k++] = M[j++];
}

void msortg3(int *g, char *a, int l, int r, int down) {
  int c,m,e;
  L=xmalloc(sizeof(int)*r);
  M=xmalloc(sizeof(int)*r);
  for(c=1;c<=r;c<<=1) {
    for(l=0;l<r;l+=2*c) {
      m = MIN(l+c-1,r);
      e = MIN(l+2*c-1,r);
      merge3(g,a,l,m,e,down);
    }
  }
  if(L){xfree(L);L=0;}
  if(M){xfree(M);M=0;}
}

static void merge4(int *g, char **a, int p, int q, int r, int down) {
  int i,j,k;
  int n1 = q-p+1;
  int n2 = r-q;
  for(i=0;i<n1;i++) L[i] = g[p+i];
  for(j=0;j<n2;j++) M[j] = g[q+j+1];

  i=j=0; k=p;
  if(down) {
    while(i<n1 && j<n2) {
      if(strcmp(a[L[i]],a[M[j]])>=0) g[k++] = L[i++];
      else g[k++] = M[j++];
    }
  } else {
    while(i<n1 && j<n2) {
      if(strcmp(a[L[i]],a[M[j]])<=0) g[k++] = L[i++];
      else g[k++] = M[j++];
    }
  }

  while(i<n1) g[k++] = L[i++];
  while(j<n2) g[k++] = M[j++];
}

void msortg4(int *g, char **a, int l, int r, int down) {
  int c,m,e;
  L=xmalloc(sizeof(int)*r);
  M=xmalloc(sizeof(int)*r);
  for(c=1;c<=r;c<<=1) {
    for(l=0;l<r;l+=2*c) {
      m = MIN(l+c-1,r);
      e = MIN(l+2*c-1,r);
      merge4(g,a,l,m,e,down);
    }
  }
  if(L){xfree(L);L=0;}
  if(M){xfree(M);M=0;}
}

static void merge0(int *g, K **a, int p, int q, int r, int down) {
  int i,j,m,c;
  int n1 = q-p+1;
  int n2 = r-q;

  for(i=0;i<n1;i++) L[i] = g[p+i];
  for(j=0;j<n2;j++) M[j] = g[q+j+1];

  i=j=0; m=p;
  if(down) {
    while(i<n1 && j<n2) {
      c = kcmp(a[L[i]],a[M[j]]);
      if(c<0) g[m++] = M[j++];
      else g[m++] = L[i++];

    }
  } else {
    while(i<n1 && j<n2) {
      c = kcmp(a[L[i]],a[M[j]]);
      if(c>0) g[m++] = M[j++];
      else g[m++] = L[i++];
    }
  }

  while(i<n1) g[m++] = L[i++];
  while(j<n2) g[m++] = M[j++];
}

void msortg0(int *g, K **a, int l, int r, int down) {
  int c,m,e;
  L=xmalloc(sizeof(int)*r);
  M=xmalloc(sizeof(int)*r);
  for(c=1;c<=r;c<<=1) {
    for(l=0;l<r;l+=2*c) {
      m = MIN(l+c-1,r);
      e = MIN(l+2*c-1,r);
      merge0(g,a,l,m,e,down);
    }
  }
  if(L){xfree(L);L=0;}
  if(M){xfree(M);M=0;}
}
