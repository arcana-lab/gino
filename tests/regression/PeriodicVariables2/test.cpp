#include <stdio.h>
#include <stdlib.h>
#include <iostream>

/*
 * This test was written to capture case where the formula for advancing a
 * periodic variable across a chunkstep is not computed correctly.
 */

int main(int argc, char *argv[]) {
  auto ntimes = atoll(argv[1]);
  ntimes *= 1000;

  int flag1 = 0;
  bool flag2 = false;
  int flag3 = 10;
  int grow = 0;

  int i = 0;
  uint64_t acc = 0;
  while (i < ntimes) {
    flag1 ^= 1;
    flag2 = !flag2;
    grow = flag3 * i;
    acc += flag1 + flag2 + grow;
    flag3 = -flag3;
    i++;
  }
  std::cout << " acc: " << acc << std::endl;

  return 0;
}
