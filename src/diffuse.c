
/* NAME
 *   diffuse - simulate diffusion limited aggregation
 * NOTES
 *   None.
 * MISCELLANY
 *   Using invisible particle will make the simulation run much
 *   faster under interactive graphic terminals, especially X Windows.
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


int width = 300, height = 300, num = 20, seed = 0, invert = 0;
int invis = 0, steps = 1000000, levels = 256, mag = 1;
char *term = NULL;

char help_string[] = "\
A special type of stochastic fractal is created by the random action of \
many particles.   The fractal starts out as a single point seed that is \
fixed in position.  Particles float about via a random walk.  Whenever \
a floating particle moves adjacent to fixed particle the floating \
particles become frozen in place.  In this way, the fractal gradually \
grows in size. \
";

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels." },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels." },
  { "-levels", OPT_INT,     &levels,
    "Number of plot (gray) levels to use." },
  { "-num",    OPT_INT,     &num,    "Number of floating particles." },
  { "-steps",  OPT_INT,     &steps,  "Number of simulated steps." },
  { "-invis",  OPT_SWITCH,  &invis,  "Invisible particles?" },
  { "-seed",   OPT_INT,     &seed,   "Random seed for initial state." },
  { "-inv",    OPT_SWITCH,  &invert, "Invert colors?" },
  { "-mag",    OPT_INT,     &mag,    "Magnification factor." },
  { "-term",   OPT_STRING,  &term,   "How to plot points." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Check to see if the any points adjacent to position (x, y) is
   fixed on. */

int nearanother(int x, int y, char **grid)
{
  int nx, ny, i, j;
  
  for(i = -1; i <= 1; i++)
    for(j = -1; j <= 1; j++) {
      if(i == 0 && j == 0) continue;
      nx = x + i; ny = y + j;
      nx = (nx < 0) ? width - 1 : (nx > width - 1) ? 0 : nx;
      ny = (ny < 0) ? height - 1 : (ny > height - 1) ? 0 : ny;
      if(grid[nx][ny]) return(1);
    }
  return(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  extern int plot_mag;
#ifdef PLOTX11
  extern int x11_force_flush;
#endif
  extern int plot_inverse;
  int i, j, lim, *xb, *yb, nx, ny, minx, miny, maxx, maxy;
  int cinc, color, cincfreq, done = 0;
  double ang;
  char **grid;

  get_options(argc, argv, options, help_string);

#ifdef PLOTX11
  x11_force_flush = invis;
#endif
  plot_mag = mag;
  plot_inverse = invert;
  plot_init(width, height, levels, term);
  plot_set_all(0);
  srandom(seed);

  cinc = 0;
  color = 1;
  cincfreq = 10;

  /* Initialize the particles to have random positions that are near
   * the origin.
   */
  xb  = xmalloc(sizeof(int) * num);
  yb  = xmalloc(sizeof(int) * num);
  for(i = 0; i < num; i++) {
    xb[i] = random_range(-width / 10, width / 10) + width / 2;
    yb[i] = random_range(-height / 10, height / 10) + height / 2;
  }
  
  /* Set up a blank grid.  */
  grid = xmalloc(sizeof(char *) * width);
  for(i = 0; i < width; i++) {
    grid[i] = xmalloc(sizeof(char) * height);
    for(j = 0; j < height; j++) {
      grid[i][j] = 0;
    }
  }
  
  /* Fix the single seed point to be on. */
  minx = maxx = width / 2.0 + 0.5;
  miny = maxy = height / 2.0 + 0.5;
  grid[minx][miny] = 1;
  plot_point(minx, miny, 1);

  /* For each simulated step ... */
  lim = (steps > 0) ? steps : 1;
  for(i = 0; i < lim && !done; i++) {

    /* For each floating particle ... */
    for(j = 0; j < num; j++) {

      /* Take a random step... */
      nx = xb[j]; ny = yb[j];
      if(random_range(0.0, 1.0) < 0.5)
        nx += random_range(0.0, 1.0) < 0.5 ? 1 : -1;
      else
        ny += random_range(0.0, 1.0) < 0.5 ? 1 : -1;

      /* ... but keep it close to the box that contains the existing
       * fractal (to save time by eliminating too much "wandering").
       */
      nx = (nx < minx - 5) ? maxx + 5 : (nx > maxx + 5) ? minx - 5 : nx;
      ny = (ny < miny - 5) ? maxy + 5 : (ny > maxy + 5) ? miny - 5 : ny;
      nx = (nx < 0) ? width - 1 : (nx > width - 1) ? 0 : nx;
      ny = (ny < 0) ? height - 1 : (ny > height - 1) ? 0 : ny;
      
      /* Show the movement of the particle, if appropriate. */
      if(!invis) plot_point(xb[j], yb[j], 0);
      xb[j] = nx; yb[j] = ny;
      if(!invis) plot_point(xb[j], yb[j], color);

      /* Check to see if this particle should be frozen in place. */
      if(nearanother(xb[j], yb[j], grid)) {

        /* Lock this particle, and recompute the size of the containing
         * box.
         */
        grid[xb[j]][yb[j]] = 1;
        minx = (nx < minx) ? nx : minx;
        maxx = (nx > maxx) ? nx : maxx;
        miny = (ny < miny) ? ny : miny;
        maxy = (ny > maxy) ? ny : maxy;
        if(minx < 5 || miny < 5 || maxx > width - 1 + 5 ||
           maxy > height - 1 + 5)
          done = 1;

        /* Show the movement of the particle, if appropriate.  */
        if(invis) plot_point(xb[j], yb[j], color);

        /* Since this particle is now locked, create a new one at a 
         * random location within the box.
         */
        while(grid[xb[j]][yb[j]]) {
          ang = random_range(0, M_PI * 2);
          nx = MAX(width / 2 - minx, maxx - width / 2) + 5;
          ny = MAX(height / 2 - miny, maxy - height / 2) + 5;
          nx = cos(ang) * nx + width / 2;
          ny = sin(ang) * ny + height / 2;
          nx = (nx < 0) ? 0 : (nx > width - 1) ? width - 1 : nx;
          ny = (ny < 0) ? 0 : (ny > height - 1) ? height - 1 : ny;
          xb[j] = nx; yb[j] = ny;
        }

        /* Show the movement of the particle, if appropriate. */
        if(!invis) plot_point(xb[j], yb[j], color);

        /* Shift the color to plot. */
        cinc++;
        if (cinc > cincfreq) {
          color = color % 255 + 1;
          cinc = 0;
        }
      }
    }
    if(steps < 0) i--;
  }

  plot_finish();

  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

