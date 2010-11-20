
/* NAME
 *   gasurf - find a surfaces peak with a genetic algorithm
 * NOTES
 *   None.
 * MISCELLANY
 *   The bit strings are converted to floating point numbers
 *   with the formula (8 * int(string) / 2^len - 4) where
 *   int(string) is the integer value of a binary string.  Thus,
 *   all numbers are forced to be between -4 and 4.
 *   
 *   A more sophisticated GA encoding would use Gray codes to
 *   represent the floating point numbers which arguably are
 *   better behaved under mutation.
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

int size = 10, gens = 50, seed = 0, len = 16;
double crate = 0.75, mrate = 0.01;

char help_string[] = "\
Use a genetic algorithm to find the maximum of a multi-humped function. \
This program serves as an example of how GAs can be used to optimize \
function which take a multiple floating point arguments. Reproduction \
of strings entails crossover and mutation with strings being selected \
based on fitness.\
";

OPTION options[] = {
  { "-size",   OPT_INT,     &size,   "Population size." },
  { "-len",    OPT_INT,     &len,    "DNA length." },
  { "-gens",   OPT_INT,     &gens,   "Number of generations." },
  { "-seed",   OPT_INT,     &seed,   "Random seed." },
  { "-crate",  OPT_DOUBLE,  &crate,  "Crossover rate." },
  { "-mrate",  OPT_DOUBLE,  &mrate,  "Mutation rate." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* 2-D radial basis function centered at (a, b). */

inline double bump(double x, double y, double a, double b)
{
  return(exp(-(x - a) * (x - a) - (y - b) * (y - b)));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* The function that we are trying to maximize with respect to x and y. */

double surface(double x, double y)
{
  /* The magic constant below make the approximate maximal
   * value equal to 1. 
   */
  return((bump(x, y, 2, 2) + bump(x, y, -2, 2) + bump(x, y, -2, -2) + 
          bump(x, y, 2, -2) + 1.5 * bump(x, y, 0, 0) +
          0.5 * (bump(x, y, 0, 3) + bump(x, y, 3, 0) +
                 bump(x, y, 0, -3) + bump(x, y, -3, 0)))
         / 1.50158867011978);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Convert a binary string to a floating-point number in [-4:4]. */

double str2num(char *str)
{
  int i;
  double x = 0;

  for(i = 0; i < len; i++)
    x = x * 2 + (str[i] - '0');
  return((x / pow(2, len)) * 8 - 4);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Compute the fitness of each string.  */

void compute_fitness(char **pop, double *fit, double *normfit)
{
  int i;
  double sum;

  sum = 0;
  /* For each member of the popluation... */
  for(i = 0; i < size; i++) {
    /* Let the raw fitness be the output of the surface function. */
    fit[i] = surface(str2num(&pop[i][0]), str2num(&pop[i][len]));
    /* Sum up the scores so that they can be normalized below. */
    sum += fit[i];
  }
  /* Normalize all fitnesses between [0:1]. */
  for(i = 0; i < size; i++)
    normfit[i] = fit[i] / sum;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Perform random roulette selection according to normalized fitness. */

int select_one(double *normfit)
{
  int i;
  double x, sum;

  sum = 0;
  x = random_range(0, 1);
  for(i = 0; i < size; i++) {
    sum += normfit[i];
    /* Accept a choice based on cumulative sum of fitness (which
     * should be equal to 1 if done over all strings).
     */
    if(x <= sum)
      return(i);
  }
  /* Just in case there was a subtle numerical error. */
  return(size - 1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Make love not war.  Optionally cross parent A (PA) with parent P (PB).
   Children may also be affected by gamma rays.  Index refers to where
   the two new children should be placed in the new population. */

void reproduce(char **oldpop, char **newpop, int pa, int pb,int index)
{
  int i, cpoint;

  /* Pick a crossover point.  Note that a choice of 0 or len 
   * does nothing.  We also use (2 * len) below because there
   * are really two substrings of length len each in the string.
   */
  cpoint = (random_range(0, 1) < crate) ? 
    (random() % (2 * len - 1)) + 1 : 2 * len;

  /* Copy over the first cpoint characters. */
  for(i = 0; i < cpoint; i++) {
    newpop[index][i] = oldpop[pa][i];
    newpop[index + 1][i] = oldpop[pb][i];
  }
  /* Copy over the remaining characters, but this time
   * swap the DNA from the two parents.
   */
  for(i = cpoint; i < 2 * len; i++) {
    newpop[index][i] = oldpop[pb][i];
    newpop[index + 1][i] = oldpop[pa][i];
  }
  /* Optionally mutate the children. */
  for(i = 0; i < 2 * len; i++) {
    if(random_range(0, 1) < mrate)
      newpop[index][i] = random() % 2 + '0';
    if(random_range(0, 1) < mrate)
      newpop[index + 1][i] = random() % 2 + '0';
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Ugly-print some statistics. */

void dump_stats(int time, char **pop, double *fit)
{
  int i, besti = -1;
  double best = -1, ave = 0;

  /* Find the best match and average the scores. */
  for(i = 0; i < size; i++) {
    if(fit[i] > best) {
      besti = i; best = fit[i];
    }
    ave += fit[i];
  }
  ave /= size;
  printf("---\ntime = %d\n", time);
  printf("average value = %f\n", ave);
  printf("best (x, y) = (%f, %f)\n", str2num(&pop[besti][0]),
         str2num(&pop[besti][len]));
  printf("best DNA = \"%s\"\n", pop[besti]);
  printf("best value = %f\n", best);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  int i, j, t, parent_a, parent_b;
  char **swap, **newpop, **oldpop;
  double *fit, *normfit;
  
  get_options(argc, argv, options, help_string);
  srandom(seed);

  /* Force the size to be even. */
  size += (size / 2 * 2 != size);

  /* Initialize the population. */
  newpop = xmalloc(sizeof(char *) * size);
  oldpop = xmalloc(sizeof(char *) * size);
  fit = xmalloc(sizeof(double) * size);
  normfit = xmalloc(sizeof(double) * size);
  for(i = 0; i < size; i++) {
    newpop[i] = xmalloc(sizeof(char) * len * 2 + 1);
    oldpop[i] = xmalloc(sizeof(char) * len * 2 + 1);
    for(j = 0; j < len * 2; j++)
      oldpop[i][j] = random() % 2 + '0';
    oldpop[i][len * 2] = 0;
    newpop[i][len * 2] = 0;
  }

  /* For each time step... */
  for(t = 0; t < gens; t++) {
    compute_fitness(oldpop, fit, normfit);
    dump_stats(t, oldpop, fit);

    /* Pick two parents by fitness and mate them until the
     * next generation has been made.
     */
    for(i = 0; i < size; i += 2) {
      parent_a = select_one(normfit);
      parent_b = select_one(normfit);
      reproduce(oldpop, newpop, parent_a, parent_b, i);
    }
    /* Make everything old new again. */
    swap = newpop; newpop = oldpop; oldpop = swap;
  }

  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
