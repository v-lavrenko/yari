#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
//#include <cuda.h>
#include <cuda_runtime.h>

extern "C" {
void show_slice (float *S, uint nr, uint nc, const char *tag) ;
void slice_to_mtx (void *M, uint r0, uint c0, uint nr, uint nc, float *S) ;
void mtx_to_slice (void *M, uint r0, uint c0, uint nr, uint nc, float *S) ;
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

// AC [a,c] = SUM_b AB[a,b] * BC[b,c] ... assume one thread per cell of AxC
__global__ void product1 (float *AC, uint na, float *AB, uint nb, float *BC, uint nc) {
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
void _product (void *AxC, void *AxB, void *BxC) {
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
  product1 <<<grid,block>>> (AC, na, AB, nb, BC, nc);
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
  _product (P, A, B);
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
