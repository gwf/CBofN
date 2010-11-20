
/* NAME
 *   phase1d - plot phase-space of a one-dimensional map
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
#include <string.h>
#include "misc.h"

int width = 640, height = 480, points = 10, skip = 0;
int mag = 1, invert = 0, arrows = 1;
double r = 1.0, x0 = 0.123456, aux = 1.0, dx = 0.0;
char *term = NULL, *func = "log";

char help_string[] = "\
A phase-space diagram is plotted for a one-dimensional map \
according to the specified options.  If the option for -dx is \
non-zero, then two trajectories are plotted: one staring at x0, \
and the other starting at (x0 + dx).  See the MAPS section of the \
manual page for details of what maps are supported.  User defined \
maps can be added to the file maps1d.c, but you must recompile \
the program. \
";

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels." },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels." },
  { "-points", OPT_INT,     &points, "Number of points to plot." },
  { "-skip",   OPT_INT,     &skip,   "Number of points to skip." },
  { "-r",      OPT_DOUBLE,  &r,      "Value for r." },
  { "-aux",    OPT_DOUBLE,  &aux,    "Auxiliary map parameter." },
  { "-x0",     OPT_DOUBLE,  &x0,     "Initial value for x." },
  { "-dx",     OPT_DOUBLE,  &dx,     "Difference for second trajectory." },
  { "-func",   OPT_STRING,  &func,
    "Map function to use (one of 'log', 'tent', 'sin', or 'gauss')." },
  { "-arrows", OPT_SWITCH,  &arrows, "Show arrows to indicate directions?" },
  { "-inv",    OPT_SWITCH,  &invert, "Invert all colors?" },
  { "-mag",    OPT_INT,     &mag,    "Magnification factor." },
  { "-term",   OPT_STRING,  &term,   "How to plot points." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* This function plots a little triangular arrow head that is located
   midway between (x1, y1) and (x2, y2). */

void plot_arrow(double x1, double y1, double x2, double y2)
{
  double xm, ym, off;

  /* 'off' is an offset for the points in the arrow head.  This
   * should probably be a parameter or a constant. */
  off = 0.01;

  /* Calculate the midpoint. */
  xm = (x1 + x2) / 2.0; ym = (y1 + y2) / 2.0;
  
  /* Moving north. */
  if(x1 == x2 && fabs(y1 - y2) > off && y2 > y1) {
    plot_line(xm, ym, xm - off, ym - off, 1);
    plot_line(xm, ym, xm + off, ym - off, 1);
    plot_line(xm - off, ym - off, xm + off, ym - off, 1);
  }
  
  /* Moving south. */
  else if(x1 == x2 && fabs(y1 - y2) > off && y1 > y2) {
    plot_line(xm, ym, xm - off, ym + off, 1);
    plot_line(xm, ym, xm + off, ym + off, 1);
    plot_line(xm - off, ym + off, xm + off, ym + off, 1);
  }

  /* Moving east. */
  else if(y1 == y2 && fabs(x1 - x2) > off && x2 > x1) {
    plot_line(xm, ym, xm - off, ym - off, 1);
    plot_line(xm, ym, xm - off, ym + off, 1);
    plot_line(xm - off, ym - off, xm - off, ym + off, 1);
  }

  /* Moving west. */
  else if(y1 == y2 && fabs(x1 - x2) > off && x1 > x2) {
    plot_line(xm, ym, xm + off, ym - off, 1);
    plot_line(xm, ym, xm + off, ym + off, 1);
    plot_line(xm + off, ym - off, xm + off, ym + off, 1);
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "maps1d.c"

int main(int argc, char **argv)
{
  extern int plot_mag;
  extern int plot_inverse;
  int i;
  double x, y, xt, yt, xinc, (*f)(double, double);

  get_options(argc, argv, options, help_string);

  plot_mag = mag;
  plot_inverse = invert;
  plot_init(width, height, 2, term);
  plot_set_range(0.0, 1.0, 0.0, 1.0);
  if(!term || strcmp(term, "ps") != 0)
    plot_set_all(0);

  f = get_named_function(func);

  /* First, draw the phase-space. */
  xinc = 1.0 / (width - 1);
  xt = 0.0; yt = f(xt, r);
  for(i = 0, x = 0.0; i < width; i++, x += xinc) {
    y = f(x, r);
    plot_line(xt, yt, x, y, 1);
    xt = x; yt = y;
  }
  
  /* Draw the identity line. */
  plot_line(0.0, 0.0, 1.0, 1.0, 1);

  /* Draw the trajectory. */
  for(i = 0, x = x0; i < points + skip; i++, x = y) {
    y = f(x, r);
    if(i >= skip) {
      plot_line(x, x, x, y, 1);
      plot_line(x, y, y, y, 1);
      if(arrows) {
        plot_arrow(x, x, x, y);
        plot_arrow(x, y, y, y);
      }
    } 
  }

  /* Draw the second trajectory, if appropriate. */
  if(dx > 0.0)
    for(i = 0, x = x0 + dx; i < points + skip; i++, x = y) {
      y = f(x, r);
      if(i >= skip) {
        plot_line(x, x, x, y, 1);
        plot_line(x, y, y, y, 1);
        if(arrows) {
          plot_arrow(x, x, x, y);
          plot_arrow(x, y, y, y);
        }
      } 
    }

  plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
