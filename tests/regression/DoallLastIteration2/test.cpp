#include <stdio.h>
#include <stdlib.h>
#include <iostream>

int main(int argc, char *argv[]) {
  int *q = (int *)malloc(101 * sizeof(int));

  int *old_q = q;
  for (uint32_t i = 100; i >= argc; i--) {
    q++;
    *q = 42 + i;
  }
  *q = 43;
  std::cout << "*q:  " << *q << std::endl;
  std::cout << "q address diff: " << (q - old_q) * sizeof(int) << std::endl;

  return 0;
}
