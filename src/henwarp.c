
/* NAME
 *   henwarp - warps a square into the phase space of the Henon system
 * NOTES
 *   None.
 * MISCELLANY
 *   You may wish to try this with a small length for the size of
 *   square and watch how the resulting plot changes as you slowly
 *   increase the value passed to the -count option starting at
 *   zero.   The square will slowly spread out and converge to the
 *   attractor of the system.
 *   
 *   The method for choosing the viewable region may seem
 *   counter-intuitive at first, but it has some nice properties.  In
 *   particular, selecting the exact (x, y) coordinates for the
 *   upper-left corner and only selecting the lower right y coordinate
 *   forces both the x and y scales to be identical since all scales
 *   are uniquely determined by these values along with the plot
 *   width and height.  If you then change the width or height of the
 *   plot, the relative scales will still match up.  The options for
 *   making a box work similarly.
 * BUGS
 *   The length of the square is in pixels and works best if it is
 *   an odd value.  With even numbered values it can produce a gap
 *   in the plot for small values supplied with the -count option.
 *   
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

int width = 480, height = 480, len = 301, swap = 1, count = 1;
int invert = 0, mag = 1;
double A = 1.29, B = 0.3, ulx = -1.75, uly = 1.75, lly = -1.75;
char *term = NULL;

char help_string[] = "\
A square (initially centered about the origin) is transformed by the Henon \
system, which is described by the equation x(t+1) = A - x(t)^2 + B * \
x(t - 1), a fixed number of times according to the specified parameters. \
";

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels." },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels." },
  { "-swap",   OPT_SWITCH,  &swap,   "Swap the x and y axis." },
  { "-len",    OPT_INT,     &len,    "Length of edge of square." },
  { "-count",  OPT_INT,     &count,  "Number of transformations." },
  { "-A",      OPT_DOUBLE,  &A,      "Value of the A parameter." },
  { "-B",      OPT_DOUBLE,  &B,      "Value of the B parameter." },
  { "-ulx",    OPT_DOUBLE,  &ulx,    "Upper-left corner x-coordinate." },
  { "-uly",    OPT_DOUBLE,  &uly,    "Upper-left corner y-coordinate." },
  { "-lly",    OPT_DOUBLE,  &lly,    "Lower-left corner y-coordinate." },
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
  int i, j, k;
  double x, t, y, a, b, lrx, xinc, yinc;
  double **gridx, **gridy;

  get_options(argc, argv, options, help_string);

  /* Force the length to be odd. */
  len += (len / 2 * 2 == len);

  /* Calculate the scaling paramters. */
  lrx = ulx + ((uly - lly) / (height - 1)) * (width - 1);
  xinc = (lrx - ulx) / (width - 1);
  yinc = (uly - lly) / (height - 1);

  /* Allocate a grid of points and store the points in the square
   * centered about the origin.
   */
  gridx = xmalloc(sizeof(double *) * len);
  gridy = xmalloc(sizeof(double *) * len);
  for(i = 0; i < len; i++) {
    gridx[i] = xmalloc(sizeof(double) * len);
    gridy[i] = xmalloc(sizeof(double) * len);
    for(j = 0; j < len; j++) {
      gridx[i][j] = xinc * (-0.5 * len + j);
      gridy[i][j] = yinc * (-0.5 * len + i);
    }
  }
  
  plot_mag = mag;
  plot_inverse = invert;
  plot_init(width, height, 2, term);
  plot_set_range(ulx, lrx, lly, uly);
  plot_set_all(0);

  /* For each grid point ... */
  for(i = 0; i < len; i++) {
    for(j = 0; j < len; j++) {
      /* Tranform the point a fixed number of times and plot it. */
      x = gridx[i][j]; y = gridy[i][j];
      for(k = 0; k < count; k++) {
        t = A - x*x + B * y; y = x; x = t;
      }
      a = swap ? y : x; b = swap ? x : y;
      plot_point(a, b, 1);
    }
  }
  
  plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

