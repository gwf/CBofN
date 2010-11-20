
/* NAME
 *   hencon - control the Henon system with the OGY control law
 * NOTES
 *   None.
 * BUGS
 *   Few sanity checks are performed to make sure that any of the
 *   options make sense.
 * AUTHOR
 *   Copyright (c) 1997, Gary William Flake.
 *   
 *   Permission granted for any use according to the standard GNU
 *   ``copyleft'' agreement provided that the author's comments are
 *   neither modified nor removed.  No warranty is given or implied.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "misc.h"


int points = 300, on1 = 50, off = 100, on2 = 200, skip = 100, seed = 0;
double A = 1.29, B = 0.3, plimit = 0.2, gauss = 0.0;
char *term = NULL;

OPTION options[] = {
  { "-points", OPT_INT,     &points, "The length of the time series." },
  { "-on1",    OPT_INT,     &on1,    "Where to turn control on." },
  { "-off",    OPT_INT,     &off,    "Where to turn control off." },
  { "-on2",    OPT_INT,     &on2,    "Where to turn control on again." },
  { "-skip",   OPT_INT,     &skip,   "Amount to skip initially." },
  { "-seed",   OPT_INT,     &seed,   "Random seed." },
  { "-plimit", OPT_DOUBLE,  &plimit, "Largest allowed size for p." },
  { "-A",      OPT_DOUBLE,  &A,      "Value of the A parameter." },
  { "-B",      OPT_DOUBLE,  &B,      "Value of the B parameter." },
  { "-gauss",  OPT_DOUBLE,  &gauss,  "Magnitude of Gaussian noise." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

char help_string[] = "\
Control the Henon system, x(t+1) = A - x(t)^2 + B * x(t - 1), \
with the OGY control law for arbitrary choices of A and B.  The \
control law is analytically calculated based on the system parameters. \
The user can select times in which control is turned on and off so that \
time-to-control and transients can be observed.  Gaussian noise can \
also be injected into the system.  The control timing options are \
constrained to obey (0 <= on1 <= off <= on2 <= points).\
";

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  int i;
  double x, xf, t, p, y, lu, ls, eu[2], es[2], gu[2], k[2];

  get_options(argc, argv, options, help_string);

  if(0 > on1 || on1 > off || off > on2 || on2 > points) {
    fprintf(stderr, "Bad choice of on1, on2, off, and/or points.\n");
    exit(1);
  }

  /* Calculate the location of the embedded fixed-point. */
  xf = 0.5 * ((B - 1.0) + sqrt(SQR(B - 1.0) + 4.0 * A));

  /* Calculate the values of the eigenvalues that correspond to the
   * unstable and stable manifolds.
   */
  lu = -xf - sqrt(SQR(xf) + B);
  ls = -xf + sqrt(SQR(xf) + B);

  /* Calculate the unstable and stable eigenvectors. */
  eu[0] = lu / sqrt(SQR(lu) + 1.0);
  eu[1] = 1.0 / sqrt(SQR(lu) + 1.0);
  es[0] = ls / sqrt(SQR(ls) + 1.0);
  es[1] = 1.0 / sqrt(SQR(ls) + 1.0);

  /* Compute the contravariant vector that corresponds to the unstable
   * eigenvector.
   */
  gu[0] = 1.0 / (eu[0] - es[0] * eu[1] / es[1]);
  gu[1] = -gu[0] * es[0] / es[1];

  /* Compute the control law. */
  k[0] = gu[0] * -lu / gu[0];
  k[1] = gu[1] * -lu / gu[0]; 

  /* Initialize system. */
  srandom(seed);
  x = random_range(-0.1, 0.1);
  y = random_range(-0.1, 0.1);
  
  /* For each time step ... */
  for(i = 0; i < points + skip; i++) {

    /* If this is a time in which control should be on, compute
     * the control force, and clip it if necessary.
     */
    if((i >= skip + on1 && i < skip + off) || (i >= skip + on2)) {
      p = k[0] * (x - xf) + k[1] * (y - xf);
      p = (fabs(p) > plimit) ? 0.0 : p;
    }
    else
      p = 0.0;

    /* Compute the next time step of the system with the control
     * force and noise.
     */
    t = A - x*x + B * y + p + gauss * random_gauss();
    y = x + gauss * random_gauss();
    x = t;

    /* Output stuff. */
    if(i >= skip)
      printf("(t,x[t],y[t],p[t])=\t%d\t% f\t% f\t% f\n",
             i - skip + 1, x, y, p);
  }

  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

