
/* NAME
 *   hopfield - solve a task assignment problem via a Hopfield network
 * NOTES
 *   Since the weight matrix is exceptionally sparse for this
 *   network, we don't actually store it but, instead, compute it
 *   on the fly by checking if two neurons are in the same row
 *   or column.  Moreover, all information about the neurons is
 *   actually stored in one-dimensional structures, since using
 *   two-dimensional structures doesn't really simplify anything
 *   in this case.
 * MISCELLANY
 *   As mentioned before, the weights and external inputs are set
 *   according to the K-out-of-N rule which states that if we have N
 *   neurons in a mutually connected sub-network and that we wish this
 *   subset to converge with exactly K neurons activated, then each
 *   neuron should be connected to every other neuron with a weights
 *   of -2 and receive an external input of (2K - 1).  Since we must
 *   produce solutions that look like permutation matrices, each
 *   neuron is in 2 K-out-of-N subsets, one for the column and one for
 *   the row.  Thus, all neurons inhibit all other neurons in the same
 *   column or row with -2 and must (on average) receive a net input
 *   of 2 since they are all in two sets.  The external inputs are
 *   slightly adjusted to favor neurons that represent more productive
 *   task performers.  See the source code for more details.
 * BUGS
 *   No sanity checks are performed to make sure that any of the
 *   options make sense.
 *   
 *   The final cost of the solution is printed at the end of the
 *   simulation; however, no check is done to insure that the system
 *   has actually converged.  Hence, it may print out nonsense
 *   results.
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
Solve a task assignment problem via a Hopfield neural network while \
plotting the activations of the neurons over time.  The program uses \
the K-out-of-N rule for setting the external inputs and synapse \
strength of the neurons. \
";

double dt = 0.1, tau = 10.0, scale = 0.5, gain = 0.5;
int seed = 0, steps = 1000, invert = 0, mag = 10, gray = 256;
char *term = NULL, *specs = "data/hop1.dat";

OPTION options[] = {
  { "-specs",  OPT_STRING,  &specs,  "Problem specification file." },
  { "-dt",     OPT_DOUBLE,  &dt,     "Time step increment." },
  { "-tau",    OPT_DOUBLE,  &tau,    "Decay term." },
  { "-gain",   OPT_DOUBLE,  &gain,   "Sigmoidal gain." },
  { "-scale",  OPT_DOUBLE,  &scale,  "Scaling for inputs." },
  { "-seed",   OPT_INT,     &seed,   "Random seed for initial state." },
  { "-steps",  OPT_INT,     &steps,  "Number of time steps." },
  { "-gray",   OPT_INT,     &gray,   "Number of gray levels." },
  { "-inv",    OPT_SWITCH,  &invert, "Invert all colors?" },
  { "-mag",    OPT_INT,     &mag,    "Magnification factor." },
  { "-term",   OPT_STRING,  &term,   "How to plot points." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

double *U, *UU, *V, *I, *cost;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Compute a sigmid function bounded by 0 and 1. */

double sigmoid(double x)
{
  return(1.0 / (1.0 + exp(-x * gain)));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Read in the specifications file which contains a single integer, n,
   that specifies the width and height and n*n numbers representing
   the costs for specific performers to do specific tasks. */

void read_specs(char *fname, int *width, int *height)
{
  double min = 10e10, max = -10e10, ave = 0.0;
  FILE *fp;
  SCANNER *scan;
  char *str;
  int i;

  if(fname == NULL || (fp = fopen(fname, "r")) == NULL) {
    fprintf(stderr, "Cannot open specification file \"%s\".\n", fname);
    exit(1);
  }
  /* Let '#' be a comment character. */
  scan = scan_init(fp, "", " \t\n", "#");

  /* Get the width (height) of the grid to work in. */
  if((str = scan_get(scan)) == NULL) goto BADFILE;
  *height = *width = atoi(str);

  /* Allocate space for the state, next state, activation,
   * external inputs, and and costs.
   */
  U = xmalloc(sizeof(double) * *width * *height);
  UU = xmalloc(sizeof(double) * *width * *height);
  V = xmalloc(sizeof(double) * *width * *height);
  I = xmalloc(sizeof(double) * *width * *height);
  cost = xmalloc(sizeof(double) * *width * *height);
  for(i = 0; i < *width * *height; i++) {
    if((str = scan_get(scan)) == NULL) goto BADFILE;
    /* Read in the costs. */
    cost[i] = I[i] = atof(str);
    /* Set the state to something random. */
    U[i] = random_range(-1, 1);
    /* Pass the state through a sigmoid to get the state. */
    V[i] = sigmoid(U[i]);

    /* Keep track of the minimum, maximum, and average cost
     * so that we rescale things.
     */
    if(I[i] < min) min = I[i];
    if(I[i] > max) max = I[i];
    ave += I[i];
  }
  ave /= (*width * *height);

  /* Rescale so that the mean is 2 (per the k-out-of-n rule). */
  for(i = 0; i < *width * *height; i++)
    I[i] = scale * (I[i] - ave) / (max - min) + 2;
    
  return;
BADFILE:
  fprintf(stderr, "Problem found in specification file.\n");
  exit(1);  
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{  
  extern int plot_mag;
  extern int plot_inverse;
  int width, height, i, j, k, l, m, o, t, n;
  double *swap, sum;

  get_options(argc, argv, options, help_string);

  srandom(seed);
  read_specs(specs, &width, &height);

  plot_mag = mag;
  plot_inverse = invert;
  plot_init(width, height, gray, term);
  plot_set_all(0);

  n = width * height;

  /* For every time step... */
  for(t = 0; t < steps; t++) {
    /* Use k as an index into U and V. */
    k = 0;
    /* For every neuron... */
    for(i = 0; i < height; i++)
      for(j = 0; j < width; j++) {
        /* Compute the activation an plot the point. */
        V[k] = sigmoid(U[k]);
        plot_point(j, i, (int) (V[k] * gray));
        k++;
      }
    /* Use k as an index into U and V. */
    k = 0;
    /* For every neuron... */
    for(i = 0; i < height; i++)
      for(j = 0; j < width; j++) {
        o = 0;
        sum = 0.0;
        /* For every other neuron... */
        for(l = 0; l < height; l++)
          for(m = 0; m < width; m++) {

            /* If neuron(l, m) is in the same row or column as
             * neuron(i, j), then it must inhibit the former
             * neuron with a weight of -2 (per the k-out-of-n rule).
             */
            if((i == l && j != m) || (i != l && j == m))
              sum += -2.0 * V[o];
            o++;
          }
        /* Update the next state. */
        UU[k] = U[k] + dt * (sum + I[k] - U[k] / tau);
        k++;    
      }
    /* Let the next state be the current state. */
    swap = U; U = UU; UU = swap;
  }

  /* We are done, so calculate the final cost.  Note that this
   * fails if the system isn't close to converging.
   */
  sum = 0.0;
  for(i = 0; i < n; i++)
    if(V[i] > 0.5) sum += cost[i];
  fprintf(stderr, "Final cost = %f\n", sum);

  plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
