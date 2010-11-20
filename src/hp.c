
/* NAME
 *   hp - simulate the hodgepodge machine
 * NOTES
 *   None.
 * RULES
 *   If each cell can be in one of N states (labelled 0 to n - 1),
 *   then cells in state 0 are ``healthy,'' cells in state n - 1
 *   are ``ill,'' and all other cells are ``infected.''
 *   
 *   Within a cell's neighborhood, let Nill, Ninf, and S, denote
 *   the number of ill cells, the number of infected cells, and the
 *   sum of the states of all neighbors plus this cell's state.
 *   The next state is determined by the three rules:
 *   
 *      Healthy: floor( Ninf / k1 ) + floor( Nill / k2 )
 *   
 *      Infected: floor( S / ( Ninf + 1 ) ) + g
 *   
 *      Ill: magically becomes healthy, thus 0.
 *   
 *   Where k1, k2, and g are the parameters specified by the
 *   command line options.
 * MISCELLANY
 *   If you move from (to) an 8 cell neighborhood to (from) a 4 cell
 *   neighborhood try dividing (multiplying) k1 and k2 by 2 to
 *   produce a similar time evolution that occurred with the previous
 *   neighborhood size.
 *   
 *   For some reason, 4 cell neighborhoods seem to produce patterns
 *   with more spirals.
 *   
 *   This simulation can be frustratingly slow at times, so you
 *   may wish to use the -freq option to cut down on the overhead
 *   of plotting the states.
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

#define floor(x) ((int)(x))

int width = 50, height = 50, states = 100, wrap = 1, mag = 1;
int seed = 0, invert = 0, steps =  1000, freq = 1, diag = 1;
double k1 = 2, k2 = 3, g = 34;
char *term = NULL;

char help_string[] = "\
The time evolution of the hodgepodge machine is simulated and plotted \
according to the specified parameters.  The neighborhood of a cell can \
optionally include or not include diagonal cells in a 3x3 area;  Moreover, \
the neighborhood can also wrap around the edges so that the grid is \
topologically toroidal.  With a proper choice of parameters, this system \
resembles the Belousov-Zhabotinsky reaction which forms self-perpetuating \
spirals in a lattice.  See the RULES section of the manual pages \
or the source code for an explanation of how the cells change over time.\
";

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels." },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels." },
  { "-states", OPT_INT,     &states, "Number of cell states." },
  { "-steps",  OPT_INT,     &steps,  "Number of simulated steps." },
  { "-seed",   OPT_INT,     &seed,   "Random seed for initial state." },
  { "-diag",   OPT_SWITCH,  &diag,   "Diagonal cells are neighbors?" },
  { "-wrap",   OPT_SWITCH,  &wrap,   "Use a wrap-around space?" },
  { "-g",      OPT_DOUBLE,  &g,      "Infection progression rate." },
  { "-k1",     OPT_DOUBLE,  &k1,     "First weighting parameter." },
  { "-k2",     OPT_DOUBLE,  &k2,     "Second weighting parameter." },
  { "-freq",   OPT_INT,     &freq,   "Plot frequency." },
  { "-inv",    OPT_SWITCH,  &invert, "Invert all colors?" },
  { "-mag",    OPT_INT,     &mag,    "Magnification factor." },
  { "-term",   OPT_STRING,  &term,   "How to plot points." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void update_cell(int **oldstate, int **newstate, int x, int y)
{
  int nx, ny, i, j, numinf, numill, sum;

  numinf = numill = sum = 0;
  sum = oldstate[x][y];

  /* For every cell in the 3x3 neighborhood. */
  for(i = -1; i <= 1; i++)
    for(j = -1; j <= 1; j++) {

      /* Skip the current cell. */
      if(i == 0 && j == 0) continue;

      /* If we are only looking at 4 neighbors, then skip diagonals. */
      if(!diag && fabs(i) + fabs(j) == 2) continue;

      /* Get the proper indices of the neighbor. */
      nx = x + i; ny = y + j;
      if(!wrap) {
        if(nx < 0 || nx > width -1 || ny < 0 || ny > height - 1) continue;
      }
      else {
        nx = (nx < 0) ? width - 1 : (nx > width - 1) ? 0 : nx;
        ny = (ny < 0) ? height - 1 : (ny > height - 1) ? 0 : ny;
      }

      /* Add to the sum of the states. */
      sum += oldstate[nx][ny];

      /* Count the number of ill and infected neighbors. */
      if(oldstate[nx][ny] == states - 1)
        numill++;
      else if(oldstate[nx][ny] > 0)
        numinf++;
    }
  
  /* Healthy cell: */
  if(oldstate[x][y] == 0)
    newstate[x][y] = floor(numinf / k1) + floor(numill / k2);
  /* Infected cell: */
  else if(oldstate[x][y] < states - 1)
    newstate[x][y] = floor(sum / (numinf + 1)) + g;
  /* Ill cell: */
  else
    newstate[x][y] = 0;

  /* Bound next state to sane limit. */
  if(newstate[x][y] > states - 1)
    newstate[x][y] = states - 1;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  extern int plot_mag;
  extern int plot_inverse;
  int t, i, j;
  int **oldstate, **newstate, **swap;

  get_options(argc, argv, options, help_string);

  plot_mag = mag;
  plot_inverse = invert;
  plot_init(width, height, states, term);
  plot_set_all(0);
  srandom(seed);

  /* Allocate and initial memory for cell states. */
  oldstate = xmalloc(sizeof(int *) * width);
  newstate = xmalloc(sizeof(int *) * width);
  for(i = 0; i < width; i++) {
    oldstate[i] = xmalloc(sizeof(int) * height);
    newstate[i] = xmalloc(sizeof(int) * height);
    for(j = 0; j < height; j++)
      oldstate[i][j] = (int) random_range(0, states - 1);
  }
  
  /* For each time step... */
  for(t = 0; t < steps; t++) {
    /* For every cell ... */
    for(j = 0; j < height; j++)
      for(i = 0; i < width; i++) {
        /* Update the cell state. */
        update_cell(oldstate, newstate, i, j);
        /* Update display, if appropriate. */
        if(t % freq == 0)
          plot_point(i, j, oldstate[i][j]);
      }
    /* Make the next state equal to the new state. */
    swap = oldstate; oldstate = newstate; newstate = swap;
  }

  plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

