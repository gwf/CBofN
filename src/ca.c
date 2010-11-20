
/* NAME
 *   ca - simulate arbitrary one-dimensional cellular automata
 * NOTES
 *   The main portion of this code is relatively straight forward,
 *   whereas the code to compute lambda (or random rules of a
 *   specified lambda) is a bit tricky and is solved by a method
 *   that resembles dynamic programming.
 *   
 *   Also, the states are stored in a circular buffer such that
 *   the sums for every cell are computed as fast as possible with
 *   the number of additions on the order of two per cell.
 * MISCELLANY
 *   When supplying a lambda value for a random rule, it may not
 *   be possible to find a string with that lambda value because
 *   one may not exist.  In this case, the program will do its
 *   best to find one as close as possible.  In any event, the
 *   algorithm for finding random rules strings for a specified
 *   lambda value is non-deterministic and may not always find
 *   a perfect match even if one exists.  However, it will work
 *   well with high probability, and even when it doesn't find
 *   a perfect match it almost always gets close.
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

double lambda = -1.0;
int width = 640, height = 480, states = 2, radius = 1, mag = 1;
int wrap = 1, seed = 0, binary = 0, invert = 0, sq = 1;
char *term = NULL, *rules = "0110", *init = "11";

char help_string[] = "\
Computes a one-dimensional cellular automata.  The evolution of the \
CA is determined by the number of states, the radius size, the initial \
state, and the supplied rule.  A rule is specified by a (states - 1) * \
(radius * 2 + 1) length string.  At each time step a sum of each cell \
plus all of its neighbors within the radius is computed.  That sum is \
used as an index into the rules string which determines the next step \
For example, with radius = 1 and states = 2 the rule \"0110\" specifies \
that sums of 0 and 3 map to the 0 state, and sums of 1 and 2 map to the \
1 state.  A negative init string randomly initializes the starting \
states. If the init string is \"-N\" then each cell has a 1 in N chance \
of being non-zero.\
";

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels." },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels." },
  { "-states", OPT_INT,     &states, "Number of CA states." },
  { "-radius", OPT_INT,     &radius, "Radius of CA neighborhood." },
  { "-seed",   OPT_INT,     &seed,   "Random seed." },
  { "-wrap",   OPT_SWITCH,  &wrap,   "Use a wrap-around space?" },
  { "-rules",  OPT_STRING,  &rules,  "CA rules to use." },
  { "-init",   OPT_STRING,  &init,   "Starting state (< 0 is random)." },
  { "-lambda", OPT_DOUBLE,  &lambda, "Lambda value for random rules." },
  { "-sq",     OPT_SWITCH,  &sq,     "Enforce strong quiescence?" },
  { "-bin",    OPT_SWITCH,  &binary, "Binary colors?" },
  { "-inv",    OPT_SWITCH,  &invert, "Invert all colors?" },
  { "-mag",    OPT_INT,     &mag,    "Magnification factor." },
  { "-term",   OPT_STRING,  &term,   "How to plot points." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* This function has two distinct modes of operation; thus, there are
   two ways of calling it.  The first way is with rules equal to a valid
   rules string (and, therefore, non-NULL).  In this case, the passed 
   lambda value is ignored and the rules string is analyzed to determine
   its lambda value.

   In the second mode, with rule equal to NULL, a random string is
   generated with a lambda value as close to the requested value
   as possible.

   In either case, both the states, radius, and sq arguments are used. */ 

char *dorules(int states, int radius, int sq, double *lambda, char *rules)
{
  int area = 2 * radius + 1;
  int len = (states - 1) * area + 1;
  int N = (int) pow(states, area);
  int **table, *bits;
  int i, j, k, sum, a, b, aval, bval, noimprove;
  double *vals, oldlambda, newlambda;

  /* Simple sanity check. */
  if(rules && strlen(rules) != len) {
    fprintf(stderr, "Rule length should be %d not %d\n", len, strlen(rules));
    exit(1);
  }

  /* Build table that contains the number of transitions to a particular
   * state.  More specifically, table[i][j] will contain the number of
   * possible ways in which a string of length (i + 1) can sum up to j
   * using any number for a state from 0 to (states - 1).  Thus, the
   * final row of table[area - 1] will contain the number of ways we
   * can sum up to the column for this area size.
   *
   * We compute this by exploiting the recurrence relationship
   *     table[i][j] = sum(k = 0 .. states - 1) table[i-1][j-k]
   * with any nonsense pair of [i-1][j-k] yielding zero.
   */
  table = xmalloc(sizeof(int *) * area);
  for(i = 0; i < area; i++)
    table[i] = xmalloc(sizeof(int) * len);
  /* Start with base condition. */
  for(i = 0; i < len; i++)
    table[0][i] = ((i < states) ? 1 : 0);
  /* Fill via recurrence relationship. */
  for(i = 1; i < area; i++)
    for(j = 0; j < len; j++) {
      sum = 0;
      for(k = 0; k < states; k++)
        if(j - k >= 0) sum += table[i-1][j-k];
      table[i][j] = sum;
    }

  /* Now turn the final row of the table into lambda units. */
  vals = xmalloc(sizeof(double) * len);
  for(i = 0; i < len; i++)
    vals[i] = table[area-1][i] / (double) N;

  /* If we fall into this statement, then we are in a query mode. */
  if(rules) {
    *lambda = 0;
    for(i = 0; i < len; i++) 
      if(rules[i] != '0')
        *lambda += vals[i];

    /* Clean up stuff that is no longer needed. */
    for(i = 0; i < area; i++)
      free(table[i]);
    free(table);
    free(vals);
/*MRM begin*/
#if __dest_os != __mac_os
    fprintf(stderr, "supplied rule = '%s'\n", rules);
    fprintf(stderr, "actual lambda = %f\n", *lambda);
#endif
/*MRM end*/
    /* Return the original rule, but also set *lambda to point to 
     * the real lambda value. */
    return rules;
  }

  /* Since we've made it this far into the code, it means that we need
   * randomly generated rules.
   */

  /* Make a random vector of bits. */
  bits = xmalloc(sizeof(int) * len);
  for(i = 0; i < len; i++)
    bits[i] = random() % 2;
  /* Optionally enforce strong quiescence. */
  if(sq) {
    bits[0] = 0;
    for(i = 1; i < states; i++)
      bits[i * area] = 1;
  }

  /* Calculate lambda for these bits.   Note that we are only
   * distinguishing quiescent from non-quiescent states at this point. 
   */
  oldlambda = 0;
  for(i = 0; i < len; i++)
    oldlambda += vals[i] * bits[i];

  /* Now, try to improve the lambda count for a bunch of steps. */
  noimprove = 0;
  while(1) {

    /* Pick two random bits to change and two new values */
    while(1) {
      a = random() % len;
      /* Make sure it doesn't violate strong quiescence. */
      if(sq && (a % area == 0)) continue;
      aval = random() % 2;
      break;
    }
    while(1) {
      b = random() % len;
      /* Make sure it doesn't violate strong quiescence. */
      if(sq && (b % area == 0)) continue;
      bval = random() % 2;
      break;
    }
    newlambda = oldlambda + (aval - bits[a]) * vals[a] +
      (bval - bits[b]) * vals[b];
    
    /* If this is better, then accept it. */
    if(fabs(newlambda - *lambda) < fabs(oldlambda - *lambda)) {
      oldlambda = newlambda;
      bits[a] = aval;
      bits[b] = bval;
      noimprove = 0;
    }
    else
      noimprove++;

    /* We've gone a long time without improving things, so quit. */
    if(noimprove == 1000) break;
  }

  /* Now that we have the bits, we can pick state transitions
   * and return the new rule. 
   */
  rules = xmalloc(sizeof(char) * (len + 1));
  rules[len] = 0;
  for(i = 0; i < len; i++)
    rules[i] = bits[i] ? ((random() % (states - 1)) + '1') : '0';
  if(sq)
    for(i = 1; i < states; i++)
      rules[i * area] = i + '0';

  for(i = 0; i < area; i++)
    free(table[i]);
  free(table);
  free(bits);
  free(vals);
/*MRM begin*/
#if __dest_os != __mac_os
	fprintf(stdout, "generated rules  = '%s'\n", rules);
	fprintf(stdout, "generated lambda = %f\n", oldlambda);
#endif
/*MRM end*/
  return rules;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  extern int plot_mag;
  extern int plot_inverse;
  char *old, *new, *swap;
  int i, j, len, sum, odds;

  get_options(argc, argv, options, help_string);

  plot_mag = mag;
  plot_inverse = invert;
  plot_init(width, height, binary ? 2 : states, term);
  srandom(seed);

  /* The extra 2 * radius + 2 cells are "buffers" that allow us to
   * compute the sums rapidly.
   */
  old = xmalloc((width + 2 * radius + 2) * sizeof(char));
  new = xmalloc((width + 2 * radius + 2) * sizeof(char));

  for(i = 0; i < width + 2 * radius + 2; i++)
    old[i] = new[i] = 0;

  /* Initialize the first state.  If negative, use random init state. */
  len = strlen(init);
  if(init[0] == '-') {
    odds = -atoi(init);
    for(i = radius + 1; i < width + radius + 1; i++)
      if(random() % odds == 0)
        old[i] = random() % (states - 1) + 1;
  }
  else
    for(i = (width - len) / 2 + radius + 1, j = 0; j < len; i++, j++)
      old[i] = init[j] - '0';
  
  /* Generate a random lambda rule if needed. */
  len = (states - 1) * (radius * 2 + 1) + 1;
  if(lambda >= 0.0 && lambda <= 1.0)
    rules = dorules(states, radius, sq, &lambda, NULL);
  else
    rules = dorules(states, radius, sq, &lambda, rules);

  /* The main loop.  All of the real work is done here. */
  for(i = 0; i < height; i++) {

    /* If wrapping, then the CA is really a ring.  Place the
     * right edge values on the buffer to the left, and the
     * left edge value on the buffer to the right.
     */
    if(wrap)
      for(j = 0; j < radius; j++) {
        old[j + 1] = old[width + 1 + j];
        old[width + radius + 1 + j] = old[radius + 1 + j];
      }

    /* Initialize the running sum. */
    for(j = 0, sum = 0; j < 2 * radius + 1; j++)
      sum += old[j];

    /* If we know the sum of the cell to the left then we just subtract
     * its left-most neighbor within radius and add our right-most
     * neighbor within radius to compute this cell's sum.
     */
    for(j = radius + 1; j < width + radius + 1; j++) {
      sum = sum + old[j + radius] - old[j - radius - 1];
      new[j] = rules[sum] - '0';
      plot_point(j - radius - 1, i, binary ? (old[j] ? 1 : 0) : old[j]);
    }
    swap = old; old = new; new = swap;
  }

  plot_finish();

  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

