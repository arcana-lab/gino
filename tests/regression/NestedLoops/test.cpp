#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void computeSum2(long long int *a,
                 long long int SIZE_Z,
                 long long int SIZE_X,
                 long long int SIZE_Y) {
  return;
}

void computeSum(long long int *a,
                long long int SIZE_Z,
                long long int SIZE_X,
                long long int SIZE_Y) {
  int x, y, z;

  for (z = -2; z < SIZE_Z + 2; z++) {
    for (y = 0; y < SIZE_Y; y++) {
      for (x = 0; x < SIZE_X; x++) {
        if (x == 0 || x == SIZE_X - 1 || y == 0 || y == SIZE_Y - 1 || z == 0
            || z == SIZE_Z - 1) {
          (*a) += x + y + z;
        } else {
          if ((z == 1 || z == SIZE_Z - 2) && x > 1 && x < SIZE_X - 2 && y > 1
              && y < SIZE_Y - 2) {
            (*a) -= x + y - z;
          }
        }
      }
    }
  }
}

int main(int argc, char *argv[]) {
  void (*fp)(long long int *a,
             long long int SIZE_Z,
             long long int SIZE_X,
             long long int SIZE_Y);

  /*
   * Check the inputs.
   */
  if (argc < 3) {
    fprintf(stderr,
            "USAGE: %s LOOP_ITERATIONS NUMBER_OF_LOOP_INVOCATIONS\n",
            argv[0]);
    return -1;
  }
  auto iterations = atoll(argv[1]);
  auto invocations = atoll(argv[2]);
  if (iterations == 0) {
    return 0;
  }
  if (invocations == 0) {
    return 0;
  }
  fp = computeSum;
  if (iterations == 532455) {
    fp = computeSum2;
  }

  iterations *= 10;
  auto array = (long long int *)calloc(1, sizeof(long long int) * iterations);
  auto boundedArgc = argc < iterations ? argc : iterations - 5;
  boundedArgc = std::abs(boundedArgc);
  if (boundedArgc >= iterations) {
    fprintf(stderr, "ERROR: the input arguments are invalid\n");
    return -1;
  }
  array[boundedArgc] = argc;

  for (auto i = 0; i < invocations; i++) {
    (*fp)(array, iterations, 2, 2);
    auto s = *array;
    printf("%lld %lld %lld %lld\n",
           s,
           array[boundedArgc],
           array[iterations - 1],
           array[1]);
  }

  return 0;
}
