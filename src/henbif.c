
/* NAME
 *   henbif - plot a bifurcation diagram for the Henon system
 * NOTES
 *   None.
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "misc.h"

int width = 640, height = 480, skip = 500, box = 0;
int invert = 0, mag = 1, ab = 1;
double abmin = 0.0, abmax = 1.4, factor = 2.0, aux = 1.0;
double ymin = -1.75, ymax = 1.75, A = 1.29, B = 0.3;
double brmin, brmax, bymin, bymax;
char *term = NULL;

char help_string[] = "\
A bifurcation diagram of the Henon system, which is described by \
the equation x(t+1) = A - x(t)^2 + B * x(t - 1), is plotted \
according to the specified parameters.  Either of the parameters \
(A or B) can be varied in the plot. \
";

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels." },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels." },
  { "-skip",   OPT_INT,     &skip,   "Number of initial points to skip." },
  { "-abmin",  OPT_DOUBLE,  &abmin,  "Smallest value for A (or B)." },
  { "-abmax",  OPT_DOUBLE,  &abmax,  "Largest value for A (or B)." },
  { "-ab",     OPT_SWITCH,  &ab,     "If TRUE, plot for A; B otherwise." },
  { "-A",      OPT_DOUBLE,  &A,      "Value of the A parameter." },
  { "-B",      OPT_DOUBLE,  &B,      "Value of the B parameter." },
  { "-factor", OPT_DOUBLE,  &factor, "Multiplicative factor for iterates." },
  { "-ymin",   OPT_DOUBLE,  &ymin,   "Smallest value for y range." },
  { "-ymax",   OPT_DOUBLE,  &ymax,   "Largest value for y range." },
  { "-box",    OPT_INT,     &box,    "Line width for a box." },
  { "-brmin",  OPT_DOUBLE,  &brmin,  "Smallest r-value for the box." },
  { "-brmax",  OPT_DOUBLE,  &brmax,  "Largest r-value for the box." },
  { "-bymin",  OPT_DOUBLE,  &bymin,  "Smallest value for box y range." },
  { "-bymax",  OPT_DOUBLE,  &bymax,  "Largest value for box y range." },
  { "-inv",    OPT_SWITCH,  &invert, "Invert all colors?" },
  { "-mag",    OPT_INT,     &mag,    "Magnification factor." },
  { "-term",   OPT_STRING,  &term,   "How to plot points." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  extern int plot_mag;
  extern int plot_inverse;
  int i, j;
  double x, t, y;
  double xt, xtt, xttt, xtttt, r, rinc, tol;

  get_options(argc, argv, options, help_string);

  /* Set ranges to requested (if reasonable) values. */
  abmin = (abmin < 0.0) ? 0.0 : (abmin > 2.0) ? 2.0 : abmin;
  abmax = (abmax < 0.0) ? 0.0 : (abmax > 2.0) ? 2.0 : abmax;
  
  plot_mag = mag;
  plot_inverse = invert;
  plot_init(width, height, 2, term);
  plot_set_range(abmin, abmax, ymin, ymax);
  plot_set_all(0);

  /* One pixel represents this much change in r. */
  rinc = (abmax - abmin) / (width - 1);

  /* Tolerance for detecting periodic behavior. */
  tol = 0.01 / height;

  /* For each value of r ... */
  for(i = 0, r = abmin; i < width; i++, r += rinc) {
    x = y = 0.0;

    /* Start iterating the system to skip some points. */
    for(j = 0; j < skip; j++) {
      /* Check if the system diverges. */
      if(fabs(x) > 10.0 || fabs(y) > 10.0) break;
      t = (ab ? r : A) - x*x + (ab ? B : r) * y; y = x; x = t;
    }
    /* Check if the system diverged. */
    if(fabs(x) > 10.0 || fabs(y) > 10.0) break;

    /* Set previous iterates to nonsense values so that the test
     * below initially fail.
     */
    xtttt = 5.0; xttt = 4.0; xtt = 3.0; xt = 2.0;

    /* We only need to do this next loop a number of times
     * proportional to the height of the plot since there are only
     * a small finite number of points that can be on.
     */
    for(j = 0; j < height * factor; j++) {
      xtttt = xttt; xttt = xtt; xtt = xt; xt = x;

      /* Check if the system diverged. */
      if(fabs(x) > 10.0 || fabs(y) > 10.0) break;

      /* Calculate a point a plot it. */
      t = (ab ? r : A) - x*x + (ab ? B : r) * y; y = x; x = t;
      plot_point(r, x, 1);

      /* Check for period 1, 2, 3, or 4 values, and if found within
       * a reasonable tolerance, bailout to speed up the computation
       * of the whole image.
       */
      if(fabs(x - xt) < tol || fabs(x - xtt) < tol ||
         fabs(x - xttt) < tol || fabs(x - xtttt) < tol) 
        break;
    } 
  }

  /* Plot a box, if appropriate. */
  plot_inverse = 0;
  if(box > 0)
    plot_box(brmin, bymax, brmax, bymin, box);

  plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

