#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void print_array(int n, double A[n][n]) {
  int i, j;

  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++) {
      fprintf(stderr, "%0.2lf ", A[i][j]);
      if ((i * n + j) % 20 == 0)
        fprintf(stderr, "\n");
    }
  fprintf(stderr, "\n");
}

int main(int argc, char **argv) {
  int dump_code = atoi(argv[1]);
  int n = atoi(argv[2]);

  double(*A)[n][n];
  A = (double(*)[n][n])malloc((n) * (n) * sizeof(double));
  printf("allocation done\n");

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      (*A)[i][j] = ((double)i * (j + 2) + 2) / n;
    }
  }
  printf("init done\n");

  for (int i = 1; i <= n - 2; i++) {
    for (int j = 1; j <= n - 2; j++) {
      (*A)[i][j] = ((*A)[i - 1][j] + (*A)[i][j - 1]) / 2.0;
    }
  }
  printf("kernel done\n");

  if (dump_code == 1)
    print_array(n, *A);

  free((void *)A);
  ;

  return 0;
}
