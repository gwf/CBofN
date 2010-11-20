
/* NAME
 *   mlp - train a multilayer perceptron with backprop
 * NOTES
 *   None
 * MISCELLANY
 *   The number of inputs and outputs must agree with the format of
 *   your training data file.  The program expects to find training
 *   patterns listed one after another with each training pattern
 *   consisting of the inputs followed by the target outputs.
 *   
 *   If the -pdump switch is used, then the patterns will printed to
 *   stdout.  Hence,redirect this to a file if you want to save it.
 *   
 *   You should always use linear outputs if your target values are
 *   continuous.
 *   
 *   The error value displayed via stderr is the root mean squared
 *   error taken over the entire data step.  Calculating this error
 *   measure is typically far more expensive than a single training
 *   step, so you may wish to use the -freq option to make it happen
 *   less frequently.
 *   
 *   If you network doesn't converge to anything useful, try
 *   increasing the number of hidden nodes.  Moreover, you may need to
 *   tweak the learning rate and momentum term.  This is just one of
 *   the curses of backprop.
 * BUGS
 *   The -gplot switch isn't very useful as it only works on the first
 *   output neuron.
 *   
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

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int numin = 2, numhid = 2, numout = 1, seed = 0, steps = 2000, linout = 0;
int freq = 1, pdump = 0, gdump = 0;
double lrate = 0.25, mrate = 0.5, winit = 0.1;
char *dfile = "data/xor.dat";

char help_string[] = "\
Train a multilayer perceptron with a single hidden layer of neurons \
on a set of data contained in a file using the backpropagation learning \
algorithm with momentum.  Output units can be linear or sigmoidal, \
allowing you to model both discrete and continuous output target values.\
";

OPTION options[] = {
  { "-dfile",  OPT_STRING, &dfile,  "Training data file." },
  { "-steps",  OPT_INT,    &steps,  "Number of simulated steps." },
  { "-seed",   OPT_INT,    &seed,   "Random seed for initial state." },
  { "-freq",   OPT_INT,    &freq,   "Status print frequency." },
  { "-numin",  OPT_INT,    &numin,  "Number of inputs." },
  { "-numhid", OPT_INT,    &numhid, "Number of hidden nodes." },
  { "-numout", OPT_INT,    &numout, "Number of outputs." },
  { "-lrate",  OPT_DOUBLE, &lrate,  "Learning rate." },
  { "-mrate",  OPT_DOUBLE, &mrate,  "Momentum rate." },
  { "-winit",  OPT_DOUBLE, &winit,  "Weight init factor" },
  { "-linout", OPT_SWITCH, &linout, "Use linear outputs?" },
  { "-pdump",  OPT_SWITCH, &pdump,  "Dump patterns at end of run?" },
  { "-gdump",  OPT_SWITCH, &gdump,  "Dump gnuplot commands at end?" },
  { NULL,      OPT_NULL,   NULL,     NULL                             }
};


/* Globals to avoid excessive parameter passing.  The network feed
   forward pass is defined as (in vector notation):

       y = sigmoid(Vz + b) = sigmoid(V sigmoid(Ux + a) + b)

   For weight variables u, v, a, and b, gVAR will contain the
   gradient of VAR and dVAR will contain the weight change for VAR.
   Two exceptional cases are for gy and gz.  For these variables,
   we first set them to dE/dy (or dE/dz) and then later overwrite
   them with dE/dy_in (or dE/dz_in) where y_in (or z_in) represents
   the appropriate net input into these neurons.

   The tx and ty arrays hold all of the training patterns. */

int numpats;
double *y, *z, *gy, *gz;
double **u, **v, **gu, **gv, **du, **dv;
double *a, *b, *ga, *gb, *da, *db;
double **tx, **ty;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* A sigmoidal function bounded by 0 and 1. */

double sigmoid(double x)
{
  return(1 / (1 + exp(-x)));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Read in the training data file, initialize memory for the NN and
   initialize weight values. */

void initialize(void)
{
  FILE *fp;
  SCANNER *scan;
  char *str;
  int i, j;

  /* Open and setup the file for reading. */
  if(dfile == NULL || (fp = fopen(dfile, "r")) == NULL) {
    fprintf(stderr, "Cannot open data file \"%s\".\n", dfile);
    exit(1);
  }
  /* Use '#' as comment character in data file.  */
  scan = scan_init(fp, "", " \t\n", "#");

  /* Get the number of patterns. */
  if((str = scan_get(scan)) == NULL) goto BADFILE;
  numpats = atoi(str);

  /* Get memory for the target input and outputs and
   * read in the data file.
   */
  tx = xmalloc(sizeof(double *) * numpats);
  ty = xmalloc(sizeof(double *) * numpats);
  for(i = 0; i < numpats; i++) {
    tx[i] = xmalloc(sizeof(double) * numin);
    ty[i] = xmalloc(sizeof(double) * numout);
    for(j = 0; j < numin; j++) {
      if((str = scan_get(scan)) == NULL) goto BADFILE;
      tx[i][j] = atof(str);
    }
    for(j = 0; j < numout; j++) {
      if((str = scan_get(scan)) == NULL) goto BADFILE;
      ty[i][j] = atof(str);
    }
  }
  fclose(fp);

  /* Allocate memory for the NN. */
  y = xmalloc(sizeof(double) * numout);
  z = xmalloc(sizeof(double) * numhid);
  gy = xmalloc(sizeof(double) * numout);
  gz = xmalloc(sizeof(double) * numhid);

  a = xmalloc(sizeof(double) * numhid);
  b = xmalloc(sizeof(double) * numout);
  ga = xmalloc(sizeof(double) * numhid);
  gb = xmalloc(sizeof(double) * numout);
  da = xmalloc(sizeof(double) * numhid);
  db = xmalloc(sizeof(double) * numout);

  u = xmalloc(sizeof(double *) * numhid);
  v = xmalloc(sizeof(double *) * numout);
  gu = xmalloc(sizeof(double *) * numhid);
  gv = xmalloc(sizeof(double *) * numout);
  du = xmalloc(sizeof(double *) * numhid);
  dv = xmalloc(sizeof(double *) * numout);

  /* Random initialization for the weights. */
  for(i = 0; i < numhid; i++) {
    a[i] = random_range(-1, 1) * winit;
    da[i] = 0;
  }
  for(i = 0; i < numout; i++) {
    b[i] = random_range(-1, 1) * winit;
    db[i] = 0;
  }
  for(i = 0; i < numhid; i++) {
    u[i] = xmalloc(sizeof(double) * numin);
    gu[i] = xmalloc(sizeof(double) * numin);
    du[i] = xmalloc(sizeof(double) * numin);
    for(j = 0; j < numin; j++) {
      u[i][j] = random_range(-1, 1) * winit;
      du[i][j] = 0;
    }
  }
  for(i = 0; i < numout; i++) {
    v[i] = xmalloc(sizeof(double) * numhid);
    gv[i] = xmalloc(sizeof(double) * numhid);
    dv[i] = xmalloc(sizeof(double) * numhid);
    for(j = 0; j < numhid; j++) {
      v[i][j] = random_range(-1, 1) * winit;
      dv[i][j] = 0;
    }
  }
  return;

BADFILE:
  fprintf(stderr, "Problem found in data file.\n");
  exit(1);  
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Pass the specified pattern through the NN and return the error. */

double feedforward(int pattern)
{
  int i, j;
  double error = 0;
  
  /* z = sigmoid(Ux + a) */
  for(i = 0; i < numhid; i++) {
    z[i] = a[i];
    for(j = 0; j < numin; j++)
      z[i] += tx[pattern][j] * u[i][j];
    z[i] = sigmoid(z[i]);
  }

  /* y = sigmoid(Vz + b) */
  for(i = 0; i < numout; i++) {
    y[i] = b[i];
    for(j = 0; j < numhid; j++)
      y[i] += z[j] * v[i][j];

    /* Only use sigmoids on the outputs if we don't want linear units. */
    if(!linout) y[i] = sigmoid(y[i]);

    error += (y[i] - ty[pattern][i]) * (y[i] - ty[pattern][i]);
  }
  return(error / numout);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Do a backprop pass on the specified pattern through the NN,
   calculate the gradients, and return the error. */

double feedback(int pattern)
{
  int i, j;
  double error = 0;

  for(i = 0; i < numout; i++) {
    /* dE/dy[i] = y[i] - target[i] */
    gy[i] = y[i] - ty[pattern][i];
    error += gy[i] * gy[i];

    /* dE/dy_in[i] = dE/dy[i] * y[i] * (1 - y[i])
     * (for sigmoidal units.)
     */
    if(!linout) gy[i] *= y[i] * (1 - y[i]);

    /* dE/db[i] = dE/dy_in[i] */
    gb[i] = gy[i];
  }
  for(i = 0; i < numhid; i++) {
    gz[i] = 0;
    for(j = 0; j < numout; j++) {

      /* dE/dv[i,j] = dE/dy[j] * z[i] */
      gv[j][i] = gy[j] * z[i];

      /* dE/dz[i] = sum_j ( dE/dy_in[j] * v[j][i] ) */
      gz[i] += gy[j] * v[j][i];
    }
    /* dE/dz_in[i] = dE/dz[i] * z[i] * (1 - z[i]) */
    gz[i] *= z[i] * (1 - z[i]);

    /* dE/da[i] = dz_in[i] */
    ga[i] = gz[i];

    for(j = 0; j < numin; j++)
      /* dE/du[i][j] = dz_in[i] * x[j] */
      gu[i][j] = gz[i] * tx[pattern][j];
  }
  return(error / numout);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Update the weights based on the current gradient and the previous
   weight change by using a momentum term. */

void update(void)
{
  int i, j;

  /* Delta_VAR(t + 1) = momentum * Delta_VAR(t) - rate * Grad_VAR(t + 1)
   * VAR(t + 1) = VAR(t) + Delta_VAR(t + 1) 
   */
  for(i = 0; i < numhid; i++) {
    da[i] = mrate * da[i] - lrate * ga[i];
    a[i] += da[i];
    for(j = 0; j < numin; j++) {
      du[i][j] = mrate * du[i][j] - lrate * gu[i][j];
      u[i][j] += du[i][j];
    }
  }
  for(i = 0; i < numout; i++) {
    db[i] = mrate * db[i] - lrate * gb[i];
    b[i] +=  db[i];
    for(j = 0; j < numhid; j++) {
      dv[i][j] =  mrate * dv[i][j] - lrate * gv[i][j];
      v[i][j] +=  dv[i][j];
    }
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* This silly little function simply prints a network in a form that
   gnuplot can understand.  You almost definitely don't need this.
   It only works for a single output. */   

void dump_gnuplot(void)
{
  char str1[256], str2[256];
  FILE *fp;
  int i, j;

  if((fp = fopen("mlp.gnp", "w")) == NULL) return;

  fprintf(fp, "g(x) = 1 / (1 + exp(-x))\n");
  fprintf(fp, "z(a,c");
  for(i = 0; i < numin; i++)
    fprintf(fp, ",wx%d", i+1);
  fprintf(fp, ") = c * g(a");
  for(i = 0; i < numin; i++)
    fprintf(fp, "+wx%d", i+1);
  fprintf(fp, ")\n");

  strcpy(str1, "x1");
  for(i = 1; i < numin; i++) {
    sprintf(str2, ",x%d", i+1);
    strcat(str1, str2);
  }

  for(i = 0; i < numhid; i++) {
    fprintf(fp, "z%d(%s) = z(%f,%f", i+1, str1, a[i], v[0][i]);
    for(j = 0; j < numin; j++)
      fprintf(fp, ",x%d*%f", j+1, u[i][j]);
    fprintf(fp, ")\n");
  }

  fprintf(fp, "y(%s) = %f", str1, b[0]);
  for(i = 0; i < numhid; i++)
    fprintf(fp, " + z%d(%s)", i+1, str1);
  fprintf(fp, "\n");
  fclose(fp);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Calculate the offline error. */

double total_error(void)
{
  int i;
  double error = 0;

  for(i = 0; i < numpats; i++)
    error += feedforward(i);
  return(sqrt(error / numpats));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void dump_patterns(void)
{
  int i, j;

  for(i = 0; i < numpats; i++) {
    feedforward(i);
#if 0
    for(j = 0; j < numin; j++)
      printf("% .3f\t", tx[i][j]);
    for(j = 0; j < numout; j++)
      printf("% .3f\t", ty[i][j]);
#endif
    for(j = 0; j < numout; j++)
      printf("% .3f ", y[j]);
    printf("\n");
  }    
}  

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  int i, j;

  get_options(argc, argv, options, help_string);
  srandom(seed);

  /* Initialize everything. */
  initialize();

  /* Display the first total error measure. */
  fprintf(stderr, "%d %f\n", 0, total_error());

  for(i = 1; i <= steps; i++) {
    if(i % freq == 0) fprintf(stderr, "%d %f\n", i, total_error());

    /* Get a random pattern, do a feedforward, feedback, and
     * update the weights.
     */
    j = random() % numpats;
    feedforward(j);
    feedback(j);
    update();
  }
  if(gdump) dump_gnuplot();
  if(pdump) dump_patterns();

  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

