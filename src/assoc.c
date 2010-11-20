
/* NAME
 *   assoc - retrieve associative memories
 * NOTES
 *   None.
 * MISCELLANY
 *   All pattern files must be in the PBM file format. You can request
 *   that multiple patterns be stored into the weights by using the
 *   -pfile option multiple time.
 *   
 *   The dimensions of the stored patterns and the test pattern must
 *   be identical.
 *   
 *   For weight pruning, the program first checks to see if a weight
 *   is "non-local" which means that for a weight that connects two
 *   neurons either the row indices or column indices differ by the
 *   amount greater than the value specified by the -local option.
 *   (If a value for local -local is zero, then all weights are used.)
 *   Next, the program prunes weights that are too small in size as
 *   specified by the -cut option.  If a weights has not been removed
 *   at this stage, then it will still be pruned with probability as
 *   specified by the -pprob option.
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

int width = 0, height = 0, mag = 1, invert = 0;
int seed = 0, steps = 1000, local = 0;
double pprob = 0.0, noise = 0.0, cutoff = 0;
char *tfile = "data/a.pbm", *term = NULL;

/* Prototype for a function that parses the -pfile option. */
int myparse(char **argv, int argc, OPTION *options, int *cargc, int opt);

char help_string[] = "\
Attempt to reconstruct a potentially corrupted image from a \
McCulloch-Pitts feedback neural network that acts as an associative \
memory.  The weights of the network are determined via Hebb's rule \
after reading in multiple patterns.  Weights can be pruned either by \
size, locality, or randomly. \
";

OPTION options[] = {
  { "-pfile",  OPT_OTHER,   myparse, "File with pattern to store." },
  { "-tfile",  OPT_STRING,  &tfile,  "File with test pattern." },
  { "-local",  OPT_INT,     &local,  "locality of permitted weights" },
  { "-cut",    OPT_DOUBLE,  &cutoff, "Cutoff size for weights." },
  { "-pprob",  OPT_DOUBLE,  &pprob,  "Probability of random pruning." },
  { "-noise",  OPT_DOUBLE,  &noise,  "Amount of noise for test case." },
  { "-seed",   OPT_INT,     &seed,   "Random seed for initial state." },
  { "-steps",  OPT_INT,     &steps,  "Number of time steps." },
  { "-inv",    OPT_SWITCH,  &invert, "Invert all colors?" },
  { "-mag",    OPT_INT,     &mag,    "Magnification factor." },
  { "-term",   OPT_STRING,  &term,   "How to plot points." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* Keep track of the number of times the -pfile option was used. */
int file_count = 0;

/* Space to hold the activations, weights, biases (thresholds), and
   the net input into the neurons.  This has global scope just for
   convenience. */

int **y;
double ****weight, **b, **x;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Make space for the weights, activations, net inputs, biases, etc... */

void init_weights(void)
{
  int i, j, k, l;

  x = xmalloc(sizeof(double *) * height);
  b = xmalloc(sizeof(double *) * height);
  for(i = 0; i < height; i++) {
    x[i] = xmalloc(sizeof(double) * width);
    b[i] = xmalloc(sizeof(double) * width);
    for(j = 0; j < width; j++)
      x[i][j] = b[i][j] = 0;
  }

  weight = xmalloc(sizeof(double ***) * height);
  for(i = 0; i < height; i++) {
    weight[i] = xmalloc(sizeof(double **) * width);
    for(j = 0; j < width; j++) {
      weight[i][j] = xmalloc(sizeof(double *) * height);
      for(k = 0; k < height; k++) {
        weight[i][j][k] = xmalloc(sizeof(double) * width);
        for(l = 0; l < width; l++)
          weight[i][j][k][l] = 0;
      }
    }
  } 
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* This function parses the -pfile option.  We do it here because the
   user is allowed to use the option multiple times in order to have
   multiple patterns stored into the weights. */

int myparse(char **argv, int argc, OPTION *options, int *cargc, int opt)
{
  int w, h, **data, i, j, k, l;

  /* If there is only one option on the line, then return an error
   * because there must be at least one more thing on the line for
   * the file name.
   */
  if(argc == *cargc) return(1);

  /* Read in the data file and get the dimensions. */
  data = read_pbm_file(argv[*cargc + 1], &w, &h);

  /* If this is the first file to be read in, set the proper dimensions. */
  if(file_count == 0) {
    width = w; height = h;
  }

  /* Do a sanity check on the dimensions to make sure all patterns
   * are the same size.
   */
  if(w != width || h != height) {
    fprintf(stderr, "Bad width (%d) or height (%d) in PPM file (%s).\n",
            w, h, argv[*cargc + 1]);
    exit(1);
  }

  /* Initialize the weights, if appropriate. */
  if(weight == NULL) init_weights();

  /* Add the correlations to the weights. */
  for(i = 0; i < height; i++)
    for(j = 0; j < width; j++)
      for(k = 0; k < height; k++)
        for(l = 0; l < width; l++)
          weight[i][j][k][l] += (2 * data[i][j] - 1) * (2 * data[k][l] - 1);

  /* Free up the space used to store the pattern file data. */
  for(i = 0; i < height; i++)
    free(data[i]);
  free(data);

  /* Eat up two arguments (the -pfile option and the file name).  Yum! */
  *cargc += 2;

  /* Increment the file count. */
  file_count++;

  /* Return to indicate no error. */
  return(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{ 
  extern int plot_mag;
  extern int plot_inverse;
  int i, j, k, l, w, h, t, n;
  double sum, big = 0, small = 10e10;

  get_options(argc, argv, options, help_string);

  /* Check that some patterns have in fact been stored. */
  if(file_count == 0) {
    fprintf(stderr, "No stored files.  Use -pfile option.\n");
    exit(1);
  }

  srandom(seed);

  plot_mag = mag;
  plot_inverse = invert;
  plot_init(width, height, 2, term);
  plot_set_all(0);

  /* Read in the test file and get it's dimensions. */
  y = read_pbm_file(tfile, &w, &h);
  if(w != width || h != height) {
    fprintf(stderr, "Bad width (%d) or height (%d) in PPM file (%s).\n",
            w, h, tfile);
    exit(1);
  }
  
  /* Optionally add some noise to the test pattern. */
  if(noise > 0 && noise < 1)
    for(i = 0; i < height; i++)
      for(j = 0; j < width; j++)
        if(random_range(0, 1) < noise)
          y[i][j] = random() % 2;

  /* For every weights. */
  for(i = 0; i < height; i++)
    for(j = 0; j < width; j++)
      for(k = 0; k < height; k++)
        for(l = 0; l < width; l++) {
          /* Normalize the weights and calculate the minimum
           * and maximum values. 
           */
          weight[i][j][k][l] /= (width * height);
          if(fabs(weight[i][j][k][l]) > big)
            big = fabs(weight[i][j][k][l]);
          if(fabs(weight[i][j][k][l]) < small)
            small = fabs(weight[i][j][k][l]);
        }
/*MRM begin*/
#if __dest_os != __mac_os
  fprintf(stderr, "|largest weight| = %f\n", big);
  fprintf(stderr, "|smallest weight| = %f\n", small);
#endif
/*MRM end*/

  /* N is the total number of weights before doing any pruning. */
  n = width * height * width * height;;

  /* For all weights. */
  for(i = 0; i < height; i++)
    for(j = 0; j < width; j++)
      for(k = 0; k < height; k++)
        for(l = 0; l < width; l++) {

          /* Optionally kill any that are non-local. */
          if(local && (fabs(i - k) > local || fabs(j - l) > local)) {
            weight[i][j][k][l] = 0.0;
            n--;
            continue;
          }

          /* Optionally kill any that are too small. */
          if(fabs(weight[i][j][k][l]) < cutoff) {
            weight[i][j][k][l] = 0.0;
            n--;
            continue;
          }
          
          /* Optionally kill a random weight. */
          if(pprob > 0 && (random_range(0, 1) < pprob)) {
            weight[i][j][k][l] = 0.0;
            n--;
            continue;
          }
        }
/*MRM begin*/
#if __dest_os != __mac_os
  fprintf(stderr, "total used weights = %d\n", n);
#endif
/*MRM end*/

  /* Now, really initialize the weights according to Hebb's rule so
   * that they store the patterns. 
   */
  for(i = 0; i < height; i++)
    for(j = 0; j < width; j++) {
      /* Calculate the sum of the weights coming into a particular neuron. */
      sum = 0.0;
      for(k = 0; k < height; k++)
        for(l = 0; l < width; l++)
          sum += weight[i][j][k][l];

      /* Set the bias equal according to this sum. */
      b[i][j] = -0.5 * sum;
      /* Put the activation to a -1 or 1 values. */
      y[i][j] = y[i][j] * 2 - 1;
      /* Plot the activation of the neuron. */
      plot_point(j, i, y[i][j]);
    }

  /* For each time step... */
  for(t = 0; t < steps; t++) {
    /* Pick a random neuron to update. */
    i = random() % width; j = random() % height;

    /* Set the net input to zero. */
    x[i][j] = 0;
    
    /* Add up the activations into this neuron weighted by
     * the weight strengths.
     */
    for(k = 0; k < height; k++)
      for(l = 0; l < width; l++)
        x[i][j] += y[k][l] * weight[i][j][k][l];

    /* Set the next state of the neuron to -1 or 1 based on
     * weather the net input exceeds the negation of the bias.
     */
    y[i][j] = x[i][j] - b[i][j] > 0 ? 1 : -1;

    /* Update the neuron's pixel. */
    plot_point(j, i, (y[i][j] + 1) / 2);
  }

  plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

