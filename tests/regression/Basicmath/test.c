#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// from pi.h
#define PI (4 * atan(1))

// from CUBIC.C
void SolveCubic(double a,
                double b,
                double c,
                double d,
                int *solutions,
                double *x) {
  long double a1 = b / a, a2 = c / a, a3 = d / a;
  long double Q = (a1 * a1 - 3.0 * a2) / 9.0;
  long double R = (2.0 * a1 * a1 * a1 - 9.0 * a1 * a2 + 27.0 * a3) / 54.0;
  double R2_Q3 = R * R - Q * Q * Q;

  double theta;

  if (R2_Q3 <= 0) {
    *solutions = 3;
    theta = acos(R / sqrt(Q * Q * Q));
    x[0] = -2.0 * sqrt(Q) * cos(theta / 3.0) - a1 / 3.0;
    x[1] = -2.0 * sqrt(Q) * cos((theta + 2.0 * PI) / 3.0) - a1 / 3.0;
    x[2] = -2.0 * sqrt(Q) * cos((theta + 4.0 * PI) / 3.0) - a1 / 3.0;
  } else {
    *solutions = 1;
    x[0] = pow(sqrt(R2_Q3) + fabs(R), 1 / 3.0);
    x[0] += Q / x[0];
    x[0] *= (R < 0.0) ? 1 : -1;
    x[0] -= a1 / 3.0;
  }
}

void cubics(int scale) {
  int upperBound = scale;
  const int printEvery = 27;
  int printCounter = 0;

  for (double a2 = 1; a2 < upperBound; a2 += 1) {
    for (double b2 = 10; b2 > 0; b2 -= .25) {
      for (double c2 = 5; c2 < upperBound; c2 += 0.61) {
        for (double d2 = -1; d2 > -5; d2 -= .451) {
          int local_solutions;
          double local_x[3];
          SolveCubic(a2, b2, c2, d2, &local_solutions, local_x);
          printCounter = (printCounter + 1);
          if (!(printCounter % printEvery)) {
            printf("Solutions:");
            for (int i = 0; i < local_solutions; i++)
              printf(" %f", local_x[i]);
            printf("\n");
          }
        }
      }
    }
  }
}

int main(int argc, char **argv) {

  cubics(atoi(argv[1]));

  return 0;
}
