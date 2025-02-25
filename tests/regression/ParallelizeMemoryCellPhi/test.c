#include <stdio.h>
#include <stdlib.h>
/*
 * This test was written to capture a case where we have two phis, each with two incoming values, each one of the
 * incoming values of the other, each with their other incoming value a (different) constant, inside of a loop,
 * where one of the phis is in the header and has its constant incoming value coming from preheader, and
 * the other is somewhere in the body and has its constant incoming value coming from a loop-internal block.
 *
 * For this setup to be possible, more features (conditional control flow inside the loop, mainly) must be present.
 *
 * EXAMPLE:
 *
 * HEADER
 * phi1 = phi((phi2, latch), (1, preheader))
 * br body block 0
 *
 * BODY BLOCK 0
 * ...
 * br %x, body block 1, body block 2
 * BODY BLOCK 1
 * ...
 * br latch
 * BODY BLOCK 2
 * ...
 * br latch
 * LATCH
 * phi2 = ((phi1, body block 1), (0, body block 2))
 * br header
 *
 * This setup replicates a memory cell -- if BODY BLOCK 2 ever executes, the value of the joined phis is forevermore 0.
 *
 * This test was originally written to capture an issue where this exact case would be mistakenly identified as a periodic variable.
 *
 * This test is similar to LICM_side_effect, but LICM_side_effect does not trigger the issue this was written to capture, probably because
 * the loop sizes in LICM_side_effect are too small to trigger the parallelizer.
 */

double arr2[1999];

void myF(int num,
         int *ctr,
         char *class,
         double *xcr) {
  *ctr = 1;
  for (int i = 0; i < 1999; i++) {
    if (*class == 'U') {
      arr2[i] = 5;
    } else if (xcr[i] <= 0.00000001F) {
      *ctr = 0;
    } else {
      arr2[i] = 10;
    }

  }
}

int main(int argc, char *argv[]) {

  int *ctr = (int *)calloc(1, sizeof(int));

  double arr[2000];

  *ctr = 1;
  int num = atoll(argv[1]);
  for (int i = 0; i < 5; i++) {
    for(int j = 0; j < 400; j++) {
      arr[i+j*5] = atof(argv[i + 2]);
    }
  }
  char *class = (char *)calloc(1, sizeof(char));

  myF(num, ctr, class, arr);

  if(arr2[0] == arr2[1] == arr2[2] == arr2[3] == arr2[4]) {
    printf("wild.\n");
  }

  if (*ctr == 0) {
    printf("but not good?\n");
  } else if (*ctr == 1) {
    printf("still good\n");
  } else {
    printf("Also, quite strange\n");
  }

  free(ctr);
  return 0;
}
