
/* NAME
 *   gaipd - breed IPD strategies with a genetic algorithm
 * NOTES
 *   See the STRINGS section for information on how the strategies
 *   are encoded as strings.   Also note that the dnaindex[] array
 *   allows you to index the first location of a strategy string
 *   that corresponds to a sub-strategy for a particular history
 *   length.
 * PAYOFFS
 *   The payoff matrix for the Prisoner's Dilemma game is usually
 *   expressed as:
 *                             Player B's Move
 *                        +-----------+-----------+
 *        Player A's Move | cooperate |  defect   |
 *            +-----------+-----------+-----------+
 *            | cooperate |  CC, CC   |  CD, DC   |
 *            +-----------+-----------+-----------+
 *            |    defect |  DC, CD   |  DD, DD   |
 *            +-----------+-----------+-----------+
 *   
 *   where the table entries are (A's payoff, B's payoff) and
 *   CC, CD, DC, and DD are the reward, sucker, temptation,
 *   and punish payoffs, respectively.  For each of these four
 *   outcomes you will probably want the payoffs to reflect the
 *   relationships:  
 *   
 *        (DC > CC > DD > CD) and ((CD + DC) / 2 < CC).
 * GENERATIONS
 *   A single generation proceeds as follows.  Each member of the
 *   population must play several bouts with randomly selected
 *   opponents.  For each opponent, several rounds are played.  The
 *   total score after these bouts is a strategy's raw fitness score.
 * STRINGS
 *   Since population strings may be optionally displayed at the end
 *   of the simulation, this section describes the format of these
 *   strings.  Given two players, A and B, and the current time, t,
 *   and letting cooperation be denoted by 0 and defection by 1, form
 *   a bit string such as:
 *   
 *        A(t-1) B(t-1) A(t-2) B(t-2) ... A(t-H) B(t-H)
 *   
 *   where A(T) and B(T) are A and B's moves from time T,
 *   respectively, and H is the number of time steps "remembered" by
 *   each player.  This bit string can take 2^(2 * H) values.  To
 *   define a complete strategy, we must have a H + 1 separate tables
 *   of this form to describe each possible history.  Thus, the rule
 *   table string used internally in the program and displayed at the
 *   end have as their first entry the move to make with no previous
 *   history, followed by four entries for H equals to 1, followed by
 *   sixteen entries for when H equals 2, and so on.
 *   
 *   As an annotated example, Tit-for-Tat is encoded as "CCDCD" so 
 *   that the first "C" indicates that the first move should be C
 *   while the last for characters indicate what to play if 
 *   A(t-1) and B(t-1) is equal to (C, C), (C, D), (D, C), and (D, D),
 *   respectively.
 * HINTS
 *   Without any parameters (and assuming you have an uncorrupted
 *   version if this source code) running this program without any
 *   parameters will probably result in Tit-for-Tat, "CCDCD",
 *   dominating. If you run it with the -noise 0.1 option, then
 *   Pavlov, "CCDDC", will probably win in the end since it is more
 *   resistant to noise.
 *   
 *   See the author's book, "The Computational Beauty of Nature," for
 *   more details.
 * MISCELLANY
 *   The fitness function relies on two steps done after the raw
 *   fitness scores are calculated.  The raw fitness score from the
 *   previous step is divided by the total number of PD rounds played.
 *   (Note that this may vary among population members since opponents
 *   are selected at random.)  This yields a scaled fitness score.
 *   The normalized fitness is then set to the scaled fitness divided
 *   by the sum of the scaled fitnesses.  Thus the sum of the
 *   normalized fitnesses must be equal to one.
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

double DC = 5, CC = 4, DD = 1, CD = 0;
int size = 100, gens = 50, bouts = 50;
int rounds = 20, hlen = 1, seed = 0, dump = 0;
double crate = 0.25, mrate = 0.001, noise = 0.0;

char help_string[] = "\
Use a genetic algorithm to evolve IPD strategies according to \
user-specified constraints.  This program illustrates how GAs can \
demonstrate co-evolution since IPD strategies can only be successful \
within the context of their likely opponents.  Reproduction of \
strategies entails crossover and mutation with strategies being \
selected based on fitness.\
";

OPTION options[] = {
  { "-size",   OPT_INT,     &size,   "Population size." },
  { "-gens",   OPT_INT,     &gens,   "Number of generations." },
  { "-bouts",  OPT_INT,     &bouts,  "Bouts per generation." },
  { "-rounds", OPT_INT,     &rounds, "Rounds per bout." },
  { "-hlen",   OPT_INT,     &hlen,   "History length." },
  { "-seed",   OPT_INT,     &seed,   "Random seed." },
  { "-crate",  OPT_DOUBLE,  &crate,  "Crossover rate." },
  { "-mrate",  OPT_DOUBLE,  &mrate,  "Mutation rate." },
  { "-noise",  OPT_DOUBLE,  &noise,  "Chance of mistake in transaction." },
  { "-CC",     OPT_DOUBLE,  &CC,     "Reward Payoff." },
  { "-CD",     OPT_DOUBLE,  &CD,     "Sucker Payoff." },
  { "-DC",     OPT_DOUBLE,  &DC,     "Temptation Payoff." },
  { "-DD",     OPT_DOUBLE,  &DD,     "Punish Payoff." },
  { "-dump",   OPT_SWITCH,  &dump,   "Print entire population at end?" },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* These are global to avoid excessive parameter passing. */
double  *fitness;
int *dnaindex, *score, *roundbout, *hista, *histb;
char **oldpop, **newpop;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Given two strategies, calculate the respective scores for a single
   round of the Prisoner's Dilemma. */

void pd(int strata, int stratb, int *scorea, int *scoreb)
{
  if(strata == 0 && stratb == 0)
    *scorea = *scoreb = CC;
  else if(strata == 1 && stratb == 0) {
    *scorea = DC; *scoreb = CD;
  }
  else if(strata == 0 && stratb == 1) {
    *scorea = CD; *scoreb = DC;
  }
  else
    *scorea = *scoreb = DD;    
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Perform a single bout of the IPD. */

void ipd(int strata, int stratb, int time, int *scorea, int *scoreb)
{
  int i, movea, moveb, indexa, indexb, hindex, h, t;

  /* Compute the amount of history to consider. */
  t = (time > hlen) ? hlen : time; 

  /* Get the current index in the history array. */
  hindex = time % hlen;

  indexa = indexb = 0;

  /* For all previous times that we "remember" ... */
  for(i = 0; i < t; i++) {
    /* Get the index of the time step (i + 1) units in the past. */
    h = (hindex - i - 1 + hlen) % hlen;
    
    /* Compute the DNA indices that corresponds to this history. */
    indexa = indexa * 2 + hista[h]; indexa = indexa * 2 + histb[h];
    indexb = indexb * 2 + histb[h]; indexb = indexb * 2 + hista[h];
  }

  /* Now grab the move from the DNA. */
  movea = oldpop[strata][dnaindex[t] + indexa];
  moveb = oldpop[stratb][dnaindex[t] + indexb];

  /* Optionally add noise to a move. */
  if(random_range(0,1) < noise) movea = random() % 2;
  if(random_range(0,1) < noise) moveb = random() % 2;

  /* Get the actual scores. */
  pd(movea, moveb, scorea, scoreb);

  /* Save the move for the next round in this bout. */
  hista[hindex] = movea;  histb[hindex] = moveb;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Compute the fitness of each member of the population.  */

void compute_fitness(void)
{
  int i, j, k, opponent, scorea, scoreb;
  double sum;
  
  /* Zero out the initial scores and the number of games played. */
  for(i = 0; i < size; i++)
    roundbout[i] = score[i] = 0;

  /* For each member of the popluation... */
  for(i = 0; i < size; i++)
    /* Perform a bunch of bouts... */
    for(j = 0; j < bouts; j++) {
      /* Get a random opponent. */
      opponent = random() % size;
      scorea = scoreb = 0;
      /* Perform the IPD for a bunch of rounds. */
      for(k = 0; k < rounds; k++) {
        ipd(i, opponent, k, &scorea, &scoreb);
        /* Tally the cumulative scores. */
        score[i] += scorea;
        score[opponent] += scoreb;
        /* Keep track of the number of rounds played by each player. */
        roundbout[i]++;
        roundbout[opponent]++;
      }
    }

  /* Normalize the scores by the number of rounds * bouts
   * and sum up all scores so that we can normalize them
   * by the total raw fitness of the population below.
   */
  sum = 0;
  for(i = 0; i < size; i++) {
    fitness[i] = score[i] / (double) roundbout[i];
    sum += fitness[i];
  }
  for(i = 0; i < size; i++)
    fitness[i] /= sum; 
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
     * should be equal to 1 if done over all string).
     */
    if(x <= sum)
      return(i);
  }
  /* Just in case there was a subtle numerical error. */
  return(size - 1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Make love not war.  Optionally cross parent A (PA) with parent P (PB).
   Children may also be affected by gamma rays.  Childindex refers to
   where the two new children should be placed in the new population. */

void reproduce(int parenta, int parentb, int childindex)
{
  int i, cpoint, dnalen;

  dnalen = dnaindex[hlen + 1];

  /* Pick a crossover point.  Note that a choice of 0 or tlen 
   * does nothing.
   */
  cpoint = (random_range(0, 1) < crate) ? 
    (random() % (dnalen - 1)) + 1 : dnalen;

  /* Copy over the first cpoint characters. */
  for(i = 0; i < cpoint; i++) {
    newpop[childindex][i] = oldpop[parenta][i];
    newpop[childindex + 1][i] = oldpop[parentb][i];
  }
  /* Copy over the remaining characters, but this time
   * swap the DNA from the two parents.
   */
  for(i = cpoint; i < dnalen; i++) {
    newpop[childindex][i] = oldpop[parentb][i];
    newpop[childindex + 1][i] = oldpop[parenta][i];
  }
  /* Optionally mutate the children. */
  for(i = 0; i < dnalen; i++) {
    if(random_range(0, 1) < mrate)
      newpop[childindex][i] = random() % 2;
    if(random_range(0, 1) < mrate)
      newpop[childindex + 1][i] = random() % 2;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Ugly-print some statistics. */

void dump_stats(int time)
{
  int i, besti = -1;
  double best = -1, ave = 0;

  /* Find the best match and average the scores. */
  for(i = 0; i < size; i++) {
    if(fitness[i] > best) {
      besti = i; best = fitness[i];
    }
    ave += score[i] / (double) roundbout[i];
  }
  ave /= size;
  fprintf(stderr, "---\ntime = %d\n", time);
  fprintf(stderr, "average score = %f\n", ave);
  fprintf(stderr, "best average score = %f\n", score[besti] /
         (double)roundbout[besti]);
  fprintf(stderr, "best = ");
  for(i = 0; i < dnaindex[hlen + 1]; i++)
    fputc(oldpop[besti][i] + 'C', stderr);
  fputc('\n', stderr);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Initialize space for the scores, round * bount counts, fitness
   and history for each IPD player. */

void inititalize_population(void)
{
  int i, j;

  score = xmalloc(sizeof(int) * size);
  roundbout = xmalloc(sizeof(int) * size);
  fitness = xmalloc(sizeof(double) * size);
  hista = xmalloc(sizeof(int) * hlen);
  histb = xmalloc(sizeof(int) * hlen);

  /* Dnaindex[] is a special array that simplifies how we determine
   * the next move based on prior moves.  It is indexed by a time
   * parameter and returns a value that indicates where in the
   * a population member's DNA the "lookup table" for that prior
   * history is contained.  As a special case, dnaindex[hlen + 1]
   * returns the total length of the DNA.
   */
  dnaindex = xmalloc(sizeof(int) * (hlen + 2));
  dnaindex[0] = 0;
  for(i = 1; i < hlen + 2; i++)
    dnaindex[i] = dnaindex[i - 1] + pow(2, (i - 1) * 2);

  oldpop = xmalloc(sizeof(char *) * size);
  newpop = xmalloc(sizeof(char *) * size);
  for(i = 0; i < size; i++) {
    oldpop[i] = xmalloc(sizeof(char) * dnaindex[hlen + 1]);
    newpop[i] = xmalloc(sizeof(char) * dnaindex[hlen + 1]);
    /* Start of with random DNA. */
    for(j = 0; j < dnaindex[hlen + 1]; j++)
      oldpop[i][j] = random() % 2;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  int t, i, j, parent_a, parent_b;
  char **swap;

  get_options(argc, argv, options, help_string);
  srandom(seed);

  /* Force the size to be even. */
  size += (size / 2 * 2 != size);

  /* Initialize the population. */
  inititalize_population();

  /* For each time step... */
  for(t = 0; t < gens; t++) {
    compute_fitness();
    dump_stats(t);

    /* Pick two parents by fitness and mate them until the
     * next generation has been made.
     */
    for(i = 0; i < size; i += 2) {
      parent_a = select_one(fitness);
      parent_b = select_one(fitness);
      reproduce(parent_a, parent_b, i);
    }
    /* Make everything old new again. */
    swap = newpop; newpop = oldpop; oldpop = swap;
  }
  
  /* Dump out all strategies to stdout for posterity. */
  if(dump)
    for(j = 0; j < size; j++) {
      for(i = 0; i < dnaindex[hlen + 1]; i++)
        fputc(oldpop[j][i] + 'C', stdout);
      fputc('\n', stdout);
    }

  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
