#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  int iterations = 100;
  if (argc > 1) {
    iterations = atoi(argv[1]);
  }

  for (int idx = 0; idx < iterations; idx++) {
    printf("meow%d\n", idx);
    printf("at idx %42i\n", INT_MAX - idx);
    printf("at idx %.42i\n", INT_MAX - idx);
    printf("%10.1000f", 1.0 + idx);
    printf("%1000.f", 1.0 + idx);
    printf("12345678901234567890123456789\n");
    printf("%3.0f\n", -1.0);
    printf("%3.0f\n", 1111.000000);
    printf("%.7f\n", DBL_MAX);
    printf("%d\n", DBL_DIG);
    printf("%15d\n", -INT_MAX);
    printf("%.15d\n", -INT_MAX);
    printf("%d\n", (INT_MIN));
    fprintf(stdout, "%dmeow\n", -idx);
    fprintf(stdout, "%dmeow\n", -idx + 1);
    fprintf(stdout, "%dmeow\n", -idx + 2);
  }
  return 0;
}
