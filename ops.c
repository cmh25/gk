#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #define strtok_r strtok_s
#else
  #include <dirent.h>
#endif
#include "ops.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include "x.h"
#include "sym.h"
#include "sort.h"
#include "dict.h"
#include "av.h"
#include "scope.h"
#include "p.h"
#include "fn.h"
#include "mc.h"
#include "adcd.h"
#include "interp.h"
#include "sys.h"

int fret; K *gk; /* function return */
static K *gtake=0;
static size_t gtakei=0;

/* plus, minus, times */
#define MCPMT(F,O) \
K* F(K *a, K *b) { \
  K *r=0; \
  int *pai,*pbi,*pri; \
  double af,bf,*paf,*pbf,*prf; \
  if(at<=0 && bt<=0 && ac!=bc) return kerror("length"); \
  if(at==0 || bt==0) { r=eache(F,a,b); return rt ? r : knorm(r); } \
  switch(at) { \
  case 1: \
    switch(bt) { \
    case  1: r=k1(a1 O b1); break; \
    case  2: r=k2(I2F(a1) O b2); break; \
    case -1: r=kv1(bc); pri=v1(r); pbi=v1(b); DO(rc, *pri++ = a1 O *pbi++); break; \
    case -2: r=kv2(bc); prf=v2(r); pbf=v2(b); af=I2F(a1); DO(rc, *prf++ = af O *pbf++); break; \
    default: return kerror("type"); \
    } break; \
  case 2: \
    switch(bt) { \
    case  1: r=k2(a2 O I2F(b1)); break; \
    case  2: r=k2(a2 O b2); break; \
    case -1: r=kv2(bc); prf=v2(r); pbi=v1(b); DO(rc, *prf++ = a2 O I2F(*pbi); ++pbi); break; \
    case -2: r=kv2(bc); prf=v2(r); pbf=v2(b); DO(rc, *prf++ = a2 O *pbf++); break; \
    default: return kerror("type"); \
    } break; \
  case -1: \
    switch(bt) { \
    case  1: r=kv1(ac); pri=v1(r); pai=v1(a); DO(rc, *pri++ = *pai++ O b1); break; \
    case  2: r=kv2(ac); prf=v2(r); pai=v1(a); DO(rc, *prf++ = I2F(*pai) O b2; ++pai); break; \
    case -1: r=kv1(ac); pri=v1(r); pai=v1(a); pbi=v1(b); DO(rc, *pri++ = *pai++ O *pbi++); break; \
    case -2: r=kv2(ac); prf=v2(r); pai=v1(a); pbf=v2(b); DO(rc, *prf++ = I2F(*pai) O *pbf++; ++pai); break; \
    default: return kerror("type"); \
    } break; \
  case -2: \
    switch(bt) { \
    case  1: bf=I2F(b1); r=kv2(ac); prf=v2(r); paf=v2(a); DO(rc, *prf++ = *paf++ O bf); break; \
    case  2: r=kv2(ac); prf=v2(r); paf=v2(a); DO(rc, *prf++ = *paf++ O b2); break; \
    case -1: r=kv2(ac); prf=v2(r); paf=v2(a); pbi=v1(b); DO(rc, *prf++ = *paf++ O I2F(*pbi); ++pbi); break; \
    case -2: r=kv2(ac); prf=v2(r); paf=v2(a); pbf=v2(b); DO(rc, *prf++ = *paf++ O *pbf++); break; \
    default: return kerror("type"); \
    } break; \
  default: return kerror("type"); \
  } \
  return rt ? r : knorm(r); \
}
MCPMT(plus2_,+)
MCPMT(minus2_,-)
MCPMT(times2_,*)

K* divide2_(K *a, K *b) {
  K *r=0;
  int *pai,*pbi;
  double af,bf,*prf,*paf,*pbf;
  if(at<=0 && bt<=0 && ac!=bc) return kerror("length");
  if(at==0 || bt==0) { r=eache(divide2_,a,b); return rt ? r : knorm(r); }
  switch(at) {
  case 1:
    switch(bt) {
    case  1: r=k2(I2F(a1)/I2F(b1)); break;
    case  2: r=k2(I2F(a1)/b2); break;
    case -1: r=kv2(bc); prf=v2(r); pbi=v1(b); af=I2F(a1); DO(rc,*prf++ = af / I2F(*pbi); ++pbi); break;
    case -2: r=kv2(bc); prf=v2(r); pbf=v2(b); af=I2F(a1); DO(rc,*prf++ = af / *pbf++); break;
    default: return kerror("type");
    } break;
  case 2:
    switch(bt) {
    case  1: r=k2(a2/I2F(b1)); break;
    case  2: r=k2(a2/b2); break;
    case -1: r=kv2(bc); prf=v2(r); pbi=v1(b); DO(rc,*prf++ = a2 / I2F(*pbi); ++pbi); break;
    case -2: r=kv2(bc); prf=v2(r); pbf=v2(b); DO(rc,*prf++ = a2 / *pbf++); break;
    default: return kerror("type");
    } break;
  case -1:
    switch(bt) {
    case  1: r=kv2(ac); prf=v2(r); pai=v1(a); bf=I2F(b1); DO(rc,*prf++ = I2F(*pai) / bf; ++pai); break;
    case  2: r=kv2(ac); prf=v2(r); pai=v1(a); DO(rc,*prf++ = I2F(*pai) / b2; ++pai); break;
    case -1: r=kv2(bc); prf=v2(r); pai=v1(a); pbi=v1(b); DO(rc,*prf++ = I2F(*pai) / I2F(*pbi); ++pai; ++pbi); break;
    case -2: r=kv2(bc); prf=v2(r); pai=v1(a); pbf=v2(b); DO(rc,*prf++ = I2F(*pai) / *pbf++; ++pai); break;
    default: return kerror("type");
    } break;
  case -2:
    switch(bt) {
    case  1: r=kv2(ac); prf=v2(r); paf=v2(a); bf=I2F(b1); DO(rc,*prf++ = *paf++ / bf); break;
    case  2: r=kv2(ac); prf=v2(r); paf=v2(a); DO(rc,*prf++ = *paf++ / b2); break;
    case -1: r=kv2(bc); prf=v2(r); paf=v2(a); pbi=v1(b); DO(rc,*prf++ = *paf++ / I2F(*pbi); ++pbi); break;
    case -2: r=kv2(bc); prf=v2(r); paf=v2(a); pbf=v2(b); DO(rc,*prf++ = *paf++ / *pbf++); break;
    default: return kerror("type");
    } break;
  default: return kerror("type");
  }
  return rt ? r : knorm(r);
}

/* minand, maxor */
#define MCMAMO(F,O,R) \
K* F(K *a, K *b) { \
  K *r=0; \
  int *pri,*pai,*pbi; \
  double af,bf,*prf,*paf,*pbf; \
  char *prc,*pac,*pbc; \
  char **prs,**pas,**pbs; \
  if(at<=0 && bt<=0 && ac!=bc) return kerror("length"); \
  if(at==0 || bt==0) { r=eache(F,a,b); return rt ? r : knorm(r); }\
  switch(at) { \
  case 1: \
    switch(bt) { \
    case  1: r=k1(a1 O b1 ? a1 : b1); break; \
    case  2: af=I2F(a1); r=k2(CMPFF(af,b2)==R ? af : b2); break; \
    case -1: r=kv1(bc); pri=v1(r); pbi=v1(b); DO(rc, *pri++ = a1 O *pbi ? a1 : *pbi; ++pbi) break; \
    case -2: r=kv2(bc); prf=v2(r); pbf=v2(b); af=I2F(a1); DO(rc, *prf++ = CMPFF(af,*pbf)==R ? af : *pbf; ++pbf) break; \
    default: return kerror("type"); \
    } break; \
  case 2: \
    switch(bt) { \
    case  1: bf=I2F(b1); r=k2(CMPFF(a2,bf)==R ? a2 : bf); break; \
    case  2: r=k2(CMPFF(a2,b2)==R ? a2 : b2); break; \
    case -1: r=kv2(bc); prf=v2(r); pbi=v1(b); DO(rc, bf=I2F(*pbi); *prf++ = CMPFF(a2,bf)==R ? a2 : bf; ++pbi) break; \
    case -2: r=kv2(bc); prf=v2(r); pbf=v2(b); DO(rc, *prf++ = CMPFF(a2,*pbf)==R ? a2 : *pbf; ++pbf) break; \
    default: return kerror("type"); \
    } break; \
  case 3: \
    switch(bt) { \
    case  3: r=k3(a3 O b3 ? a3 : b3); break; \
    case -3: r=kv3(bc); prc=v3(r); pbc=v3(b); DO(rc, *prc++ = a3 O *pbc ? a3 : *pbc; ++pbc) break; \
    default: return kerror("type"); \
    } break; \
  case 4: \
    switch(bt) { \
    case  4: r=k4(strcmp(a4,b4) O 0 ? a4 : b4); break; \
    case -4: r=kv4(bc); prs=v4(r); pbs=v4(b); DO(rc, *prs++ = strcmp(a4,*pbs) O 0 ? a4 : *pbs; ++pbs) break; \
    default: return kerror("type"); \
    } break; \
  case -1: \
    switch(bt) { \
    case  1: r=kv1(ac); pri=v1(r); pai=v1(a); DO(rc, *pri++ = *pai O b1 ? *pai : b1; ++pai) break; \
    case  2: r=kv2(ac); prf=v2(r); pai=v1(a); DO(rc, af=I2F(*pai); *prf++ = CMPFF(af,b2)==R ? af : b2; ++pai) break; \
    case -1: r=kv1(bc); pri=v1(r); pai=v1(a); pbi=v1(b); DO(rc, *pri++ = *pai O *pbi ? *pai : *pbi; ++pai; ++pbi) break; \
    case -2: r=kv2(bc); prf=v2(r); pai=v1(a); pbf=v2(b); DO(rc, af=I2F(*pai); *prf++ = CMPFF(af,*pbf)==R ? af : *pbf; ++pai; ++pbf) break; \
    default: return kerror("type"); \
    } break; \
  case -2: \
    switch(bt) { \
    case  1: r=kv2(ac); prf=v2(r); paf=v2(a); bf=I2F(b1); DO(rc, *prf++ = CMPFF(*paf,bf)==R ? *paf : bf; ++paf) break; \
    case  2: r=kv2(ac); prf=v2(r); paf=v2(a); DO(rc, *prf++ = CMPFF(*paf,b2)==R ? *paf : b2; ++paf) break; \
    case -1: r=kv2(bc); prf=v2(r); paf=v2(a); pbi=v1(b); DO(rc, bf=I2F(*pbi); *prf++ = CMPFF(*paf,bf)==R ? *paf : bf; ++paf; ++pbi) break; \
    case -2: r=kv2(bc); prf=v2(r); paf=v2(a); pbf=v2(b); DO(rc, *prf++ = CMPFF(*pbf,*paf)==R ? *pbf : *paf; ++paf; ++pbf) break; \
    default: return kerror("type"); \
    } break; \
  case -3: \
    switch(bt) { \
    case  3: r=kv3(ac); prc=v3(r); pac=v3(a); DO(rc, *prc++ = *pac O b3 ? *pac : b3; ++pac) break; \
    case -3: r=kv3(ac); prc=v3(r); pac=v3(a); pbc=v3(b); DO(rc, *prc++ = *pac O *pbc ? *pac : *pbc; ++pac; ++pbc) break; \
    default: return kerror("type"); \
    } break; \
  case -4: \
    switch(bt) { \
    case  4: r=kv4(ac); prs=v4(r); pas=v4(a); DO(rc, *prs++ = strcmp(*pas,b4) O 0 ? *pas : b4; ++pas) break; \
    case -4: r=kv4(ac); prs=v4(r); pas=v4(a); pbs=v4(b); DO(rc, *prs++ = strcmp(*pas,*pbs) O 0 ? *pas : *pbs; ++pas; ++pbs) break; \
    default: return kerror("type"); \
    } break; \
  default: return kerror("type"); \
  } \
  return rt ? r : knorm(r); \
}
MCMAMO(minand2_,<,-1)
MCMAMO(maxor2_,>,1)

/* less, more, equal */
#define MCLME(F,O,R) \
K* F(K *a, K *b) { \
  K *r=0; \
  int *pri,*pai,*pbi; \
  double *paf,*pbf,af; \
  char *pac,*pbc,**pas,**pbs; \
  if(at<=0 && bt<=0 && ac!=bc) return kerror("length"); \
  if(at==0 || bt==0) { r=eache(F,a,b); return rt ? r : knorm(r); } \
  switch(at) { \
  case  1: \
    if(bt==-1||bt==-2) { r=kv1(bc); pri=v1(r); } \
    switch(bt) { \
    case  1: r=k1(a1 O b1); break; \
    case  2: r=k1(CMPFFT(I2F(a1),b2)==R); break; \
    case -1: pbi=v1(b); DO(rc, *pri++ = a1 O *pbi++) break; \
    case -2: pbf=v2(b); af=I2F(a1); DO(rc, *pri++ = CMPFFT(af,*pbf)==R; ++pbf) break; \
    default: return kerror("type"); \
    } break; \
  case  2: \
    if(bt==-1||bt==-2) { r=kv1(bc); pri=v1(r); } \
    switch(bt) { \
    case  1: r=k1(CMPFFT(a2,I2F(b1))==R); break; \
    case  2: r=k1(CMPFFT(a2,b2)==R); break; \
    case -1: pbi=v1(b); DO(rc, *pri++ = CMPFFT(a2,I2F(*pbi))==R; ++pbi) break; \
    case -2: pbf=v2(b); DO(rc, *pri++ = a2 O *pbf++) break; \
    default: return kerror("type"); \
    } break; \
  case  3: \
    switch(bt) { \
    case  3: r=k1(a3 O b3); break; \
    case -3: r=kv1(bc); pri=v1(r); pbc=v3(b); DO(rc, *pri++ = a3 O *pbc++) break; \
    default: return kerror("type"); \
    } break; \
  case  4: \
    switch(bt) { \
    case  4: r=k1(strcmp(v3(a),v3(b)) O 0); break; \
    case -4: r=kv1(bc); pri=v1(r); pbs=v4(b); DO(rc, *pri++ = strcmp(v3(a),*pbs++) O 0) break; \
    default: return kerror("type"); \
    } break; \
  case -1: \
    if(bt==1||bt==2||bt==-1||bt==-2) {  r=kv1(ac); pri=v1(r); pai=v1(a); } \
    switch(bt) { \
    case  1: DO(rc, *pri++ = *pai++ O b1) break; \
    case  2: DO(rc, *pri++ = CMPFFT(I2F(*pai),b2)==R; pai++) break; \
    case -1: pbi=v1(b); DO(rc, *pri++ = *pai++ O *pbi++) break; \
    case -2: pbf=v2(b); DO(rc, *pri++ = CMPFFT(I2F(*pai),*pbf)==R; ++pai; ++pbf) break; \
    default: return kerror("type"); \
    } break; \
  case -2: \
    if(bt==1||bt==2||bt==-1||bt==-2) {  r=kv1(ac); pri=v1(r); paf=v2(a); } \
    switch(bt) { \
    case  1: DO(rc, *pri++ = CMPFFT(*paf,I2F(b1))==R; ++paf) break; \
    case  2: DO(rc, *pri++ = CMPFFT(*paf,b2)==R; ++paf) break; \
    case -1: pbi=v1(b); DO(rc, *pri++ = CMPFFT(*paf,I2F(*pbi))==R; ++paf; ++pbi) break; \
    case -2: pbf=v2(b); DO(rc, *pri++ = CMPFFT(*paf,*pbf)==R; ++paf; ++pbf) break; \
    default: return kerror("type"); \
    } break; \
  case -3: \
    if(bt==3||bt==-3) { r=kv1(ac); pri=v1(r); pac=v3(a); } \
    switch(bt) { \
    case  3: DO(rc, *pri++ = *pac++ O b3) break; \
    case -3: pbc=v3(b); DO(rc, *pri++ = *pac++ O *pbc++) break; \
    default: return kerror("type"); \
    } break; \
  case -4: \
    if(bt==4||bt==-4) { r=kv1(ac); pri=v1(r); pas=v4(a); } \
    switch(bt) { \
    case  4: DO(rc, *pri++ = strcmp(*pas++,v3(b)) O 0) break; \
    case -4: pbs=v4(b); DO(rc, *pri++ = strcmp(*pas++,*pbs++) O 0) break; \
    default: return kerror("type"); \
    } break; \
  default: return kerror("type"); \
  } \
  return rt ? r : knorm(r); \
}
MCLME(less2_,<,-1)
MCLME(more2_,>,1)
MCLME(equal2_,==,0)

MC2D(power2_,pow,pow,2,2,v2,v2)

K* modrot2_(K *a, K *b) {
  K *r=0;
  double rd;
  int ri;

  switch(at) {
  case  1:
    switch(bt) {
    case  1: r=k1(0); if(!b1) r->i=INT_MIN; else { ri = a1 % b1; r->i = ri < 0 ? ri+b1 : ri; } break;
    case  2: r=k2(0); rd = fmod(a1,b2); r->f = rd < 0 ? rd+b2 : rd; break;
    case  0: r=kv0(bc); DO(rc, ri=(i+a1)%(int)bc; if(ri<0)ri=ri+bc; v0(r)[i] = kcp(v0(b)[ri]);) break;
    case -1: r=kv1(bc); DO(rc, ri=(i+a1)%(int)bc; if(ri<0)ri=ri+bc; v1(r)[i]=v1(b)[ri];) break;
    case -2: r=kv2(bc); DO(rc, ri=(i+a1)%(int)bc; if(ri<0)ri=ri+bc; v2(r)[i]=v2(b)[ri];) break;
    case -3: r=kv3(bc); DO(rc, ri=(i+a1)%(int)bc; if(ri<0)ri=ri+bc; v3(r)[i]=v3(b)[ri];) break;
    case -4: r=kv4(bc); DO(rc, ri=(i+a1)%(int)bc; if(ri<0)ri=ri+bc; v4(r)[i]=v4(b)[ri];) break;
    default: return kerror("type");
    } break;
  case  2:
    switch(bt) {
    case 1: r=k2(0); rd = fmod(a2,b1); r->f = rd < 0 ? rd+b1 : rd; break;
    case 2: r=k2(0); rd = fmod(a2,b2); r->f = rd < 0 ? rd+b2 : rd; break;
    default: return kerror("int");
    } break;
  case -1:
    switch(bt) {
    case 1: r=kv1(ac); if(!b1) DO(rc,v1(r)[i]=INT_MIN) else DO(rc, ri=v1(a)[i]%b1; v1(r)[i]=ri<0?ri+b1:ri) break;
    case 2: r=kv2(ac); DO(rc, rd=fmod(v1(a)[i],b2); v2(r)[i]=rd<0?rd+b2:rd) break;
    default: return kerror("int");
    } break;
  case -2:
    switch(bt) {
    case 1: r=kv2(ac); DO(rc, rd=fmod(v2(a)[i],b1); if(rd==-0)rd=0; v2(r)[i]=rd<0?rd+b1:rd) break;
    case 2: r=kv2(ac); DO(rc, rd=fmod(v2(a)[i],b2); if(rd==-0)rd=0; v2(r)[i]=rd<0?rd+b2:rd) break;
    default: return kerror("int");
    } break;
  default: return kerror("int");
  }
  return rt ? r : knorm(r);
}

K* match2_(K *a, K *b) {
  return k1(!kcmpr(a,b));
}

K* join2_(K *a, K *b) {
  K *r=0;
  int j=0;

  if(at<=0&&bt<=0) VSIZE((int)ac+(int)bc)
  else if(at<=0&&bt>0) VSIZE((int)ac+1)
  else if(at>0&&bt<=0) VSIZE(1+(int)bc)

  if(at>0 && bt>0) { r=kv0(2); v0(r)[0]=kcp(a); v0(r)[1]=kcp(b); }
  else if(at>0 && bt==0) { r=kv0(1+bc); v0(r)[0]=kcp(a); DO(bc,v0(r)[i+1]=kcp(v0(b)[i])); }
  else if(at==0 && bt>0) { r=kv0(ac+1); DO(ac,v0(r)[i]=kcp(v0(a)[i])) v0(r)[rc-1]=kcp(b); }
  else if(at>0 && bt<0 && bt!=-at) {
    r=kv0(1+bc); v0(r)[0] = kcp(a);
    switch(bt) {
    case -1: DO(bc, v0(r)[i+1] = k1(v1(b)[i])) break;
    case -2: DO(bc, v0(r)[i+1] = k2(v2(b)[i])) break;
    case -3: DO(bc, v0(r)[i+1] = k3(v3(b)[i])) break;
    case -4: DO(bc, v0(r)[i+1] = k4(v4(b)[i])) break;
    default: return kerror("type");
    }
  }
  else if(at>0 && bt==-at) {
    switch(bt) {
    case -1: r=kv1(1+bc); v1(r)[0] = a1; DO(bc, v1(r)[i+1] = v1(b)[i]) break;
    case -2: r=kv2(1+bc); v2(r)[0] = a2; DO(bc, v2(r)[i+1] = v2(b)[i]) break;
    case -3: r=kv3(1+bc); v3(r)[0] = a3; DO(bc, v3(r)[i+1] = v3(b)[i]) break;
    case -4: r=kv4(1+bc); v4(r)[0] = a4; DO(bc, v4(r)[i+1] = v4(b)[i]) break;
    default: return kerror("type");
    }
  }
  else if(at<0 && bt==-at) {
    switch(bt) {
    case  1: r=kv1(ac+1); DO(ac, v1(r)[i] = v1(a)[i]) v1(r)[rc-1] = b1; break;
    case  2: r=kv2(ac+1); DO(ac, v2(r)[i] = v2(a)[i]) v2(r)[rc-1] = b2; break;
    case  3: r=kv3(ac+1); DO(ac, v3(r)[i] = v3(a)[i]) v3(r)[rc-1] = b3; break;
    case  4: r=kv4(ac+1); DO(ac, v4(r)[i] = v4(a)[i]) v4(r)[rc-1] = b4; break;
    default: return kerror("type");
    }
  }
  else if(at<=0 && bt<=0 && at!=bt) {
    r=kv0(ac+bc);
    switch(at) {
      case  0: DO(ac, v0(r)[j++] = kcp(v0(a)[i])) break;
      case -1: DO(ac, v0(r)[j++] = k1(v1(a)[i])) break;
      case -2: DO(ac, v0(r)[j++] = k2(v2(a)[i])) break;
      case -3: DO(ac, v0(r)[j++] = k3(v3(a)[i])) break;
      case -4: DO(ac, v0(r)[j++] = k4(v4(a)[i])) break;
      default: return kerror("type");
    }
    switch(bt) {
      case  0: DO(bc, v0(r)[j++] = kcp(v0(b)[i])) break;
      case -1: DO(bc, v0(r)[j++] = k1(v1(b)[i])) break;
      case -2: DO(bc, v0(r)[j++] = k2(v2(b)[i])) break;
      case -3: DO(bc, v0(r)[j++] = k3(v3(b)[i])) break;
      case -4: DO(bc, v0(r)[j++] = k4(v4(b)[i])) break;
      default: return kerror("type");
    }
  }
  else if(at<0 && bt>0) {
    switch(at) {
    case -1: r=kv0(ac+1); DO(ac, v0(r)[i] = k1(v1(a)[i])) break;
    case -2: r=kv0(ac+1); DO(ac, v0(r)[i] = k2(v2(a)[i])) break;
    case -3: r=kv0(ac+1); DO(ac, v0(r)[i] = k3(v3(a)[i])) break;
    case -4: r=kv0(ac+1); DO(ac, v0(r)[i] = k4(v4(a)[i])) break;
    default: return kerror("type");
    }
    v0(r)[rc-1] = kcp(b);
  }
  else if(at<=0 && bt<=0 && at==bt) {
    switch(at) {
    case  0: r=kv0(ac+bc); DO(ac, v0(r)[j++] = kcp(v0(a)[i])); DO(bc, v0(r)[j++] = kcp(v0(b)[i])) break;
    case -1: r=kv1(ac+bc); DO(ac, v1(r)[j++] = v1(a)[i]); DO(bc, v1(r)[j++] = v1(b)[i]) break;
    case -2: r=kv2(ac+bc); DO(ac, v2(r)[j++] = v2(a)[i]); DO(bc, v2(r)[j++] = v2(b)[i]) break;
    case -3: r=kv3(ac+bc); DO(ac, v3(r)[j++] = v3(a)[i]); DO(bc, v3(r)[j++] = v3(b)[i]) break;
    case -4: r=kv4(ac+bc); DO(ac, v4(r)[j++] = v4(a)[i]); DO(bc, v4(r)[j++] = v4(b)[i]) break;
    default: return kerror("type");
    }
  }
  return rt ? r : knorm(r);
}

K* take2_(K *a, K *b) {
  K *r=0,*a0=0,*a_1=0,*bb=0,*rr=0;
  int n=0,gt=0,ai=0;

  if(at<=0 && !ac) {
    switch(bt) {
    case  1: case  2: case  3: case  4: case  5: case  6:
    case  7: case 27: case 37: case 67: case 77: case 87: r=kcp(b); break;
    case  0: if(bc) r=kcp(v0(b)[0]); else r=null; break;
    case -1: if(bc) r=k1(v1(b)[0]); else r=null; break;
    case -2: if(bc) r=k2(v2(b)[0]); else r=null; break;
    case -3: if(bc) r=k3(v1(b)[0]); else r=k3(' '); break;
    case -4: if(bc) r=k4(v4(b)[0]); else r=k4(""); break;
    }
    return r;
  }

  if(at==1&&a1<0) bb=reverse_(b);
  else bb=kref(b);
  ai=abs(a->i);
  switch(at) {
  case 1:
    VSIZE(a1);
    switch(bb->t) {
    case  1: r=kv1(ai); DO(ai, v1(r)[i] = bb->i); break;
    case  2: r=kv2(ai); DO(ai, v2(r)[i] = bb->f); break;
    case  3: r=kv3(ai); DO(ai, v3(r)[i] = bb->i); break;
    case  4: r=kv4(ai); DO(ai, v4(r)[i] = bb->v;); break;
    case  5: r=kv0(ai); DO(ai, v0(r)[i] = kcp(bb)); break;
    case  6: r=kv0(ai); DO(ai, v0(r)[i] = kcp(bb)); break;
    case  7: case 27: case 37: case 67: case 77: case 87: r=kv0(ai); DO(ai, v0(r)[i] = kcp(bb)); break;
    case  0: r=kv0(ai);
      if(bc&&!gtake){DO(ai, v0(r)[i] = kcp(v0(bb)[i%bc]));}
      else if(bc) DO(ai, if(gtakei==gtake->c) gtakei=0; v0(r)[i]=kcp(v0(gtake)[gtakei++]))
      else DO(ai, v0(r)[i]=null) break;
    case -1: r=kv1(ai);
      if(bc&&!gtake){DO(ai/bc, memcpy(&v1(r)[n], v1(bb), sizeof(int)*bc); n+=bc;); DO(ai%bc, v1(r)[n++] = v1(bb)[i]);}
      else if(bc) DO(ai, if(gtakei==gtake->c) gtakei=0; v1(r)[i]=v1(gtake)[gtakei++])
      else DO(ai, v1(r)[i]=0) break;
    case -2: r=kv2(ai);
      if(bc&&!gtake){DO(ai/bc, memcpy(&v2(r)[n], v2(bb), sizeof(double)*bc); n+=bc;); DO(ai%bc, v2(r)[n++] = v2(bb)[i]);}
      else if(bc) DO(ai, if(gtakei==gtake->c) gtakei=0; v2(r)[i]=v2(gtake)[gtakei++])
      else DO(ai, v2(r)[i]=0.0) break;
    case -3: r=kv3(ai);
      if(bc&&!gtake){DO(ai/bc, memcpy(&v3(r)[n], v3(bb), sizeof(char)*bc); n+=bc;); DO(ai%bc, v3(r)[n++] = v3(bb)[i]);}
      else if(bc) {DO(ai, if(gtakei==gtake->c) gtakei=0; v3(r)[i]=v3(gtake)[gtakei++]);}
      else DO(ai, v3(r)[i]=' ') break;
    case -4: r=kv4(ai);
      if(bc&&!gtake){DO(ai, v4(r)[i] = v4(bb)[i%bc]);}
      else if(bc) DO(ai, if(gtakei==gtake->c) gtakei=0; v4(r)[i]=v4(gtake)[gtakei++])
      else DO(ai, v4(r)[i]="") break;
    default: return kerror("type");
    } break;
  case -1:
    DO(ac,VSIZE(v1(a)[i]));
    if(!gtake) {gtake=bb;gt=1;gtakei=0;}
    a0=drop2_(one,a); EC(a0);
    if(a0->c==1) {a_1=k1(v1(a0)[0]); kfree(a0);}
    else a_1=a0;
    r=kv0(v1(a)[0]); DO(rc,v0(r)[i]=take2_(a_1,bb); EC(v0(r)[i]));
    kfree(a_1);
    if(gt) {gtake=0;gtakei=0;}
    break;
  default: return kerror("int");
  }
  kfree(bb);
  if(at==1&&a1<0) rr=reverse_(r);
  else rr=kref(r);
  kfree(r);
  return rr->t ? rr : knorm(rr);
}

static K* splitc(char *s, char c) {
  K *r=0;
  int i=0,n=0;
  char *t=s,*u=s;
  if(!*s) return kv0(0);
  DO(strlen(s),if(s[i]==c)++n)
  r=kv0(1+n);
  while(*t) {
    if(*t==c) { v0(r)[i++]=knew(-3,t-u,u,0,0,0); u=++t; }
    else ++t;
  }
  v0(r)[i]=knew(-3,t-u,u,0,0,0);
  return r;
}
K* drop2_(K *a, K *b) {
  K *r=0,*r2=0;
  int i,j,m,p,q;
  char *s;

  if(at==-1) {
    DO(ac,if(v1(a)[i]>(int)bc)return kerror("length"))
    DO(ac-1,if(v1(a)[i]>v1(a)[i+1])return kerror("domain"))
  }

  switch(at) {
  case -1:
    switch(bt) {
    case -1: r=kv0(ac); DO(ac,p=v1(a)[i]; j=i==(int)ac-1?(int)bc:v1(a)[i+1]; v0(r)[i]=r2=kv1(j-p); for(q=0,m=p;m<j;m++) v1(r2)[q++]=v1(b)[m]) break;
    case -3: r=kv0(ac); DO(ac,p=v1(a)[i]; j=i==(int)ac-1?(int)bc:v1(a)[i+1]; v0(r)[i]=r2=kv3(j-p); for(q=0,m=p;m<j;m++) v3(r2)[q++]=v3(b)[m];) break;
    default: return kerror("type");
    } break;
  case 1:
    switch(bt) {
    case  1: case 2: case 3: case 4: case 5: case 6:
    case  7: case 27: case 37: case 67: case 77: case 87: r=kcp(b); break;
    case  0:
      if((unsigned int)abs(a1)>=bc) r=kv0(0);
      else if(!a1) r=kcp(b);
      else if(a1>0) { r=kv0(bc-a1); DO(bc-a1,v0(r)[i]=kcp(v0(b)[i+a1])) }
      else if(a1<0) { r=kv0(bc+a1); DO(bc+a1,v0(r)[i]=kcp(v0(b)[i])) }
      else return kerror("type");
      break;
    case -1:
      if((unsigned int)abs(a1)>=bc) r=kv1(0);
      else if(!a1) r=kcp(b);
      else if(a1>0) { r=kv1(bc-a1); DO(bc-a1,v1(r)[i]=v1(b)[i+a1]) }
      else if(a1<0) { r=kv1(bc+a1); DO(bc+a1,v1(r)[i]=v1(b)[i]) }
      else return kerror("type");
      break;
    case -2:
      if((unsigned int)abs(a1)>=bc) r=kv2(0);
      else if(!a1) r=kcp(b);
      else if(a1>0) { r=kv2(bc-a1); DO(bc-a1,v2(r)[i]=v2(b)[i+a1]) }
      else if(a1<0) { r=kv2(bc+a1); DO(bc+a1,v2(r)[i]=v2(b)[i]) }
      else return kerror("type");
      break;
    case -3:
      if((unsigned int)abs(a1)>=bc) r=kv3(0);
      else if(!a1) r=kcp(b);
      else if(a1>0) { r=kv3(bc-a1); DO(bc-a1,v3(r)[i]=v3(b)[i+a1]) }
      else if(a1<0) { r=kv3(bc+a1); DO(bc+a1,v3(r)[i]=v3(b)[i]) }
      else return kerror("type");
      break;
    case -4:
      if((unsigned int)abs(a1)>=bc) r=kv4(0);
      else if(!a1) r=kcp(b);
      else if(a1>0) { r=kv4(bc-a1); DO(bc-a1,v4(r)[i]=v4(b)[i+a1]) }
      else if(a1<0) { r=kv4(bc+a1); DO(bc+a1,v4(r)[i]=v4(b)[i]) }
      else return kerror("type");
      break;
    default: return kerror("type");
    } break;
  case 2:
    switch(bt) {
    case 1:
      if(a2==0.0) { r=kcp(a); r->f=I2F(b1); }
      else if(isnan(a2)||isinf(a2)) r=k2(NAN);
      else if(a2<0.00) { r=kcp(a); for(i=1;;i++) if(CMPFF(-a2*i,I2F(b1))>=0) { r->f=-a2*i; break; } }
      else { r=kcp(a); for(i=1;;i++) if(CMPFF(a2*i,I2F(b1))>0) { r->f=a2*(i-1); break; } }
      break;
    case  2:
      if(isnan(a2)||isinf(a2)) r=k2(NAN);
      else r=k2(rint(b2));
      break;
    case -1:
      if(a2==0.0) { r=kv2(bc); DO(bc,v2(r)[i]=I2F(v1(b)[i])) }
      else if(isnan(a2)||isinf(a2)) { r=kv2(bc); DO(bc,v2(r)[i]=NAN) }
      else if(a2<0.00) { r=kv2(bc); DO(bc,for(j=1;;j++) if(CMPFF(-a2*j,I2F(v1(b)[i]))>=0) { v2(r)[i]=-a2*j; break; }) }
      else { r=kv2(bc); DO(bc,for(j=1;;j++) if(CMPFF(a2*j,I2F(v1(b)[i]))>0) { v2(r)[i]=a2*(j-1); break; }) }
      break;
    case -2:
      if(a2==0.0) { r=kv2(bc); DO(bc,v2(r)[i]=v2(b)[i]) }
      else if(isnan(a2)||isinf(a2)) { r=kv2(bc); DO(bc,v2(r)[i]=NAN) }
      else if(a2<0.00) { r=kv2(bc); DO(bc,for(j=1;;j++) if(CMPFF(-a2*j,v2(b)[i])>=0) { v2(r)[i]=-a2*j; break; }) }
      else { r=kv2(bc); DO(bc,for(j=1;;j++) if(CMPFF(a2*j,v2(b)[i])>0) { v2(r)[i]=a2*(j-1); break; }) }
      break;
    default: return kerror("type");
    } break;
  case 3:
    switch(bt) {
    case -3:
      s=xstrndup(v3(b),bc);
      r=splitc(s,a1);
      xfree(s);
      break;
    case  4:
      s=xstrdup(b4);
      r=splitc(s,a1);
      xfree(s);
      break;
    default: return kerror("type");
    } break;
  default: return kerror("type");
  }
  return rt ? r : knorm(r);
}

static K* form2w(char *t, int w) {
  K *r=0;
  int n,l,m;
  char *s=t;
  l=strlen(t); m=abs(w); n=abs(l-m);
  if(m>l) {
    s=xmalloc(m+1);
    if(w<0) sprintf(s,"%s%*s",t,n,"");
    else if(w>0) sprintf(s,"%*s%s",n,"",t);
    r=knew(-3,strlen(s),s,0,0,0);
    xfree(s);
  } else if(m<l) {
    *s=0; DO(m,strcat(s,"*"))
    r=knew(-3,strlen(s),s,0,0,0);
  } else r=knew(-3,l,t,0,0,0);
  return r;
}
K* form2_(K *a, K *b) {
  K *r=0,*p=0,*q=0;
  int n,l,m,x,y;
  char *c,t[2048],*s=t;

  if(at<=0 && bt<=0 && ac!=bc && at!=-3 && bt!=-3) return kerror("length");

  switch(at) {
  case 1:
    switch(bt) {
    case  1:
    case  2:
      if(bt==1) sprintf(t,"%d",b1);
      else sprintf(t,"%0.*g",precision,b2);
      r=form2w(t,a1);
      break;
    case  3:
      if(!a1) { if(b1<48||b1>57) return kerror("domain"); r=k1(b1-48); break; }
      sprintf(t,"%c",b1);
      r=form2w(t,a1);
      break;
    case  4: sprintf(t,"%s",b4); r=form2w(t,a1); break;
    case  5: sprintf(t,".(..)"); r=form2w(t,a1); break;
    case  0: r=kv0(bc); DO(bc,v0(r)[i]=form2_(a,v0(b)[i]); EC(v0(r)[i])) break;
    case -1:
    case -2: p=kmix(b); r=kv0(p->c); DO(p->c,v0(r)[i]=form2_(a,v0(p)[i]); EC(v0(r)[i])); kfree(p); break;
    case -3:
      b->v=xrealloc(b->v,bc+1); v3(b)[bc]=0;
      if(!a1) {
        c=v3(b); DO(bc, if(c[i]<48||c[i]>57) return kerror("domain"))
        r=k1(xatoi(c));
        break;
      }
      l=bc; m=abs(a1); n=abs(l-m);
      if(m>l) {
        s=xmalloc(m+1);
        if(a1<0) sprintf(s,"%s%*s",v3(b),n,"");
        else if(a1>0) sprintf(s,"%*s%s",n,"",v3(b));
        r=knew(-3,strlen(s),s,0,0,0);
        xfree(s);
      } else if(m<l) {
        s=xstrdup(v3(b));
        if(a1<0) { s[m]=0; r = knew(-3,m,s,0,0,0); }
        else r=knew(-3,m,s+l-m,0,0,0);
        xfree(s);
      } else {
        if(a1) r=knew(-3,l,v3(b),0,0,0);
        else return kerror("domain");
      }
      break;
    case -4: p=kmix(b); r=kv0(p->c); DO(p->c,v0(r)[i]=form2_(a,v0(p)[i]); EC(v0(r)[i])); kfree(p); break;
    default: return kerror("type");
    } break;
  case 2:
    switch(bt) {
    case  1:
    case  2:
      if(bt==1) b2=I2F(b1);
      sprintf(t,"%g",roundf(a2*10)/10); s=strchr(t,'.');
      x=a2; m=abs(x); y=s?s[1]-48:0;
      if(x<0||*t=='-') sprintf(t,"%s%0.*e",b2<0?"":" ",y,b2);
      else if(isinf(b2)) *t=0;
      else if(isnan(b2)) *t=0;
      else sprintf(t,"%0.*f",y,b2);
      l=strlen(t); s=xmalloc(l+m+1);
      if(!m) sprintf(s,"%s",t);
      else if(m<l) { *s=0; DO(m,strcat(s,"*")) }
      else if(x<0) sprintf(s,"%s%*s",t,m-l,"");
      else sprintf(s,"%*s%s",m-l,"",t);
      r=knew(-3,strlen(s),s,0,0,0);
      xfree(s);
      break;
    case  3: if(b3<48||b3>57) return kerror("domain"); sprintf(s,"%c",b3); r=k2(xstrtod(s)); break;
    case  4: return kerror("type");
    case  0: r=kv0(bc); DO(bc,v0(r)[i]=form2_(a,v0(b)[i]); EC(v0(r)[i])) break;
    case -1:
    case -2: p=kmix(b); r=kv0(p->c); DO(p->c,v0(r)[i]=form2_(a,v0(p)[i]); EC(v0(r)[i])); kfree(p); break;
    case -3: s=xmalloc(bc+1); DO(bc,s[i]=v3(b)[i]); s[bc]=0; r=k2(xstrtod(s)); xfree(s); break;
    default: return kerror("type");
    } break;
  case 3:
    switch(bt) {
    case  3:
    case -3: r=kcp(b); break;
    case  0: r=kv0(bc); DO(bc,v0(r)[i]=form2_(a,v0(b)[i]); EC(v0(r)[i])) break;
    default: return kerror("type");
    } break;
  case 4:
    if(strlen(a4)) return kerror("nonce");
    switch(bt) {
    case  3: sprintf(s,"%c",b3); r=knew(4,0,s,0,0,0); break;
    case  0: r=kv0(bc); DO(bc,v0(r)[i]=form2_(a,v0(b)[i]); EC(v0(r)[i])) break;
    case -3: r=k4(0); s=xmalloc(bc+1); DO(bc,s[i]=v3(b)[i]); s[bc]=0; r->v=sp(s); xfree(s); break;
    default: return kerror("type");
    } break;
  case 37: /* {} only */
    switch(bt) {
    case  0: r=kv0(bc); DO(bc,v0(r)[i]=form2_(a,v0(b)[i]); EC(v0(r)[i])) break;
    case -3: s=xmalloc(bc+1); DO(bc,s[i]=v3(b)[i]); s[bc]=0; r=knew(7,0,fnnew(s),0,0,0); xfree(s); break;
    default: return kerror("type");
    } break;
  case 0:
    switch(bt) {
    case  3:
    case -3: r=kv0(ac); DO(ac,v0(r)[i]=form2_(v0(a)[i],b); EC(v0(r)[i])) break;
    case  0: r=kv0(ac); DO(ac,v0(r)[i]=form2_(v0(a)[i],v0(b)[i]); EC(v0(r)[i])) break;
    default: return kerror("type");
    } break;
  case -1:
    switch(bt) {
    case  1:
    case  2:
    case  3:
    case  4:
    case  5: r=kv0(ac); p=kmix(a); DO(ac,v0(r)[i]=form2_(v0(p)[i],b); EC(v0(r)[i])); kfree(p); break;
    case  0: r=kv0(ac); p=kmix(a); DO(ac,v0(r)[i]=form2_(v0(p)[i],v0(b)[i]); EC(v0(r)[i])); kfree(p); break;
    case -1:
    case -2: r=kv0(ac); p=kmix(a); q=kmix(b); DO(ac,v0(r)[i]=form2_(v0(p)[i],v0(q)[i]); EC(v0(r)[i])); kfree(p); kfree(q); break;
    case -3: r=kv0(ac); p=kmix(a); DO(ac,v0(r)[i]=form2_(v0(p)[i],b); EC(v0(r)[i])); kfree(p); break;
    case -4: r=kv0(ac); p=kmix(a); q=kmix(b); DO(ac,v0(r)[i]=form2_(v0(p)[i],v0(q)[i]); EC(v0(r)[i])); kfree(p); kfree(q); break;
    default: return kerror("type");
    } break;
  case -2:
    switch(bt) {
    case  3: r=kv0(ac); p=kmix(a); DO(ac,v0(r)[i]=form2_(v0(p)[i],b); EC(v0(r)[i])); kfree(p); break;
    case  0: r=kv0(ac); p=kmix(a); DO(ac,v0(r)[i]=form2_(v0(p)[i],v0(b)[i]); EC(v0(r)[i])); kfree(p); break;
    case -1:
    case -2: r=kv0(ac); p=kmix(a); q=kmix(b); DO(ac,v0(r)[i]=form2_(v0(p)[i],v0(q)[i]); EC(v0(r)[i])); kfree(p); kfree(q); break;
    case -3: r=kv0(ac); p=kmix(a); DO(ac,v0(r)[i]=form2_(v0(p)[i],b); EC(v0(r)[i])); kfree(p); break;
    default: return kerror("type");
    } break;
  case -3:
    switch(bt) {
    case  3: r=kv3(1); v3(r)[0]=b3; break;
    case  0: r=kv0(bc); DO(bc,v0(r)[i]=form2_(a,v0(b)[i]); EC(v0(r)[i])) break;
    case -3: r=kcp(b); break;
    default: return kerror("type");
    } break;
  case -4:
    switch(bt) {
    case  3:
    case -3: r=kv0(ac); p=kmix(a); DO(ac,v0(r)[i]=form2_(v0(p)[i],b); EC(v0(r)[i])); kfree(p); break;
    case  0: r=kv0(ac); p=kmix(a); DO(ac,v0(r)[i]=form2_(v0(p)[i],v0(b)[i]); EC(v0(r)[i])); kfree(p); break;
    default: return kerror("type");
    } break;
  default: return kerror("type");
  }

  return rt ? r : knorm(r);
}

K* find2_(K *a, K *b) {
  K *r=0;

  if(at==1) return draw2_(a,b);
  if(at>0) return kerror("domain");

  r=k1(ac);
  switch(at) {
  case  0: DO(ac,if(!kcmpr(v0(a)[i],b)){r->i=i; break;}) break;
  case -1: if(bt==1) DO(ac,if(v1(a)[i]==b1){r->i=i; break;}) break;
  case -2: if(bt==2) DO(ac,if(!CMPFFT(v2(a)[i],b2)){r->i=i; break;}) break;
  case -3: if(bt==3) DO(ac,if(v3(a)[i]==b3){r->i=i; break;}) break;
  case -4: if(bt==4) DO(ac,if(!strcmp(v4(a)[i],v3(b))){r->i=i; break;}) break;
  default: return kerror("type");
  }
  return rt ? r : knorm(r);
}

K* at2s_(K *a, K *b) {
  K *r=0,*p=0,*q=0;
  unsigned int bi,n;

  if(!(at==-1 && bt==-1)) {
    if(at!=6) {
      if(bt==1 && b1>=(int)ac) return kerror("index");
      if(bt==-1) DO(bc, if(v1(b)[i]>=(int)ac) return kerror("index"));
    }
  }

  if(bt==11) bt=0;

  switch(at) {
  case 6: r=bt==6?kv0(0):kcp(b); break;
  case 5:
  case 0:
    switch(bt) {
    case 0:
      bi=0;
      p=v0(b)[bi];
      if(p->t==16) p=null;
      if(p->t<=0) n=p->c; else if(p->t==6) n=ac; else n=1;
      r=at2_(a,v0(b)[bi++]);
      if(bi==bc) break;
      else if(bi==bc-1) {
        if(n==1) q = at2_(r,v0(b)[bi]);
        else { q=kv0(rc); DO(q->c,v0(q)[i]=at2_(v0(r)[i],v0(b)[bi]); EC(v0(q)[i])) }
        kfree(r); r=q;
      }
      else {
        p=kv0(bc-1);
        DO(p->c,v0(p)[i]=kcp(v0(b)[i+1]))
        if(n==1) { q=at2s_(r,p); EC(q); }
        else { q=kv0(rc); DO(q->c,v0(q)[i]=at2s_(v0(r)[i],p); EC(v0(q)[i])) }
        kfree(r); r=q; kfree(p);
      } break;
    default: return kerror("type");
    } break;
  default: return kerror("rank");
  }
  return rt ? r : knorm(r);
}

K* at2_(K *a, K *b) {
  K *r=0,*q=0,*bm=0;
  fn *f;

  if(!(at==-1 && bt==-1)) {
    if(at!=6 && at!=27) {
      if(at!=37 && bt==1 && b1>=(int)ac) return kerror("index");
      if(bt==-1) DO(bc, if(v1(b)[i] >= (int)ac) return kerror("index"));
    }
  }

  if(bt==16) b=null;
  if(bt==6&&at!=5) { r=kcp(a); return r; }
  if(at!=37&&bt==0) { r=kv0(bc); DO(rc,v0(r)[i]=at2_(a,v0(b)[i]); EC(v0(r)[i])); }
  else if(bt==11) { r=at2s_(a,b); EC(r); }
  else {
  if(at<=0&&bt==1&&(b1<0||b1>=(int)ac)) return kerror("index");
  switch(at) {
  case 37: bm=bt<0?kmix(b):b; r=fne2(a,bm,0); if(bm!=b) kfree(bm); break;
  case  5:
    switch(bt) {
    case  4: r=dget(a5,b4);if(!r)r=null; break;
    case  6: r=dvals(a5); break;
    case -4: r=kv0(bc);DO(bc,q=dget(a5,v4(b)[i]);if(!q)q=null;v0(r)[i]=q) break;
    case 99: r=dget(a5,b->v);if(!r)r=kerror("value"); break;
    default: return kerror("type");
    } break;
  case  6: r=bt==6?kv0(0):kcp(b); break;
  case 27: f=a->v; r=avdo(a,f->l,b,f->av); if(!r) r=dt2[f->i](f->l,b); break;
  case  0:
    switch(bt) {
    case  1: r=kcp(v0(a)[b1]); break;
    case -1: r=kv0(bc); DO(rc,if(v1(b)[i]<0||v1(b)[i]>=(int)ac){kfree(r);return kerror("index");} v0(r)[i]=kcp(v0(a)[v1(b)[i]])); break;
    default: return kerror("type");
    } break;
  case -1:
    switch(bt) {
    case  1: r=k1(v1(a)[b1]); break;
    case -1: r=kv1(bc); DO(rc,if(v1(b)[i]<0||v1(b)[i]>=(int)ac){kfree(r);return kerror("index");} v1(r)[i]=v1(a)[v1(b)[i]]); break;
    default: return kerror("type");
    } break;
  case -2:
    switch(bt) {
    case  1: r=k2(v2(a)[b1]); break;
    case -1: r=kv2(bc); DO(rc,if(v1(b)[i]<0||v1(b)[i]>=(int)ac){kfree(r);return kerror("index");} v2(r)[i]=v2(a)[v1(b)[i]]); break;
    default: return kerror("type");
    } break;
  case -3:
    switch(bt) {
    case  1: r=k3(v3(a)[b1]); break;
    case -1: r=kv3(bc); DO(rc,if(v1(b)[i]<0||v1(b)[i]>=(int)ac){kfree(r);return kerror("index");} v3(r)[i]=v3(a)[v1(b)[i]]); break;
    default: return kerror("type");
    } break;
  case -4:
    switch(bt) {
    case  1: r=k4(v4(a)[b1]); break;
    case -1: r=kv4(bc); DO(rc,if(v1(b)[i]<0||v1(b)[i]>=(int)ac){kfree(r);return kerror("index");} v4(r)[i]=v4(a)[v1(b)[i]]); break;
    default: return kerror("type");
    } break;
  default: return kerror("type");
  }
  }
  return rt ? r : knorm(r);
}

K* dot0(K* a, K *b) {
  K *r,*p,*q,*t,*pm;
  K*(*ff)(K*,K*);
  if(bc==0) r=kcp(a); /* a . () */
  else if(bc==1) { r=at2_(a,v0(b)[0]); EC(r); }/* a . ,0 */
  else { /* a . (0 1;1 2;1 2) */
    p=v0(b)[0];
    t=bc==2?kcp(v0(b)[1]):drop2_(one,b);
    ff=bc==2?at2_:dot2_;
    if(p->c) {
      r=kv0(p->c);
      pm=p->t<0?kmix(p):p;
      DO(p->c,q=at2_(a,v0(pm)[i]); EC(q); v0(r)[i]=ff(q,t); EC(v0(r)[i]); kfree(q))
      if(pm!=p) kfree(pm);
    }
    else { q=at2_(a,p); EC(q); r=ff(q,t); EC(r); kfree(q); }
    kfree(t);
  }
  return r;
}
K* dot2_(K *a, K *b) {
  K *r=0,*t=0,*bm=0,*q,*f;
  fn *z=0,*ff;

  if(at!=4 && at!=5 && at!=7 && at!=37 && !ac) { if(bc) return kerror("rank"); else return kcp(a); }
  if(at<0 && bc>1) return kerror("rank");
  SG(b);

  switch(at) {
  case  4:
    f=scope_get(cs,a->v); EC(f);
    ff=f->v;
    if(bt<0) { bm=kmix(b); bm->t=11; } else bm=b;
    if(f->t==37) r=fne2(f,bm,0);
    else if(f->t==7&&bm->c==2) r=dt2[ff->i](v0(bm)[0],v0(bm)[1]);
    else if(f->t==7&&bm->c==1) r=dt1[ff->i](v0(bm)[0]);
    else if(f->t==7&&bm->t>0) r=dt1[ff->i](bm);
    else if(f->t==7) r=kerror("valence");
    else if(f->t==27&&bm->c==1) r=dt2[ff->i](ff->l,v0(bm)[0]);
    else if(f->t==27&&bm->t>0) r=dt2[ff->i](ff->l,bm);
    else if(f->t==27) r=kerror("valence");
    else if(f->t==67&&bm->c==2) r=applyfc2(f,v0(bm)[0],v0(bm)[1],0);
    else if(f->t==67&&bm->c==1) r=applyfc1(f,v0(bm)[0],0);
    else if(f->t==67&&bm->t>0) r=applyfc1(f,bm,0);
    else if(f->t==67) r=kerror("valence");
    else r=kerror("type");
    if(bm!=b) kfree(bm);
    kfree(f);
    break;
  case  5:
    switch(bt) {
    case  4: r=at2_(a,b); break;
    case  6: r=dvals(a5); break;
    case  0: r=dot0(a,b); break;
    case -4: if(bc) { t=a; DO(bc, r=dget(t->v,v4(b)[i]); t=r; kfree(r);) r=kcp(t); } else r=kcp(a); break;
    default: return kerror("type");
    } break;
  case 37: bm=bt<0?kmix(b):b; if(!bm->t) bm->t=11; r=fne2(a,bm,0); if(bm!=b) kfree(bm); break;
  case  7: z=a->v; bm=bt<0?kmix(b):b; if(bc==2) r=dt2[z->i](v0(bm)[0],v0(bm)[1]); if(bm!=b) kfree(bm); break;
  case  0:
    switch(bt) {
    case  1: r=at2_(a,b); break;
    case  0: r=dot0(a,b); break;
    case -1: bm=kmix(b); r=kcp(a); DO(bc,q=at2_(r,v0(bm)[i]); kfree(r); r=q; EC(r)) kfree(bm); break;
    default: return kerror("type");
    } break;
  case -1: case -2: case -3: case -4:
    switch(bt) {
    case  1: r=at2_(a,b); break;
    case  0: r=kv0(bc); DO(bc,v0(r)[i]=dot2_(a,v0(b)[i]); EC(v0(r)[i])) break;
    case -1: r=at2_(a,b); break;
    default: return kerror("type");
    } break;
  default: return kerror("type");
  }
  return rt ? r : knorm(r);
}

K* assign2_(K *a, K *b) {
  if(at!=99) return kerror("type");
  if(!b) return kerror("parse");
  if(kreserved(a4)) return kerror("reserved");
  EC(scope_set(cs,a->v,b));
  quiet2=1;
  return kcp(a);
}
K* assign2g_(K *a, K *b) {
  if(at!=99) return kerror("type");
  if(!b) return kerror("parse");
  if(kreserved(a4)) return kerror("reserved");
  EC(scope_set(gs,a->v,b));
  quiet2=1;
  return kcp(a);
}

K* flip_(K *a) {
  K *r=0,*p=0;
  int m=-1;
  unsigned int i;
  switch(at) {
  case 1: case 2: case 3: case 4: case 5: case 6:
  case 7: case 27: case 37: case 67: case 77: case 87: r=kcp(a); break;
  case 0:
    DO(ac, p=v0(a)[i]; if(p->t<=0) { if(m==-1)m=p->c; else if((int)p->c!=m)return kerror("length"); } )
    if(m==-1) r=kcp(a);
    else if(m==0) r=kv0(0);
    else {
      r=kv0(m); DO(rc,v0(r)[i]=kv0(ac))
      for(i=0;i<ac;i++) {
        if(v0(a)[i]->t>0) DO2(rc,v0(v0(r)[j])[i]=kcp(v0(a)[i]))
        else { p=kmix(v0(a)[i]); DO2(rc,v0(v0(r)[j])[i]=kcp(v0(p)[j])) kfree(p); }
      }
      DO(rc,v0(r)[i]=rt?r:knorm(v0(r)[i]))
    }
    break;
  case -1: case -2: case -3: case -4: r=kcp(a); break;
  default: return kerror("type");
  }
  return rt ? r : knorm(r);
}

K* negate_(K *a) {
  K *r=0;
  switch(at) {
  case  1: r=k1(-a1); break;
  case  2: r=k2(-a2); break;
  case  0: r=kv0(ac); DO(rc,v0(r)[i]=negate_(v0(a)[i]); EC(v0(r)[i])); break;
  case -1: r=kv1(ac); DO(rc,v1(r)[i]=-v1(a)[i]); break;
  case -2: r=kv2(ac); DO(rc,v2(r)[i]=-v2(a)[i]); break;
  default: return kerror("type");
  }
  return rt ? r : knorm(r);
}

K* first_(K *a) {
  K *r=0;
  if(at>0) return kcp(a);
  switch(at) {
  case  0: r=ac?kcp(v0(a)[0]):null; break;
  case -1: r=ac?k1(v1(a)[0]):k1(0); break;
  case -2: r=ac?k2(v2(a)[0]):k2(0.0); break;
  case -3: r=ac?k3(v3(a)[0]):k3(' '); break;
  case -4: r=ac?k4(v4(a)[0]):k4(""); break;
  default: return kerror("type");
  }
  return rt ? r : knorm(r);
}

K* reciprocal_(K *a) {
  K *r=0;
  switch(at) {
  case  1: r=k2(1.0/I2F(a1)); break;
  case  2: r=k2(1.0/a2); break;
  case  0: r=kv0(ac); DO(rc,v0(r)[i]=reciprocal_(v0(a)[i]); EC(v0(r)[i])) break;
  case -1: r=kv2(ac); DO(rc,v2(r)[i]=1.0/I2F(v1(a)[i])) break;
  case -2: r=kv2(ac); DO(rc,v2(r)[i]=1.0/v2(a)[i]) break;
  default: return kerror("type");
  }
  return rt ? r : knorm(r);
}

K* where_(K *a) {
  K *r=0;
  int j,kk;
  switch(at) {
  case  1: if(a1<0) return kerror("domain"); r=kv1(a1); DO(rc,v1(r)[i]=0) break;
  case  0: if(!ac) r=kv1(0); else return kerror("type"); break;
  case -1: DO(ac,if(v1(a)[i]<0) return kerror("domain"))
           j=0; DO(ac,j+=v1(a)[i]); r=kv1(j); j=0; DO(ac,kk=v1(a)[i]; while(kk-->0)v1(r)[j++]=i) break;
  default: return kerror("type");
  }
  return rt ? r : knorm(r);
}

K* reverse_(K *a) {
  K *r=0;
  switch(at) {
  case  1: case  2: case  3: case  4: case  5: case  6:
  case  7: case 27: case 37: case 67: case 77: case 87: r=kcp(a); break;
  case  0: r=kv0(ac); DO(ac, v0(r)[i]=kcp(v0(a)[ac-i-1])) break;
  case -1: r=kv1(ac); DO(ac, v1(r)[i]=v1(a)[ac-i-1]) break;
  case -2: r=kv2(ac); DO(ac, v2(r)[i]=v2(a)[ac-i-1]) break;
  case -3: r=kv3(ac); DO(ac, v3(r)[i]=v3(a)[ac-i-1]) break;
  case -4: r=kv4(ac); DO(ac, v4(r)[i]=v4(a)[ac-i-1]) break;
  default: return kerror("type");
  }
  return rt ? r : knorm(r);
}

K* upgrade_(K *a) {
  K *r=0;
  switch(at) {
  case  1: case  2: case  3: case  4: case  5: case  6:
  case  7: case 27: case 37: case 67: case 77: case 87: return kerror("rank");
  case  0: r=kv1(ac); DO(rc,v1(r)[i]=i); msortg0(v1(r),v0(a),0,rc-1,0); break;
  case -1: r=kv1(ac); r->v=rcsortg(v1(r),v1(a),ac,0); break;
  case -2: r=kv1(ac); DO(rc,v1(r)[i]=i); msortg2(v1(r),v2(a),0,rc-1,0); break;
  case -3: r=kv1(ac); DO(rc,v1(r)[i]=i); msortg3(v1(r),v3(a),0,rc-1,0); break;
  case -4: r=kv1(ac); DO(rc,v1(r)[i]=i); msortg4(v1(r),v4(a),0,rc-1,0); break;
  default: return kerror("type");
  }
  return rt ? r : knorm(r);
}

K* downgrade_(K *a) {
  K *r=0;
  switch(at) {
  case  1: case  2: case  3: case  4: case  5: case  6:
  case  7: case 27: case 37: case 67: case 77: case 87: return kerror("rank");
  case  0: r=kv1(ac); DO(rc,v1(r)[i]=i); msortg0(v1(r),v0(a),0,rc-1,1); break;
  case -1: r=kv1(ac); r->v=rcsortg(v1(r),v1(a),ac,1); break;
  case -2: r=kv1(ac); DO(rc,v1(r)[i]=i); msortg2(v1(r),v2(a),0,rc-1,1); break;
  case -3: r=kv1(ac); DO(rc,v1(r)[i]=i); msortg3(v1(r),v3(a),0,rc-1,1); break;
  case -4: r=kv1(ac); DO(rc,v1(r)[i]=i); msortg4(v1(r),v4(a),0,rc-1,1); break;
  default: return kerror("type");
  }
  return rt ? r : knorm(r);
}

K* group_(K *a) {
  K *r=0,*p=0,**ht,**pk,**hk;
  int *n,min=INT_MAX,max=INT_MIN,*hi,bni=0;
  unsigned int i;
  uint64_t m,w,h,rs=256,ri=0,*hm,q;
  char *c,**s,**hs;
  double *f,*hd;
  if(at<=0&&ac==0) return kv0(0);
  switch(at) {
  case  0:
    r=kv0(rs); rc=0;
    m=ac;
    w=1; while(w<=m) w<<=1; q=w-1;
    ht=xcalloc(w,sizeof(K*));
    hm=xcalloc(w,sizeof(uint64_t));
    hk=xcalloc(w,sizeof(K*));
    pk=v0(a)-1;
    for(i=0;i<ac;i++) {
      pk++;
      h=khash(*pk)&q;
      if(*pk) while(!h || (hk[h] && kcmpr(hk[h],*pk))) h=(h+1)&q; /* h=0 iff *s=0 */
      hk[h]=*pk;
      hm[h]++;
    }
    pk=v0(a)-1;
    for(i=0;i<ac;i++) {
      pk++;
      h=khash(*pk)&q;
      if(*pk) while(!h || (hk[h] && kcmpr(hk[h],*pk))) h=(h+1)&q; /* h=0 iff *s=0 */
      p=ht[h];
      if(!p) {
        p=ht[h]=kv1(hm[h]);
        p->c=0;
        if(rs==ri++){rs<<=1;r->v=xrealloc(r->v,sizeof(K*)*rs);}
        v0(r)[rc++]=p;
      }
      v1(p)[p->c++]=i;
    }
    xfree(ht); xfree(hm); xfree(hk);
    break;
  case -1:
    r=kv0(rs); rc=0;
    for(i=0;i<ac;i++) {
      if(v1(a)[i]==INT_MAX) { bni=1; continue; }
      if(v1(a)[i]==INT_MIN) { bni=1; continue; }
      if(v1(a)[i]==INT_MIN+1) { bni=1; continue; }
      if(max<v1(a)[i])max=v1(a)[i];
      if(min>v1(a)[i])min=v1(a)[i];
    }
    if(min>=0 && !bni) { /* optimize for all positive n */
      ht=xcalloc(max+1,sizeof(K*));
      hm=xcalloc(max+1,sizeof(uint64_t));
      n=v1(a);n--;
      DO(ac, n++; hm[*n]++)
      n=v1(a);n--;
      for(i=0;i<ac;i++) {
         n++;
         p=ht[*n];
         if(!p) {
           p=ht[*n]=kv1(hm[*n]);
           v1(p)[0]=i;
           p->c=1;
           if(rs==ri++){rs<<=1;r->v=xrealloc(r->v,sizeof(K*)*rs);}
           v0(r)[rc++]=p;
         }
         else v1(p)[p->c++]=i;
      }
      xfree(hm);
      xfree(ht);
    }
    else {
      m=max-min+1;
      if(!m) m=2; /* handle ?0I 0N */
      if(ac<m) m=ac;
      w=1; while(w<=m) w<<=1; q=w-1;
      ht=xcalloc(w,sizeof(K*));       /* groups */
      hm=xcalloc(w,sizeof(uint64_t)); /* max's */
      hi=xcalloc(w,sizeof(int));
      n=v1(a);n--;
      for(i=0;i<ac;i++) {
         n++;
         h=(*n*2654435761)&q;
         if(*n) while(!h || (hi[h] && hi[h]!=*n)) h=(h+1)&q; /* h=0 iff *n=0 */
         hi[h]=*n;
         hm[h]++;
      }
      n=v1(a);n--;
      for(i=0;i<ac;i++) {
         n++;
         h=(*n*2654435761)&q;
         if(*n) while(!h || (hi[h] && hi[h]!=*n)) h=(h+1)&q;
         p=ht[h];
         if(!p){
           p=ht[h]=kv1(hm[h]);
           p->c=0;
           if(rs==ri++){rs<<=1;r->v=xrealloc(r->v,sizeof(K*)*rs);}
           v0(r)[rc++]=p;
         }
         v1(p)[p->c++]=i;
      }
      xfree(ht); xfree(hm); xfree(hi);
    }
    break;
  case -2:
    r=kv0(rs); rc=0;
    m=ac;
    w=1; while(w<=m) w<<=1; q=w-1;
    ht=xcalloc(w,sizeof(K*));       /* groups */
    hm=xcalloc(w,sizeof(uint64_t)); /* max's */
    hd=xcalloc(w,sizeof(double));
    f=v2(a);f--;
    for(i=0;i<ac;i++) {
       f++;
       h=((uint64_t)*f*2654435761)&q;
       if(*f) while(!h || (hd[h] && CMPFFT(hd[h],*f))) h=(h+1)&q; /* h=0 iff *f=0 */
       hd[h]=*f;
       hm[h]++;
    }
    f=v2(a);f--;
    for(i=0;i<ac;i++) {
       f++;
       h=((uint64_t)*f*2654435761)&q;
       if(*f) while(!h || (hd[h] && CMPFFT(hd[h],*f))) h=(h+1)&q; /* h=0 iff *f=0 */
       p=ht[h];
       if(!p){
         p=ht[h]=kv1(hm[h]);
         p->c=0;
         if(rs==ri++){rs<<=1;r->v=xrealloc(r->v,sizeof(K*)*rs);}
         v0(r)[rc++]=p;
       }
       v1(p)[p->c++]=i;
    }
    xfree(ht); xfree(hm); xfree(hd);
    break;
  case -3:
    r=kv0(rs); rc=0;
    ht=xcalloc(256,sizeof(K*));
    hm=xcalloc(256,sizeof(uint64_t));
    c=v3(a)-1;
    DO(ac, c++; hm[(unsigned char)*c]++)
    c=v3(a)-1;
    for(i=0;i<ac;i++) {
       c++;
       p=ht[(unsigned char)*c];
       if(!p){ p=ht[(unsigned char)*c]=kv1(hm[(unsigned char)*c]); v1(p)[0]=i; p->c=1; v0(r)[rc++]=p; }
       else v1(p)[p->c++]=i;
    }
    xfree(hm); xfree(ht);
    break;
  case -4:
    r=kv0(rs); rc=0;
    m=ac;
    w=1; while(w<=m) w<<=1; q=w-1;
    ht=xcalloc(w,sizeof(K*));
    hm=xcalloc(w,sizeof(uint64_t));
    hs=xcalloc(w,sizeof(char*));
    s=v4(a)-1;
    for(i=0;i<ac;i++) {
      s++;
      h=xfnv1a((char*)*s, strlen(*s))&q;
      if(*s) while(!h || (hs[h] && strcmp(hs[h],*s))) h=(h+1)&q; /* h=0 iff *s=0 */
      hs[h]=*s;
      hm[h]++;
    }
    s=v4(a)-1;
    for(i=0;i<ac;i++) {
      s++;
      h=xfnv1a((char*)*s, strlen(*s))&q;
      if(*s) while(!h || (hs[h] && strcmp(hs[h],*s))) h=(h+1)&q; /* h=0 iff *s=0 */
      p=ht[h];
      if(!p) {
        p=ht[h]=kv1(hm[h]);
        p->c=0;
        if(rs==ri++){rs<<=1;r->v=xrealloc(r->v,sizeof(K*)*rs);}
        v0(r)[rc++]=p;
      }
      v1(p)[p->c++]=i;
    }
    xfree(ht); xfree(hm); xfree(hs);
    break;
  default: return kerror("rank");
  }
  return rt ? r : knorm(r);
}

K* shape_(K *a) {
  K *r=0,*p=0,**q=0,**t=0;;
  unsigned int i,c[256],ci,qc,tc;

  switch(at) {
  case 1: case 2: case 3: case 4: case 5: case 6:
  case 7: case 27: case 37: case 67: case 77: case 87: r=kv1(0); break;
  case 0:
    q=xmalloc(sizeof(K*));
    q[0]=a;
    qc=1;
    ci=0;
    while(q) { /* breadth first traversal */
      c[ci]=tc=0; t=0;
      for(i=0;i<qc;i++) {
        p=q[i];
        if(p->t>0) { ci--; break; }
        if(!c[ci]&&p->c) c[ci]=p->c;
        else if(c[ci]!=p->c) { if(t){xfree(t);t=0;}; ci--; break; }
        if(!p->t&&p->c) { /* enqueue next round */
          if(t) t=xrealloc(t,sizeof(K*)*(p->c+tc));
          else t=xmalloc(sizeof(K*)*p->c);
          DO2(p->c,t[tc++]=v0(p)[j])
        }
      }
      xfree(q);
      q=t; qc=tc; /* next queue */
      ci++;
    }
    r=kv0(ci);
    DO(ci,v0(r)[i]=k1(c[i]))
    break;
  case -1: case -2: case -3: case -4: r=kv1(1); v1(r)[0]=ac; break;
  default: return kerror("type");
  }
  return rt ? r : knorm(r);
}

#ifdef _WIN32
static K* lsdir(char *p) {
  K *r=0,*q=0,*s=0;
  int n=0;
  char *t=xmalloc(5+strlen(p));
  strcpy(t,p);
  strcat(t,"\\*.*");
  WIN32_FIND_DATA ffd;
  HANDLE h;
  h=FindFirstFile(t,&ffd);
  if(h==INVALID_HANDLE_VALUE) return kerror("value");
  q=kv0(32);
  do {
    if(!strcmp(ffd.cFileName,".")||!strcmp(ffd.cFileName,"..")) continue;
    ++n;
    if(n==q->c) { q->c<<=1; q->v=xrealloc(q->v,q->c*sizeof(K*)); }
    v0(q)[n-1]=kv3(1+strlen(ffd.cFileName));
    strcpy(v0(q)[n-1]->v,ffd.cFileName);
    v0(q)[n-1]->c--;
  } while(FindNextFile(h,&ffd));
  q->c=n;
  FindClose(h);
  s=upgrade_(q);
  r=at2_(q,s);
  kfree(q);
  kfree(s);
  return r;
}
#else
static K* lsdir(char *p) {
  K *r=0,*q=0,*s=0;
  unsigned int n=0;
  DIR *f=opendir(p);
  struct dirent *e;
  if(!f) return kerror("value");
  q=kv0(32);
  while((e=readdir(f))) {
    if(!strcmp(e->d_name,".")||!strcmp(e->d_name,"..")) continue;
    ++n;
    if(n==q->c) { q->c<<=1; q->v=xrealloc(q->v,q->c*sizeof(K*)); }
    v0(q)[n-1]=kv3(1+strlen(e->d_name));
    strcpy(v0(q)[n-1]->v,e->d_name);
    v0(q)[n-1]->c--;
  }
  q->c=n;
  closedir(f);
  s=upgrade_(q);
  r=at2_(q,s);
  kfree(q);
  kfree(s);
  return r;
}

#endif
K* enumerate_(K *a) {
  K *r=0,*q=null;
  char s[2],*p=s;

  if(at==4) { /* ktree */
    if(a4==sp("")) return dkeys(ktree);
    else if(a4[0]=='.') { q=scope_get(ks,a4); EC(q); a=q; }
    else { q=scope_get(cs,a4); EC(q); a=q; }
    if(q->t!=5) { kfree(q); return null; }
  }

  switch(at) {
  case  1: if(a1<0||a1==INT_MAX) return kerror("domain"); r=kv1(a1); DO(a1,v1(r)[i]=i); break;
  case  2: return kerror("int");
  case  3: p[0]=a3; p[1]=0; r=lsdir(p); break;
  case  5: r=dkeys(a5); break;
  case  6: r=kv4(0); break;
  case  7: case 27: case 37: case 67: case 77: case 87: return kerror("int");
  case  0: return kerror("type");
  case -1: return kerror("type");
  case -2: return kerror("type");
  case -3: p=xmalloc(1+ac); strncpy(p,v3(a),ac); p[ac]=0; r=lsdir(p); xfree(p); break;
  case -4: return kerror("type");
  default: return kerror("type");
  }
  kfree(q);
  return rt ? r : knorm(r);
}

K* not_(K *a) {
  K *r=0;
  switch(at) {
  case  1: r=k1(a1==0); break;
  case  2: r=k1(a2==0.0); break;
  case  0: r=kv0(ac); DO(rc,v0(r)[i]=not_(v0(a)[i]); EC(v0(r)[i])) break;
  case -1: r=kv1(ac); DO(rc,v1(r)[i]=v1(a)[i]==0) break;
  case -2: r=kv1(ac); DO(rc,v1(r)[i]=v2(a)[i]==0) break;
  default: return kerror("type");
  }
  return rt ? r : knorm(r);
}

K* enlist_(K *a) {
  K *r=kv0(1);
  v0(r)[0]=kcp(a);
  return rt ? r : knorm(r);
}

K* count_(K *a) {
  return k1(at>0?1:ac);
}

K* flr_(K *a) {
  K *r=0;
  switch(at) {
  case  1: r=kcp(a); break;
  case  2: r=k1(floor(a2+EPSILON)); break;
  case  0: r=kv0(ac); DO(rc,v0(r)[i]=flr_(v0(a)[i]); EC(v0(r)[i])) break;
  case -1: r=kcp(a); break;
  case -2: r=kv1(ac); DO(rc, v1(r)[i]=floor(v2(a)[i]+EPSILON)) break;
  default: return kerror("type");
  }
  return rt ? r : knorm(r);
}

K* format_(K *a) {
  K *r=0;
  char ds[32];
  switch(at) {
  case  1:
    if(a1==INT_MAX) sprintf(ds,"0I");
    else if(a1==INT_MIN+1) sprintf(ds,"-0I");
    else if(a1==INT_MIN) ds[0]=0;
    else sprintf(ds,"%d",a1);
    r=knew(-3,strlen(ds),ds,0,0,0);
    break;
  case  2:
    if(isinf(a2)&&a2>0.0) sprintf(ds,"0i");
    else if(isinf(a2)&&a2<0.0) sprintf(ds,"-0i");
    else if(isnan(a2)) ds[0]=0;
    else sprintf(ds, "%0.*g", precision, a2);
    r=knew(-3,strlen(ds),ds,0,0,0);
    break;
  case  3: r=kv3(1); v3(r)[0]=a3; break;
  case  4: r=kv3(strlen(v3(a))); strncpy(v3(r),v3(a),rc); break;
  case  5: r=knew(-3,5,".(..)",0,0,0); break;
  case  6: r=kv3(0); break;
  case  7: case 27: case 37: case 67: case 77: case 87: r=fivecolon1_(a); break;
  case  0: r=kv0(ac); DO(ac,v0(r)[i]=format_(v0(a)[i]); EC(v0(r)[i])) break;
  case -1:
    r=kv0(ac);
    DO(ac,
      if(v1(a)[i]==INT_MAX) sprintf(ds,"0I");
      else if(v1(a)[i]==INT_MIN+1) sprintf(ds,"-0I");
      else if(v1(a)[i]==INT_MIN) ds[0]=0;
      else sprintf(ds,"%d",v1(a)[i]);
      v0(r)[i]=knew(-3,strlen(ds),ds,0,0,0)
    )
    break;
  case -2:
    r=kv0(ac);
    DO(ac,
      if(isinf(v2(a)[i])&&v2(a)[i]>0.0) sprintf(ds,"0i");
      else if(isinf(v2(a)[i])&&v2(a)[i]<0.0) sprintf(ds,"-0i");
      else if(isnan(v2(a)[i])) ds[0]=0;
      else sprintf(ds, "%0.*g", precision, v2(a)[i]);
      v0(r)[i]=knew(-3,strlen(ds),ds,0,0,0)
    ) break;
  case -3: r=kcp(a); break;
  case -4: r=kv0(ac); DO(ac, v0(r)[i]=kv3(strlen(v4(a)[i])); strncpy(v3(v0(r)[i]),v4(a)[i],v0(r)[i]->c)) break;
  default: return kerror("type");
  }
  return rt ? r : knorm(r);
}

K* unique_(K *a) {
  K *r=0,**hk,**kp;
  int *hi,*n;
  char hc[256]={0},*c,**hs,**s,*pr;
  double *hd,*f;
  uint64_t m,w,h,q;
  unsigned int i;
  int j=0,z=0,t=0,min=INT_MAX,max=INT_MIN,bni=0;

  switch(at) {
  case  1: case  2: case  3: case  4: case  5: case  6:
  case  7: case 27: case 37: case 67: case 77: case 87: return kerror("rank");
  case  0:
    r=kv0(ac);
    if(!ac) return r;
    w=1; while(w<=ac) w<<=1; q=w-1;
    hk=xcalloc(w,sizeof(K*));
    kp=v0(a);kp--;
    for(i=0;i<ac;i++) {
      kp++;
      h=khash(*kp)&q;
      while(hk[h] && kcmpr(hk[h],*kp)) h=(h+1)&q;
      if(!hk[h]) hk[h]=v0(r)[j++]=kcp(*kp);
    }
    rc=j;
    xfree(hk);
    break;
  case -1:
    r=kv1(ac);
    if(!ac) return r;
    for(i=0;i<ac;i++) {
      if(v1(a)[i]==INT_MAX) { bni=1; continue; }
      if(v1(a)[i]==INT_MIN) { bni=1; continue; }
      if(v1(a)[i]==INT_MIN+1) { bni=1; continue; }
      if(max<v1(a)[i])max=v1(a)[i];
      if(min>v1(a)[i])min=v1(a)[i];
    }
    if(min>=0 && !bni) { /* optimize for all positive n */
      hi=xcalloc(max+1,sizeof(int));
      n=v1(a);n--;
      DO(rc, n++; if(!hi[*n]) { hi[*n]=1; v1(r)[j++]=*n; if(++t==max+1) break; })
      rc=j;
      xfree(hi);
    }
    else {
      m=max-min+1;
      m+=3; /* handle 0I 0N -0I */
      if(ac<m) m=ac;
      w=1; while(w<=m) w<<=1; q=w-1;
      hi=xcalloc(w,sizeof(int));
      n=v1(a);n--;
      for(i=0,j=0;i<ac;i++) {
        n++;
        if(!*n) { if(!z) { v1(r)[j++]=0; z=1; } continue; }
        h=(*n*2654435761)&q;
        while(hi[h] && hi[h]!=*n) h=(h+1)&q;
        if(hi[h]!=*n) hi[h]=v1(r)[j++]=*n;
        //if(++t==m) break;
      }
      rc=j;
      xfree(hi);
    }
    break;
  case -2:
    r=kv2(ac);
    if(!ac) return r;
    w=1; while(w<=ac) w<<=1; q=w-1;
    hd=xcalloc(w,sizeof(double));
    f=v2(a);f--;
    for(i=0;i<ac;i++) {
      f++;
      if(!*f) { if(!z) { v2(r)[j++]=0;z=1; } continue; }
      h=((uint64_t)*f*2654435761)&q;
      while(hd[h]!=0 && CMPFF(hd[h],*f)) h=(h+1)&q;
      if(CMPFF(hd[h],*f)) hd[h]=v2(r)[j++]=*f;
    }
    rc=j;
    xfree(hd);
    break;
  case -3:
    r=kv3(ac);
    if(!ac) return r;
    c=v3(a);c--;
    pr=v3(r);
    DO(ac, c++; if(!hc[(unsigned char)*c]) { hc[(unsigned char)*c]=1; *pr++=*c; j++; })
    rc=j;
    break;
  case -4:
    r=kv4(ac);
    if(!ac) return r;
    w=1; while(w<=ac) w<<=1; q=w-1;
    hs=xcalloc(w,sizeof(char*));
    s=v4(a);s--;
    for(i=0;i<ac;i++) {
      s++;
      h=xfnv1a((char*)*s, strlen(*s))&q;
      while(hs[h]!=0 && strcmp(hs[h],*s)) h=(h+1)&q;
      if(!hs[h]) hs[h]=v4(r)[j++]=*s;
    }
    rc=j;
    xfree(hs);
    break;
  default: return kerror("type");
  }
  return rt ? r : knorm(r);
}

K* atom_(K *a) {
  return k1(at>0);
}

K* value_(K *a) {
  K *r=0,*f=0,*am=0,*p=0;
  pgs *s;
  node *n;
  int k;
  unsigned int i;
  fn *ff;
  char *ss;
  switch(at) {
  case  0:
    for(i=0;i<ac;i++) if(v0(a)[i]->t!=-3) break;
    if(ac&&i==ac) { r=kv0(ac); DO(rc,v0(r)[i]=value_(v0(a)[i]); EC(v0(r)[i])); break; }
    for(i=0;i<ac;i++) {
      p=v0(a)[i];
      if(p->c!=2&&p->c!=3) break;
      else if(p->t!=0&&p->t!=-4) break;
      else if(p->t==0&&v0(p)[0]->t!=4) break;
    }
    if(i==ac) { r=k5(l2d(a)); break; }
    if((ac==2&&v0(a)[0]->t==4)||v0(a)[0]->t==-3||v0(a)[0]->t==3) {
      if(v0(a)[i]->t==-3) { ss=xstrndup(v0(a)[0]->v,v0(a)[0]->c); f=scope_get(cs,ss); xfree(ss); }
      else if(v0(a)[0]->t==4) f=scope_get(cs,v0(a)[0]->v);
      else { ss=xmalloc(2); sprintf(ss,"%c",v0(a)[i]->i); f=scope_get(cs,ss); xfree(ss); }
      EC(f);
      ff=f->v;
      if(v0(a)[1]->t<0) { am=kmix(v0(a)[1]); am->t=11; } else am=kcp(v0(a)[1]);
      if(f->t==37) r=fne2(f,am,0);
      else if(f->t==7&&am->c==2) r=dt2[ff->i](v0(am)[0],v0(am)[1]);
      else if(f->t==7&&am->c==1) r=dt1[ff->i](v0(am)[0]);
      else if(f->t==7&&am->t>0) r=dt1[ff->i](am);
      else if(f->t==7) r=kerror("valence");
      else if(f->t==27&&am->c==1) r=dt2[ff->i](ff->l,v0(am)[0]);
      else if(f->t==27&&am->t>0) r=dt2[ff->i](ff->l,am);
      else if(f->t==27) r=kerror("valence");
      else if(f->t==67&&am->c==2) r=applyfc2(f,v0(am)[0],v0(am)[1],0);
      else if(f->t==67&&am->c==1) r=applyfc1(f,v0(am)[0],0);
      else if(f->t==67&&am->t>0) r=applyfc1(f,am,0);
      else if(f->t==67) r=kerror("valence");
      else r=kerror("type");
      if(am!=a) kfree(am);
      kfree(f);
      break;
    }
    r=kerror("value");
    break;
  case  5: r=d2l(a5); break;
  case  3: s=pgnew(); s->p=xmalloc(3); sprintf(s->p,"%c\n",a3); break;
  case  4:
    if(a4==sp("")) { r=k5(dcp(ktree)); return r; } /* . ` */
    s=pgnew();
    s->p=xmalloc(2+strlen(a4));
    sprintf(s->p,"%s\n",a4);
    break;
  case -3:
    s=pgnew();
    s->p=xmalloc(2+ac);
    strncpy(s->p,v3(a),ac);
    s->p[ac]='\n';
    s->p[ac+1]=0;
    break;
  default: return kerror("type"); break;
  }
  switch(at) {
    case 3: case 4: case -3:
    n=pgparse(s);
    r=null;
    for(k=n->v-1;k>=0;k--) {
      kfree(r);
      quiet2=0;
      r=node_reduce(n->a[k],0); EC(r);
      if(fret) { fret=0; break; }
    }
    node_free(n);
    pgfree(s);
    break;
  }
  if(quiet2) { kfree(r); r=null; quiet2=0; quiet=0; }
  return rt ? r : knorm(r);
}

K* return_(K *a) {
  //if(fret) kfree(gk);
  gk=a;
  fret=1;
  return kref(a);
}

K* fourcolon1_(K *a) {
  int t=at;
  if(t==27||t==37||t==67||t==77||t==87||t==97) t=7;
  return knew(1,0,0,t,0,0);
}

K* fivecolon1_(K *a) {
  K *r=0;
  int n;
  char *s=kprint5(a,"","",0);
  if(s==(char*)-1) return kerror("wsfull");
  n=strlen(s);
  VSIZE(n)
  r=knew(-3,n,s,0,0,0);
  xfree(s);
  return r;
}

K* precision1_(K *a) {
  if(at==6) printf("%d\n",precision);
  else precision=a->i;
  return null;
}

K* kdump1_(K *a) {
  if(at==4&&a4==sp("-l")) kdump(1);
  else kdump(0);
  return null;
}

K* apply4(K *f, K *a, K *b, K *c, K *d) {
  K *r=0;
  fn *ff=f->v;
  quiet2=0;
  SG2(a); SG2(b); SG2(c); SG2(d);
  if(at==98||bt==98||ct==98||dt==98) return kerror("value");
  if(ff->i=='@') r=amendi4_(a,b,c,d);
  else if(ff->i=='.') r=amend4_(a,b,c,d);
  kfree(a); kfree(b); kfree(c); kfree(d);
  return r;
}
K* apply3(K *f, K *a, K *b, K *c) {
  K *r=0;
  fn *ff=f->v;
  quiet2=0;
  SG2(a); SG2(b); SG2(c);
  if(at==98||bt==98||ct==98) return kerror("value");
  if(ff->i=='@') r=amendi3_(a,b,c);
  else if(ff->i=='.') r=amend3_(a,b,c);
  else if(ff->i=='_') r=slide3_(a,b,c);
  kfree(a); kfree(b); kfree(c);
  return r;
}
K* apply2(K *f, K *a, K *b, char *av) {
  K *r=0,*p,*aa;
  fn *fp;
  char av2[32];
  fn *ff=f->v;
  if(at==16||(bt==16&&ff->i!='@')) {
    p=knew(7,0,fnnew(""),'p',0,0); p->t=87;
    fp=p->v;
    fp->l=kref(f);
    aa=kv0(2); v0(aa)[0]=kcp(a); v0(aa)[1]=kcp(b); aa->t=11;
    fp->a=aa;
    fp->v=1;
    return p;
  }
  av2[0]=0;
  if(ff->av) strcat(av2,ff->av);
  if(av) strcat(av2,av);
  quiet2=0;
  if(ff->i!=':') SG2(a);
  SG2(b);
  if(at==98||bt==98) return kerror("value");
  if(at==77) r=applyprj(a,b);
  else { r=avdo(f,a,b,av2); if(!r) r=dt2[ff->i](a,b); }
  if(ff->i!=':') kfree(a);
  kfree(b);
  return r;
}
K* apply1(K *f, K *a, char *av) {
  K *r=0;
  char av2[32];
  fn *ff=f->v;
  av2[0]=0;
  if(f->c==2) return kerror("valence");
  if(ff->av) strcat(av2,ff->av);
  if(av) strcat(av2,av);
  quiet2=0;
  SG2(f); SG2(a);
  if(f->t==98||at==98) return kerror("value");
  r=avdom(f,a,av2);
  if(!r) r=dt1[ff->i](a);
  kfree(f); kfree(a);
  return r;
}
K* applyfc2_(K *f, K *a, K *b) {
  K *r=0,*p=0;
  fn *ff=f->v;
  int i;
  i=ff->cn-1;
  r=apply2(ff->c[i],a,b,0);
  for(i--;i>=0;i--) { p=apply1(ff->c[i],r,0); kfree(r); r=p; }
  return r;
}
K* applyfc2(K *f, K *a, K *b, char *av) {
  K *r=0;
  char av2[32];
  fn *ff=f->v;
  av2[0]=0;
  if(ff->av) strcat(av2,ff->av);
  if(av) strcat(av2,av);
  SG2(f); SG2(a); SG2(b);
  if(f->t==98||at==98) return kerror("value");
  r=avdofc(f,a,b,av2);
  if(!r) r=applyfc2_(f,a,b);
  kfree(f); kfree(a); kfree(b);
  return r;
}
K* applyfc1_(K *f, K *a) {
  K *r=0,*p=0;
  fn *ff=f->v;
  int i;
  i=ff->cn-1;
  if(ff->c[i]->t==27) r=apply2(ff->c[i],((fn*)ff->c[i]->v)->l,a,0);
  else r=apply1(ff->c[i],a,0);
  EC(r);
  for(i--;i>=0;i--) {
    if(ff->c[i]->t==27) p=apply2(ff->c[i],((fn*)ff->c[i]->v)->l,r,0);
    else p=apply1(ff->c[i],r,0);
    EC(p);
    kfree(r);
    r=p;
  }
  return r;
}
K* applyfc1(K *f, K *a, char *av) {
  K *r=0;
  char av2[32];
  fn *ff=f->v;
  av2[0]=0;
  if(ff->av) strcat(av2,ff->av);
  if(av) strcat(av2,av);
  SG2(f); SG2(a);
  if(f->t==98||at==98) return kerror("value");
  r=avdomfc(f,a,av2);
  if(!r) r=applyfc1_(f,a);
  kfree(f); kfree(a);
  return r;
}
K* applyprj(K *f, K *a) {
  K *r=0,*pf,*pa,*aa,*p;
  fn *ff=f->v;
  int n=0,pe=0;
  unsigned int i,j=0;

  if((p=fnprj(f,a))) return p; /* projection of projection? */

  pf=ff->l;
  pa=ff->a;
  DO(pa->c,if(v0(pa)[i]->t==16)pe++)
  n=pa->c+(at==11?ac:1)-pe;
  aa=kv0(n); aa->t=11;
  for(i=0;i<pa->c;i++) v0(aa)[i]=kcp(v0(pa)[i]);
  while(i<aa->c) v0(aa)[i++]=inull;
  if(at==11) {
    for(i=0;i<aa->c;i++) {
      if(v0(aa)[i]->t==16) {
        if(j<ac) v0(aa)[i]=kcp(v0(a)[j++]);
        else break;
      }
    }
  }
  else {
    for(i=0;i<aa->c;i++) {
      if(v0(aa)[i]->t==16) {
        v0(aa)[i++]=kcp(a);
        break;
      }
    }
  }
  while(i<aa->c) v0(aa)[i++]=inull;
  if(((fn*)pf->v)->v==aa->c&&pf->t==37) r=fne2(pf,aa,0);
  else if(((fn*)pf->v)->v==aa->c&&pf->t==77) r=applyprj(pf,aa);
  kfree(aa);
  return r;
}
extern int ecount;
K* signal_(K* a) {
  char e[32];
  if(cs->p) cs=cs->p;
  if(ecount) --ecount;
  if(at==-3) { strncpy(e,a->v,ac); e[ac]=0; gk=kerror(e); }
  else gk=kerror("");
  fret=1;
  return gk;
}
extern int A;
K* abort1_(K* a) {
  A=1;
  return a;
}
K* dir1_(K* a) {
  char *s,*p,*ss,*pp,*sr;
  dict *d;
  K *q,*t=0;
  scope *es;

  if(strlen(v3(D))==2&&v3(D)[0]=='.'&&v3(D)[1]!='k') return kerror("reserved");
  if(strlen(v3(D))>2&&v3(D)[0]=='.'&&v3(D)[2]=='.'&&v3(D)[1]!='k') return kerror("reserved");

  if(at==6) printf("%s\n",(char*)D->v);
  else if(a->v==D->v) { /* do nothing */ }
  else if(at==3&&a->i=='^') { /* go up one level */
    s=xstrdup(D->v);
    p=strrchr(s,'.');
    if(p) { *p=0; if(cs->p) cs=cs->p; D->v=sp(s); }
    xfree(s);
  }
  else if(at==-3||at==4) {
    if(v3(a)[0]=='.') {
      if(at==-3) s=xstrndup(v3(a),ac);
      else s=xstrdup(v3(a));
    }
    else {
      if(at==-3) { s=xmalloc(strlen(v3(D))+ac+2); sprintf(s,"%s.",v3(D)); strncat(s,v3(a),ac); }
      else { s=xmalloc(strlen(v3(D))+strlen(v3(a))+2); sprintf(s,"%s.%s",v3(D),v3(a)); }
    }
    ss=xstrdup(s); /* set global D at the end */
    pp=xstrdup(s); /* incremental path for scope->k */
    es=ks;         /* start at top of ktree */
    d=es->d;
    p=strtok_r(s,".",&sr);
    pp[0]=0;
    do {
      strcat(pp,".");
      strcat(pp,p);
      q=dget(d,p);
      if(!q) { /* ktree path does not exist */
        t=k5(dnew());
        dset(d,p,t);
        kfree(t);
        es=scope_newk(es,sp(pp));
        d=t->v;
      }
      else if(q->t==5) { /* ensure scope path exists */
        d=q->v;
        t=scope_get(es,p);
        if(t->t==98) { /* nope */
          es=scope_newk(es,sp(pp));
        }
        else {
          es=scope_find(t->v);
          if(!es) {
            es=scope_newk(es,sp(pp));
            es->d->r++;
          }
        }
        kfree(t);
      }
      else return kerror("type");
    } while((p=strtok_r(0,".",&sr)));
    gs=cs=es; /* set current global scope */
    D->v=sp(ss); /* set global D */
    xfree(s); xfree(ss); xfree(pp);
  }
  else fprintf(stderr,"domain\n");
  return null;
}

K* val1_(K *a) {
  K *r=0;
  switch(at) {
   case 27: r=k1(1); break;
   case  7: case 67: r=k1(2); break;
   case 37: case 77: case 87: case 97: r=k1(((fn*)a->v)->v); break;
   default: r=kerror("type");
  }
  return r;
}

K* help1_(K *a) {
  if(!a) {
    fprintf(stderr,""
"\\0 data\n"
"\\+ verbs\n"
"\\' adverbs\n"
"\\_ reserved verbs/nouns\n"
"\\. assign define control debug\n"
"\\: input/output\n"
"\\- client/server\n"
"\\` os commands\n"
"\\? commands\n");
  }
  else if(a->i=='0') {
    fprintf(stderr,""
"       scalar  vector  empty  inf  nul\n"
"int    0       1 2     !0     0I   0N\n"
"float  0.0     1 2.0   0#0.0  0i   0n\n"
"char   \" \"     \"12\"    \"\"          \"\\0\"\n"
"symbol `       `a`b    0#`\n"
"\n"
"lambda {}\n"
"null   nul\n"
"list   (a;b;c;...) ,a () empty\n"
"\n"
"dict   .((`a;1);(`b;2);...)\n"
" !d    keys\n"
" d[]   values\n"
" d.k  d`k  d[`k]  d@`k  d.`k\n"
"\n"
"4:x    type: atom(1..7)[ifcsdnx] list(0..-4)[KIFCS]\n"
"5:x    ascii representation\n");
  }
  else if(a->i=='+') {
    fprintf(stderr,""
"   monad      dyad\n"
"+  flip       plus\n"
"-  negate     minus\n"
"*  first      times\n"
"%%  recip      divide\n"
"&  where      min/and\n"
"|  reverse    max/or\n"
"<  upgrade    less\n"
">  downgrade  more\n"
"=  group      equal\n"
"^  shape      power\n"
"!  enumerate  mod/rotate\n"
"~  not        match\n"
",  enlist     join\n"
"#  count      take/reshape\n"
"_  floor      drop/cut\n"
"$  format     form\n"
"?  unique     find/draw/deal\n"
"@  atom       at\n"
".  value      dot\n"
"\n"
"   triad      tetrad\n"
"_  slide\n"
"@  amend/trap amend\n"
".  amend/trap amend\n");
  }
  else if(a->i=='\'') {
    fprintf(stderr,""
"      monad: each      f'x\n"
" infix dyad: each      x f'y\n"
"prefix dyad: each      f'[x;y]\n"
"      other: each      f'[x;y;z]\n"
"\n"
"      monad: scanm     f\\x\n"
"      monad: do        n f\\x\n"
"      monad: while     b f\\x\n"
" infix dyad: eachleft  x f\\y\n"
"prefix dyad: scand     f\\x\n"
"      other: scan      f\\[x;y;z]\n"
"\n"
"      monad: overm     f/x\n"
"      monad: do        n f/x\n"
"      monad: while     b f/x\n"
" infix dyad: eachright x f/y\n"
"prefix dyad: overd     f/x\n"
"      other: over      f/[x;y;z]\n"
"\n"
"eachprior is ep[f;x], ex: ep[-;3 2 1]\n");
  }
  else if(a->i=='_') {
    fprintf(stderr,""
"variables/constants:\n"
"      nul   null\n"
"      .z.h  hostname\n"
"      .z.P  pid\n"
"      .z.i  cli args\n"
"      .z.T  days since 2035 (float)\n"
"      .z.t  seconds since 2035 (int)\n"
"      .z.f  self reference to current function\n"
"\n"
"math: log exp abs sqr sqrt floor ceil dot mul inv\n"
"      sin cos tan asin acos atan sinh cosh tanh x atan2 y\n"
"      x euclid y (hypot) erf erfc gamma lgamma (log gamma) n choose k\n"
"      rint (round to whole number) x round y (round to y) trunc (nearest)\n"
"      div  and or xor not rot shift (bitwise)\n"
"      y lsq A is least squares approximation x for y~+/A*x\n"
"\n"
"rand: x draw y (from !y); x draw -y (deal from !y); x draw 0 (from (0,1))\n"
"\n"
"time: lt ts (local timestamp, days since 2035)\n"
"      ltime ts (local day time, ex: 20231225 123005)\n"
"      gtime ts (gmt day time)\n"
"      jd yyyymmdd (and dj) to/from julian day number (0 is monday)\n"
"      sleep ms\n"
"\n"
"list: x at y indexes giving appropriate null values instead of index error\n"
"      x in y is 1 if x is an item of y; 0 otherwise (list: lin)\n"
"      x dv y and x di y to delete by value and index (list: dvl)\n"
"      x sv v (scalar from vector) and x vs s (vector from scalar)\n"
"      ci i (character from integer) and ic c (integer from character)\n"
"      x sm y is string match. y can have *?[^-].\n"
"      x ss y is string/symbol search for start indices. y can have ?[^-].\n"
"      ssr[x;y;z] is string/symbol search and replace. z can be a function.\n"
"\n"
"data: bd d (bytes from data)  and db b (data from bytes)\n"
"      bh d (bytes from hex)   and hb b (hex from bytes)\n"
"      zb b (compress bytes)   and bz z (decompress bytes)\n"
"      kv v (list from vector) and vk k (vector from list)\n"
"      val f (valence of fun)\n"
"      md5 x (128-bit), sha1 x (160-bit), sha2 x (256-bit)\n"
"        x is type -3 4 -4 0\n"
"      (iv;key) encrypt x, (iv;key) decrypt x (aes256)\n"
"        x is type -3 4 -4 0, iv:16 draw 256, key:32 draw 256\n"
"\n"
"sys:  v setenv s, getenv v (set/get environment variable)\n"
"      del f (delete file), f rename g (rename f to g)\n"
"      exit code\n");
  }
  else if(a->i=='.') {
    fprintf(stderr,""
"assign\n"
"a:1   local\n"
"a::1  global\n"
"\n"
"amend\n"
".[d;i;f]   @[d;i;f]\n"
".[d;i;f;y] @[d;i;f;y]\n"
"\n"
"control\n"
" $[c;t;f]   conditional\n"
" if[c;...]\n"
" do[n;...]\n"
" while[c;...]\n"
" /  comment\n"
" \\  escape/abort\n"
" :  return\n"
" '  signal\n"
"\n"
"error trap .[f;(x;y;z);:] and @[f;x;:]\n");
  }
  else if(a->i==':') {
    fprintf(stderr,""
" 0:f   f 0:x  read/write text (` for console)\n"
" 1:f   f 1:x  read (map)/write k data\n"
"       f 5:x  append k data\n"
" 2:f          read k data\n"
" 6:f   f 6:x  read/write bytes\n"
"     (,f)6:x  append bytes\n"
"\n"
" (type;width)0:f   fixedwidth text(IFCS )\n"
" (type;width)1:f   fixedwidth data(cbsifd CS)\n"
" blank skips. f can be (f;index;length).\n");
  }
  else if(a->i=='-') {
    fprintf(stderr,""
"coming soon...\n");
  }
  else if(a->i=='`') {
    fprintf(stderr,""
"`3:\"cmd\"  set\n"
"`4:\"cmd\"  get stdout\n"
"`8:\"cmd\"  get (rc;stdout;stderr)\n");
  }
  else if(a->i=='?') {
    fprintf(stderr,""
"\\d [d|^]  k directory [go to]\n"
"\\e [0|1]  error flag [off|on]\n"
"\\l f      load script f\n"
"\\p [n]    print precision\n"
"\\t x      time in milliseconds to execute x\n"
"\\v        variables\n"
"\\V        variables (long listing)\n");
  }
  return null;
}

int serialize(K *a, char **b, char **v, int *m) {
  char *p;
  int n=sizeof(int),t=at,si=sizeof(int),sd=sizeof(double),pn,dv;
  K *q;
  switch(t) {
  case  1: n+=si; break;
  case  2: n+=sd; break;
  case  3: n+=si; break;
  case  4: n+=1+strlen(a->v); break;
  case  5: n+=si; q=d2l(a->v); break;
  case  6: break;
  case  7: case 27: case 37: case 67: case 77: case 87: p=kprint5(a,"","",0); n+=pn=1+strlen(p); break;
  case -4: n+=si; DO(ac,n+=1+strlen(v4(a)[i])); break;
  case -3: n+=si; n+=ac; break;
  case -2: n+=si; n+=ac*sd; break;
  case -1: n+=si; n+=ac*si; break;
  case  0: n+=si; break;
  default: return 0;
  }
  dv=*v-*b;
  while(dv+n>*m) { *m<<=1; *b=xrealloc(*b,*m); *v=*b+dv; dv=*v-*b; }
  memcpy(*v,&t,si); *v+=si;
  switch(t) {
  case  1: memcpy(*v,&a->i,si); *v+=si; break;
  case  2: memcpy(*v,&a->f,sd); *v+=sd; break;
  case  3: memcpy(*v,&a->i,si); *v+=si; break;
  case  4: memcpy(*v,a->v,1+strlen(a->v)); *v+=1+strlen(a->v); break;
  case  5: memcpy(*v,&q->c,si); *v+=si; DO(q->c,n+=serialize(v0(q)[i],b,v,m)); kfree(q); break;
  case  6: break;
  case  7: case 27: case 37: case 67: case 77: case 87: memcpy(*v,p,pn); xfree(p); *v+=pn; break;
  case -4: memcpy(*v,&ac,si); *v+=si; DO(ac,memcpy(*v,v4(a)[i],1+strlen(v4(a)[i]));*v+=1+strlen(v4(a)[i])); break;
  case -3: memcpy(*v,&ac,si); *v+=si; memcpy(*v,a->v,ac); *v+=ac; break;
  case -2: memcpy(*v,&ac,si); *v+=si; memcpy(*v,a->v,ac*sd); *v+=ac*sd; break;
  case -1: memcpy(*v,&ac,si); *v+=si; memcpy(*v,a->v,ac*si); *v+=ac*si; break;
  case  0: memcpy(*v,&ac,si); *v+=si; DO(ac,n+=serialize(v0(a)[i],b,v,m)); break;
  default: return 0;
  }
  return n;
}
K* deserialize(char **s) {
  K *r=0,*q;
  dict *d;
  int t=0,si=sizeof(int),sd=sizeof(double),n;
  memcpy(&t,*s,si); *s+=si;
  switch(t) {
  case  1: r=k1(0); memcpy(&r->i,*s,si); *s+=si; break;
  case  2: r=k2(0); memcpy(&r->f,*s,sd); *s+=sd; break;
  case  3: r=k3(0); memcpy(&r->i,*s,si); *s+=si; break;
  case  4: r=k4(0); r->v=sp(*s); *s+=1+strlen(r->v); break;
  case  5: q=kv0(*(int*)*s); *s+=si; DO(q->c,v0(q)[i]=deserialize(s)); d=l2d(q); r=k5(d); kfree(q); break;
  case  6: r=null; break;
  case  7: case 27: case 37: r=knew(t,0,fnnew(*s),0,0,0); *s+=1+strlen(*s); break;
  case 67: case 77: case 87: n=strlen(*s); q=kv3(n); memcpy(q->v,*s,q->c); *s+=1+n; r=value_(q); kfree(q); break;
  case -4: r=kv4(*(int*)*s); *s+=si; DO(rc,v4(r)[i]=sp(*s);*s+=1+strlen(*s)); break;
  case -3: r=kv3(*(int*)*s); *s+=si; memcpy(r->v,*s,rc); *s+=rc; break;
  case -2: r=kv2(*(int*)*s); *s+=si; memcpy(r->v,*s,rc*sd); *s+=rc*sd; break;
  case -1: r=kv1(*(int*)*s); *s+=si; memcpy(r->v,*s,rc*si); *s+=rc*si; break;
  case  0: r=kv0(*(int*)*s); *s+=si; DO(rc,v0(r)[i]=deserialize(s)); break;
  default: r=kerror("implement");
  }
  return r;
}
K* bd1_(K *a) {
  K *r=0;
  int n=4,m=4;
  char *b=xcalloc(m,1);
  char *d=b;
  char h[4]={1,0,0,0};
  memcpy(d,h,4); d+=4;
  n=serialize(a,&b,&d,&m);
  if(!n) return kerror("implement");
  r=kv3(0); xfree(r->v); r->v=b; rc=4+n;
  return r;
}

K* db1_(K *a) {
  char h[4],*s;
  if(at!=-3) return kerror("type");
  memcpy(h,a->v,4);
  s=4+(char*)a->v;
  return deserialize(&s);
}

K* load1_(K *a) {
  if(at!=4) return kerror("type");
  load(a4);
  return null;
}

K* slide3_(K *a, K *b, K *c) {
  K *r=0,*p;
  if(bt==7) r=slide(b,a,c,"");
  else if(bt==37) { p=kv0(2); v0(p)[0]=a; v0(p)[1]=c; r=slide37(b,p,""); p->c=0; kfree(p); }
  else if(bt==67) r=slidefc(b,a,c,"");
  EC(r);
  return r;
}
