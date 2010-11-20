
/* NAME
 *   mg - plot the phase space of the Mackey-Glass system
 * MISCELLANY
 *   The plot region is determined by the points that are initially
 *   skipped.  If this number is too small (i.e., it is not very
 *   representative of the range of the plotted values), then you
 *   may need to increase the number specified by the -skip option.
 *   Alternatively, you can adjust the value given to -factor, which
 *   simply fractionally increases the border of the plot.
 *   
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "misc.h"

int width = 480, height = 480, skip = 2000, points = 10000;
int delta = 6, data = 0, invert = 0, mag = 1, tau = 17;
double a = 0.2, b = 0.1, dt = 0.1, factor = 0.2, xx0 = 1.23456789;
char *term = NULL;

char help_string[] = "\
The phase space of the Mackey-Glass system, which is described by the \
delay differential equation \
\
dx(t)/dt = A * x(t-Tau) / (1 + x(t-Tau)^10) - B * x(t), \
\
is plotted according to the specified parameters.  The x-coordinate of \
the plot is determined by x(t) while the y-coordinate is determined by \
x(t-delta).\
";

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels." },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels." },
  { "-skip",   OPT_INT,     &skip,   "Number of initial points to skip." },
  { "-points", OPT_INT,     &points, "Number of points to plot." },
  { "-delta",  OPT_INT,     &delta,  "Time steps to delay for." },
  { "-tau",    OPT_INT,     &tau,    "Value of the Tau parameter." },
  { "-A",      OPT_DOUBLE,  &a,      "Value of the A parameter." },
  { "-B",      OPT_DOUBLE,  &b,      "Value of the B parameter." },
  { "-dt",     OPT_DOUBLE,  &dt,     "Time step size." },
  { "-x0",     OPT_DOUBLE,  &xx0,    "Initial X value." },
  { "-factor", OPT_DOUBLE,  &factor, "Auto-scale expansion factor." },
  { "-data",   OPT_SWITCH,  &data,   "Don't plot, but print points." },
  { "-inv",    OPT_SWITCH,  &invert, "Invert all colors?" },
  { "-mag",    OPT_INT,     &mag,    "Magnification factor." },
  { "-term",   OPT_STRING,  &term,   "How to plot points." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Perform a second-order Euler step according to the MG system equation. */

double euler(double xt, double xtau, double a, double b, double dt)
{
  double x1, x2;

  x1 = (a * xtau) / (1 + pow(xtau, 10.0)) - b * xt;
  x2 = (a * xtau) / (1 + pow(xtau, 10.0)) - b * (dt * x1 + xt);
  return(xt + 0.5 * dt * (x1 + x2));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  extern int plot_mag;
  extern int plot_inverse;
  int i, hsz, h, ttau, tdelta;
  /* Variables used to compute auto-scaling. */
  double xmin, xmax, ymin, ymax;
  double x, y, xx = 0, yy = 0, xt, xtau, temp, *hold;

  get_options(argc, argv, options, help_string);

  if(!data) {
    plot_mag = mag;
    plot_inverse = invert;
    plot_init(width, height, 2, term);
    plot_set_all(0);
  }

  /* Recalculate Tau and delta to reflect time step size. */
  ttau = 1 / dt * tau + 0.5;
  tdelta = 1 / dt * delta + 0.5;

  /* Initialize the buffer space. */
  hsz = MAX(tdelta + 1, ttau + 1);
  hold = xmalloc(hsz * sizeof(double));

  /* Initialize the system. */
  h = 0;
  for(i = 0; i < hsz; i++)
    hold[i] = xx0;
  
  /* Set insane minimum and maximum guesses. */
  xmin = ymin = 10e10;
  xmax = ymax = -10e10;

  /* For all points (plus the skip and delay values. */
  for(i = 0; i < points + skip + tdelta + 1; i++) {

    /* Compute the time evolution of the system. */
    xtau = hold[(h + hsz - (ttau + 1)) % hsz];
    xt = hold[(h + hsz - 1) % hsz];
    x = euler(xt, xtau, a, b, dt);

    /* Save the state so that we can remember delayed values. */
    hold[h] = x; h++; h %= hsz;

    /* Get the delayed value. */
    y = hold[(h + hsz - tdelta) % hsz];

    if(data) {
      if(i >= skip + delta + 1)
        printf("%f\t%f\n", x, y);
    }
    else {
      /* If we are still skipping points, adjust the best guesses for
       * the minimum and maximums.
       */
      if(i < skip + delta + 1) {
        xmin = (x < xmin) ? x : xmin; xmax = (x > xmax) ? x : xmax;
        ymin = (y < ymin) ? y : ymin; ymax = (y > ymax) ? y : ymax;
      }

      /* If this is the last point to be skipped, reset the plotting
       * range based on the minimum and maximums.
       */
      if(i == skip + delta + 1) {
        temp = (xmax - xmin) * factor; xmin -= temp; xmax += temp;
        temp = (ymax - ymin) * factor; ymin -= temp; ymax += temp;
        plot_set_range(xmin, xmax, ymin, ymax);
      }

      /* Plot a line from the last point to the current point. */
      if(i >= skip + delta + 1)
        plot_line(xx, yy, x, y, 1);

      /* Save the last point. */
      xx = x; yy = y;
    }
  }

  if(!data) plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

