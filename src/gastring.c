
/* NAME
 *   gastring - breed strings with a genetic algorithm
 * NOTES
 *   None.
 * MISCELLANY
 *   The fitness function works in three steps.  First, the number of
 *   correct characters is tallied and denoted the raw fitness.  The
 *   scaled fitness is then set to pow(PBASE, raw fitness - string
 *   len).  The normalized fitness is then set to the scaled fitness
 *   divided by the sum of the scaled fitnesses.  Thus the sum of the
 *   normalized fitnesses must be equal to one while a string with one
 *   letter more correct than another string is PBASE times as likely
 *   to reproduce, where PBASE is the value supplied with the -pbase
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

int size = 500, steps = 50, seed = 0;
double crate = 0.75, mrate = 0.01, pbase = 2;
char *target = "furious green ideas sweat profusely";

char help_string[] = "\
Use a genetic algorithm to breed strings that match a user-specified \
target string.  This program illustrates how GAs can perform a type of \
stochastic search in a space of discrete objects.  Reproduction of \
strings entails crossover and mutation with strings being selected \
based on fitness.\
";

OPTION options[] = {
  { "-target", OPT_STRING,  &target, "Target string." },
  { "-size",   OPT_INT,     &size,   "Population size." },
  { "-steps",  OPT_INT,     &steps,  "Number of generations." },
  { "-seed",   OPT_INT,     &seed,   "Random seed." },
  { "-crate",  OPT_DOUBLE,  &crate,  "Crossover rate." },
  { "-mrate",  OPT_DOUBLE,  &mrate,  "Mutation rate." },
  { "-pbase",  OPT_DOUBLE,  &pbase,  "Power base for fitness." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Generate a random letter or space with equal probability. */

int random_letter_or_space(void)
{
  int letter;

  letter = (random() % ('z' - 'a' + 2)) + 'a';
  if(letter > 'z')
    letter = ' ';
  return(letter);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Compute the fitness of each string.  */

void compute_fitness(int tlen, char **pop, int *correct, double *fit)
{
  int i, j, count;
  double sum;

  sum = 0;
  /* For each member of the popluation... */
  for(i = 0; i < size; i++) {
    /* Count the number of letters that are correct. */
    count = 0;
    for(j = 0; j < tlen; j++)
      if(pop[i][j] == target[j]) count++;
    correct[i] = count;
    /* Compute pbase raised to the (no. correct - len) power.
     * Thus, having one more letter correct is pbase times
     * as good.
     */
    fit[i] = pow(pbase, correct[i] - tlen);

    /* Sum up the powers so that they can be normalized below. */
    sum += fit[i];
  }
  for(i = 0; i < size; i++)
    fit[i] /= sum;
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

void reproduce(int tlen, char **oldpop, char **newpop, int pa, int pb,
               int index)
{
  int i, cpoint;

  /* Pick a crossover point.  Note that a choice of 0 or tlen 
   * does nothing.
   */
  cpoint = (random_range(0, 1) < crate) ? 
    (random() % (tlen - 1)) + 1 : tlen;

  /* Copy over the first cpoint characters. */
  for(i = 0; i < cpoint; i++) {
    newpop[index][i] = oldpop[pa][i];
    newpop[index + 1][i] = oldpop[pb][i];
  }
  /* Copy over the remaining characters, but this time
   * swap the DNA from the two parents.
   */
  for(i = cpoint; i < tlen; i++) {
    newpop[index][i] = oldpop[pb][i];
    newpop[index + 1][i] = oldpop[pa][i];
  }
  /* Optionally mutate the children. */
  for(i = 0; i < tlen; i++) {
    if(random_range(0, 1) < mrate)
      newpop[index][i] = random_letter_or_space();
    if(random_range(0, 1) < mrate)
      newpop[index + 1][i] = random_letter_or_space();
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Ugly-print some statistics. */

void dump_stats(int time, int tlen, double *fit, int *correct, char **pop)
{
  int i, besti = -1;
  double best = -1, ave = 0;

  /* Find the best match and average the scores. */
  for(i = 0; i < size; i++) {
    if(fit[i] > best) {
      besti = i; best = fit[i];
    }
    ave += correct[i];
  }
  ave /= size;
  ave /= tlen;
  printf("---\ntime = %d\n", time);
  printf("average %% letters correct = %f\n", ave);
  printf("best %% letters correct = %f\n", correct[besti] / (double)tlen);
  printf("best = \"%s\"\n", pop[besti]);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  int i, j, t, targetlen, parent_a, parent_b, *numcorrect;
  char **swap, **newpop, **oldpop;
  double *normfit;
  
  get_options(argc, argv, options, help_string);
  srandom(seed);

  /* Force the size to be even. */
  size += (size / 2 * 2 != size);

  /* Initialize the population. */
  targetlen = strlen(target);
  newpop = xmalloc(sizeof(char *) * size);
  oldpop = xmalloc(sizeof(char *) * size);
  numcorrect = xmalloc(sizeof(int) * size);
  normfit = xmalloc(sizeof(double) * size);
  for(i = 0; i < size; i++) {
    newpop[i] = xmalloc(sizeof(char) * targetlen + 1);
    oldpop[i] = xmalloc(sizeof(char) * targetlen + 1);
    for(j = 0; j < targetlen; j++)
      oldpop[i][j] = random_letter_or_space();
    oldpop[i][targetlen] = 0;
    newpop[i][targetlen] = 0;
  }

  /* For each time step... */
  for(t = 0; t < steps; t++) {
    compute_fitness(targetlen, oldpop, numcorrect, normfit);
    dump_stats(t, targetlen, normfit, numcorrect, oldpop);

    /* Pick two parents by fitness and mate them until the
     * next generation has been made.
     */
    for(i = 0; i < size; i += 2) {
      parent_a = select_one(normfit);
      parent_b = select_one(normfit);
      reproduce(targetlen, oldpop, newpop, parent_a, parent_b, i);
    }
    /* Make everything old new again. */
    swap = newpop; newpop = oldpop; oldpop = swap;
  }

  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  
  
