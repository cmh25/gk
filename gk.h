#ifndef GK_H
#define GK_H
/* ============================================================================
 * gk.h -- foreign-function ABI for gk 2: link object code.
 *
 *   A gk value (K) is a 64-bit tagged word passed BY VALUE (not a pointer).
 *   A 2:-linked C or C++ function takes K arguments and returns a K:
 *
 *       #include "gk.h"
 *       GK_FN K add(K x, K y) { return gk_mki(gk_i(x) + gk_i(y)); }
 *
 *   GK_FN marks a function for export (needed on Windows; no-op elsewhere).
 *   Build it as a shared object and link it in gk:
 *       cc -shared -fPIC add.c -o add.so          (Linux)
 *       add: "add.so" 2: ("add"; 2)               (in gk)
 *       add[2;3]                                   -> 5
 *
 *   Arguments are BORROWED: do not free them (gk owns them). To keep one past
 *   the call, retain it with gk_ref(); return a freshly made value (or
 *   gk_ref(arg)). Return gk_err("msg") to raise an error in the gk caller.
 *
 *   Self-contained: include ONLY this file.
 * ==========================================================================*/
#include <stdint.h>

/* GK_API -- linkage of the gk_* functions. gk itself exports them (it builds
   ffi.c with GK_BUILD defined); a client .so/.dll imports them; on POSIX it is
   empty (the gk build publishes them by name via --dynamic-list /
   -export_dynamic). This keeps the declaration here and the definition in
   ffi.c agreeing on linkage (MSVC C2375 otherwise). */
#if defined(_WIN32)
#  if defined(GK_BUILD)
#    define GK_API __declspec(dllexport)
#  else
#    define GK_API __declspec(dllimport)
#  endif
#else
#  define GK_API
#endif

/* GK_FN -- mark each function you expose to 2:. On Windows a DLL exports
   nothing by default, so 2: (GetProcAddress) can't find an unmarked function;
   GK_FN adds the needed __declspec(dllexport). No-op on POSIX (ELF/Mach-O
   export non-static functions already). Usage:  GK_FN K add(K x, K y){...}  */
#if defined(_WIN32)
#  define GK_FN __declspec(dllexport)
#else
#  define GK_FN
#endif

#ifndef GK_NO_K_TYPEDEF
typedef uint64_t K;             /* a gk value: a 64-bit tagged word, by value */
#endif

/* The heap object behind a vector (or a boxed atom). You rarely touch it
   directly -- use the gk_<t>v() accessors below to get a typed element ptr. */
typedef struct {
  uint64_t n;                   /* element count */
  union { void *v; double f; int64_t j; };
  uint32_t r;                   /* refcount */
  uint32_t m;                   /* flags */
} gk_ko;

/* Type codes returned by gk_t(): positive = atom/list, negative = vector
   (a vector type is the negation of its atom type, e.g. gk_mkiv -> GK_I32v). */
#define GK_LIST   0  /* general (mixed) K list */
#define GK_I32    1  /* i32    atom */
#define GK_F64    2  /* f64    atom */
#define GK_CHAR   3  /* char   atom */
#define GK_SYM    4  /* symbol atom */
#define GK_NULL   6  /* null */
#define GK_I64    8  /* i64    atom */
#define GK_F32    9  /* f32    atom */
#define GK_I32V  -1  /* i32    vector */
#define GK_F64V  -2  /* f64    vector */
#define GK_CHARV -3  /* char   vector */
#define GK_SYMV  -4  /* symbol vector */
#define GK_I64V  -8  /* i64    vector */
#define GK_F32V  -9  /* f32    vector */

/* internal: the heap object behind x (strip the 16 tag bits) */
#define GK_KO(x) ((gk_ko*)((K)(x) & 0xFFFFFFFFFFFFULL))

/* ---- inspect a value ------------------------------------------------------ */
#define gk_t(x)  ((int)(((signed char)((((K)(x) >> 56) & 0x1F) << 3)) >> 3)) /* type code (signed) */
#define gk_n(x)  (GK_KO(x)->n)                          /* element count */

/* ---- read an atom --------------------------------------------------------- */
#define gk_i(x)  ((int32_t)((K)(x) & 0xFFFFFFFFULL))    /* i32  atom value */
#define gk_f(x)  (GK_KO(x)->f)                          /* f64  atom value */
#define gk_j(x)  (GK_KO(x)->j)                          /* i64  atom value */
#define gk_c(x)  ((unsigned char)((K)(x) & 0xFFULL))    /* char atom value */
#define gk_s(x)  ((char*)((K)(x) & 0xFFFFFFFFFFFFULL))  /* symbol atom -> C string */
static inline float gk_e(K x) {                         /* f32  atom value */
  union { uint32_t u; float f; } c;
  c.u = (uint32_t)(x & 0xFFFFFFFFULL);
  return c.f;
}

/* ---- read a vector's elements (typed pointer to gk_n(x) items) ------------ */
#define gk_iv(x) ((int32_t*)(GK_KO(x)->v))              /* i32    vector elements */
#define gk_fv(x) ((double*) (GK_KO(x)->v))              /* f64    vector elements */
#define gk_jv(x) ((int64_t*)(GK_KO(x)->v))              /* i64    vector elements */
#define gk_ev(x) ((float*)  (GK_KO(x)->v))              /* f32    vector elements */
#define gk_cv(x) ((char*)   (GK_KO(x)->v))              /* char   vector elements */
#define gk_sv(x) ((char**)  (GK_KO(x)->v))              /* symbol vector elements */
#define gk_kv(x) ((K*)      (GK_KO(x)->v))              /* K-list elements */

#ifdef __cplusplus
extern "C" {
#endif

/* ---- make an atom --------------------------------------------------------- */
GK_API K gk_mki(int32_t v);            /* i32 atom */
GK_API K gk_mkf(double v);             /* f64 atom */
GK_API K gk_mkj(int64_t v);            /* i64 atom */
GK_API K gk_mke(float v);              /* f32 atom */
GK_API K gk_mkc(int v);                /* char atom */
GK_API K gk_mks(const char *s);        /* symbol atom (interns s) */
GK_API K gk_mknull(void);              /* the null value */

/* ---- make a vector of n elements (uninitialized; fill via gk_<t>v()) ------ */
GK_API K gk_mkiv(int32_t n);           /* i32    vector */
GK_API K gk_mkfv(int32_t n);           /* f64    vector */
GK_API K gk_mkjv(int32_t n);           /* i64    vector */
GK_API K gk_mkev(int32_t n);           /* f32    vector */
GK_API K gk_mkcv(int32_t n);           /* char   vector */
GK_API K gk_mksv(int32_t n);           /* symbol vector */
GK_API K gk_mkkv(int32_t n);           /* general K list */

/* ---- make a char vector from a C string ----------------------------------- */
GK_API K gk_mkstr(const char *s);              /* from a NUL-terminated string */
GK_API K gk_mkstrn(const char *s, int32_t n);  /* from s[0..n) */

/* ---- utilities ------------------------------------------------------------ */
GK_API K    gk_err(const char *msg);   /* an error value; return it to raise in gk */
GK_API K    gk_ref(K x);               /* retain (bump refcount); returns x */
GK_API void gk_unref(K x);             /* release (drop refcount) */
GK_API K    gk_norm(K x);              /* normalize a built K list to a typed vector */

/* ---- dictionaries --------------------------------------------------------- */
/* A gk dict maps symbol keys to K values. gk_t() of a dict reports GK_LIST (0);
   use gk_isdict() to tell them apart. Nesting is free: a value may itself be a
   dict (build inner ones first and store them as values).
   OWNERSHIP: gk_dset and gk_dict CONSUME each value -- the reference is moved
   into the dict, so build a value with gk_mk*() and hand it straight in; do not
   reuse or unref it. To insert a borrowed argument, gk_ref() it first. Keys are
   C strings that the dict interns (copying as needed); you keep your strings.
   The reads return owned values (retained/copied), freed once by gk, like every
   other gk_* return. */
GK_API K    gk_dnew(void);                 /* a fresh empty dict */
GK_API void gk_dset(K d, const char *key, K val);  /* insert/update; CONSUMES val */
GK_API K    gk_dict(const char **keys, const K *vals, int32_t n); /* column sugar over dnew+dset; CONSUMES each val */
GK_API int  gk_isdict(K x);                /* 1 if x is a dict, else 0 */
GK_API K    gk_dkeys(K d);                 /* key symbol vector (owned) */
GK_API K    gk_dvals(K d);                 /* value list (owned) */
GK_API K    gk_dget(K d, const char *key); /* value for key, or the null value */

#ifdef __cplusplus
}
#endif

#endif /* GK_H */
