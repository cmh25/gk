/* cuda.cu -- CUDA f32 matrix multiply exposed to gk via 2: link object code.
 *
 *   gmul :"cuda.so" 2:("gmul";2)
 *   ginit:"cuda.so" 2:("ginit";0)
 *   ginit`                        / initialize gpu
 *   gmul[A;B]                     / A:MxK real matrix, B:KxN real matrix -> MxN
 *
 * A gk f32 matrix is a list (type 0) of f32 vectors (type -9).
 * We read that layout with the public gk.h accessors, multiply on the GPU
 * with cuBLAS sgemm, and rebuild the list of f32 vectors result.
 *
 * Build:  make   (nvcc -O3 -shared -Xcompiler -fPIC -I<gkdir> cuda.cu -o cuda.so -lcublas) */
#include <cuda_runtime.h>
#include <cublas_v2.h>
#include "gk.h"

/* One cuBLAS handle for the process. cublasCreate is expensive (tens of ms)
   and materializes the CUDA context, so we make it once -- ideally via
   ginit[] before timing, but gmul[] also creates it lazily if you skip that. */
static cublasHandle_t g_handle = 0;

static cublasStatus_t ensure_handle(void) {
  if (g_handle) return CUBLAS_STATUS_SUCCESS;
  return cublasCreate(&g_handle);
  /* To trade f32 precision for tensor-core speed, add after create:
     cublasSetMathMode(g_handle, CUBLAS_TF32_TENSOR_OP_MATH);
     -- much faster, but only 10-bit mantissa, so it stops matching CPU mul. */
}

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

/* ginit() -- create the cuBLAS handle (and thus the CUDA context) up front so
   the first real gmul[] call isn't charged the one-time init. */
extern "C" GK_FN K ginit(void) {
  if (ensure_handle() != CUBLAS_STATUS_SUCCESS) return gk_err("cublas init");

  /* Warm the *compute* path too, not just the context: the first real
     cublasSgemm otherwise pays for lazy CUDA module loading (the GEMM
     kernels load into the context on first use; CUDA 12+ defaults to
     CUDA_MODULE_LOADING=LAZY) plus the first sizeable cudaMalloc growing the
     device arena. One small throwaway multiply loads the shared cuBLAS
     infrastructure. Buffers are zeroed so the dummy gemm is deterministic
     (uninitialized device memory could be NaN/denormal). */
  enum { W = 64 };
  float *dA = 0, *dB = 0, *dC = 0;
  size_t sz = (size_t)W * W * sizeof(float);
  if (cudaMalloc(&dA, sz) == cudaSuccess &&
      cudaMalloc(&dB, sz) == cudaSuccess &&
      cudaMalloc(&dC, sz) == cudaSuccess) {
    cudaMemset(dA, 0, sz);
    cudaMemset(dB, 0, sz);
    const float alpha = 1.0f, beta = 0.0f;
    cublasSgemm(g_handle, CUBLAS_OP_N, CUBLAS_OP_N, W, W, W,
                &alpha, dB, W, dA, W, &beta, dC, W);
    cudaDeviceSynchronize();
  }
  cudaFree(dA); cudaFree(dB); cudaFree(dC);
  return gk_mki(0);
}

extern "C" GK_FN K gmul(K a, K b) {
  int M, Ka, Kb, N;
  float *hA = read_mat(a, &M, &Ka);
  if (!hA) return gk_err("type");
  float *hB = read_mat(b, &Kb, &N);
  if (!hB) { free(hA); return gk_err("type"); }
  if (Ka != Kb) { free(hA); free(hB); return gk_err("length"); }
  if (ensure_handle() != CUBLAS_STATUS_SUCCESS) {
    free(hA); free(hB); return gk_err("cublas init");
  }

  float *dA = 0, *dB = 0, *dC = 0;
  size_t szA = (size_t)M * Ka * sizeof(float);
  size_t szB = (size_t)Kb * N * sizeof(float);
  size_t szC = (size_t)M * N * sizeof(float);
  cudaError_t e = cudaSuccess;
  if ((e = cudaMalloc(&dA, szA)) != cudaSuccess) goto cuda_fail;
  if ((e = cudaMalloc(&dB, szB)) != cudaSuccess) goto cuda_fail;
  if ((e = cudaMalloc(&dC, szC)) != cudaSuccess) goto cuda_fail;
  cudaMemcpy(dA, hA, szA, cudaMemcpyHostToDevice);
  cudaMemcpy(dB, hB, szB, cudaMemcpyHostToDevice);

  /* cuBLAS is column-major; our buffers are row-major. Computing C = A*B in
     row-major is the same as computing C^T = B^T * A^T in column-major, which
     we get by passing B and A swapped: a column-major read of row-major B is
     B^T, of row-major A is A^T, and a column-major C^T (ld N) is exactly our
     row-major C. So no explicit transpose is needed -- just swap the operands.
        op = N,N   m=N  n=M  k=K   lhs=dB(ld N)  rhs=dA(ld K)  out=dC(ld N) */
  {
    const float alpha = 1.0f, beta = 0.0f;
    cublasStatus_t s = cublasSgemm(g_handle, CUBLAS_OP_N, CUBLAS_OP_N,
                                   N, M, Ka, &alpha,
                                   dB, N, dA, Ka, &beta, dC, N);
    if (s != CUBLAS_STATUS_SUCCESS) {
      cudaFree(dA); cudaFree(dB); cudaFree(dC); free(hA); free(hB);
      return gk_err("cublas sgemm");
    }
    cudaDeviceSynchronize();
  }

  /* build the MxN result as a list of N-long real vectors. Pull the whole
     result back in ONE D2H transfer, then scatter rows into the gk-owned
     vectors with cheap host memcpys (vs. M separate device transfers). */
  {
    float *hC = (float *)malloc(szC);
    if (!hC) { e = cudaErrorMemoryAllocation; goto cuda_fail; }
    cudaMemcpy(hC, dC, szC, cudaMemcpyDeviceToHost);
    cudaFree(dA); cudaFree(dB); cudaFree(dC);
    free(hA); free(hB);

    K r = gk_mkkv(M);
    K *rr = gk_kv(r);
    for (int i = 0; i < M; ++i) {
      rr[i] = gk_mkev(N);
      memcpy(gk_ev(rr[i]), hC + (size_t)i * N, (size_t)N * sizeof(float));
    }
    free(hC);
    return r;
  }

cuda_fail:
  cudaFree(dA); cudaFree(dB); cudaFree(dC);
  free(hA); free(hB);
  return gk_err(cudaGetErrorString(e));
}
