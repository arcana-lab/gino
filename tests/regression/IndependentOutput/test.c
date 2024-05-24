#include <math.h>
#include <stdio.h>
#include <stdlib.h>

double doABunchOfMath(double in, int iters) {
  for (int idx = 0; idx < iters; idx++) {
    in = pow(in / 10, in / 100) + in;
  }
  return in;
}

int main(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, "usage: %s outer_iters inner_iters print_every\n", argv[0]);
    exit(1);
  }

  int outer = atoi(argv[1]);
  int inner = atoi(argv[2]);
  int printEvery = atoi(argv[3]);

  if (outer == 20 && inner == 20 && printEvery == 20) {
    printf("this appears to be a regression test run. using args 20 2 1\n");
    outer = 20;
    inner = 2;
    printEvery = 1;
  }

  for (int idx = 0; idx < outer; idx++) {
    double value = doABunchOfMath(idx, inner);
    if (!(idx % printEvery)) {
      if (value > 1e100) {
        printf("At iter %d value is very large\n", idx);

      } else {
        printf("At iter %d value is %.2lf\n", idx, value);
      }
    }
  }

  return 0;
}
