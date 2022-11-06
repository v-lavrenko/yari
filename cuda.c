#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <cuda_runtime.h>
#include "cublas_v2.h"
#define n 32

int main (int argc, char *argv[]) {
  int version;
  cublasHandle_t H;
  cublasStatus_t ok, OK = CUBLAS_STATUS_SUCCESS;
  ok = cublasCreate(&H);
  if (ok != OK) { printf ("[%d] faild %s %d %d\n", ok, "cublasCreate", CUBLAS_STATUS_NOT_INITIALIZED, CUBLAS_STATUS_ALLOC_FAILED);  return 1; }
  ok = cublasGetVersion(H, &version);
  if (ok != OK) { printf ("[%d] faild %s\n", ok, "cublasVersion"); return 1; }
  ok = cublasDestroy(H);
  if (ok != OK) { printf ("[%d] faild %s\n", ok, "cublasDestroy"); return 1; }
  return 0;
}
