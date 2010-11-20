
/* NAME
 *   eipd - simulate the ecological iterated Prisoner's Dilemma
 * NOTES
 *   None.
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
 * MISCELLANY
 *   random noise (via the -noise option) manifests itself as a
 *   cell making a randomly selected move in a single round.  In
 *   this case, both the cell whose action was altered as well as
 *   that cell's opponents "remember" what the random move was on
 *   the next round.
 *   
 *   During each time step, every strategy plays against every 
 *   other strategy as well as against itself.
 *   
 *   The initial population levels for all strategies will be
 *   normalized, so the scaling of the option values is irrelevant.
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

#define ALLC 0
#define TFT  1
#define RAND 2
#define PAV  3
#define ALLD 4

#define NUMSTRATS 5

int steps = 100000, rounds = 10, seed = 0;
double CC = 3, CD = 0, DC = 5, DD = 1, rcp = 0.5, noise = 0;
double Irand = 0.2, Iallc = 0.2, Ialld = 0.2 , Itft = 0.2, Ipav = 0.2;
double pops[NUMSTRATS], scores[NUMSTRATS];

char help_string[] = "\
The ecological iterated Prisoner's Dilemma is simulated  over time \
according to the specified parameters.  At every time step the population \
of each strategy is calculated as a function of the expected scores \
earned against all strategies weighted by the populations of the \
opponents.  Possible strategies include 'Always Cooperate,' 'Always Defect,' \
'Random,' 'Pavlov,' and 'Tit-for-Tat.'\
";

OPTION options[] = {
  { "-steps",  OPT_INT,     &steps,  "Number of steps to simulate." },
  { "-rounds", OPT_INT,     &rounds, "Number of rounds per step." },
  { "-seed",   OPT_INT,     &seed,   "Random seed for initial state." },
  { "-CC",     OPT_DOUBLE,  &CC,     "Reward Payoff." },
  { "-CD",     OPT_DOUBLE,  &CD,     "Sucker Payoff." },
  { "-DC",     OPT_DOUBLE,  &DC,     "Temptation Payoff." },
  { "-DD",     OPT_DOUBLE,  &DD,     "Punish Payoff." },
  { "-Iallc",  OPT_DOUBLE,  &Iallc,  "Initial population of All-C." },
  { "-Itft",   OPT_DOUBLE,  &Itft,   "Initial population of TFT." },
  { "-Irand",  OPT_DOUBLE,  &Irand,  "Initial population of Random." },
  { "-Ipav",   OPT_DOUBLE,  &Ipav,   "Initial population of Pavlov." },
  { "-Ialld",  OPT_DOUBLE,  &Ialld,  "Initial population of All-D." },
  { "-rcp",    OPT_DOUBLE,  &rcp,    "Probability of C for Random strategy." },
  { "-noise",  OPT_DOUBLE,  &noise,  "Probability of noise." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* This function returns the play made by a strategy given the last 
   move of his opponent and himself. */

int pd(int strat, int last_him, int last_me)
{
  /* First see if a random play is appropriate. */
  if(noise > 0 && random_range(0, 1) < noise)
    return(random_range(0, 1) < 0.5 ? 0 : 1);

  /* Always cooperate, regardless. */
  if(strat == ALLC)
    return(0);

  /* Since the histories are initialize with all cooperation,
   * Tit-for-Tat simply plays whatever his opponent did the last
   * round. 
   */
  if(strat == TFT)
    return(last_him);

  /* Random cooperates with probability rcp and defect other times. */
  if(strat == RAND)
    return((random_range(0,1) < rcp) ? 0 : 1);

  /* Pavlov does what he did if he wasn't punished, but does what he
   * didn't do when he was punished.
   */
  if(strat == PAV)
    return(last_him ? !last_me : last_me);

  /* Always Defect is just a mean bastard! */
  if(strat == ALLD)
    return(1);

  return(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Returns the score earned by player one if he played s1 and his
   opponent played s2. */

double pay(int s1, int s2)
{
  if(s1) {
    if(s2) return(DD);
    else return(DC);
  }
  else {
    if(s2) return(CD);
    else return(CC);
  }      
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  int i, j, k, l, acthim, actme, lasthim, lastme;
  double sum, payout;

  get_options(argc, argv, options, help_string);
  srandom(seed);

  /* Normalize all of the populations so that they equal probabilities,
   * Then accumulate them so that we can do roulette selection.
   */
  sum = Iallc + Ialld + Irand + Itft + Ipav;
  pops[ALLC] = Iallc / sum;
  pops[TFT]  = Itft  / sum;
  pops[RAND] = Irand / sum;
  pops[PAV]  = Ipav  / sum;
  pops[ALLD] = Ialld / sum;

  /* For each time step... */
  for(i = 0; i < steps; i++) {

    /* Print out the normalized population levels. */
    for(j = 0; j < NUMSTRATS; j++)
      printf("%f\t", pops[j]);
    printf("\n");

    /* For each strategy... */
    for(j = 0; j < NUMSTRATS; j++) {
      /* Zero out the score. */
      scores[j] = 0;

      /* Skip any strategy that is completely dead. */
      if(pops[j] == 0.0) continue;

      /* For every strategy (again) ... */
      for(k = 0; k < NUMSTRATS; k++) {
        
        /* Set running sum to 0. */
        sum = 0;

        /* Assume cooperation on previous move for first game. */
        lasthim = lastme = 0;

        /* For every round. */
        for(l = 0; l < rounds; l++) {
          
          /* Get two actions for the two players. */
          actme = pd(j, lasthim, lastme);
          acthim = pd(k, lastme, lasthim);

          /* Get the payout. */
          payout = pay(actme, acthim);

          /* Same the last move for the next game. */
          lastme = actme; lasthim = acthim;

          /* Keep a running total of the scores. */
          sum += payout;
        }

        /* Set the score weighted by the population of the opponents. */
        scores[j] += sum * pops[k];
      }
    }

    /* Get the sum of the populations and renormalize. */
    sum = 0;
    for(j = 0; j < NUMSTRATS; j++) {
      pops[j] = pops[j] * scores[j];
      sum += pops[j];
    }
    for(j = 0; j < NUMSTRATS; j++)
      pops[j] /= sum;
  }

  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
