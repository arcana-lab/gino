#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {

  int firstArgc = argc;
  if (argc < 2) {
    fprintf(stderr, "USAGE: %s LOOP_ITERATIONS\n", argv[0]);
    return -1;
  }
  auto iterations = atoll(argv[1]);
  if (iterations == 0)
    return 0;

  long long int *array =
      (long long int *)calloc(argc * 10, sizeof(long long int));

  // Loop id 1, not so nice. Exits to loop id 2 OR loop id 3
  while (true) {

  // One of go-to pair producing Loop id 3 NOT identified by LLVM
  LOOP3_PART1:

    if (argc < -1) // must be incomplete range
      goto LOOP3_PART2;
    if (argc >= 0) {
      array[1] = argc * 2;
      break;
    }
  }

// Other of go-to pair producing Loop id 3 NOT identified by LLVM
LOOP3_PART2:

  // Breaks to end of loop 3
  if (argc-- < 0)
    goto END;

  argc += 19 + argc; // NEEDED

  goto LOOP3_PART1;
END:

  printf("%lld\n", array[1]);
  printf("%lld\n", array[2]);
  printf("%lld\n", array[firstArgc * 3]);
  return 0;
}
