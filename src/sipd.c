
/* NAME
 *   sipd - simulate the spatial iterated Prisoner's Dilemma
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
 *   The option for the probability of mutation (-mute) corresponds
 *   to the act of a cell spontaneously changing to a randomly
 *   selected strategy independent of the outcome of the most recent
 *   set of rounds.
 *   
 *   random noise (via the -noise option) manifests itself as a
 *   cell making a randomly selected move in a single round.  In
 *   this case, both the cell whose action was altered as well as
 *   that cell's opponents "remember" what the random move was on
 *   the next round.
 *   
 *   The value supplied with the -term option may be "none," in
 *   which case no graphic output is performed.  This is useful
 *   if you simply want the statistics to be calculated for each
 *   time step (via the -stats option).
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

int width = 100, height = 100, steps = 10000000, seed = 0, invert = 0;
int rounds = 5, stats = 0, mag = 1;
double CC = 3, CD = 0, DC = 5, DD = 1, rcp = 0.5, noise = 0, mute = 0;
double Irand = 0.2, Iallc = 0.2, Ialld = 0.2 , Itft = 0.2, Ipav = 0.2;
char *term = NULL;

char help_string[] = "\
The spatial iterated Prisoner's Dilemma is simulated and plotted over \
time according to the specified parameters.  Each cell in a grid plays \
a specific strategy against its eight neighbors for several rounds. \
At the end of the last round, each cell copies the strategy of its \
most succesful neighbor, which is then used for the next time step. \
Possible strategies include 'Always Cooperate,' 'Always Defect,' \
'Random,' 'Pavlov,' and 'Tit-for-Tat.'\
";

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "Width of world." },
  { "-height", OPT_INT,     &height, "Height of world." },
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
  { "-mute",   OPT_DOUBLE,  &mute,   "Probability of mutation." },
  { "-stats",  OPT_SWITCH,  &stats,  "Print statistics?" },
  { "-inv",    OPT_SWITCH,  &invert, "Invert all colors?" },
  { "-mag",    OPT_INT,     &mag,    "Magnification factor." },
  { "-term",   OPT_STRING,  &term,   "How to plot points." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* Macros so that we can refer to the strategies by nickname instead
   of number. */

#define ALLC 0
#define TFT  1
#define RAND 2
#define PAV  3
#define ALLD 4

/* Strings to pretty-print the strategy name. */

#define NUMSTRATS 5

char *stratname[NUMSTRATS] = {
  "All C       :",
  "Tit for Tat :",
  "Random      :",
  "Pavlov      :",
  "All D       :"
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* This function returns the play made by a strategy given the last 
   move of his opponent and himself. */

int pd(int strat, int last_him, int last_me)
{
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

/* The function below, ipd(), forms the heart of this program.  It
   performs the gist of the IPD competition in a manner that is both
   time and space efficient.  As such, some of the hackery below is
   somewhat nonintuitive, so some effort will put into this comment
   to explain how it works.

   The first two arguments of ipd() are the indices i and j which
   represent a single cell in the grid.  If this function is called on
   all legal pairs of i and j (as is done in the main loop of the
   main() function), then the IPD will have been played such that
   every cell has played all of its neighbors.  Thus, the whole
   competition is done in a single pass.  Here is how this portion of
   the code works:

   For any i and j, this routine plays cell(i, j) against cell(i - 1,
   j), cell(i - 1, j - 1), cell(i, j - 1), and cell(i + 1, j - 1).
   These are, respectively, the W, NW, N, and NE neighbors.  Notice
   that this is exactly one-half of the cells that cell(i, j) is
   supposed to compete against.  When cell(i, j) is played against
   these neighbors, we get the scores for both cell(i, j) and the
   cells that it was played against.  But when does cell(i, j) play
   against any of the remaining four neighbors?  The trick is that
   when this routine is eventually called on, say, cell(i + 1, j), our
   cell(i, j) will be west of cell(i + 1, j), which means that this
   portion of the competition will be fulfilled at the latter function
   call.  Similarly, when the cells that sit E, SE, SE, and SW are
   called such that their coordinates are the i and j arguments to
   this function, all of cell(i,j)'s competitions will be fulfilled.

   There are some helpful macros and arrays below that assist in
   making all of this work.  They are explained in greater detail
   below.

   The next bit of hackery involves how we keep track of how a cell
   played with respect to another cell on the previous time step.
   Since there are eight neighbors, we can store the eight last moves
   in the bits of a single unsigned character.  The macros GETBIT()
   and SETBIT() are used for this purpose.

   The routine below does all of what was explained above, and not
   much more.  */


/* Given the integers 0, 1, 2, and 3, the macros INT2POSi() and
   INT2POSj() give the relative (i, j) offsets of the cells that sit
   W, NW, N, and NE to our cell.  Thus, we can reference the
   coordinates of the four cells by the four integers, which is
   nice. */

static int int2pos[4][2] = { { -1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 } };

#define INT2POSi(i) int2pos[i][0]
#define INT2POSj(i) int2pos[i][1]


/* The purpose of the POS2INT() macro, below, is to have a handy way
   of addressing the bits in an unsigned char that contains the last
   action played by some cell with respect to another cell.  Let
   cell(i, j) be the center cell whose coordinates are the i, j
   arguments to ipd().  For any of the four other cells that cell(i,
   j) plays against, let k and l be the offset from i and j for the
   position of the opponent.  Thus, ignoring wrap-around of the grid,
   the opponent is cell(i+k, j+l) where k and l can be any of the
   pairs returned from the INT2POS macros.

   Notice that cell(i, j) = cell(i+k-k, j+l-l), which means that the
   original cell's position relative to the opponent resides at an
   offset that is negative k and l.  The POS2INT() macro has the
   property that for all eight unique near-neighbor offsets, it
   returns an integer between 0 and 7.

   The bottom line is that we can plug in k and l in POS2INT() to
   reference a position of a byte that means "This cell played this
   bit against cell(i+k,j+l) on the last move."  But we can also use
   POS2INT() with -k and -l as arguments to refer to what cell(i+k,j+l)
   played against cell(i,j).
   
   The integers returned by POS2INT() are more or less arbitrary as
   long as they come from any mapping that is one-to-one.  The table
   below explains things in a little more detail.

     i   j   POS2INT(i, j)   POS2INT(-i, -j)
   ----------------------------------------------
    -1  -1       5               2
    -1   0       3               4
    -1   1       0               7
     0  -1       6               1
    #0###0######-1##############-1#  nonsense numbers
     0   1       1               6
     1  -1       7               0
     1   0       4               3
     1   1       2               5

   In a nutshell, POS2INT() gives us bit indices into a byte that can
   be partially reversed by the signed arguments that it take.   This
   will be much clearer when used below.  */

static int pos2int[3][3] = { { 5, 3, 0 }, { 6, -1, 1 }, { 7, 4,  2} };

#define POS2INT(i, j) pos2int[(i)+1][(j)+1]


/* Finally, the function itself. */

void ipd(int i, int j, unsigned char **strat, unsigned char **act,
               unsigned char **lastact, double **score)
{
  int wi, k, l, ii, jj, act1, act2, prev1, prev2;
  
  /* For the four neighbors that sit relative to cell(i,j) at positions
   * W, NW, N, and NE, do the following...
   */
  for(wi = 0; wi < 4; wi++) {
    
    /* Get the relative offsets from cell(i,j). */
    k = INT2POSi(wi); l = INT2POSj(wi);

    /* Turn these into indices that can reference the other data
     * structures. 
     */
    ii = (i + width + k) % width;
    jj = (j + height + l) % height;

    /* Get cell(i,j)'s last move when played against cell(i+k,j+l). */
    prev1 = GETBIT(lastact[i][j], POS2INT(k, l));

    /* Get cell(i+k,j+l)'s last move when played against cell(i,j). */
    prev2 = GETBIT(lastact[ii][jj], POS2INT(-k, -l));

    /* Chose a random action for cell(i, j), if appropriate, or... */
    if(noise > 0 && random_range(0, 1) < noise)
      act1 = random() % 2;
    /* Make the action determined by the players and the previous moves. */
    else
      act1 = pd(strat[i][j], prev2, prev1);

    
    /* Chose a random action for cell(ii, jj), if appropriate, or... */
    if(noise > 0 && random_range(0, 1) < noise)
      act2 = random() % 2;
    /* Make the action determined by the players and the previous moves. */
    else
      act2 = pd(strat[ii][jj], prev1, prev2);
    
    /* Update the two scores appropriately by swapping actions. */
    score[i][j] += pay(act1, act2);
    score[ii][jj] += pay(act2, act1);

    /* Set cell(i,j)'s action bit for this opponent to be his play
     * this game, and do the same for the opponent. 
     */
    act[i][j] = SETBIT(act[i][j], POS2INT(k, l), act1);
    act[ii][jj] = SETBIT(act[ii][jj], POS2INT(-k, -l), act2);
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Within the 3x3 grid centered on cell(i, j), find and return the best
   strategy found. */

int beststrat(int i, int j, unsigned char **strat, double **score)
{
  int k, l, ii, jj, besti = 0, bestj = 0;
  double best = -1;

  /* For all of the near neighbors... */
  for(k = -1; k <= 1; k++)
    for(l = -1; l <= 1; l++) {
      ii = (i + width + k) % width;
      jj = (j + height + l) % height;
      if(score[ii][jj] >= best) {
        /* If this is definitely better, than take it. */
        if(score[ii][jj] > best) {
          besti = ii;
          bestj = jj;
          best = score[ii][jj];
        }
        /* But if this was a tie, then only take it if they use the
         * same strategies.
         */
        else if(strat[ii][jj] == strat[i][j]) {
          besti = ii;
          bestj = jj;
          best = score[ii][jj];
        }
      }
    }
  return(strat[besti][bestj]);      
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  extern int plot_mag;
  extern int plot_inverse;
  double num, stratpop[NUMSTRATS], stratfit[NUMSTRATS];
  unsigned char **strat, **newstrat, **act, **lastact, **temp;
  double **score;
  int i, j, k, t, doplot = 1;

  get_options(argc, argv, options, help_string);

  /* Special case:  If term is "none" then we will not do any graphics. */
  if(term && strcmp(term, "none") == 0) doplot = 0;

  if(doplot) {
    plot_mag = mag;
    plot_inverse = invert;
    plot_init(width, height, NUMSTRATS, term);
    plot_set_all(0);
  }

  srandom(seed);

  /* Normalize all of the populations so that they equal probabilities,
   * Then accumulate them so that we can do roulette selection.
   */
  num = Iallc + Ialld + Irand + Itft + Ipav;
  Iallc /= num;  Ialld /= num;  Itft /= num; Irand /= num; Ipav /= num;
  Ialld += Iallc; Itft += Ialld; Irand += Itft; Ipav += Irand;

  /* Allocate memory for the scores, strategies, actions, previous
   * actions, and new strategies.
   */
  score = xmalloc(sizeof(double *) * width);
  strat = xmalloc(sizeof(unsigned char *) * width);
  act = xmalloc(sizeof(unsigned char *) * width);
  lastact = xmalloc(sizeof(unsigned char *) * width);
  newstrat = xmalloc(sizeof(unsigned char *) * width);
  for(i = 0; i < width; i++) {
    score[i] = xmalloc(sizeof(double) * height);
    strat[i] = xmalloc(sizeof(unsigned char) * height);
    newstrat[i] = xmalloc(sizeof(unsigned char) * height);
    act[i] = xmalloc(sizeof(unsigned char) * height);
    lastact[i] = xmalloc(sizeof(unsigned char) * height);

    for(j = 0; j < height; j++) {
      /* Do roulette selection for the initial configuration. */
      num = random_range(0, 1);
      if(num < Iallc) strat[i][j] = ALLC;
      else if(num < Ialld) strat[i][j] = ALLD;
      else if(num < Itft) strat[i][j] = TFT;
      else if(num < Irand) strat[i][j] = RAND;
      else strat[i][j] = PAV;
      lastact[i][j] = act[i][j] = 0;
    }
  }

  /* For each time step... */
  for(t = 0; t < steps; t++) {

    /* Plot the strategies, if appropriate, and zero out the scores. */
    for(j = 0; j < height; j++)
      for(i = 0; i < width; i++) {
        if(doplot)
          plot_point(i, j, strat[i][j]);
        act[i][j] = lastact[i][j] = 0;
        score[i][j] = 0.0;
      }

    /* For each round ... */
    for(k = 0; k < rounds; k++) {
      /* For every cell in the grid... */
      for(i = 0; i < width; i++)
        for(j = 0; j < height; j++) {
          /* Have this grid cell fight it out with its neighbors. */
          ipd(i, j, strat, act, lastact, score);
        }
      /* Remember what happened last round. */
      temp = act; act = lastact; lastact = temp;

    }
    
    /* Get ready to collect statistics. */
    for(j = 0; j < NUMSTRATS; j++)
      stratpop[j] = stratfit[j] = 0;    

    /* For every cell... */
    for(j = 0; j < height; j++)
      for(i = 0; i < width; i++) {

        /* Make the new strategy the best strategy... */
        if(mute == 0 || random_range(0, 1) > mute)
          newstrat[i][j] = beststrat(i, j, strat, score);
        /* ...or mutate it if appropriate. */
        else
          newstrat[i][j] = random() % NUMSTRATS;

        /* Collect statistics. */
        stratpop[strat[i][j]]++;
        stratfit[strat[i][j]] += score[i][j];
      }

    /* Pretty-print statistics, if appropriate. */
    if(stats) {
      fprintf(stderr, "----------------------------------------------\n");
      fprintf(stderr, "time %.8d :\taverage score\tpopulation\n", t + 1);
      fprintf(stderr, "----------------------------------------------\n");
      for(j = 0; j < NUMSTRATS; j++) {
        if(stratfit[j] > 0.0) stratfit[j] /= (stratpop[j] * rounds * 8);
        stratpop[j] /= (width * height);
        fprintf(stderr, "  %s %.6f\t%.6f\n", stratname[j], 
                stratfit[j], stratpop[j]);
      }
    }

    /* Make the new strategies the current strategies. */
    temp = strat; strat = newstrat; newstrat = temp;
  }
  
  if(doplot) plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
