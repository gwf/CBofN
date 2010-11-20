
/* NAME
 *   mandel - make a plot the Mandelbrot set
 * NOTES
 *   The autoscaling features of the plot library are not used because
 *   there is a slim chance of round-off problems resulting in a
 *   skipped line.
 * COLORS
 *   The four permutations of using or not using -rev and -inv will
 *   yield four different coloring schemes.  Try it and see.
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
 *   options make sense.  In particular, the bounding corners can be
 *   screwed up, and division by zero can occur with a malicious
 *   setting of IDIV.
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

int width = 640, height = 480, maxit = 160, invert = 0;
int levels = 256, rev = 0, box = 0, idiv = 1, mag = 1;
double ulx = -2.4, uly = 1.4, lly = -1.4, bulx, buly, blly, bail = 16.0;
char *term = NULL;

char help_string[] = "\
The Mandelbrot set is drawn according to the specified parameters.  The  \
image is computed by iterating the complex equation z(t) = (z(t-1))^2  \
+ c, where c is the complex point (x + yi) that corresponds to an (x, \
y) screen coordinate and the initial value of z(0) = 0.  If the system \
diverges at time k (i.e., |z(k)| > BAIL) then a point at (x, y) is \
plotted with the grayscale color (k / IDIV + (k % IDIV) * (LEVELS / \
IDIV)) % LEVELS), which reduces to (k % LEVELS) with an IDIV of 1. \
";

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels." },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels." },
  { "-maxit",  OPT_INT,     &maxit,
    "Maximum number of iterations before automatic bail-out." },
  { "-levels", OPT_INT,     &levels,
    "Number of plot (gray) levels to use." },
  { "-bail",   OPT_DOUBLE,  &bail,
    "Value of |z| to end iteration, i.e., the bailout value." },
  { "-ulx",    OPT_DOUBLE,  &ulx,    "Upper-left corner x-coordinate." },
  { "-uly",    OPT_DOUBLE,  &uly,    "Upper-left corner y-coordinate." },
  { "-lly",    OPT_DOUBLE,  &lly,
    "Lower-left corner y-coordinate." },
  { "-box",    OPT_INT,     &box,
    "Line width for a box.  If zero, no box is drawn." },
  { "-bulx",   OPT_DOUBLE,  &bulx,   "Box's upper-left x-coordinate." },
  { "-buly",   OPT_DOUBLE,  &buly,   "Box's upper-left y-coordinate." },
  { "-blly",   OPT_DOUBLE,  &blly,   "Box's lower-left y-coordinate." },
  { "-idiv",   OPT_INT,     &idiv,
    "Iteration divisor.   When greater than one, this creates a "
    "banding effect." },
  { "-rev",    OPT_SWITCH,  &rev,    "Reverse all colors but first?" },
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
  double inc, binc, a, b, u, v, w, x, y;

  get_options(argc, argv, options, help_string);

  plot_mag = mag;
  plot_inverse = invert;
  plot_init(width, height, levels, term);
  plot_set_all(0);

  /* Compute the size increment of a single pixel. */
  inc = (uly - lly) / (height - 1);

  /* For each vertical line... */
  for(j = 0, y = uly; j < height; j++, y -= inc) {

    /* For each  horizontal line... */
    for(i = 0, x = ulx; i < width; i++, x += inc) {
      a = x;
      b = y;

      /* Inner loop for the point (x, y). */
      for(k = 1; k <= maxit; k++) {

        /* The complex equation, which sets the updates to (a, b). */
        u = a * a;
        v = b * b;
        w = 2.0 * a * b;
        a = u - v + x;
        b = w + y;

        /* Check bailout condition, and plot as appropriate. */
        if(u + v > bail) {
          if(rev)
            plot_point(i, j, -((k / idiv + (k % idiv) * (levels / idiv)) %
                               levels) + levels - 1);
          else
            plot_point(i, j, (k / idiv + (k % idiv) *
                              (levels / idiv)) % levels);
          break;
        }
      }
    } 
  }

  /* Show a box, if appropriate. */
  plot_inverse = 0;
  if(box > 0) {
    binc = (buly - blly) / (height - 1);
    plot_box((bulx - ulx) / inc, (uly - buly) / inc,
             (bulx + width * binc - ulx) / inc,
             (uly + height * binc - buly) / inc, box);
  }

  plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
