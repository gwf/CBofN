
/* NAME
 *   henon - plot the phase space of the Henon system
 * NOTES
 *   None.
 * MISCELLANY
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

int width = 480, height = 480, skip = 100, points = 1000, invert = 0;
int box = 0, swap = 1, data = 0, delay = 1, mag = 1;
double A = 1.29, B = 0.3;
double ulx = -1.75, uly = 1.75, lly = -1.75, bulx, buly, blly;
char *term = NULL;

char help_string[] = "\
The phase space of the Henon system, which is described by the equation \
x(t+1) = A - x(t)^2 + B * x(t - 1), is plotted according to the \
specified parameters. \
";

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels." },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels." },
  { "-skip",   OPT_INT,     &skip,   "Number of initial points to skip." },
  { "-swap",   OPT_SWITCH,  &swap,   "Swap the x and y axis?" },
  { "-points", OPT_INT,     &points, "Number of points to plot." },
  { "-delay",  OPT_INT,     &delay,  "Time steps to delay for." },
  { "-A",      OPT_DOUBLE,  &A,      "Value of the A parameter." },
  { "-B",      OPT_DOUBLE,  &B,      "Value of the B parameter." },
  { "-ulx",    OPT_DOUBLE,  &ulx,    "Upper-left corner x-coordinate." },
  { "-uly",    OPT_DOUBLE,  &uly,    "Upper-left corner y-coordinate." },
  { "-lly",    OPT_DOUBLE,  &lly,    "Lower-left corner y-coordinate." },
  { "-box",    OPT_INT,     &box,    "Line width for a box." },
  { "-bulx",   OPT_DOUBLE,  &bulx,   "Box's upper-left x-coordinate." },
  { "-buly",   OPT_DOUBLE,  &buly,   "Box's upper-left y-coordinate." },
  { "-blly",   OPT_DOUBLE,  &blly,   "Box's lower-left y-coordinate." },
  { "-data",   OPT_SWITCH,  &data,   "Don't plot, but print points." },
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
  int i, h;
  double x, t, y, a, b, lrx, blrx;
  double *hold;

  get_options(argc, argv, options, help_string);
  
  /* Calculate the area covered by a single pixel. */
  lrx = ulx + ((uly - lly) / (height - 1)) * (width - 1);

  if(!data) {
    plot_mag = mag;
    plot_inverse = invert;
    plot_init(width, height, 2, term);
    plot_set_range(ulx, lrx, lly, uly);
    plot_set_all(0);
  }

  /* Use random initial conditions. */
  x = random_range(-0.1, 0.1);
  y = random_range(-0.1, 0.1);

  /* Get space to hold delayed values. */
  hold = xmalloc(sizeof(double) * delay); h = 0;

  /* For each point to plot ... */
  for(i = 0; i < points + skip + delay; i++) {
    /* Hold old values. */
    hold[h] = x; h++; h %= delay;

    /* Calculate the next iterate. */
    t = A - x*x + B * y; y = x; x = t;

    /* Swap axes, if appropriate. */
    a = swap ? hold[h] : x; b = swap ? x : hold[h];

    /* Plot or display a point. */
    if(i >= skip + delay) {
      /* Only show points that are within the asked for region. */
      if(a > ulx && a < lrx && b > lly && b < uly) {
        if(data) printf("%f\t%f\n", a, b);
        else plot_point(a, b, 1);
      }
    }
  }

  /* Show the box, if appropriate. */
  plot_inverse = 0;
  if(!data && box > 0) {
    blrx = bulx + ((buly - blly) / (height - 1)) * (width - 1);
    plot_box(bulx, buly, blrx, blly, box);
  }

  if(!data) plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

