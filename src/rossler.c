
/* NAME
 *   rossler - plot the phase space of the Rossler system
 * NOTES
 *   Because this program allows the plot to take 30 distinct forms
 *   as specified by unique combinations of the -xp and -yp options,
 *   it is necessary to perform some subtle pointer hackery to enable
 *   the program to run relatively fast.  The alternative would be to
 *   have a huge if/then or case statement in the middle of the inner
 *   most loop, which would have been ugly.
 *   
 *   The trick I used works as follows.  The point to be plotted can
 *   always be reached by accessing **ppx and **ppy which are initially
 *   set to point to the proper pointers in the delay array.  The delay
 *   array is adjusted at each loop iteration to reflect the rotating
 *   space in the buffer array.  This way, we essentially use a lookup
 *   table instead of the big if/then statement.
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

int width = 480, height = 480, skip = 2000, points = 5000;
int delta = 20, data = 0, invert = 0, mag = 1;
double a = 0.2, b = 0.2, c = 5.7, dt = 0.02;
double factor = 0.2, xx0 = 1, yy0 = 1, zz0 = 1;
char *xp = "x(t)", *yp = "y(t)";
char *term = NULL;

char help_string[] = "\
The phase space of the Rossler system, which is described by the \
three differential equations \
\
dx/dt = -y - z, \
\
dy/dt = A * y + x, and \
\
dz/dt = B + x * z - C * z, \
\
is plotted according to the specified parameters.  Valid arguments \
passed with the -xp and -yp options can be any one of x(t), y(t), z(t), \
x(t-delta), y(t-delta), or z(t-delta).  Thus, the displayed plot can \
take the form of a state space plot or a delayed coordinate plot.\
";

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels." },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels." },
  { "-skip",   OPT_INT,     &skip,   "Number of initial points to skip." },
  { "-points", OPT_INT,     &points, "Number of points to plot." },
  { "-A",      OPT_DOUBLE,  &a,      "Value of the A parameter." },
  { "-B",      OPT_DOUBLE,  &b,      "Value of the B parameter." },
  { "-C",      OPT_DOUBLE,  &c,      "Value of The C parameter." },
  { "-delta",  OPT_INT,     &delta,  "Time delay term." },
  { "-dt",     OPT_DOUBLE,  &dt,     "Time step." },
  { "-x0",     OPT_DOUBLE,  &xx0,    "Initial X value." },
  { "-y0",     OPT_DOUBLE,  &yy0,    "Initial Y value." },
  { "-z0",     OPT_DOUBLE,  &zz0,    "Initial Z value." },
  { "-data",   OPT_SWITCH,  &data,   "Don't plot, but print points." },
  { "-xp",     OPT_STRING,  &xp,     "X-coordinate for plot." },
  { "-yp",     OPT_STRING,  &yp,     "Y-coordinate for plot." },
  { "-factor", OPT_DOUBLE,  &factor, "Auto-scale expansion factor." },
  { "-inv",    OPT_SWITCH,  &invert, "Invert all colors?" },
  { "-mag",    OPT_INT,     &mag,    "Magnification factor." },
  { "-term",   OPT_STRING,  &term,   "How to plot points." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* This function sets the value of PTR to be the appropriate element
   of DELAYS, as determined by STR. */

void assign_pointer(double ***ptr, double *delays[3][2], char *str)
{
  if(!strcmp(str, "x(t)"))
    *ptr = &delays[0][0];
  else if(!strcmp(str, "y(t)"))
    *ptr = &delays[1][0];
  else if(!strcmp(str, "z(t)"))
    *ptr = &delays[2][0];
  else if(!strcmp(str, "x(t-delta)"))
    *ptr = &delays[0][1];
  else if(!strcmp(str, "y(t-delta)"))
    *ptr = &delays[1][1];
  else if(!strcmp(str, "z(t-delta)"))
    *ptr = &delays[2][1];
  else {
    fprintf(stderr, "Bad option passed to -xp or -yp: \"%s\"\n", str);
    exit(1);
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* The function to numerically integrate; in this case, the Rossler
   system.  If you want to modify this code to work with another
   three-dimensional system, then this is the only function that you'll
   need to modify. */

void myfunc(double x, double y, double z,
            double *xx, double *yy, double *zz)
{
  *xx = -y - z;
  *yy =  x + a * y;
  *zz =  b + x * z - c * z;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Perform one second-order Euler step. */

void euler(double dt, double *x, double *y, double *z)
{
  double x1, x2, y1, y2, z1, z2;
  
  myfunc(*x, *y, *z, &x1, &y1, &z1);
  myfunc(dt * x1 + *x, dt * y1 + *y, dt * z1 + *z, &x2, &y2, &z2);
  *x += 0.5 * dt * (x1 + x2);
  *y += 0.5 * dt * (y1 + y2);
  *z += 0.5 * dt * (z1 + z2);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  extern int plot_mag;
  extern int plot_inverse;
  int i, j, k;

  /* SSZ is the size of the buffers, while SI is the current index. */
  int ssz, si;

  /* Variables used to calculate the time evolution of the system. */
  double x, y, z;

  /* Buffers to store delay values of all three variables. */
  double *buffer[3];

  /* Pointers into the buffers that always refer to the current,
   * and once delayed values. 
   */
  double *delays[3][2];

  /* Pointers to the pointers that refer what is to be plotted. */
  double **ppx, **ppy;

  /* Variables used to compute auto-scaling. */
  double xmin, xmax, ymin, ymax;

  /* Values that are to be plotted. */
  double px, py, pxx = 0, pyy = 0, temp;

  get_options(argc, argv, options, help_string);

  /* Set up the pointers so that they refer to the proper values to
   * plot with respect to. */
  assign_pointer(&ppx, delays, xp);
  assign_pointer(&ppy, delays, yp);

  if(!data) {
    plot_mag = mag;
    plot_inverse = invert;
    plot_init(width, height, 2, term);
    plot_set_all(0);
  }
  
  /* Initialize the buffer space. */
  ssz = delta + 1;
  for(i = 0; i < 3; i++)
    buffer[i] = xmalloc(sizeof(double) * ssz);
  si = 0;

  /* Initialize the system. */
  x = xx0; y = yy0; z = zz0;

  /* Set insane minimum and maximum guesses. */
  xmin = ymin = 10e10;
  xmax = ymax = -10e10;

  /* For all points (plus the skip and delay values. */
  for(i = 0; i < points + skip + ssz; i++) {

    /* Compute the time evolution of the system. */
    euler(dt, &x, &y, &z);

    /* Save the state so that we can remember delayed values. */
    buffer[0][si] = x; buffer[1][si] = y; buffer[2][si] = z;

    /* For each state value ... , and for each delay value ... */
    for(j = 0; j < 3; j++)
      for(k = 0; k < 2; k++)
        /* Adjust the pointers to refer to the proper delays. */
        delays[j][k] = &buffer[j][(si + ssz - k * delta) % ssz];

    si = (si + 1) % ssz;

    if(data) {
      if(i >= skip + ssz)
        printf("%f\t%f\t%f\n", x, y, z);
    }
    else {
      /* Get the point that we want to plot. */
      px = **ppx; py = **ppy;

      /* If we are still skipping points, adjust the best guesses for
       * the minimum and maximums.
       */
      if(i <= skip + ssz) {
        xmin = (px < xmin) ? px : xmin; xmax = (px > xmax) ? px : xmax;
        ymin = (py < ymin) ? py : ymin; ymax = (py > ymax) ? py : ymax;
      }
      
      /* If this is the last point to be skipped, reset the plotting
       * range based on the minimum and maximums.
       */
      if(i == skip + ssz) {
        temp = (xmax - xmin) * factor; xmin -= temp; xmax += temp;
        temp = (ymax - ymin) * factor; ymin -= temp; ymax += temp;
        plot_set_range(xmin, xmax, ymin, ymax);
      }

      /* Plot a line from the last point to the current point. */
      if(i >= skip + ssz)
        plot_line(pxx, pyy, px, py, 1);

      /* Save the last point. */
      pxx = px; pyy = py;
    }
  }

  if(!data) plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

