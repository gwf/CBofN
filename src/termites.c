
/* NAME
 *   termite - simulate a population of termites
 * NOTES
 *   None.
 * MISCELLANY
 *   The walking pattern of the termites is not actually a strict
 *   random walk.  Instead, at each time step they randomly turn
 *   -45, 0, or 45 degrees with equal probability.
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

int width = 100, height = 100, mag = 2, num = 20;
int seed = 0, invert = 0, steps = 10000000;
double dense = 0.3;
char *term = NULL;

char help_string[] = "\
Simulate a population of termites which do a random walk while possibly \
carrying a wood chip.  Under normal circumstances, the termites will \
self-organize and move the wood chips into piles without a global leader. \
The termites' behavior is dictated by the following set of rules: \
If a termite is not carrying anything and she bumps into \
a chip, then she picks it up, reverses direction, and continues with the \
random walk.  If she is carrying a chip and bumps into another, she drops \
her chip, turns around, and starts walking again.  Otherwise, she just \
does a random walk whether she is carrying a chip or not.\
";

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels." },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels." },
  { "-num",    OPT_INT,     &num,    "Number of termites in population." },
  { "-dense",  OPT_DOUBLE,  &dense,  "Density of chips at start." },
  { "-steps",  OPT_INT,     &steps,  "Number of simulated steps." },
  { "-seed",   OPT_INT,     &seed,   "Random seed for initial state." },
  { "-inv",    OPT_SWITCH,  &invert, "Invert all colors?" },
  { "-mag",    OPT_INT,     &mag,    "Magnification factor." },
  { "-term",   OPT_STRING,  &term,   "How to plot points." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* This handy array contains delta-x and delta-y values for moving
   N, NE, E, SE, S, SW, W, and NW.  By keeping an index to this
   array for each termite, we can easily calculate turns for any
   multiple of 45 degrees. */

int dir[8][2] = {
  {  0,  1 }, {  1,  1 }, {  1,  0 }, {  1, -1 },
  {  0, -1 }, { -1, -1 }, { -1,  0 }, { -1,  1 }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void newstep(int *a, int *b, int *d)
{
  /* Rotate 45 degrees left or right, or continue in the same direction. */
  *d = (*d + ((random() % 3) - 1) + 8) % 8;
  /* Move in the proper direction. */
  *a = (*a + dir[*d][0] + width) % width;
  *b = (*b + dir[*d][1] + height) % height;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  extern int plot_mag;
  extern int plot_inverse;
  int i, j, *tx, *ty, *td, nx, ny;
  char **chips;

  get_options(argc, argv, options, help_string);

  plot_mag = mag;
  plot_inverse = invert;
  plot_init(width, height, 2, term);
  plot_set_all(0);
  srandom(seed);

  /* Allocate space for the termites (x, y) position and the direction
   * that they are heading in, and initialize them to sane values.
   */
  tx  = xmalloc(sizeof(int) * num);
  ty  = xmalloc(sizeof(int) * num);
  td  = xmalloc(sizeof(int) * num);
  for(i = 0; i < num; i++) {
    tx[i] = (int) random_range(0, width);
    ty[i] = (int) random_range(0, height);
    td[i] = (int) random_range(0, 8);
  }
  
  /* Allocate the space needed to keep track of the wood chips. */
  chips = xmalloc(sizeof(char *) * width);
  for(i = 0; i < width; i++) {
    chips[i] = xmalloc(sizeof(char) * height);
    for(j = 0; j < height; j++) {
      /* Populate the grid with random chips. */
      chips[i][j] = (random_range(0.0, 1.0) < dense) ? 1 : 0;
      if(chips[i][j])
        plot_point(i, j, 1);
    }
  }

  /* For each time step... */
  for(i = 0; i < steps; i++) {
    /* For each termite... */
    for(j = 0; j < num; j++) {

      /* Save the current position and take a new step. */
      nx = tx[j]; ny = ty[j];
      newstep(&nx, &ny, &td[j]);

      /* Carrying a chip and miving to an empty spot: */
      if(chips[tx[j]][ty[j]] && !chips[nx][ny]) {
        /* Erase the old spot. */
        chips[tx[j]][ty[j]] = 0;
        plot_point(tx[j], ty[j], 0);
        /* Update the new position and draw the new spot. */
        tx[j] = nx; ty[j] = ny;
        chips[tx[j]][ty[j]] = 1;
        plot_point(tx[j], ty[j], 1);
      }
      /* Carrying a chip and moving to a chip: */
      else if(chips[tx[j]][ty[j]] && chips[nx][ny]) {
        /* Reverse direction and step, thereby dropping the
         * current chip (and perhaps getting a new one?).
         */
        td[j] = (td[j] + 4) % 8;
        tx[j] = (tx[j] + dir[td[j]][0] + width) % width;
        ty[j] = (ty[j] + dir[td[j]][1] + height) % height;
      }
      /* Not carrying anything, so just move. */
      else {
        tx[j] = nx; ty[j] = ny;
      }
    }
  }

  plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

