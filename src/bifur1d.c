
/* NAME
 *   bifur1d - plot bifurcations from a one-dimensional map
 * NOTES
 *   Includes the file "maps1d.c".
 * MAPS
 *   The following four one-dimensional maps are allowed:
 *   .IP Logistic\ Map:
 *   x(t+1) = 4 * r * x(t) * (1.0 - x(t))
 *   .IP Tent\ Map:
 *   x(t+1) = (x(t) <= 0.5) ? 2 * r * x(t) : 2r * (1.0 - x(t))
 *   .IP Sine\ Map:
 *   x(t+1) = sin(x(t) * PI * aux * 2 * r) / 2 + 0.5
 *   .IP Gaussian\ Map:
 *   x(t+1) = r * exp(-aux * (x(t) - 0.5) * (x(t) - 0.5))
 *   .LP
 *   See the file "maps1d.c" to see how to add user-defined maps.
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
int mag = 1, invert = 0;
double rmin = 0.0, rmax = 1.0, factor = 2.0, aux = 1.0;
double ymin = 0.0, ymax = 1.0, brmin, brmax, bymin, bymax;
char *term = NULL, *func = "log";

char help_string[] = "\
A bifurcation diagram is plotted for a one-dimensional map \
according to the specified options.  In general, the map is iterated \
for several different values of the 'r' parameter so that the long \
term behavior of the map can be observed as a function of \
'r'.  See the MAPS section of the manual page for details of what maps \
are supported.  User defined maps can be added to the file maps1d.c, \
but you must recompile the program. \
";

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels." },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels." },
  { "-skip",   OPT_INT,     &skip,   "Number of initial points to skip." },
  { "-rmin",   OPT_DOUBLE,  &rmin,   "Smallest value for r." },
  { "-rmax",   OPT_DOUBLE,  &rmax,   "Largest value for r." },
  { "-func",   OPT_STRING,  &func,
    "Map function to use (one of 'log', 'tent', 'sin', or 'gauss')." },
  { "-factor", OPT_DOUBLE,  &factor,
    "Multiplicative factor for number of iterates." },
  { "-ymin",   OPT_DOUBLE,  &ymin,   "Smallest value for y range." },
  { "-ymax",   OPT_DOUBLE,  &ymax,   "Largest value for y range." },
  { "-aux",    OPT_DOUBLE,  &aux,    "Auxiliary map parameter." },
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

#include "maps1d.c"

int main(int argc, char **argv)
{
  extern int plot_mag;
  extern int plot_inverse;
  int i, j;
  double x, xt, xtt, xttt, xtttt, r, rinc, tol;
  double (*f)(double, double);

  get_options(argc, argv, options, help_string);

  plot_mag = mag;
  plot_inverse = invert;
  plot_init(width, height, 2, term);
  plot_set_all(0);

  /* Set ranges to requested (if reasonable) values. */
  rmin = (rmin < 0.0) ? 0.0 : (rmin > 1.0) ? 1.0 : rmin;
  rmax = (rmax < 0.0) ? 0.0 : (rmax > 1.0) ? 1.0 : rmax;
  plot_set_range(rmin, rmax, ymin, ymax);

  f = get_named_function(func);

  /* One pixel represents this much change in r. */
  rinc = (rmax - rmin) / (width - 1);

  /* Tolerance for detecting periodic behavior. */
  tol = 0.01 / height;

  /* For each value of r ... */
  for(i = 0, r = rmin; i < width; i++, r += rinc) {
    /* The following is needed for float rounding errors. */
    if(r > 1.0) r = 1.0;

    /* Start iterating the system to skip some points. */
    x = 0.5;
    for(j = 0; j < skip; j++)
      x = f(x, r);

    /* Set previous iterates to nonsense values so that the test
     * below initially fail.
     */
    xtttt = 5.0; xttt = 4.0; xtt = 3.0; xt = 2.0;

    /* We only need to do this next loop a number of times
     * proportional to the height of the plot since there are only
     * a small finite number of points that can be on.
     */
    for(j = 0; j < height * factor; j++) {
      /* Save previous values. */
      xtttt = xttt; xttt = xtt; xtt = xt; xt = x; x = f(x, r); 
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
