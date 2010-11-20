
/* NAME
 *   predprey - integrate the Lotka-Volterra system
 * NOTES
 *   None.
 * MISCELLANY
 *   The program uses a second-order Euler's method to perform the
 *   numerical integration, which is sufficient for simple tasks such
 *   as this.
 * BUGS
 *   No sanity checks are performed to make sure that any of the
 *   options make sense.
 * AUTHOR
 *   Copyright (c) 1997, Gary William Flake.
 *   
 *   Permission granted for any use according to the standard GNU
 *   ``copyleft'' agreement provided that the author's comments are
 *   neither modified nor removed.  No warranty is given or implied.
 */

/* Copyright (c) 1995 Gary W. Flake -- Permission granted for any use
   provided that this comment remains unaltered. No warranty is given */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "misc.h"

int points = 2500, seed = -1;
double f0 = 1.0, s0 = 0.2;
double dt = 0.1, a = 1.5, b = 1.5, c = 0.5, d = 1.5;
char *term = NULL;

OPTION options[] = {
  { "-seed",   OPT_INT,     &seed,   "Seed for random parameters." },
  { "-points", OPT_INT,     &points, "Number of points to produce." },
  { "-f0",     OPT_DOUBLE,  &f0,     "Initial fish population." },
  { "-s0",     OPT_DOUBLE,  &s0,     "Initial shark population." },
  { "-a",      OPT_DOUBLE,  &a,      "Fish growth rate." },
  { "-b",      OPT_DOUBLE,  &b,      "Shark consumption rate." },
  { "-c",      OPT_DOUBLE,  &c,      "Fish nutritional value." },
  { "-d",      OPT_DOUBLE,  &d,      "Shark death rate." },
  { "-dt",     OPT_DOUBLE,  &dt,     "Time step increment." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

char help_string[] = "\
Integrates the two-species Lotka-Volterra predator-prey system, \
\
dF/dt = F(a - bS), dS/dt = S(cF - d), \
\
according to the specified parameters.\
";

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* The two differential equations to integrate. */

void myfunc(double x, double y, double *xx, double *yy)
{
  *xx = x * (a - b * y);
  *yy = y * (c * x - d);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Perform one second-order Euler step. */

void euler(double dt, double *x, double *y)
{
  double x1, x2, y1, y2;
  
  myfunc(*x, *y, &x1, &y1);
  myfunc(dt * x1 + *x, dt * y1 + *y, &x2, &y2);
  *x += 0.5 * dt * (x1 + x2);
  *y += 0.5 * dt * (y1 + y2);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  double F, S;
  int i;

  get_options(argc, argv, options, help_string);
  
  if(seed >= 0) {
    srandom(seed);
    a = random_range(0, 5); b = random_range(0, 5);
    c = random_range(0, 5); d = random_range(0, 5);
    fprintf(stderr, "%f %f %f %f\n", a, b, c, d);
  }

  F = f0; S = s0;
  for(i = 0; i < points; i++) {
    euler(dt, &F, &S);
    printf("% f\t% f\n", F, S);
    F = F < 0 ? 0 : F;
    S = S < 0 ? 0 : S;    
  }

  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

