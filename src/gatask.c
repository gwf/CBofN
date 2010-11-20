
/* NAME
 *   gatask - solve a task assignment problem via a genetic algorithm
 * NOTES
 *   None.
 * MISCELLANY
 *   Feasible solutions are represented as an array of LEN integers
 *   which must contain all integers between 0 and LEN - 1 to denote
 *   which person performs which task.  As such, the mutation and
 *   crossover operators have to be subtly redefined so that candidate
 *   solutions are still feasible after they are crossed and mutated.
 *   
 *   For mutation, we simply swap two locations in a solution array.
 *   
 *   Crossing two solutions is a little more complicated.  Consult
 *   the source code to see how it's done in a manner that preserves
 *   the feasibility of the two children while blending portions of
 *   each parent solution.
 *   
 *   The fitness function works in three steps.  First, the score of a
 *   solution is calculated and denoted the raw fitness.  The scaled
 *   fitness is then set to pow(PBASE, raw fitness - worst raw fitness).
 *   The normalized fitness is then set to the scaled fitness divided by
 *   the sum of the scaled fitnesses.  Thus the sum of the normalized
 *   fitnesses must be equal to one while a string with a raw score
 *   of one better than another string is PBASE times as likely to
 *   reproduce, where PBASE is the value supplied with the -pbase
 *   option.
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

int size = 10, gens = 30, seed = 0, len;
double crate = 0.75, mrate = 0.01, pbase = 2.0, **cost;
char *specs  = "data/hop1.dat";

char help_string[] = "\
Use a genetic algorithm to solve a task assignment problem with \
user-specified costs.  This program illustrates how GAs can perform \
combinatorial optimization.  Reproduction of strings entails \
special crossover and mutation operations which preserve \
constraints on the form of feasible solutions with strings being \
selected based on fitness.\
";

OPTION options[] = {
  { "-specs",  OPT_STRING,  &specs,  "Problem specification file." },
  { "-size",   OPT_INT,     &size,   "Population size." },
  { "-gens",   OPT_INT,     &gens,   "Number of generations." },
  { "-seed",   OPT_INT,     &seed,   "Random seed." },
  { "-crate",  OPT_DOUBLE,  &crate,  "Crossover rate." },
  { "-mrate",  OPT_DOUBLE,  &mrate,  "Mutation rate." },
  { "-pbase",  OPT_DOUBLE,  &pbase,  "Exponentiation base." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Read in the specifications file which contains a single integer, n,
   that specifies the width and height and n*n numbers representing
   the costs for specific performers to do specific tasks. */

void read_specs(char *fname)
{
  FILE *fp;
  SCANNER *scan;
  char *str;
  int i, j;

  if(fname == NULL || (fp = fopen(fname, "r")) == NULL) {
    fprintf(stderr, "Cannot open specification file \"%s\".\n", fname);
    exit(1);
  }
  /* Let '#' be a comment character. */
  scan = scan_init(fp, "", " \t\n", "#");

  /* Get the length of the strings to work in, hwich is just equal
   * to the number of performers and tasks.
   */
  if((str = scan_get(scan)) == NULL) goto BADFILE;
  len = atoi(str);

  /* Allocate space for the costs to perform specific tasks with
   * specific workers.
   */
  cost = xmalloc(sizeof(double *) * len);
  for(i = 0; i < len; i++) {
    cost[i] = xmalloc(sizeof(double) * len);
    for(j = 0; j < len; j++) {
      if((str = scan_get(scan)) == NULL) goto BADFILE;
      cost[i][j] = atoi(str);
    }
  }
    
  return;
BADFILE:
  fprintf(stderr, "Problem found in specification file.\n");
  exit(1);  
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Generate a random but feasible solution which must have all integers
   between 0 and len - 1 in a random order. */

void random_solution(int *x)
{
  int i, j, t;

  /* Set the array to initially contain the vanilla numbers. */
  for(i = 0; i < len; i++)
    x[i] = i;
  /* Shuffle the deck... */
  for(i = 0; i < len - 1; i++) {
    /* Randomly pick a number between i and len - 1, inclusive. */
    j = (random() % (len - i)) + i;
    /* Swap x[i] and x[j]. */
    t = x[i]; x[i] = x[j]; x[j] = t;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Calculate the cost of a solution. */

int task_cost(int *solution)
{
  int i, sum = 0;
  
  for(i = 0; i < len; i++)
    sum += cost[i][solution[i]];
  return(sum);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Compute the fitness of each string.  */

void compute_fitness(int **pop, double *fit, double *normfit)
{
  int i;
  double sum, min = 10e10;

  sum = 0;
  /* For each member of the popluation... */
  for(i = 0; i < size; i++) {
    /* Get the raw fitness and figure out the minimum score. */
    fit[i] = task_cost(pop[i]);
    if(fit[i] < min) min = fit[i];
  }
  /* Temporarily compute the scaled fitness which forces a raw
   * fitness with a score one better than another to be twice as
   * fit (that is, twice as likely to reproduce).
   */
  for(i = 0; i < size; i++) {
    normfit[i] = pow(pbase, fit[i] - min);
    /* Sum up the powers so that they can be normalized below. */
    sum += normfit[i];
  }
  for(i = 0; i < size; i++)
    normfit[i] /= sum;
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

/* Make love not war.  Optionally cross parent A (PA) with parent B (PB)
   by swapping parts of the DNA in a manner consistant with the
   constraints imposed on what feasible solutions must look like.
   Children may also be affected by gamma rays.  Index refers to where
   the two new children should be placed in the new population. */

void reproduce(int **oldpop, int **newpop, int pa, int pb,int index)
{
  int i, j, a, b, ai, bi = 0;

  /* Copy over the parents to the children. */
  for(i = 0; i < len; i++) {
    newpop[index][i] = oldpop[pa][i];
    newpop[index + 1][i] = oldpop[pb][i];
  }
  /* Optionally do crossover... */
  if(random_range(0, 1) < crate) {

    /* NB: Let CA and CB be the children of PA and PB. */
    
    /* Get a random index into CA and note the value at that index. */
    ai = random() % len;
    a = newpop[index][ai];

    /* Note what CB had at the same location. */
    b = newpop[index + 1][ai];
    /* Look in CA to see where CB's value occurs and rember that index. */
    for(i = 0; i < len; i++)
      if(newpop[index][i] == b) {
        bi = i; break;
      }
    /* Set CA at the first random index to be what CB had at the same
     * location.  But also make the location in which CB had CA's value
     * equal to what was originally in the first random index.
     */
    newpop[index][ai] = b;
    newpop[index][bi] = a;

    /* Do a similar swap for CB. */
    a = newpop[index + 1][bi];
    newpop[index + 1][ai] = a;
    newpop[index + 1][bi] = b;
  }
  /* Optionally mutate by swapping the values at a random pair
   * of indices.
   */
  for(i = 0; i < len; i++) {
    if(random_range(0, 1) < mrate) {
      j = random() % len;
      a = newpop[index][i];
      newpop[index][i] = newpop[index][j];
      newpop[index][j] = a;
    }
    if(random_range(0, 1) < mrate) {
      j = random() % len;
      a = newpop[index + 1][i];
      newpop[index + 1][i] = newpop[index + 1][j];
      newpop[index + 1][j] = a;
    }
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Ugly-print some statistics. */

void dump_stats(int time, int **pop, double *fit)
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
  printf("best DNA      = ");
  for(i = 0; i < len; i++)
    printf((i < len - 1) ? "%d, " : "%d\n", pop[besti][i] + 1);
  printf("best score    = %d\n", (int)best);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  int i, j, t, parent_a, parent_b;
  int **swap, **newpop, **oldpop;
  double *fit, *normfit;
  
  get_options(argc, argv, options, help_string);
  srandom(seed);

  /* Read in the specifications for this task assignment problem. */
  read_specs(specs);

  /* Force the size to be even. */
  size += (size / 2 * 2 != size);

  /* Initialize the population. */
  newpop = xmalloc(sizeof(int *) * size);
  oldpop = xmalloc(sizeof(int *) * size);
  fit = xmalloc(sizeof(double) * size);
  normfit = xmalloc(sizeof(double) * size);
  for(i = 0; i < size; i++) {
    newpop[i] = xmalloc(sizeof(int) * len);
    oldpop[i] = xmalloc(sizeof(int) * len);
    for(j = 0; j < len * 2; j++)
      random_solution(oldpop[i]);
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
