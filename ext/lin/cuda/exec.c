/* exec.c -- a scalar k lambda is a CUDA kernel, exec is the launch.  Part of the cuda package:
 *
 *   \l cuda
 *   cuda.exec[{x*x+1};v]                    1D grid: one thread per element of v, v[i] is x
 *   cuda.exec[{a[x]+b[x]};!#a]              index space: x is the thread index, a b are free vectors
 *   cuda.exec[{x*y};(X;Y)]                  2D grid: one thread per (x;y), x from X, y from Y -> (#X;#Y) matrix
 *   cuda.exec[{x+y+z};(X;Y;Z)]              3D grid -> list of matrices
 *   cuda.exec[{+/x*x};v]                    a reduction (+ * & |): per-block shared-memory tree, then the partials -> an atom
 *   g:{a:v;b:w;cuda.exec[{a[x]+b[x]};!n]}   free names resolve through the lambda's closure: g's locals
 *   cuda.src[f;x]   the generated CUDA source      cuda.stats[]   timings of the last exec      cuda.info[]   a table of devices
 *
 * The lambda itself is passed in. Its text (gk_fntext) is transpiled here to CUDA C, compiled with
 * NVRTC, cached by source, and launched with the driver API. Free names are resolved the way the
 * body would see them (gk_fnget: the enclosing function's locals, then globals); free vectors become
 * pointer args indexed a[x], free atoms become by-value args. --fmad=false keeps results bit-identical
 * to gk. An undefined name is a value error, wrong arity a valence error, as anywhere in gk.
 * A list of vectors is the AXES of the grid (the outer product, k's f/:\:), not each-both: for f'[a;b]
 * write the index form {a[x]+b[x]} over !#a.  Block shape is chosen here (256 / 16x16 / 8x8x4).
 * Scalar subset: + - * % & | < > = ! ^ ~ dyads; - % _ ~ monads; literals; a[e] indexing; parens;
 * right to left, no precedence (k rules). i32, f64, i64 element types. No sentinels, no bounds checks.
 *
 * Build: see makefile (cc for this file, nvcc links it with cuda.cu into cuda.so; -lnvrtc -lcuda). */
#include "gk.h"
#include <cuda.h>
#include <nvrtc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <stdarg.h>

typedef enum { T_I=1, T_F=2, T_J=3 } ty;                   /* int32, double, int64 */
static const char *cty[] = { 0, "int", "double", "long long" };

/* ---------------------------------------------------------------- errors */
static char errbuf[512];
static int failed;
#define FAIL(...) do{ if(!failed){ snprintf(errbuf,sizeof errbuf,"cuda: " __VA_ARGS__); errbuf[sizeof errbuf-1]=0; failed=1; } }while(0)

/* ---------------------------------------------------------------- lexer */
typedef enum { K_END, K_NUM, K_NAME, K_VERB, K_LBR, K_RBR, K_LP, K_RP } kind;
typedef struct { kind k; char s[64]; int isf; } tok;
#define MAXTOK 1024
static tok toks[MAXTOK]; static int ntok, tp;

static int isverb(int c){ return c && strchr("+-*%&|<>=!^~_", c) != 0; }

static void lex(const char *s) {
  ntok = 0;
  while(*s && !failed) {
    if(ntok >= MAXTOK-1) { FAIL("lambda too long"); return; }
    if(isspace((unsigned char)*s)) { s++; continue; }
    tok *t = &toks[ntok];
    int prevterm = ntok && (toks[ntok-1].k==K_NUM || toks[ntok-1].k==K_NAME || toks[ntok-1].k==K_RBR || toks[ntok-1].k==K_RP);
    if(isdigit((unsigned char)*s) || (*s=='.' && isdigit((unsigned char)s[1])) || (*s=='-' && isdigit((unsigned char)s[1]) && !prevterm)) {
      int i = 0; t->isf = 0;
      if(*s=='-') t->s[i++] = *s++;
      while(isdigit((unsigned char)*s) || *s=='.' || ((*s=='e'||*s=='E') && (isdigit((unsigned char)s[1]) || s[1]=='-' || s[1]=='+'))) {
        if(*s=='.' || *s=='e' || *s=='E') t->isf = 1;
        if(*s=='e' || *s=='E') { t->s[i++] = *s++; if(*s=='-'||*s=='+') t->s[i++] = *s++; continue; }
        if(i < 60) t->s[i++] = *s;
        s++;
      }
      t->s[i] = 0; t->k = K_NUM; ntok++; continue;
    }
    if(isalpha((unsigned char)*s)) {
      int i = 0; while(isalnum((unsigned char)*s)) { if(i < 60) t->s[i++] = *s; s++; }
      t->s[i] = 0; t->k = K_NAME; ntok++; continue;
    }
    if(isverb(*s)) { t->k = K_VERB; t->s[0] = *s++; t->s[1] = 0; ntok++; continue; }
    if(*s=='[') { t->k = K_LBR; s++; ntok++; continue; }
    if(*s==']') { t->k = K_RBR; s++; ntok++; continue; }
    if(*s=='(') { t->k = K_LP;  s++; ntok++; continue; }
    if(*s==')') { t->k = K_RP;  s++; ntok++; continue; }
    FAIL("unsupported character '%c' in lambda body", *s); return;
  }
  toks[ntok].k = K_END;
}

/* ---------------------------------------------------------------- symbols */
typedef struct { char name[64]; ty t; int vec; int isarg; } sym;   /* isarg: per-thread arg (scalar in kernel) */
#define MAXSYM 24
static sym syms[MAXSYM]; static int nsym;
static sym *find(const char *n){ for(int i=0;i<nsym;i++) if(!strcmp(syms[i].name,n)) return &syms[i]; return 0; }
static void addsym(const char *n, ty t, int vec, int isarg){
  if(nsym >= MAXSYM) { FAIL("too many names"); return; }
  snprintf(syms[nsym].name, 64, "%s", n); syms[nsym].t = t; syms[nsym].vec = vec; syms[nsym].isarg = isarg; nsym++;
}

/* ---------------------------------------------------------------- parser -> C expression */
typedef struct { ty t; char *code; } node;
static node mk(ty t, const char *fmt, ...) {
  char buf[4096]; va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
  node n; n.t = t; n.code = strdup(buf); return n;
}
static ty join(ty a, ty b){ return (a==T_F||b==T_F) ? T_F : (a==T_J||b==T_J) ? T_J : T_I; }
static node parse_expr(void);

/* k names never contain '_' (it is the drop verb), so every generated identifier carries one and cannot
   collide with a user name.  A user name that is a C++ keyword or a CUDA builtin would break the compile
   (or quietly shadow blockIdx); those few are emitted with a _v suffix. */
static const char *cname(const char *n) {
  static const char *reserved[] = { "auto","bool","break","case","catch","char","class","const","continue","default","delete",
    "do","double","else","enum","explicit","export","extern","false","float","for","friend","goto","if","inline","int","long",
    "mutable","namespace","new","noexcept","nullptr","operator","private","protected","public","register","requires","return",
    "short","signed","sizeof","static","struct","switch","template","this","throw","true","try","typedef","typeid","typename",
    "union","unsigned","using","virtual","void","volatile","while","and","or","not","xor","compl","bitand","bitor","asm",
    "blockIdx","threadIdx","blockDim","gridDim","warpSize","k","kmin","kmax","kmod","pow","floor","fmod", 0 };
  static char buf[8][80]; static int bi;
  for(int i = 0; reserved[i]; i++) if(!strcmp(n, reserved[i])) { char *b = buf[bi++ & 7]; snprintf(b, 80, "%s_v", n); return b; }
  return n;
}

static node parse_term(void) {
  node r = { T_I, 0 };
  tok *t = &toks[tp];
  if(t->k == K_NUM) {
    tp++;
    if(t->isf) return mk(T_F, "%s", t->s);
    long long v = strtoll(t->s, 0, 10);
    if(v > INT32_MAX || v < INT32_MIN) return mk(T_J, "%sLL", t->s);
    return mk(T_I, "%s", t->s);
  }
  if(t->k == K_NAME) {
    tp++;
    sym *s = find(t->s);
    if(!s) { FAIL("undefined name %s", t->s); return r; }
    if(s->vec) {
      if(toks[tp].k != K_LBR) { FAIL("vector %s used without an index", t->s); return r; }
      tp++; node e = parse_expr(); if(failed) return r;
      if(toks[tp].k != K_RBR) { FAIL("expected ] after index of %s", t->s); return r; }
      tp++;
      r = mk(s->t, "%s[(long)(%s)]", cname(t->s), e.code); free(e.code); return r;
    }
    return mk(s->t, "%s", cname(t->s));
  }
  if(t->k == K_LP) {
    tp++; node e = parse_expr(); if(failed) return r;
    if(toks[tp].k != K_RP) { FAIL("expected )"); return r; }
    tp++; r = mk(e.t, "(%s)", e.code); free(e.code); return r;
  }
  if(t->k == K_END) FAIL("unexpected end of expression"); else FAIL("unexpected token");
  return r;
}

static node monad(char v, node a) {
  node r = { T_I, 0 };
  switch(v) {
  case '-': r = mk(a.t, "(-(%s))", a.code); break;
  case '%': r = mk(T_F, "(1.0/(double)(%s))", a.code); break;
  case '_': r = (a.t==T_F) ? mk(T_I, "((int)floor(%s))", a.code) : mk(a.t, "(%s)", a.code); break;
  case '~': r = mk(T_I, "((%s)==0)", a.code); break;
  default: FAIL("monadic %c is not scalar", v);
  }
  free(a.code); return r;
}

static node dyad(node a, char v, node b) {
  node r = { T_I, 0 }; ty t = join(a.t, b.t); const char *T = cty[t];
  switch(v) {
  case '+': case '-': case '*': r = mk(t, "((%s)(%s)%c(%s)(%s))", T, a.code, v, T, b.code); break;
  case '%': r = mk(T_F, "((double)(%s)/(double)(%s))", a.code, b.code); break;
  case '^': r = mk(T_F, "pow((double)(%s),(double)(%s))", a.code, b.code); break;
  case '!': r = mk(t, "kmod((%s)(%s),(%s)(%s))", T, a.code, T, b.code); break;
  case '&': r = mk(t, "kmin((%s)(%s),(%s)(%s))", T, a.code, T, b.code); break;
  case '|': r = mk(t, "kmax((%s)(%s),(%s)(%s))", T, a.code, T, b.code); break;
  case '<': r = mk(T_I, "((%s)<(%s))", a.code, b.code); break;
  case '>': r = mk(T_I, "((%s)>(%s))", a.code, b.code); break;
  case '=': case '~': r = mk(T_I, "((%s)==(%s))", a.code, b.code); break;
  default: FAIL("dyadic %c unsupported", v);
  }
  free(a.code); free(b.code); return r;
}

static node parse_expr(void) {               /* k: right to left, no precedence */
  node bad = { T_I, 0 };
  if(toks[tp].k == K_VERB) { char v = toks[tp].s[0]; tp++; node e = parse_expr(); if(failed) return bad; return monad(v, e); }
  node t = parse_term(); if(failed) return bad;
  if(toks[tp].k == K_VERB) { char v = toks[tp].s[0]; tp++; node e = parse_expr(); if(failed) { free(t.code); return bad; } return dyad(t, v, e); }
  return t;
}

/* ---------------------------------------------------------------- lambda header: {[a;b] body} */
static int header(char *src, char names[3][64], int *np, char **body) {
  char *p = src;
  while(isspace((unsigned char)*p)) p++;
  if(*p != '{') { FAIL("not a lambda"); return 0; }
  p++; *np = 0;
  if(*p == '[') {
    p++;
    for(;;) {
      while(isspace((unsigned char)*p)) p++;
      int i = 0; while(isalnum((unsigned char)*p)) { if(i < 60) names[*np][i++] = *p; p++; }
      names[*np][i] = 0; if(i == 0 || *np >= 3) { FAIL("bad parameter list"); return 0; }
      (*np)++;
      while(isspace((unsigned char)*p)) p++;
      if(*p == ';') { p++; continue; }
      if(*p == ']') { p++; break; }
      FAIL("bad parameter list"); return 0;
    }
  }
  char *end = strrchr(p, '}');
  if(!end) { FAIL("missing }"); return 0; }
  *end = 0; *body = p;
  return 1;
}

static char *ktext(K text) {                 /* NUL-terminated copy of a gk char vector */
  if(gk_t(text) != GK_CHARV) return 0;
  int64_t n = gk_n(text); char *s = malloc(n + 1); memcpy(s, gk_cv(text), n); s[n] = 0; return s;
}

/* ---------------------------------------------------------------- CUDA state */
static CUdevice dev; static CUcontext ctx; static int inited; static char arch[64];
static double stats[6];                       /* compile, h2d, kernel, d2h, total (ms), n */
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec*1e3 + t.tv_nsec/1e6; }

static const char *cuerr(CUresult r){ const char *s = 0; cuGetErrorString(r, &s); return s ? s : "cuda error"; }
#define CU(x) do{ CUresult r_=(x); if(r_!=CUDA_SUCCESS){ FAIL("%s (%s)", cuerr(r_), #x); goto fail; } }while(0)

/* kernel cache: source text -> function */
typedef struct { char *src; CUmodule m; CUfunction f; } kent;
#define MAXK 64
static kent kcache[MAXK]; static int nk;

/* Tear the context down before the process exits, while gk's terminal is still open.  Left to
   exit_group, the kernel closes fds 0-2 first and then spends ~50 ms releasing the driver's
   fds, during which a wrapper like rlwrap sees its pty go away under a live child and reports
   "read error on master pty".  Registered with atexit from the .so, so gk's dlclose at exit
   (link_shutdown) runs it via __cxa_finalize, before exit(). */
static void cu_shutdown(void) {
  if(!inited) return;
  cuCtxSetCurrent(ctx); cuCtxSynchronize();
  for(int i = 0; i < nk; i++) { if(kcache[i].m) cuModuleUnload(kcache[i].m); free(kcache[i].src); kcache[i].m = 0; kcache[i].src = 0; }
  nk = 0;
  cuDevicePrimaryCtxRelease(dev);
  inited = 0;
}

static int cuinit(void) {
  if(inited) { cuCtxSetCurrent(ctx); return 1; }
  CUresult r = cuInit(0); if(r) { FAIL("cuInit: %s", cuerr(r)); return 0; }
  if(cuDeviceGet(&dev, 0)) { FAIL("no CUDA device"); return 0; }
  int maj, min; cuDeviceGetAttribute(&maj, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, dev);
  cuDeviceGetAttribute(&min, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, dev);
  snprintf(arch, sizeof arch, "--gpu-architecture=compute_%d%d", maj, min);
  if(cuDevicePrimaryCtxRetain(&ctx, dev)) { FAIL("primary context"); return 0; }
  cuCtxSetCurrent(ctx); inited = 1; atexit(cu_shutdown); return 1;
}

static char *lastsrc;                          /* source of the kernel the last exec used */

static const char *preamble =
  "template<class T> __device__ inline T kmin(T a,T b){return a<b?a:b;}\n"
  "template<class T> __device__ inline T kmax(T a,T b){return a<b?b:a;}\n"
  "__device__ inline int kmod(int a,int b){if(!b)return a;int m=a%b;return (m&&((m<0)!=(b<0)))?m+b:m;}\n"
  "__device__ inline long long kmod(long long a,long long b){if(!b)return a;long long m=a%b;return (m&&((m<0)!=(b<0)))?m+b:m;}\n"
  "__device__ inline double kmod(double a,double b){if(b==0.0)return a;double m=fmod(a,b);return (m!=0.0&&((m<0)!=(b<0)))?m+b:m;}\n";

static CUfunction getkernel(const char *src) {
  for(int i = 0; i < nk; i++) if(!strcmp(kcache[i].src, src)) return kcache[i].f;
  if(nk >= MAXK) { FAIL("kernel cache full"); return 0; }
  double t0 = now();
  nvrtcProgram p; if(nvrtcCreateProgram(&p, src, "cu.cu", 0, 0, 0)) { FAIL("nvrtcCreateProgram"); return 0; }
  const char *opts[] = { arch, "--std=c++17", "--fmad=false" };   /* no FMA contraction: bit-identical to the CPU */
  nvrtcResult cr = nvrtcCompileProgram(p, 3, opts);
  if(cr != NVRTC_SUCCESS) {
    size_t ls; nvrtcGetProgramLogSize(p, &ls); char *l = malloc(ls + 1); nvrtcGetProgramLog(p, l); l[ls] = 0;
    char *nl = strchr(l, '\n'); if(nl) *nl = 0;            /* first line of the log */
    FAIL("nvrtc: %s", l); free(l); nvrtcDestroyProgram(&p); return 0;
  }
  size_t ps; nvrtcGetPTXSize(p, &ps); char *ptx = malloc(ps); nvrtcGetPTX(p, ptx); nvrtcDestroyProgram(&p);
  CUmodule m; CUfunction f;
  if(cuModuleLoadData(&m, ptx)) { FAIL("cuModuleLoadData"); free(ptx); return 0; }
  free(ptx);
  if(cuModuleGetFunction(&f, m, "k")) { FAIL("cuModuleGetFunction"); return 0; }
  kcache[nk].src = strdup(src); kcache[nk].m = m; kcache[nk].f = f; nk++;
  stats[0] = now() - t0;
  return f;
}

/* ---------------------------------------------------------------- bind a gk value to a type */
static int vty(K v, ty *t, int *vec) {       /* 1 ok */
  switch(gk_t(v)) {
  case GK_I32V: *t = T_I; *vec = 1; return 1;  case GK_F64V: *t = T_F; *vec = 1; return 1;  case GK_I64V: *t = T_J; *vec = 1; return 1;
  case GK_I32:  *t = T_I; *vec = 0; return 1;  case GK_F64:  *t = T_F; *vec = 0; return 1;  case GK_I64:  *t = T_J; *vec = 0; return 1;
  }
  return 0;
}
static size_t esz(ty t){ return t==T_I ? 4 : 8; }
static const void *vptr(K v, ty t){ return t==T_I ? (void*)gk_iv(v) : t==T_F ? (void*)gk_fv(v) : (void*)gk_jv(v); }

/* ================================================================ a lambda's text and free names */
static char *fntext(K f) {                   /* malloc'd source text of a lambda, or 0 (errbuf set) */
  if(!gk_isfn(f)) { FAIL("a lambda expected"); return 0; }
  K t = gk_fntext(f); char *s = ktext(t); gk_unref(t);
  if(!s) FAIL("lambda text");
  return s;
}
/* names in the lexed body that are not the lambda's own params: its free variables */
static int freenames(char pn[3][64], int np, char names[MAXSYM][64]) {
  int n = 0;
  for(int i = 0; i < ntok; i++) if(toks[i].k == K_NAME) {
    const char *s = toks[i].s; int skip = 0;
    if(np) { for(int j = 0; j < np; j++) if(!strcmp(s, pn[j])) skip = 1; }
    else if(!strcmp(s,"x") || !strcmp(s,"y") || !strcmp(s,"z")) skip = 1;
    for(int j = 0; j < n; j++) if(!strcmp(names[j], s)) skip = 1;
    if(!skip && n < MAXSYM) snprintf(names[n++], 64, "%s", s);
  }
  return n;
}

static K mkvec(ty t, int64_t n) { return t == T_I ? gk_mkiv(n) : t == T_F ? gk_mkfv(n) : gk_mkjv(n); }

/* ================================================================ exported: exec[f;X] = f'X;  exec[f;(X;Y)] / exec[f;(X;Y;Z)] = a 2D/3D grid, one thread per (x;y;z) */
GK_FN K cu_exec(K f, K x) {
  failed = 0; memset(stats, 0, sizeof stats);
  double t0 = now();
  if(!cuinit()) return gk_err(errbuf);
  char *src = fntext(f); if(!src) return gk_err(errbuf);
  K result = 0; char *ksrc = 0; node e = { T_I, 0 };
  CUdeviceptr dptr[MAXSYM + 4]; int nd = 0;
  memset(dptr, 0, sizeof dptr);
  K fv[MAXSYM]; int nfv = 0;                 /* free-name values from gk_fnget: owned, released on every exit */
  K err = 0;                                 /* an error value handed back by gk, returned as-is */
  char names[MAXSYM][64]; ty ft[MAXSYM]; int fvec[MAXSYM]; int nf = 0;
  void *stage = 0;                           /* host staging for a 2D/3D result */
  CUdeviceptr dres = 0;

  /* axes: one vector (a 1D grid, x = its elements) or a list of two or three (a 2D/3D grid, one
     thread per combination: x from the first axis, y from the second, z from the third) */
  K ax[3]; ty at[3]; int64_t dim[3] = { 1, 1, 1 }; int na = 1; ax[0] = x;
  if(gk_t(x) == GK_LIST) {
    na = (int)gk_n(x); if(na < 1 || na > 3) { FAIL("1 to 3 axes"); goto fail; }
    for(int i = 0; i < na; i++) ax[i] = gk_kv(x)[i];
  }
  int64_t n = 1;
  for(int i = 0; i < na; i++) {
    int vec;
    if(!vty(ax[i], &at[i], &vec) || !vec) { FAIL("axis %d must be an int, long or float vector", i); goto fail; }
    dim[i] = gk_n(ax[i]);
    if(dim[i] && n > INT64_MAX / dim[i]) { err = gk_err("wsfull"); goto fail; }
    n *= dim[i];
  }
  /* lambda header and symbol table */
  char pn[3][64]; int np; char *body;
  if(!header(src, pn, &np, &body)) goto fail;
  /* a reduction: the body is V/ expr with V one of + * & | (associative, so a tree is valid).  expr is the
     per-thread scalar part as usual; the launch then folds each 256-thread block in shared memory, writes one
     partial per block, and the host folds the partials.  The result is an atom, as +/ gives in k. */
  char red = 0;
  { char *bp = body; while(isspace((unsigned char)*bp)) bp++;
    if(bp[0] && bp[1] == '/') {
      if(!strchr("+*&|", bp[0])) { FAIL("only + * & | reductions"); goto fail; }
      red = bp[0]; body = bp + 2;
    } }
  if(red && na != 1) { FAIL("a reduction runs over a 1D grid"); goto fail; }
  int np0 = np;                              /* 0: implicit params x y z */
  if(np == 0) { np = na; strcpy(pn[0],"x"); strcpy(pn[1],"y"); strcpy(pn[2],"z"); }
  if(np != na) { err = gk_err("valence"); goto fail; }
  nsym = 0;
  for(int i = 0; i < na; i++) addsym(pn[i], at[i], 0, 1);
  lex(body); if(failed) goto fail;
  if(np0 == 0) {                             /* implicit valence: the highest of x y z the body uses */
    int need = 1;
    for(int i = 0; i < ntok; i++) if(toks[i].k == K_NAME) { if(!strcmp(toks[i].s,"y") && need < 2) need = 2; if(!strcmp(toks[i].s,"z")) need = 3; }
    if(need != na) { err = gk_err("valence"); goto fail; }   /* as gk: {x+1}'[a;b] is a valence error */
  }
  /* free names: resolved through the lambda's own scope -- its closure, then the globals */
  nf = freenames(pn, np0, names);
  for(int i = 0; i < nf; i++) {
    K v = gk_fnget(f, names[i]);
    if(gk_iserr(v)) { err = v; goto fail; }   /* undefined: gk's own value error, raised at the caller */
    fv[nfv++] = v;
    if(!vty(v, &ft[i], &fvec[i])) { FAIL("free name %.63s must be an int, long or float atom or vector", names[i]); goto fail; }
    addsym(names[i], ft[i], fvec[i], 0);
  }
  if(failed) goto fail;
  /* parse */
  tp = 0; e = parse_expr(); if(failed) goto fail;
  if(toks[tp].k != K_END) { FAIL("trailing tokens"); goto fail; }

  /* generate the kernel.  Axis 0 is the slowest-varying (blockIdx.z/.y), the last axis maps to
     threadIdx.x, so adjacent threads write adjacent result elements. */
  {
    char sig[4096] = "", load[2048] = "";
    static const char *idx[3] = { "i__", "j__", "k__" };
    for(int i = 0; i < na; i++) { char b[256]; const char *nm = cname(pn[i]);
                                  snprintf(b, sizeof b, "const %s* %s_, ", cty[at[i]], nm); strcat(sig, b);
                                  snprintf(b, sizeof b, "%s %s=%s_[%s]; ", cty[at[i]], nm, nm, idx[i]); strcat(load, b); }
    for(int i = 0; i < nf; i++) { char b[256]; const char *nm = cname(names[i]);
                                  if(fvec[i]) snprintf(b, sizeof b, "const %s* %s, ", cty[ft[i]], nm); else snprintf(b, sizeof b, "%s %s, ", cty[ft[i]], nm);
                                  strcat(sig, b); }
    static const char *grid[3] = {
      "  long i__ = blockIdx.x*(long)blockDim.x + threadIdx.x;\n"
      "  if(i__<n0__){ %sr__[i__] = %s; }\n",
      "  long j__ = blockIdx.x*(long)blockDim.x + threadIdx.x, i__ = blockIdx.y*(long)blockDim.y + threadIdx.y;\n"
      "  if(i__<n0__&&j__<n1__){ %sr__[i__*n1__+j__] = %s; }\n",
      "  long k__ = blockIdx.x*(long)blockDim.x + threadIdx.x, j__ = blockIdx.y*(long)blockDim.y + threadIdx.y, i__ = blockIdx.z*(long)blockDim.z + threadIdx.z;\n"
      "  if(i__<n0__&&j__<n1__&&k__<n2__){ %sr__[(i__*n1__+j__)*n2__+k__] = %s; }\n" };
    size_t L = strlen(preamble) + strlen(sig) + strlen(load) + 2 * strlen(e.code) + 2048;
    ksrc = malloc(L);
    snprintf(ksrc, L, "%s\nextern \"C\" __global__ void k(%s%s* r__, long n0__, long n1__, long n2__){\n", preamble, sig, cty[e.t]);
    size_t h = strlen(ksrc);
    if(!red) snprintf(ksrc + h, L - h, grid[na - 1], load, e.code);
    else {
      /* identity of V for the type (padding for the last block); min/max use the type's extreme */
      const char *id = red == '+' ? "0" : red == '*' ? "1"
        : red == '&' ? (e.t == T_I ? "2147483647" : e.t == T_J ? "9223372036854775807LL" : "__longlong_as_double(0x7ff0000000000000LL)")
        :              (e.t == T_I ? "(-2147483647-1)" : e.t == T_J ? "(-9223372036854775807LL-1)" : "__longlong_as_double(0xfff0000000000000LL)");
      const char *op = red == '+' ? "s__[threadIdx.x]+s__[threadIdx.x+o__]" : red == '*' ? "s__[threadIdx.x]*s__[threadIdx.x+o__]"
        : red == '&' ? "kmin(s__[threadIdx.x],s__[threadIdx.x+o__])" : "kmax(s__[threadIdx.x],s__[threadIdx.x+o__])";
      snprintf(ksrc + h, L - h,
        "  __shared__ %s s__[256];\n"
        "  long i__ = blockIdx.x*(long)blockDim.x + threadIdx.x;\n"
        "  %s v__ = %s;\n"
        "  if(i__<n0__){ %sv__ = %s; }\n"
        "  s__[threadIdx.x] = v__;\n"
        "  __syncthreads();\n"
        "  for(int o__=128;o__>0;o__>>=1){ if(threadIdx.x<o__) s__[threadIdx.x] = %s; __syncthreads(); }\n"
        "  if(threadIdx.x==0) r__[blockIdx.x] = s__[0];\n",
        cty[e.t], cty[e.t], id, load, e.code, op);
    }
    strcat(ksrc, "}\n");
  }
  CUfunction kf = getkernel(ksrc); if(!kf) goto fail;
  free(lastsrc); lastsrc = strdup(ksrc);

  /* device buffers: axes, free vectors, result */
  double t1 = now();
  void *args[MAXSYM + 8]; int nargs = 0;
  union { int i; double f; long long j; } sc[MAXSYM];
  for(int i = 0; i < na; i++) {
    size_t bytes = esz(at[i]) * dim[i];
    if(bytes) { CU(cuMemAlloc(&dptr[nd], bytes)); CU(cuMemcpyHtoD(dptr[nd], vptr(ax[i], at[i]), bytes)); }
    args[nargs++] = &dptr[nd]; nd++;
  }
  for(int i = 0; i < nf; i++) {
    if(fvec[i]) {
      size_t bytes = esz(ft[i]) * gk_n(fv[i]);
      if(bytes) { CU(cuMemAlloc(&dptr[nd], bytes)); CU(cuMemcpyHtoD(dptr[nd], vptr(fv[i], ft[i]), bytes)); }
      args[nargs++] = &dptr[nd]; nd++;
    } else {
      if(ft[i] == T_I) sc[i].i = gk_i(fv[i]); else if(ft[i] == T_F) sc[i].f = gk_f(fv[i]); else sc[i].j = gk_j(fv[i]);
      args[nargs++] = &sc[i];
    }
  }
  int64_t nblk = (n + 255) / 256;            /* partials of a reduction */
  size_t rbytes = esz(e.t) * (red ? nblk : n);
  if(n) CU(cuMemAlloc(&dres, rbytes));
  args[nargs++] = &dres; args[nargs++] = &dim[0]; args[nargs++] = &dim[1]; args[nargs++] = &dim[2];
  CU(cuCtxSynchronize());
  double t2 = now();
  if(n) {
    unsigned bx = 256, by = 1, bz = 1, gx, gy = 1, gz = 1;
    if(na == 1) gx = (unsigned)((dim[0] + 255) / 256);
    else if(na == 2) { bx = 16; by = 16; gx = (unsigned)((dim[1] + 15) / 16); gy = (unsigned)((dim[0] + 15) / 16); }
    else { bx = 8; by = 8; bz = 4; gx = (unsigned)((dim[2] + 7) / 8); gy = (unsigned)((dim[1] + 7) / 8); gz = (unsigned)((dim[0] + 3) / 4); }
    if(gy > 65535 || gz > 65535) { FAIL("an axis is too long for a %dD grid", na); goto fail; }
    CU(cuLaunchKernel(kf, gx, gy, gz, bx, by, bz, 0, 0, args, 0));
    CU(cuCtxSynchronize());
  }
  double t3 = now();
  /* result: an atom (reduction), a vector, a list of rows, or a list of matrices */
  if(red) {
    union { int i; double f; long long j; } acc;
    if(e.t == T_I) acc.i = red == '+' ? 0 : red == '*' ? 1 : red == '&' ? INT32_MAX : INT32_MIN;
    else if(e.t == T_J) acc.j = red == '+' ? 0 : red == '*' ? 1 : red == '&' ? INT64_MAX : INT64_MIN;
    else acc.f = red == '+' ? 0.0 : red == '*' ? 1.0 : red == '&' ? HUGE_VAL : -HUGE_VAL;
    if(n) {
      stage = malloc(rbytes);
      CU(cuMemcpyDtoH(stage, dres, rbytes));
      for(int64_t i = 0; i < nblk; i++) {
        if(e.t == T_I) { int p = ((int*)stage)[i], a = acc.i;
          acc.i = red == '+' ? (int)((unsigned)a + (unsigned)p) : red == '*' ? (int)((unsigned)a * (unsigned)p) : red == '&' ? (p < a ? p : a) : (p > a ? p : a); }
        else if(e.t == T_J) { long long p = ((long long*)stage)[i], a = acc.j;
          acc.j = red == '+' ? (long long)((unsigned long long)a + (unsigned long long)p) : red == '*' ? (long long)((unsigned long long)a * (unsigned long long)p) : red == '&' ? (p < a ? p : a) : (p > a ? p : a); }
        else { double p = ((double*)stage)[i], a = acc.f;
          acc.f = red == '+' ? a + p : red == '*' ? a * p : red == '&' ? (p < a ? p : a) : (p > a ? p : a); }
      }
    } else if(red == '&' || red == '|') {    /* gk: &/!0 is 1, |/!0 is 0 */
      if(e.t == T_I) acc.i = red == '&'; else if(e.t == T_J) acc.j = red == '&'; else acc.f = red == '&';
    }
    result = e.t == T_I ? gk_mki(acc.i) : e.t == T_F ? gk_mkf(acc.f) : gk_mkj(acc.j);
  }
  else if(na == 1) {
    result = mkvec(e.t, n);
    if(n) CU(cuMemcpyDtoH((void*)vptr(result, e.t), dres, rbytes));
  } else {
    stage = malloc(rbytes ? rbytes : 1);
    if(n) CU(cuMemcpyDtoH(stage, dres, rbytes));
    size_t es = esz(e.t); char *p = stage;
    if(na == 2) {
      result = gk_mkkv(dim[0]);
      for(int64_t i = 0; i < dim[0]; i++) { K row = mkvec(e.t, dim[1]); memcpy((void*)vptr(row, e.t), p, es * dim[1]); p += es * dim[1]; gk_kv(result)[i] = row; }
    } else {
      result = gk_mkkv(dim[0]);
      for(int64_t i = 0; i < dim[0]; i++) {
        K mat = gk_mkkv(dim[1]);
        for(int64_t j = 0; j < dim[1]; j++) { K row = mkvec(e.t, dim[2]); memcpy((void*)vptr(row, e.t), p, es * dim[2]); p += es * dim[2]; gk_kv(mat)[j] = row; }
        gk_kv(result)[i] = mat;
      }
    }
  }
  double t4 = now();
  if(dres) cuMemFree(dres);
  for(int i = 0; i < nd; i++) if(dptr[i]) cuMemFree(dptr[i]);
  stats[1] = t2 - t1; stats[2] = t3 - t2; stats[3] = t4 - t3; stats[4] = t4 - t0; stats[5] = (double)n;
  for(int i = 0; i < nfv; i++) gk_unref(fv[i]);
  free(stage); free(ksrc); free(e.code); free(src);
  return result;

fail:
  if(dres) cuMemFree(dres);
  for(int i = 0; i < nd; i++) if(dptr[i]) cuMemFree(dptr[i]);
  for(int i = 0; i < nfv; i++) gk_unref(fv[i]);
  if(result) gk_unref(result);
  free(stage);
  if(ksrc) free(ksrc);
  if(e.code) free(e.code);
  free(src);
  return err ? err : gk_err(errbuf);
}

/* ================================================================ exported: info[] = a table of the CUDA devices, one row each */
static int cores_per_sm(int maj, int min) {        /* CUDA cores per multiprocessor by compute capability; 0 if unknown */
  switch(maj * 10 + min) {
  case 30: case 32: case 35: case 37: return 192;
  case 50: case 52: case 53: return 128;
  case 60: return 64;  case 61: case 62: return 128;
  case 70: case 72: case 75: return 64;
  case 80: return 64;  case 86: case 87: case 89: return 128;
  case 90: return 128;
  case 100: case 101: case 103: case 120: case 121: return 128;
  }
  return 0;
}
GK_FN K cu_info(K x) {
  (void)x; failed = 0;
  CUresult r = cuInit(0); if(r) { FAIL("cuInit: %s", cuerr(r)); return gk_err(errbuf); }
  int cnt = 0, drv = 0, nvmaj = 0, nvmin = 0;
  cuDeviceGetCount(&cnt); cuDriverGetVersion(&drv); nvrtcVersion(&nvmaj, &nvmin);
  K name = gk_mkkv(cnt), bus = gk_mksv(cnt), cc = gk_mkfv(cnt), sm = gk_mkiv(cnt), cores = gk_mkiv(cnt), clock = gk_mkjv(cnt),
    mem = gk_mkjv(cnt), bw = gk_mkjv(cnt), l2 = gk_mkiv(cnt), shmem = gk_mkiv(cnt), threads = gk_mkiv(cnt),
    integrated = gk_mkiv(cnt), unified = gk_mkiv(cnt), driver = gk_mkfv(cnt), nvrtc = gk_mkfv(cnt);
  for(int d = 0; d < cnt; d++) {
    CUdevice dv; cuDeviceGet(&dv, d);
    char nm[256] = "", pci[64] = "";
    cuDeviceGetName(nm, sizeof nm, dv); gk_kv(name)[d] = gk_mkstr(nm);
    cuDeviceGetPCIBusId(pci, sizeof pci, dv); gk_sv(bus)[d] = gk_intern(pci);
    int maj = 0, min = 0, v = 0, mclk = 0, width = 0; size_t bytes = 0;
    cuDeviceGetAttribute(&maj, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, dv);
    cuDeviceGetAttribute(&min, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, dv);
    gk_fv(cc)[d] = maj + min / 10.0;
    cuDeviceGetAttribute(&v, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, dv); gk_iv(sm)[d] = v;
    gk_iv(cores)[d] = v * cores_per_sm(maj, min);
    cuDeviceGetAttribute(&v, CU_DEVICE_ATTRIBUTE_CLOCK_RATE, dv); gk_jv(clock)[d] = (int64_t)v * 1000;        /* Hz (the attribute is kHz) */
    cuDeviceTotalMem(&bytes, dv); gk_jv(mem)[d] = (int64_t)bytes;
    cuDeviceGetAttribute(&mclk, CU_DEVICE_ATTRIBUTE_MEMORY_CLOCK_RATE, dv);                                   /* kHz */
    cuDeviceGetAttribute(&width, CU_DEVICE_ATTRIBUTE_GLOBAL_MEMORY_BUS_WIDTH, dv);                            /* bits */
    gk_jv(bw)[d] = (int64_t)(2.0 * mclk * 1e3 * (width / 8.0));                                                /* bytes/s: DDR, kHz clock, bits wide */
    cuDeviceGetAttribute(&v, CU_DEVICE_ATTRIBUTE_L2_CACHE_SIZE, dv); gk_iv(l2)[d] = v;
    cuDeviceGetAttribute(&v, CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_BLOCK, dv); gk_iv(shmem)[d] = v;
    cuDeviceGetAttribute(&v, CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_BLOCK, dv); gk_iv(threads)[d] = v;
    cuDeviceGetAttribute(&v, CU_DEVICE_ATTRIBUTE_INTEGRATED, dv); gk_iv(integrated)[d] = v;
    cuDeviceGetAttribute(&v, CU_DEVICE_ATTRIBUTE_PAGEABLE_MEMORY_ACCESS, dv); gk_iv(unified)[d] = v;
    gk_fv(driver)[d] = drv / 1000 + (drv % 1000) / 100 / 10.0;
    gk_fv(nvrtc)[d] = nvmaj + nvmin / 10.0;
  }
  K t = gk_dnew();                           /* a table: every column is a vector with one item per device.
                                                units are base SI: bytes, bytes/s, Hz */
  gk_dset(t, "name", name);                  /* device name */
  gk_dset(t, "bus", bus);                    /* PCI bus id */
  gk_dset(t, "arch", cc);                    /* compute capability, e.g. 8.6 (Ampere); what NVRTC targets */
  gk_dset(t, "sm", sm);                      /* multiprocessors */
  gk_dset(t, "cores", cores);                /* CUDA cores (0 if the architecture is unknown here) */
  gk_dset(t, "clock", clock);                /* Hz */
  gk_dset(t, "mem", mem);                    /* memory, bytes */
  gk_dset(t, "bw", bw);                      /* theoretical memory bandwidth, bytes/s */
  gk_dset(t, "l2", l2);                      /* L2 cache, bytes */
  gk_dset(t, "shmem", shmem);                /* shared memory per block, bytes */
  gk_dset(t, "threads", threads);            /* max threads per block */
  gk_dset(t, "integrated", integrated);      /* 1: shares memory with the host (Jetson, Spark) */
  gk_dset(t, "unified", unified);            /* 1: the GPU can address pageable host memory (no copies needed) */
  gk_dset(t, "driver", driver);              /* driver API version, e.g. 13.0 */
  gk_dset(t, "nvrtc", nvrtc);                /* NVRTC version */
  return t;
}

/* ================================================================ exported: timing of the last run */
GK_FN K cu_stats(K x) {                      /* compile h2d kernel d2h total (ms), n of the last exec; x ignored */
  (void)x;
  K r = gk_mkfv(6); for(int i = 0; i < 6; i++) gk_fv(r)[i] = stats[i]; return r;
}

/* ================================================================ exported: the generated source of a lambda (debug) */
GK_FN K cu_src(K f, K x) {
  K r = cu_exec(f, x);
  if(gk_iserr(r)) return r;
  gk_unref(r);
  return gk_mkstr(lastsrc ? lastsrc : "");
}
