
/* NAME
 *   vants - simulate a population of generalized virtual ants
 * NOTES
 *   None.
 * MISCELLANY
 *   It is definitely worthwhile to simulate multiple vants as
 *   the possible composite behavior of multiple vants is far 
 *   different than a single vant.  In fact, multiple vants can
 *   invert the work of other vants, which yields interesting
 *   deconstructive behavior.
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

int width = 200, height = 200, num = 1, seed = 0, invert = 0;
int states = 2, steps = 100000000, mag = 1;
double dense = 0.0;
char *term = NULL, *rule = "10";

char help_string[] = "\
Simulate and plot a population of virtual ants (vants).  The behavior \
of the vants is determined by a bit string with length equal to the \
number of states that each cell in the vants' grid world can take.  If \
a vant walks on a cell in state S, then the vant turns right if the \
S'th bit of the rule string is 1 and left if it's 0.  As it leaves the \
cell the vant changes the state of the old cell to \
(S + 1) % NUMSTATES.\
";

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels." },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels." },
  { "-num",    OPT_INT,     &num,    "Number of ants." },
  { "-rule",   OPT_STRING,  &rule,   "Rule string." },
  { "-dense",  OPT_DOUBLE,  &dense,  "Density of random crud." },
  { "-steps",  OPT_INT,     &steps,  "Number of simulated steps." },
  { "-seed",   OPT_INT,     &seed,   "Random seed for initial state." },
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
  int i, j, *vx, *vy, *vd, nx, ny, old;
  char **grid;

  get_options(argc, argv, options, help_string);
  states = strlen(rule);

  plot_mag = mag;
  plot_inverse = invert;
  plot_init(width, height, states, term);
  plot_set_all(0);
  srandom(seed);

  /* Allocate space for the vants (x, y) position and direction, and
   * initialize to sane values.
   */
  vx  = xmalloc(sizeof(int) * num);
  vy  = xmalloc(sizeof(int) * num);
  vd = xmalloc(sizeof(int) * num);
  for(i = 0; i < num; i++) {
    vx[i] = random_range(0, width);
    vy[i] = random_range(0, height);
    vd[i] = random_range(0, 4); 
  }
  
  /* Make our grid world and optionally fill with some crud. */
  grid = xmalloc(sizeof(char *) * width);
  for(i = 0; i < width; i++) {
    grid[i] = xmalloc(sizeof(char) * height);
    for(j = 0; j < height; j++) {
      if(dense > 0)
        grid[i][j] = (random_range(0.0, 1.0) < dense) ?
          random() % states : 0;
      else
        grid[i][j] = 0;
      plot_point(i, j, grid[i][j]);
    }
  }

  /* For each time step... */
  for(i = 0; i < steps; i++) {
    /* For each vant... */
    for(j = 0; j < num; j++) {
      /* Get the current position. */
      nx = vx[j]; ny = vy[j];

      /* Walk in the proper direction. */
      if(vd[j] == 0) ny++;
      else if(vd[j] == 1) nx++;
      else if(vd[j] == 2) ny--;
      else nx--;

      /* Wrap around coordinates if necessary. */
      nx = (nx < 0) ? width - 1 : (nx > width - 1) ? 0 : nx;
      ny = (ny < 0) ? height - 1 : (ny > height - 1) ? 0 : ny;

      /* Change the state of the grid to reflect the vants
       * new position.
       */
      old = grid[nx][ny];
      grid[nx][ny] = (grid[nx][ny] + 1) % states;
      plot_point(nx, ny, grid[nx][ny]); 

      /* Updates th vant's position and change it's direction
       * according to the rule string.
       */
      vx[j] = nx; vy[j] = ny;      
      vd[j] += (rule[old] - '0') ? 4 + 1 : 4 - 1;
      vd[j] %= 4;
    }
  }

  plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

