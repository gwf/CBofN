
/* NAME
 *   life - play Conway's Game of Life
 * NOTES
 *   This implementation is relatively fast compared to a more
 *   straightforward implementation because the sum of live
 *   neighbors is computed once initially and then only updated
 *   when cells are changed.  Thus, the runtime will be
 *   bounded by the copying of the new sums to the current sum
 *   count plus plus any changes to the sum (which updates
 *   only the sums which change).
 * MISCELLANY
 *   You will need to use the -extra option in conjuction with
 *   the -wrap option if you wish to simulate moving objects that
 *   tend to fly off in one direction.  Periodic patterns may need
 *   the -extra option to give them a little bit of extra room to
 *   work in.
 * BUGS
 *   No sanity checks are performed to make sure that any of the
 *   options make sense.
 * CREDITS
 *   The data files for the initial states supplied with this
 *   program were lifted and converted from the excellent and free
 *   program xlife. 
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

char help_string[] =  "\
Simulate Conway's Game of Life with an arbitrary set of initial \
conditions.  Input files need to be in the PBM file format.\
";

int steps = 1000, invert = 1, mag = 10, width = 0, height = 0;
int extra = 5, wrap = 1;
char *term = NULL, *infile = "data/life/p8.pbm";

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels." },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels." },
  { "-extra",  OPT_INT,     &extra,  "Number of extra border pixels." },
  { "-wrap",   OPT_SWITCH,  &wrap,   "Wrap around world?" },
  { "-infile", OPT_STRING,  &infile, "Initial configuration file" },
  { "-steps",  OPT_INT,     &steps,  "Number of time steps to simulate." },
  { "-inv",    OPT_SWITCH,  &invert, "Invert all colors?" },
  { "-mag",    OPT_INT,     &mag,    "Magnification factor." },
  { "-term",   OPT_STRING,  &term,   "How to plot points." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* When a change is made to any of the cell states, this function is
   called so that the sum of live neighbors of the changed cell is
   updated for all eight neighbors. */

void updatecount(int **sum, int rw, int rh, int ii,
                 int jj, int wrap, int change)
{
  int i, j;

  if(wrap) {
    for(i = -1; i <= 1; i++)
      for(j = -1; j <= 1; j++)
        if(i != 0 || j != 0)
          sum[(i + ii + rh) % rh][(j + jj + rw) % rw] += change;
  }
  else {
    for(i = -1; i <= 1; i++)
      for(j = -1; j <= 1; j++)
        if((i != 0 || j != 0) &&
           (i + ii) >= 0 && (i + ii) < rh &&
           (j + jj) >= 0 && (j + jj) < rw)
          sum[i + ii][j + jj] += change;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{ 
#ifdef PLOTX11
  extern int x11_force_flush;
#endif
  extern int plot_mag;
  extern int plot_inverse;
  int w, h, rw, rh;
  int i, j, t, ii, jj, change;
  int **data, **state, **sum, **newsum, **swap;

  get_options(argc, argv, options, help_string);

  /* Read in the data file and pick the real width and height
   * to be the max of the requested size and the save of the
   * initial configurations.
   */
  data = read_pbm_file(infile, &w, &h);
  rw = MAX(w + extra * 2, width);
  rh = MAX(h + extra * 2, height);

  plot_mag = mag;
  plot_inverse = invert;
  plot_init(rw, rh, 2, term);
  plot_set_all(0);

  steps = (steps < 0) ? 10e10 : steps;

  /* Allocate memory for the states, the sum of live cells,
   * and the next sum of live cells.
   */
  state = xmalloc(sizeof(int *) * rh);
  sum = xmalloc(sizeof(int *) * rh);
  newsum = xmalloc(sizeof(int *) * rh);
  for(i = 0; i < rh; i++) {
    state[i] = xmalloc(sizeof(int) * rw);
    sum[i] = xmalloc(sizeof(int) * rw);
    newsum[i] = xmalloc(sizeof(int) * rw);
    for(j = 0; j < rw; j++)
      state[i][j] = sum[i][j] = 0;
  }

  /* Set the state to the initial configuration and set the
   * sum of live neighbors appropriately.
   */
#ifdef PLOTX11
  x11_force_flush = 1;
#endif
  for(i = 0; i < h; i++)
    for(j = 0; j < w; j++) {
      ii = (rh - h) / 2 + i;
      jj = (rw - w) / 2 + j;
      state[ii][jj] = data[i][j];
      if(data[i][j]) updatecount(sum, rw, rh, ii, jj, wrap, 1);
      plot_point(jj, ii, data[i][j]);
    }
#ifdef PLOTX11
  x11_force_flush = 0;
#endif

  /* For each time step... */
  for(t = 0; t < steps; t++) {
    change = 0;

    /* Copy the old sums to the new sum space. */
    for(i = 0; i < rh; i++)
      for(j = 0; j < rw; j++)
        newsum[i][j] = sum[i][j];

    /* For every cell in the grid... */
    for(i = 0; i < rh; i++)
      for(j = 0; j < rw; j++) {
        /* If a dead cell should come alive... */
        if(state[i][j] == 0 && sum[i][j] == 3) {
          updatecount(newsum, rw, rh, i, j, wrap, 1);
          state[i][j] = 1;
          plot_point(j, i, 1);
          change = 1;
        }
        /* If a live cell should die... */
        else if(state[i][j] == 1 && (sum[i][j] < 2 || sum[i][j] > 3)) {
          updatecount(newsum, rw, rh, i, j, wrap, -1);
          state[i][j] = 0;
          plot_point(j, i, 0);
          change = 1;
        }
      }
    /* Make the next sum count to be the new one. */
    swap = sum; sum = newsum; newsum = swap;

    /* If there has been no change of state, then the X11 plot
     * driver will need to be forcefully flushed so that the screen
     * reflects the current state.
     */
#ifdef PLOTX11
    if(!change && !x11_force_flush) {
      x11_force_flush = 1;
      plot_point(0, 0, state[0][0]);      
    }
#endif
  }

  plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
