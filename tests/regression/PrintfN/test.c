#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// This test makes sure that the %n format specifier is considered.
// It fails if printf is assumed to not modify any memory pointed to by its
// arguments, incorrectly allowing the constant 27 to be propagated.
int main(int argc, char **argv) {

  int *p = (int *)malloc(sizeof(int));
  *p = 27;

  printf("%d%n\n", *p, p);

  int q = *p;
  free(p);
  printf("%s than 10 characters were printed on the previous line\n",
         q > 10 ? "more" : "fewer");
  return 0;
}
