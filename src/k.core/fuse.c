#include "fuse.h"
#include "v.h"
#include "x.h"
#include <string.h>

/* Fused &a<x / &a>x / &a=x */
#define WC_RUN(PRED)                                                      \
  do {                                                                    \
    i64 j=0;                                                              \
    if(N>BIGV) {                                                          \
      i64 tot=0;                                                          \
      for(u64 i=0;i<N;++i) tot+=(invert^!!(PRED));                        \
      r=tn(8,tot); { i64 *p=(i64*)px(r);                                  \
        for(u64 i=0; j<tot; ++i){ p[j]=(i64)i;                            \
          j+=(invert^!!(PRED)); } }                                      \
    } else {                                                              \
      r=tn(1,N); { i32 *p=(i32*)px(r);                                    \
        for(u64 i=0;i<N;++i){ p[j]=(i32)i;                               \
          j+=(invert^!!(PRED)); } }                                      \
      if(j<(i64)N) r=kresize(r,j); else n(r)=j;                           \
    }                                                                     \
  } while(0)

int wherecmp(K a, K x, i8 op, i8 invert, K *out) {
  K r=0; u64 N;
  if(s(a)||s(x)||ta>=0) return 0;      /* left operand must be a plain vector */
  N=na;
  if(N>VMAX) return 0;
  if     (ta==-1&&tx== 1){ i32 *pa=(i32*)px(a);    i32 c=ik(x);          WC_RUN(icmp(pa[i],c,op)); }
  else if(ta==-8&&tx== 8){ i64 *pa=(i64*)px(a);    i64 c=jk(x);          WC_RUN(jcmp(pa[i],c,op)); }
  else if(ta==-2&&tx== 2){ double *pa=(double*)px(a); double c=fk(x);    WC_RUN(cmpfft(pa[i],c)==op); }
  else if(ta==-9&&tx== 9){ float *pa=(float*)px(a);   double c=(double)ek(x); WC_RUN(cmpfft((double)pa[i],c)==op); }
  else if(ta==-1&&tx==-1&&na==nx){ i32 *pa=(i32*)px(a),*pb=(i32*)px(x);  WC_RUN(icmp(pa[i],pb[i],op)); }
  else if(ta==-8&&tx==-8&&na==nx){ i64 *pa=(i64*)px(a),*pb=(i64*)px(x);  WC_RUN(jcmp(pa[i],pb[i],op)); }
  else if(ta==-2&&tx==-2&&na==nx){ double *pa=(double*)px(a),*pb=(double*)px(x); WC_RUN(cmpfft(pa[i],pb[i])==op); }
  else if(ta==-9&&tx==-9&&na==nx){ float *pa=(float*)px(a),*pb=(float*)px(x);    WC_RUN(cmpfft((double)pa[i],(double)pb[i])==op); }
  else return 0;
  *out=knorm(r);
  return 1;
}

/* Terminal consumers of a comparison.  `invert` represents a literal
   monadic ~ around the comparison result; do not rewrite it to the opposite
   comparator because NaNs make ~(a<x) observably different from a>=x. */
#define CT_RUN(P0)                                                        \
  do {                                                                    \
    for(u64 i=0;i<N;++i) {                                               \
      i32 v=invert^!!(P0);                                                \
      if(v && first==N) first=i;                                         \
      matches+=(u64)v;                                                    \
    }                                                                     \
  } while(0)

#define CT_FIRST(P0)                                                      \
  do {                                                                    \
    for(u64 i=0;i<N;++i) if(invert^!!(P0)) { first=i; break; }            \
  } while(0)

#define CMP_DISPATCH(RUN)                                                 \
  if     (ta==-1&&tx== 1){ i32 *pa=(i32*)px(a); i32 z=ik(x);             RUN(icmp(pa[i],z,op)); } \
  else if(ta==-8&&tx== 8){ i64 *pa=(i64*)px(a); i64 z=jk(x);             RUN(jcmp(pa[i],z,op)); } \
  else if(ta==-2&&tx== 2){ double *pa=(double*)px(a); double z=fk(x);     RUN(cmpfft(pa[i],z)==op); } \
  else if(ta==-9&&tx== 9){ float *pa=(float*)px(a); double z=(double)ek(x); RUN(cmpfft((double)pa[i],z)==op); } \
  else if(ta==-1&&tx==-1&&na==nx){ i32 *pa=(i32*)px(a),*pb=(i32*)px(x);  RUN(icmp(pa[i],pb[i],op)); } \
  else if(ta==-8&&tx==-8&&na==nx){ i64 *pa=(i64*)px(a),*pb=(i64*)px(x);  RUN(jcmp(pa[i],pb[i],op)); } \
  else if(ta==-2&&tx==-2&&na==nx){ double *pa=(double*)px(a),*pb=(double*)px(x); RUN(cmpfft(pa[i],pb[i])==op); } \
  else if(ta==-9&&tx==-9&&na==nx){ float *pa=(float*)px(a),*pb=(float*)px(x); RUN(cmpfft((double)pa[i],(double)pb[i])==op); } \
  else return 0

int cmpterminal(K a, K x, i8 op, i8 invert, i8 mode, K *out) {
  K r=0; u64 N,matches=0,first;
  if(s(a)||s(x)||ta>=0) return 0;
  N=na; first=N; if(N>VMAX) return 0;
  if(mode==CMP_FIRST_WHERE) {
    CMP_DISPATCH(CT_FIRST);
    if(first==N) first=0;
    *out=N>BIGV?tj((i64)first):t(1,(u32)first);
    return 1;
  }
  CMP_DISPATCH(CT_RUN);
  switch(mode) {
  case CMP_COUNT_WHERE: r=matches>BIGV?tj((i64)matches):t(1,(u32)matches); break;
  case CMP_SUM_BOOL:    r=t(1,(u32)matches); break;
  case CMP_ALL_BOOL:    r=t(1,matches==N); break;
  case CMP_ANY_BOOL:    r=t(1,matches!=0); break;
  default: return 0;
  }
  *out=r;
  return 1;
}

/* Direct y@&comparison.  Restrict the fast path to a plain payload vector of
   exactly the comparison length.  Other legal (or error-producing) shapes
   decline to the ordinary where + at pipeline, preserving its bounds/errors. */
#define FILTER_COPY(P0)                                                   \
  do {                                                                    \
    u64 m=0,j=0;                                                          \
    for(u64 i=0;i<N;++i) m+=(u64)(invert^!!(P0));                         \
    r=tn(-ty,m);                                                          \
    { char *py=px(y),*pr=px(r);                                          \
      for(u64 i=0;i<N;++i) if(invert^!!(P0)) {                           \
        memcpy(pr+j*width,py+i*width,width); ++j;                         \
      }                                                                   \
    }                                                                     \
  } while(0)

#define FILTER_COPY_4(P0)                                                 \
  do {                                                                    \
    u64 m=0,j=0;                                                          \
    for(u64 i=0;i<N;++i) m+=(u64)(invert^!!(P0));                         \
    r=tn(-ty,m);                                                          \
    { u32 *py=px(y),*pr=px(r);                                           \
      for(u64 i=0;i<N;++i) if(invert^!!(P0)) pr[j++]=py[i];               \
    }                                                                     \
  } while(0)

int filtercmp(K y, K a, K x, i8 op, i8 invert, K *out) {
  K r=0; u64 N,ny,width; i8 ty=T(y);
  if(s(y)||s(a)||s(x)||ty>=0||ta>=0) return 0;
  N=n(a); ny=n(y);
  if(ty==-3) width=1;
  else if(ty==-1||ty==-9) width=4;
  else if(ty==-8||ty==-2) width=8;
  else if(ty==-4) width=sizeof(char*);
  else return 0;
  if(N>VMAX||ny!=N) return 0;
  if(ty==-1||ty==-9) {
    CMP_DISPATCH(FILTER_COPY_4);
    *out=r;
    return 1;
  }
  CMP_DISPATCH(FILTER_COPY);
  *out=r;
  return 1;
}

/* Fold selected payload values without constructing a mask, indices, or the
   filtered vector.  reducer uses the primitive table indices: +=1, &=5, |=6.
   Empty identities and wrapping integer addition mirror overd() exactly. */
#define FILTER_REDUCE(P0)                                                 \
  do {                                                                    \
    for(u64 i=0;i<N;++i) if(invert^!!(P0)) {                              \
      switch(payload_type) {                                              \
      case -1: { i32 z=((i32*)px(y))[i];                                 \
        if(red==1) isu+=(u32)z; else if(!seen){iv=z;seen=1;}              \
        else if(red==5){if(z<iv)iv=z;} else if(z>iv)iv=z; } break;        \
      case -8: { i64 z=((i64*)px(y))[i];                                 \
        if(red==1) jsu+=(u64)z; else if(!seen){jv=z;seen=1;}              \
        else if(red==5){if(z<jv)jv=z;} else if(z>jv)jv=z; } break;        \
      case -2: { double z=((double*)px(y))[i];                            \
        if(red==1) dv+=z; else if(!seen){dv=z;seen=1;}                    \
        else if(red==5){if(cmpfft(dv,z)!=-1)dv=z;}                        \
        else if(cmpfft(dv,z)!=1)dv=z; } break;                            \
      case -9: { float z=((float*)px(y))[i];                              \
        if(red==1) ev+=z; else if(!seen){ev=z;seen=1;}                    \
        else if(red==5){if(cmpfft((double)ev,(double)z)!=-1)ev=z;}        \
        else if(cmpfft((double)ev,(double)z)!=1)ev=z; } break;            \
      }                                                                   \
    }                                                                     \
  } while(0)

#define FILTER_REDUCE_I32(P0)                                            \
  do {                                                                    \
    i32 *py=(i32*)px(y);                                                  \
    for(u64 i=0;i<N;++i) if(invert^!!(P0)) { i32 z=py[i];                \
      if(reducer==1) isu+=(u32)z;                                        \
      else if(!seen){iv=z;seen=1;}                                       \
      else if(reducer==5){if(z<iv)iv=z;} else if(z>iv)iv=z;               \
    }                                                                     \
  } while(0)

int filterredcmp(K y, K a, K x, i8 op, i8 invert, i32 reducer, K *out) {
  K r=0; u64 N,ny,isu=0,jsu=0; i8 ty=T(y);
  i32 iv=reducer==5?1:0,seen=0;
  i64 jv=reducer==5?J_INF:reducer==6?J_NINF:0;
  double dv=reducer==5?INFINITY:reducer==6?-INFINITY:0.0;
  float ev=reducer==5?(float)INFINITY:reducer==6?-(float)INFINITY:0.0f;
  volatile i8 payload_type=ty;
  volatile i32 red=reducer;
  if(s(y)||s(a)||s(x)||ty>=0||ta>=0) return 0;
  N=n(a); ny=n(y);
  if((ty!=-1&&ty!=-8&&ty!=-2&&ty!=-9)
     ||(reducer!=1&&reducer!=5&&reducer!=6)) return 0;
  if(N>VMAX||ny!=N) return 0;
  if(ty==-1) {
    CMP_DISPATCH(FILTER_REDUCE_I32);
    *out=reducer==1?t(1,(u32)isu):t(1,(u32)iv);
    return 1;
  }
  CMP_DISPATCH(FILTER_REDUCE);
  switch(ty) {
  case -1: r=reducer==1?t(1,(u32)isu):t(1,(u32)iv); break;
  case -8: r=reducer==1?tj((i64)jsu):tj(jv); break;
  case -2: r=t2(dv); break;
  case -9: r=te(ev); break;
  default: return 0;
  }
  *out=r;
  return 1;
}

#undef FILTER_REDUCE
#undef FILTER_REDUCE_I32
#undef FILTER_COPY
#undef FILTER_COPY_4
#undef CMP_DISPATCH
#undef CT_FIRST
#undef CT_RUN

/* +\\0j+x.  ji() is deliberately used for every element: the widening dyad
   maps int null/infinities to their long sentinels before the ordinary
   wrapping long scan sees them. */
int widenintscan(K x, K *out) {
  K r; i32 *p; u64 su=0,N;
  if(s(x)||T(x)!=-1) return 0;
  N=n(x); p=(i32*)px(x);
  i64 *q;
  r=tn(8,N); q=(i64*)px(r);
  for(u64 i=0;i<N;++i) { su+=(u64)ji(p[i]); q[i]=(i64)su; }
  *out=r;
  return 1;
}

/* #'=x: count groups in first-occurrence order without allocating an index
   vector for every group.  The hash/equality pairs mirror group(). */
static inline u64 fuse_hash_f64_(double v) {
  u64 u;
  if(v==0) v=0.0;
  memcpy(&u,&v,sizeof(u));
  return hmul(u);
}
static inline u64 fuse_hash_f32_(float v) {
  u32 u;
  if(v==0) v=0.0f;
  memcpy(&u,&v,sizeof(u));
  return hmul((u64)u);
}

#define GC_RUN(HASH,EQ)                                                   \
  do {                                                                    \
    for(u64 i=0;i<N;++i) {                                                \
      u64 h=(HASH)&q;                                                      \
      while(cnt[h] && !(EQ)) h=(h+1)&q;                                  \
      if(!cnt[h]) { first[h]=i; order[ng++]=h; }                          \
      ++cnt[h];                                                           \
    }                                                                     \
  } while(0)

int groupcounts(K x, K *out) {
  K r; u64 N,w=1,q,ng=0,*cnt,*first,*order;
  if(E(x)||s(x)||T(x)>0) return 0;
  N=n(x);
  if(!N) { *out=tn(0,0); return 1; }
  if(N>BIGV) return 0;                 /* groupj can produce long counts */
  if(T(x)==-1) {
    i32 *p=px(x),min=INT32_MAX,max=INT32_MIN;
    for(u64 i=0;i<N;++i) { if(p[i]<min)min=p[i]; if(p[i]>max)max=p[i]; }
    u64 span=(u64)(u32)max+1;
    if(min>=0 && span<=N*8) {
      u32 *dc=xcalloc(span,sizeof(u32));
      u32 *ord=xmalloc((span<N?span:N)*sizeof(u32));
      for(u64 i=0;i<N;++i) { u32 v=(u32)p[i]; if(!dc[v])ord[ng++]=v; ++dc[v]; }
      r=tn(1,ng);
      { i32 *z=px(r); for(u64 i=0;i<ng;++i)z[i]=(i32)dc[ord[i]]; }
      xfree(dc);xfree(ord);*out=r;return 1;
    }
  }
  while(w<=N) w<<=1;
  q=w-1;
  cnt=xcalloc(w,sizeof(u64));
  first=xmalloc(w*sizeof(u64));
  order=xmalloc(N*sizeof(u64));
  switch(T(x)) {
  case -1: { i32 *p=px(x); GC_RUN(hmul((u32)p[i]),p[first[h]]==p[i]); } break;
  case -8: { i64 *p=px(x); GC_RUN(hmul((u64)p[i]),p[first[h]]==p[i]); } break;
  case -2: { double *p=px(x); GC_RUN(fuse_hash_f64_(p[i]),!cmpfft(p[first[h]],p[i])); } break;
  case -9: { float *p=px(x); GC_RUN(fuse_hash_f32_(p[i]),!cmpfft((double)p[first[h]],(double)p[i])); } break;
  case -3: { u8 *p=px(x); GC_RUN((u64)p[i],p[first[h]]==p[i]); } break;
  case -4: { char **p=px(x); GC_RUN(xfnv1a(p[i],strlen(p[i])),!strcmp(p[first[h]],p[i])); } break;
  case 0:  { K *p=px(x); GC_RUN(khash(p[i]),!kcmpr(p[first[h]],p[i])); } break;
  default: xfree(cnt); xfree(first); xfree(order); return 0;
  }
  r=tn(1,ng);
  { i32 *p=px(r); for(u64 i=0;i<ng;++i) p[i]=(i32)cnt[order[i]]; }
  xfree(cnt); xfree(first); xfree(order);
  *out=r;
  return 1;
}

#undef GC_RUN

/* Stable direct value sort.  Flat numeric/char vectors use an LSD radix on
   values themselves (no grade vector and no gather); symbols/general lists
   use the stable merge below.  Equal values retain input order, including
   distinct NaN payloads and signed zeroes which cmpfft treats as ties. */
static inline u64 fuse_key_i32_(i32 v) { return (u32)v^0x80000000u; }
static inline u64 fuse_key_i64_(i64 v) { return (u64)v^0x8000000000000000ULL; }
static inline u64 fuse_key_f64_(double v) {
  u64 u;
  if(isnan(v)) return 0;
  if(v==0.0) u=0; else memcpy(&u,&v,sizeof(u));
  return u^(((u64)((i64)u>>63))|0x8000000000000000ULL);
}
static inline u64 fuse_key_f32_(float v) {
  u32 u;
  if(isnan(v)) return 0;
  if(v==0.0f) u=0; else memcpy(&u,&v,sizeof(u));
  return (u64)(u^(((u32)((i32)u>>31))|0x80000000u));
}
static inline u64 fuse_key_char_(char v) { return (u64)((i32)v-CHAR_MIN); }

static int sort_count_i32_(i32 *p,u64 N,i8 down,K *out) {
  i32 min=INT32_MAX,max=INT32_MIN;
  if(!N) { *out=tn(1,0); return 1; }
  for(u64 i=0;i<N;++i) { if(p[i]<min)min=p[i];if(p[i]>max)max=p[i]; }
  u64 range=(u64)((i64)max-(i64)min)+1;
  if(range>=0x10000000u || range>N*8+1000000) return 0;
  u32 *cnt=xcalloc(range,sizeof(u32));
  for(u64 i=0;i<N;++i) ++cnt[(u32)((i64)p[i]-(i64)min)];
  K r=tn(1,N); i32 *z=px(r); u64 j=0;
  if(down) for(u64 b=range;b--;) while(cnt[b]--)z[j++]=(i32)((i64)min+(i64)b);
  else for(u64 b=0;b<range;++b) while(cnt[b]--)z[j++]=(i32)((i64)min+(i64)b);
  xfree(cnt);*out=r;return 1;
}

#define RADIX_RUN(TY,KEY,NPASS)                                          \
  do {                                                                    \
    typedef TY radix_ty_;                                                 \
    radix_ty_ *base=(radix_ty_*)px(r),*src=base;                          \
    radix_ty_ *dst=xmalloc(N*sizeof(radix_ty_)),*tmp=dst;                 \
    u32 *cnt=xmalloc(65536*sizeof(u32));                                  \
    for(i32 pass=0;pass<(NPASS);++pass) {                                \
      i32 sh=pass*16; memset(cnt,0,65536*sizeof(u32));                    \
      for(u64 z=0;z<N;++z) ++cnt[((KEY(src[z]))>>sh)&0xffff];             \
      if(down) for(i32 z=65534;z>=0;--z) cnt[z]+=cnt[z+1];                \
      else for(i32 z=1;z<65536;++z) cnt[z]+=cnt[z-1];                     \
      for(u64 z=N;z--;) { u32 b=(u32)(((KEY(src[z]))>>sh)&0xffff);        \
                       dst[--cnt[b]]=src[z]; }                            \
      { radix_ty_ *z=src;src=dst;dst=z; }                                \
    }                                                                     \
    if(src!=base) memcpy(base,src,N*sizeof(radix_ty_));                   \
    xfree(cnt);xfree(tmp);                                                \
  } while(0)

#define SORT_RUN(TY,CMP)                                                  \
  do {                                                                    \
    typedef TY sort_ty_;                                                  \
    sort_ty_ *base=(sort_ty_*)px(r),*src=base;                            \
    sort_ty_ *dst=xmalloc(N*sizeof(sort_ty_)),*tmp=dst;                    \
    for(u64 width=1;width<N;width<<=1) {                                  \
      for(u64 lo=0;lo<N;lo+=width<<1) {                                  \
        u64 mid=lo+width<N?lo+width:N,hi=lo+(width<<1)<N?lo+(width<<1):N; \
        u64 l=lo,j=mid,k=lo;                                              \
        while(l<mid&&j<hi) { i32 c=(CMP);                                \
          if((!down&&c<=0)||(down&&c>=0)) dst[k++]=src[l++];              \
          else dst[k++]=src[j++]; }                                      \
        while(l<mid) dst[k++]=src[l++];                                  \
        while(j<hi) dst[k++]=src[j++];                                   \
      }                                                                   \
      { sort_ty_ *z=src; src=dst; dst=z; }                               \
    }                                                                     \
    if(src!=base) memcpy(base,src,N*sizeof(sort_ty_));                    \
    xfree(tmp);                                                           \
  } while(0)

int sortvalues(K x, i8 down, K *out) {
  K r; u64 N; i8 t;
  if(E(x)||s(x)||(t=T(x))>0) return 0;
  N=n(x); if(N>BIGV) return 0;
  if(t==-1 && N>65536) return 0; /* existing large-int grade is exceptionally fast */
  if(t==-1 && sort_count_i32_((i32*)px(x),N,down,out)) return 1;
  r=tn(t?-t:0,N);
  switch(t) {
  case -1: { i32 *p=px(x),*q=px(r); memcpy(q,p,N*sizeof(i32));
             RADIX_RUN(i32,fuse_key_i32_,2); } break;
  case -8: { i64 *p=px(x),*q=px(r); memcpy(q,p,N*sizeof(i64));
             RADIX_RUN(i64,fuse_key_i64_,4); } break;
  case -2: { double *p=px(x),*q=px(r); memcpy(q,p,N*sizeof(double));
             RADIX_RUN(double,fuse_key_f64_,4); } break;
  case -9: { float *p=px(x),*q=px(r); memcpy(q,p,N*sizeof(float));
             RADIX_RUN(float,fuse_key_f32_,2); } break;
  case -3: { char *p=px(x),*q=px(r); memcpy(q,p,N);
             RADIX_RUN(char,fuse_key_char_,1); } break;
  case -4: { char **p=px(x),**q=px(r); memcpy(q,p,N*sizeof(char*));
             SORT_RUN(char*,strcmp(src[l],src[j])); } break;
  case 0:  { K *p=px(x),*q=px(r); for(u64 i=0;i<N;++i) q[i]=k_(p[i]);
             SORT_RUN(K,kcmpr(src[l],src[j])); r=knorm(r); } break;
  default: _k(r); return 0;
  }
  *out=r;
  return 1;
}

#undef RADIX_RUN
#undef SORT_RUN

/* Stable top-k grade.  The retained indices form a max-heap in the requested
   grade order (root = worst retained item); heap-sort then emits best first.
   Ties always prefer the lower source index, exactly as stable grade does. */
#define TOP_DEFINE(NAME,TY,BASECMP)                                       \
static inline i32 topcmp_##NAME(TY *p,u32 a,u32 b,i8 down) {              \
  i32 c=(BASECMP); c=(c>0)-(c<0); if(down)c=-c;                           \
  return c?c:(a>b)-(a<b);                                                \
}                                                                         \
static void topsift_##NAME(i32 *h,u32 n,u32 root,TY *p,i8 down) {         \
  for(;;) {                                                               \
    u32 child=root*2+1; if(child>=n) return;                              \
    if(child+1<n && topcmp_##NAME(p,(u32)h[child+1],(u32)h[child],down)>0) ++child; \
    if(topcmp_##NAME(p,(u32)h[child],(u32)h[root],down)<=0) return;        \
    { i32 z=h[root];h[root]=h[child];h[child]=z; } root=child;            \
  }                                                                       \
}                                                                         \
static K top_##NAME(TY *p,u32 N,u32 k,i8 down) {                          \
  K r=tn(1,k); i32 *h=px(r); u32 m=0;                                    \
  for(u32 i=0;i<N;++i) {                                                  \
    if(m<k) {                                                             \
      u32 z=m++; h[z]=(i32)i;                                             \
      while(z) { u32 parent=(z-1)>>1;                                    \
        if(topcmp_##NAME(p,(u32)h[z],(u32)h[parent],down)<=0) break;      \
        { i32 v=h[z];h[z]=h[parent];h[parent]=v; } z=parent; }             \
    } else if(k && topcmp_##NAME(p,i,(u32)h[0],down)<0) {                 \
      h[0]=(i32)i; topsift_##NAME(h,k,0,p,down);                          \
    }                                                                     \
  }                                                                       \
  for(u32 end=k;end>1;) { i32 z=h[0];h[0]=h[--end];h[end]=z;             \
                           topsift_##NAME(h,end,0,p,down); }              \
  return r;                                                               \
}

TOP_DEFINE(i32,i32,(p[a]>p[b])-(p[a]<p[b]))
TOP_DEFINE(i64,i64,(p[a]>p[b])-(p[a]<p[b]))
TOP_DEFINE(f64,double,cmpfft(p[a],p[b]))
TOP_DEFINE(f32,float,cmpfft((double)p[a],(double)p[b]))
TOP_DEFINE(chr,char,(p[a]>p[b])-(p[a]<p[b]))
TOP_DEFINE(sym,char*,strcmp(p[a],p[b]))
TOP_DEFINE(any,K,kcmpr(p[a],p[b]))

#undef TOP_DEFINE

static int top_count_i32_(i32 *p,u32 N,u32 k,i8 down,K *out) {
  i32 min=INT32_MAX,max=INT32_MIN;
  if(!k) { *out=tn(1,0); return 1; }
  for(u32 i=0;i<N;++i) { if(p[i]<min)min=p[i];if(p[i]>max)max=p[i]; }
  u64 range=(u64)((i64)max-(i64)min)+1;
  if(range>0x1000000u || range>(u64)N*8+1000000) return 0;
  u32 *cnt=xcalloc(range,sizeof(u32)),*cur=xmalloc(range*sizeof(u32));
  for(u32 i=0;i<N;++i) ++cnt[(u32)((i64)p[i]-(i64)min)];
  u32 pos=0;
  if(down) {
    for(u64 b=range;b--;) { u32 c=cnt[b];
      if(pos<k&&c) { cur[b]=pos; pos+=(c<k-pos?c:k-pos); cnt[b]=pos; }
      else cnt[b]=0;
    }
  } else {
    for(u64 b=0;b<range;++b) { u32 c=cnt[b];
      if(pos<k&&c) { cur[b]=pos; pos+=(c<k-pos?c:k-pos); cnt[b]=pos; }
      else cnt[b]=0;
    }
  }
  K r=tn(1,k); i32 *z=px(r);
  for(u32 i=0;i<N;++i) { u32 b=(u32)((i64)p[i]-(i64)min);
    if(cnt[b]&&cur[b]<cnt[b]) z[cur[b]++]=(i32)i;
  }
  xfree(cnt);xfree(cur);*out=r;return 1;
}

/* Order-independent fallback for descending sparse/wide int data.  A
   three-way quickselect finds the cutoff value; a stable source scan retains
   the earliest cutoff ties, then heap-sorts only the k chosen indices. */
static int top_select_i32_(i32 *p,u32 N,u32 k,i8 down,K *out) {
  if(!down||!k) return 0;
  i32 *v=xmalloc((u64)N*sizeof(i32)); memcpy(v,p,(u64)N*sizeof(i32));
  u32 lo=0,hi=N,target=N-k;
  i32 pivot=0; u32 depth=0,q=N,seed=N^(k*0x9e3779b9u);
  while(q) { depth+=3; q>>=1; }
  while(hi-lo>1) {
    if(!depth--) { xfree(v);*out=top_i32(p,N,k,down);return 1; }
    u32 span=hi-lo;
    seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
    i32 a=v[lo+seed%span];
    seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
    i32 b=v[lo+seed%span];
    seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
    i32 c=v[lo+seed%span];
    pivot=a<b?(b<c?b:(a<c?c:a)):(a<c?a:(b<c?c:b));
    u32 lt=lo,i=lo,gt=hi;
    while(i<gt) {
      if(v[i]<pivot) { i32 z=v[lt];v[lt++]=v[i];v[i++]=z; }
      else if(v[i]>pivot) { i32 z=v[--gt];v[gt]=v[i];v[i]=z; }
      else ++i;
    }
    if(target<lt) hi=lt;
    else if(target>=gt) lo=gt;
    else break;
  }
  if(hi-lo==1) pivot=v[lo];
  xfree(v);
  u32 strict=0; for(u32 i=0;i<N;++i) strict+=(u32)(p[i]>pivot);
  u32 ties=k-strict,j=0;
  K r=tn(1,k); i32 *h=px(r);
  for(u32 i=0;i<N;++i) {
    if(p[i]>pivot) h[j++]=(i32)i;
    else if(p[i]==pivot&&ties) { h[j++]=(i32)i;--ties; }
  }
  for(u32 root=k/2;root--;) topsift_i32(h,k,root,p,down);
  for(u32 end=k;end>1;) { i32 z=h[0];h[0]=h[--end];h[end]=z;
                          topsift_i32(h,end,0,p,down); }
  *out=r;return 1;
}

int topgrade(K x, K take, i8 down, K *out) {
  K r; i64 z; u64 N; i8 t;
  if(E(x)||E(take)||s(x)||(t=T(x))>0||s(take)) return 0;
  if(T(take)==1) z=ik(take); else if(T(take)==8) z=jk(take); else return 0;
  N=n(x);
  if(N>BIGV||z<0||(u64)z>N) return 0;
  /* A full grade's radix/counting paths win once k is a substantial part of
     N; leave those cases to the ordinary implementation. */
  if(z && (u64)z>N/4) return 0;
  if(t==-1 && top_count_i32_((i32*)px(x),(u32)N,(u32)z,down,out)) return 1;
  if(t==-1 && top_select_i32_((i32*)px(x),(u32)N,(u32)z,down,out)) return 1;
  switch(t) {
  case -1: r=top_i32((i32*)px(x),(u32)N,(u32)z,down); break;
  case -8: r=top_i64((i64*)px(x),(u32)N,(u32)z,down); break;
  case -2: r=top_f64((double*)px(x),(u32)N,(u32)z,down); break;
  case -9: r=top_f32((float*)px(x),(u32)N,(u32)z,down); break;
  case -3: r=top_chr((char*)px(x),(u32)N,(u32)z,down); break;
  case -4: r=top_sym((char**)px(x),(u32)N,(u32)z,down); break;
  case 0:  r=top_any((K*)px(x),(u32)N,(u32)z,down); break;
  default: return 0;
  }
  *out=r;
  return 1;
}

int topgradeat(K y, K x, K take, i8 down, K *out) {
  K g,r;
  if(E(y)||E(x)||E(take)||s(y)||s(x)||T(y)>0||T(x)>0) return 0;
  if(n(y)!=n(x)) return 0;
  if(!topgrade(x,take,down,&g)) return 0;
  r=at(y,g);
  _k(g);
  if(E(r)) return 0; /* shape restrictions above make this defensive only */
  *out=r;
  return 1;
}

/* Direct `*|x` (first reverse / last) without materialising the reverse.
   Atoms and subtypes are unchanged by both verbs.  Empty vectors keep
   first()'s type-specific prototype; nonempty vectors return their final
   item directly. */
K last_(K x) {
  if(T(x)>0||s(x)) return k_(x);
  if(!n(x)) return first(x);
  switch(T(x)) {
  case -1: case -2: case -3: case -4: case -8: case -9: case 0:
    return xi_(x,n(x)-1,T(x));
  default:
    return KERR_TYPE;
  }
}

/* Fast `,/x` (join-over / raze) for a general list x (Tx==0, nx>=2).
   The generic fold does r=r,x[i] one item at a time -- O(n^2) copying.
   We always know the result up front and build it in a single O(total) pass:
     - all items general lists (T==0)  -> concatenate (share) into one list
     - all items the same base scalar type (vectors and/or atoms of one of
       int/float/char/sym/long/real) -> one flat vector of that type
     - otherwise -> box every leaf into a general list (gk's join never
       promotes numeric types; it boxes), then knorm -- exactly join()'s
       box path.  This replaces the O(n^2) generic fold even for the mixed
       case (e.g. `(...)`,1.0), which would otherwise re-box and re-copy a
       growing accumulator quadratically.
   Only a subtyped item (dict/function/...) returns 0 to fall through to the
   generic fold, which knows those valences.  Results match join()
   byte-for-byte (including the no-knorm pure-general-list case). */
K raze_(K x) {
  if(E(x)||s(x)||T(x)!=0||n(x)<2) return 0;
  K *pxk=(K*)px(x);
  u64 nx_=n(x),total=0,j=0;
  i8 B=0;          /* common scalar base type among non-list items (0=unset) */
  int hasgl=0;     /* saw a general-list item (T==0) */
  int hasva=0;     /* saw a vector/atom item (T!=0) */
  int mismatch=0;  /* base types disagree */
  i(nx_, K xi=pxk[i];
        if(E(xi)||s(xi)) return 0;       /* error/subtype -> generic */
        i8 t=T(xi);
        total += t<=0 ? n(xi) : 1;
        if(t==0) hasgl=1;
        else { i8 b=t<0?(i8)-t:t; if(!B) B=b; else if(B!=b) mismatch=1; hasva=1; })
  if(hasgl && !hasva) {                  /* pure general lists -> share-concat, no knorm */
    K r=tn(0,total); K *pr=(K*)px(r);
    i(nx_, K xi=pxk[i]; K *pe=px(xi); u64 ni=n(xi); u64 q=0; while(q<ni) pr[j++]=k_(pe[q++]);)
    return r;
  }
  if(!hasgl && !mismatch) switch(B) {    /* uniform scalar base -> flat vector */
  case 1: { K r=tn(1,total); i32  *pr=px(r); i(nx_, K xi=pxk[i]; if(T(xi)<0){u64 ni=n(xi); memcpy(pr+j,px(xi),ni*sizeof(i32));    j+=ni;} else pr[j++]=ik(xi);) return r; }
  case 2: { K r=tn(2,total); double*pr=px(r); i(nx_, K xi=pxk[i]; if(T(xi)<0){u64 ni=n(xi); memcpy(pr+j,px(xi),ni*sizeof(double)); j+=ni;} else pr[j++]=fk(xi);) return r; }
  case 3: { K r=tn(3,total); char  *pr=px(r); i(nx_, K xi=pxk[i]; if(T(xi)<0){u64 ni=n(xi); memcpy(pr+j,px(xi),ni);              j+=ni;} else pr[j++]=ck(xi);) return r; }
  case 4: { K r=tn(4,total); char **pr=px(r); i(nx_, K xi=pxk[i]; if(T(xi)<0){u64 ni=n(xi); memcpy(pr+j,px(xi),ni*sizeof(char*));  j+=ni;} else pr[j++]=sk(xi);) return r; }
  case 8: { K r=tn(8,total); i64   *pr=px(r); i(nx_, K xi=pxk[i]; if(T(xi)<0){u64 ni=n(xi); memcpy(pr+j,px(xi),ni*sizeof(i64));    j+=ni;} else pr[j++]=jk(xi);) return r; }
  case 9: { K r=tn(9,total); float *pr=px(r); i(nx_, K xi=pxk[i]; if(T(xi)<0){u64 ni=n(xi); memcpy(pr+j,px(xi),ni*sizeof(float));  j+=ni;} else pr[j++]=ek(xi);) return r; }
  }
  /* mixed types -> box each leaf into a general list (matches join's box path) */
  { K r=tn(0,total); K *pr=(K*)px(r);
    i(nx_, K xi=pxk[i]; i8 t=T(xi);
          if(t<=0){ u64 ni=n(xi); u64 q=0; while(q<ni) pr[j++]=xi_(xi,q++,t); }
          else pr[j++]=xi_(xi,0,t);)
    return knorm(r); }
}
