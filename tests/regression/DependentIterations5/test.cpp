#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N 500

int main(int argc, char** argv) {
  int n = N;

  auto B = (double(*)[n][n])malloc(n*n*sizeof(double));

  for (int i = 0; i < n; i++){
    for (int j = 0; j < n; j++) {
      (*B)[i][j] = ((double) i*(j+3) + 3) / n;
      printf("Initialized B[%d][%d] with %f\n", i, j, (*B)[i][j]);
    }
  }

  /* Run kernel. */
  for (int i1 = 1; i1 < n; i1++){
    for (int i2 = 0; i2 < n; i2++) {
      (*B)[i1][i2] = (*B)[i1][i2] / (*B)[i1-1][i2];
    }
  }

  for (int i1 = 1; i1 < n; i1++){
    for (int i2 = 0; i2 < n; i2++) {
      printf("%f\n", (*B)[i1][i2]);
    }
  }

  free((void*)B);

  return 0;
}
