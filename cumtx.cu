#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
//#include <cuda.h>
#include <cuda_runtime.h>

extern "C" {
  void show_slice (float *S, uint nr, uint nc, const char *tag) ;
  void slice_to_mtx (void *M, uint r0, uint c0, uint nr, uint nc, float *S) ;
  void mtx_to_slice (void *M, uint r0, uint c0, uint nr, uint nc, float *S) ;
  void mtx_to_buf (void *M, void *buf, ulong *end, uint r0, uint nr) ;  
  void *open_coll (char *path, const char *access) ;
  void free_coll (void *P);
  uint num_rows (void *rows) ;
  uint num_cols (void *rows) ;
}

// see /usr/local/cuda-11.8/targets/x86_64-linux/include/driver_types.h
size_t gpu_ram (int dev) {
  cudaSetDevice(dev);
  struct cudaDeviceProp prm;
  cudaGetDeviceProperties(&prm, dev);  
  size_t bytes = prm.totalGlobalMem;
  double GB = (bytes>>20) / 1024.0;
  fprintf(stderr,"%.2fG VRAM on %s\n", GB, prm.name);
  return bytes;
}

typedef struct { unsigned i; float x; } ix_t;

__global__ void sparse_dot (float *AB, uint na, uint nb,
			    ix_t *A, ulong *aoff, 
			    ix_t *B, ulong *boff) {
  uint a = blockIdx.x * blockDim.x + threadIdx.x; // x is the row of A
  uint b = blockIdx.y * blockDim.y + threadIdx.y; // y is the row of B  
  if (a >= na || b >= nb) return; // off the grid
  ulong a0 = aoff[a], a1 = aoff[a+1]; // row a = A[a0:a1]
  ulong b0 = boff[b], b1 = boff[b+1]; // row b = B[b0:b1]
  double result = 0;
  while ((a0 < a1) && (b0 < b1)) {
    int eq = (A[a0].i == B[b0].i); // 1 iff a matches b
    int da = (A[a0].i <= B[b0].i); // 1 iff a needs to advance
    int db = (B[b0].i <= A[a0].i); // 1 iff b needs to advance
    result += eq * A[a0].x * B[b0].x;
    a0 += da;
    b0 += db;
  }
  AB[a*nb + b] = result;
}

void sparse_product (void *_C, void *_A, void *_B) {
  fprintf(stderr, "orly?\n");
  assert (num_cols(_A) == num_cols(_B)); 
  uint nA = num_rows(_A), nB = num_rows (_B), nC = num_cols (_A);
  size_t vram = gpu_ram (0);
  ix_t *A, *B;
  float *C;
  ulong *aoff, *boff;
  cudaMallocManaged(&C, 3607101440); // FIXME!
  cudaMallocManaged(&A, 3607101440);
  cudaMallocManaged(&B, 3607101440);
  cudaMallocManaged(&aoff, (nA+1)*sizeof(ulong));
  cudaMallocManaged(&boff, (nB+1)*sizeof(ulong));
  
  mtx_to_buf (_A, A, aoff, 0, nA);
  mtx_to_buf (_B, B, boff, 0, nB);
  
  dim3 block(32,32); // max threads per block is 2014
  dim3 grid(nA/32, nB/32); // [nA x nB] total threads, one per cell C[a,b]
  sparse_dot <<<grid,block>>> (C, nA, nB, A, aoff, B, boff);
  cudaDeviceSynchronize();
  
  //show_slice (AB, na, nb, "AB");
  //show_slice (BC, nb, nc, "BC");  
  //show_slice (AC, na, nc, "AC");
  
  slice_to_mtx (_C, 0, 0, nA, nB, C);
  
  cudaFree(A); cudaFree(B); cudaFree(C);
  cudaFree (aoff); cudaFree (boff);
}


// AC [a,c] = SUM_b AB[a,b] * BC[b,c] ... assume one thread per cell of AxC, dense
__global__ void dense_dot (float *AC, uint na, float *AB, uint nb, float *BC, uint nc) {
  uint a = blockIdx.x * blockDim.x + threadIdx.x, b; // x is the row
  uint c = blockIdx.y * blockDim.y + threadIdx.y;    // y is the col
  if (a >= na || c >= nc) return;
  double result = 0;
  for (b = 0; b < nb; ++b) 
    result += AB[a*nb + b] * BC[b*nc + c]; // += AB[a,b] * BC[b,c]
  AC[a*nc + c] = result; // 'a' rows of length 'nc', then 'c' elements
}

__global__ void set (float *AC, uint na, float *AB, uint nb, float *BC, uint nc) {
  uint a = blockIdx.x * blockDim.x + threadIdx.x; // x is the row
  uint c = blockIdx.y * blockDim.y + threadIdx.y; // y is the col
  if (a >= na || c >= nc) return;
  AC[a*nc + c] = AB[a*nb + c] * BC[a*nb + c]; 
}


// [AxC] += [AxB] x [BxC]
void dense_product (void *AxC, void *AxB, void *BxC) {
  assert (num_cols(AxB) == num_rows(BxC)); 
  uint nA = num_rows(AxB), nB = num_rows (BxC), nC = num_cols (BxC);
  size_t vram = gpu_ram (0), sz = sizeof(float);
  assert ((nA * nC + nA * nB + nB * nC) * sz < vram); // ensure vram
  float *AC, *AB, *BC;
  cudaMallocManaged(&AC, nA * nC * sz);
  cudaMallocManaged(&AB, nA * nB * sz);
  cudaMallocManaged(&BC, nB * nC * sz);
  
  uint na = nA, nb = nB, nc = nC;
  
  //                     r0 c0 nr  nc
  mtx_to_slice (AxB, 0, 0, na, nb, AB);
  mtx_to_slice (BxC, 0, 0, nb, nc, BC);
  
  dim3 block(32,32); // max threads per block is 2014
  dim3 grid(na/32, nc/32); // [na x nc] total threads, one per cell AC[a,c]  
  dense_dot <<<grid,block>>> (AC, na, AB, nb, BC, nc);
  cudaDeviceSynchronize();
  
  //show_slice (AB, na, nb, "AB");
  //show_slice (BC, nb, nc, "BC");  
  //show_slice (AC, na, nc, "AC");
  
  slice_to_mtx (AxC, 0, 0, na, nc, AC);
  
  cudaFree(AC); cudaFree(AB); cudaFree(BC);
}

// P = A x B.T
int gpu_product (char *_P, char *_A, char *_B, char *prm) {
  void *P = open_coll (_P, "w+");
  void *A = open_coll (_A, "r+");
  void *B = open_coll (_B, "r+");
  if (strstr(prm,"sparse")) sparse_product (P, A, B);  
  else                      dense_product (P, A, B);
  free_coll(P); free_coll(A); free_coll(B);
  return 0;
}
  
#define arg(i) ((i < argc) ? argv[i] : NULL)
#define a(i) ((i < argc) ? argv[i] : ((char*)""))

int main (int argc, char *argv[]) {
  // cumtx P = A x C [prm]
  if ((a(2)[0] == '=') && (a(4)[0] == 'x'))
    return gpu_product (arg(1), arg(3), arg(5), a(6));
  gpu_ram(0);
  return 0;
}

/*
#include <cuda_runtime.h>
#include "cublas_v2.h"
  cublasHandle_t H;
  cublasStatus_t ok, OK = CUBLAS_STATUS_SUCCESS;
  ok = cublasCreate(&H);
  if (ok != OK) { printf ("[%d] faild %s %d %d\n", ok, "cublasCreate", CUBLAS_STATUS_NOT_INITIALIZED, CUBLAS_STATUS_ALLOC_FAILED);  return 1; }
  ok = cublasGetVersion(H, &version);
  if (ok != OK) { printf ("[%d] faild %s\n", ok, "cublasVersion"); return 1; }
  ok = cublasDestroy(H);
  if (ok != OK) { printf ("[%d] faild %s\n", ok, "cublasDestroy"); return 1; }
*/
