/* accelerate.c -- f32 matrix multiply via Apple's Accelerate (BLAS) framework,
 * exposed to gk through `2:` link object code. The macOS counterpart to
 * ext/lin/cuda: same gk matrix layout, same test, but CPU SIMD/AMX (cblas_sgemm)
 * instead of the GPU (cuBLAS).
 *
 *   mm :"accelerate.dylib" 2:("mm";2)
 *   mm[A;B]    A:MxK real matrix, B:KxN real matrix -> MxN
 *
 * A gk f32 matrix is a general list (type 0) of real vectors (type -9), one
 * per row, each `cols` floats long -- exactly what la.c's mul_ consumes. We
 * read that with the public gk.h accessors, multiply with cblas_sgemm, and
 * rebuild the same list-of-real-vectors result.
 *
 * Build:  make
 *   cc -O2 -shared -fPIC -undefined dynamic_lookup -I<gkdir> \
 *      accelerate.c -o accelerate.dylib -framework Accelerate
 * (-undefined dynamic_lookup lets the gk_* symbols resolve at load time against
 *  the gk executable, exactly like dlopen on Linux.)
 */
#include <Accelerate/Accelerate.h>
#include <stdlib.h>
#include <string.h>
#include "gk.h"

/* Read an MxK gk real-matrix into a freshly malloc'd row-major host buffer.
   Returns 0 on a shape/type error (rows must be real vectors of equal length);
   on success sets rows/cols and returns the buffer (caller frees). */
static float *read_mat(K m, int *rows, int *cols) {
  if (gk_t(m) != GK_LIST || gk_n(m) == 0) return 0;
  int r = (int)gk_n(m);
  K *row = gk_kv(m);
  if (gk_t(row[0]) != GK_F32V) return 0;       /* require real (-9) rows */
  int c = (int)gk_n(row[0]);
  for (int i = 1; i < r; ++i)
    if (gk_t(row[i]) != GK_F32V || (int)gk_n(row[i]) != c) return 0;
  float *buf = (float *)malloc((size_t)r * c * sizeof(float));
  if (!buf) return 0;
  for (int i = 0; i < r; ++i)
    memcpy(buf + (size_t)i * c, gk_ev(row[i]), (size_t)c * sizeof(float));
  *rows = r; *cols = c;
  return buf;
}

GK_FN K mm(K a, K b) {
  int M, Ka, Kb, N;
  float *hA = read_mat(a, &M, &Ka);
  if (!hA) return gk_err("type");
  float *hB = read_mat(b, &Kb, &N);
  if (!hB) { free(hA); return gk_err("type"); }
  if (Ka != Kb) { free(hA); free(hB); return gk_err("length"); }

  float *hC = (float *)malloc((size_t)M * N * sizeof(float));
  if (!hC) { free(hA); free(hB); return gk_err("wsfull"); }

  /* Accelerate's CBLAS reads row-major directly, so -- unlike the cuBLAS
     (column-major) sibling -- there's no operand-swap trick: just
     C(MxN) = A(MxK) * B(KxN), lda=K, ldb=N, ldc=N. */
  cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
              M, N, Ka, 1.0f, hA, Ka, hB, N, 0.0f, hC, N);
  free(hA); free(hB);

  /* build the MxN result as a list of N-long real vectors */
  K r = gk_mkkv(M);
  K *rr = gk_kv(r);
  for (int i = 0; i < M; ++i) {
    rr[i] = gk_mkev(N);
    memcpy(gk_ev(rr[i]), hC + (size_t)i * N, (size_t)N * sizeof(float));
  }
  free(hC);
  return r;
}
