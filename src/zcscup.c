
/* NAME
 *   zcscup - run a zeroth level classifier system on the cups problem
 * NOTES
 *   None.
 * MISCELLANY
 *   The file format for the specifications files must contain the
 *   width and height of the world followed by the details of the
 *   world in ASCII format, where '.' is an empty cell, 'O' is a wall,
 *   and 'F' represent a cup.  See any of the examples in the 'data'
 *   subdirectory of this distribution for more details.
 *   
 *   The log file, 'zcscup.log', contains one line per classifier with
 *   the condition string, the action string, and the strength of the
 *   classifier.  The classifiers are sorted so the the strongest
 *   classifiers are printed first.
 *   
 *   The ASCII output of the program shows the most recent, windowed
 *   average and the total average for the number of steps needed to
 *   find both cups.
 * HINTS
 *   See the author's book, "The Computational Beauty of Nature," for
 *   more details.
 * BUGS
 *   The simulation always restarts with the ZCS location at position
 *   (0, 4), which really only makes sense for the data file
 *   'data/cups1.txt'.  Hence, you may need to tailor your data files
 *   to work around this or tweak the source code to try a different
 *   problem variation.
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
#include "misc.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int width, height, avelen = 50;
int size = 100, steps = 100, seed = 1, mag = 10, invert = 1;
double sinit = 20, lrate = 0.2, drate = 0.71, trate = 0.1, crate = 0.1;
double mrate = 0.002, grate = 0.25, cover = 0.5, wild = 0.33;
char *term = NULL, *specs = "data/cup1.txt";

char help_string[] = "\
Train a zeroth level classifier system (ZCS) to solve the cups problem \
with the implicit bucket brigade algorithm and a genetic algorithm. \
Solving this problem requires the ZCS to learn to remember important \
features from previous states, which makes this problem very challenging. \
The ZCS always starts in the same initial position. It interacts with its \
environment until it finds both cups, which (only at that point) \
yields a reward.  The simulation then restarts with the ZCS placed at \
the original location.  The progress of the ZCS is continuously \
plotted, while the statistics on the time to find both cups are calculated \
and displayed.  At the end of the simulation the classifiers that \
make up the final ZCS are saved to a log file. \
";

OPTION options[] = {
  { "-specs",  OPT_STRING,  &specs,  "World specification file." },
  { "-steps",  OPT_INT,     &steps,  "Number of simulated trials." },
  { "-seed",   OPT_INT,     &seed,   "Random seed for initial state." },
  { "-size",   OPT_INT,     &size,   "Population size." },
  { "-sinit",  OPT_DOUBLE,  &sinit,  "Initial classifier strength." },
  { "-lrate",  OPT_DOUBLE,  &lrate,  "BB learning rate." },
  { "-drate",  OPT_DOUBLE,  &drate,  "BB discount rate." },
  { "-trate",  OPT_DOUBLE,  &trate,  "Tax rate for strength reduce." },
  { "-crate",  OPT_DOUBLE,  &crate,  "GA crossover rate." },
  { "-mrate",  OPT_DOUBLE,  &mrate,  "GA mutation rate." },
  { "-grate",  OPT_DOUBLE,  &grate,  "GA invocation rate." },
  { "-cover",  OPT_DOUBLE,  &cover,  "Covering factor." },
  { "-wild",   OPT_DOUBLE,  &wild,   "Probability of # in cover." },
  { "-avelen", OPT_INT,     &avelen, "Length of windowed average." },
  { "-inv",    OPT_SWITCH,  &invert, "Invert all colors?" },
  { "-mag",    OPT_INT,     &mag,    "Magnification factor." },
  { "-term",   OPT_STRING,  &term,   "How to plot points." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* cup = 1, empty = 0 */

/* These define how the objects are represented internally and
   also the color that objects are drawn in. */

#define EMPTY  0
#define CUP    1
#define WALL   2
#define ME     3
#define MECUP  4

/* How much to reward the CS when food is found. */

#define REWARD 1000


/* Macros for handy conversions. */

#define CHAR2NUM(c) (c == 'F' ? CUP : c == 'O' ? WALL : EMPTY)
#define NUM2BIN(n) (n == CUP ? '1' : '0')


/* The length of a classifier and the action bit strings. */

#define CLEN (4+1)
#define ALEN (2+1)


/* A type declaration for a single classifier: */

typedef struct CLASSIFIER {
  double str;         /* Strength. */
  char cond[CLEN+1];  /* Condition. */
  char act[ALEN+1];   /* Action. */
} CLASSIFIER;


/* A list structure for classifiers. */

typedef struct CLIST {
  CLASSIFIER *class;
  struct CLIST *next;
} CLIST;

/* Globals to minimize parameter passing: an array of classifiers,
   A two-dimensional array to hold the state of the world, our
   ZCS's (x, y) coordinates, the collision sensor bits, the ZCS,
   register, and the number of cups. */

CLASSIFIER *pop;
char **world, **origworld;
int me_w, me_h;
int col_l, col_r, reg1, cups;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* A function to compare classifiers based on strength. */

int classcomp(const void *a, const void *b)
{
  const CLASSIFIER *ac = a, *bc = b;
  
  return(ac->str < bc->str ? 1 : ac->str > bc->str ? -1 : 0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Read in the world specification and initialize memory. */

void initialize(void)
{
  FILE *fp;
  SCANNER *scan;
  char *str;
  int i, j;

  /* Read world specifications. */

  if(specs == NULL || (fp = fopen(specs, "r")) == NULL) {
    fprintf(stderr, "Cannot open specs file \"%s\".\n", specs);
    exit(1);
  }

  /* The characters 'F', 'O', and '.' are tokens.  '#' is for comments. */
  scan = scan_init(fp, "FO.", " \t\n", "#");

  /* Width and height are read in first. */
  if((str = scan_get(scan)) == NULL) goto BADFILE;
  width = atoi(str);
  if((str = scan_get(scan)) == NULL) goto BADFILE;
  height = atoi(str);
  
  /* Make space for the world.   Save the inital world, because
   * we will need to restore it later.
   */
  world = xmalloc(sizeof(char *) * height);
  origworld = xmalloc(sizeof(char *) * height);
  for(i = 0; i < height; i++) {
    world[i] = xmalloc(sizeof(char) * width);
    origworld[i] = xmalloc(sizeof(char) * width);
    for(j = 0; j < width; j++) {
      if((str = scan_get(scan)) == NULL) goto BADFILE;
      world[i][j] = origworld[i][j] = CHAR2NUM(*str);
    }
  }

  /* Initially place the ZCS in the center.  Note that this is
   * hard-coded with respect to the data file.
   */
  me_h = 0;  me_w = 4;
  world[me_h][me_w] = ME;
  col_l = col_r = reg1 = cups = 0;

  /* Initialize classifier system. */

  pop = xmalloc(size * sizeof(CLASSIFIER));
  for(i = 0; i < size; i++) {
    pop[i].str = sinit;
    for(j = 0; j < CLEN; j++) {
      pop[i].cond[j] = '0' + (random() % 3);
      pop[i].cond[j] = (pop[i].cond[j] == '2') ? '#' : pop[i].cond[j];
    }
    pop[i].cond[CLEN] = 0;
    for(j = 0; j < ALEN; j++)
      pop[i].act[j] = '0' + (random() % 2);
    pop[i].act[ALEN] = 0;
  }

  return;
BADFILE:
  fprintf(stderr, "Problem found in specs file.\n");
  exit(1);  
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Redraw everything. */

void draw_world(void)
{
  int i, j;

  for(i = 0; i < height; i++)
    for(j = 0; j < width; j++)
      plot_point(j, i, world[i][j]);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Restart the simulation after both cups have been found. */

void restart(void)
{
  int i, j;

  for(i = 0; i < height; i++)
    for(j = 0; j < width; j++)
      world[i][j] = origworld[i][j];

  me_h = 0;  me_w = 4;
  world[me_h][me_w] = ME;
  col_l = col_r = reg1 = cups = 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Builds a new list from a new front element and a sublist. */

CLIST *cons(CLASSIFIER *class, CLIST *list)
{
  CLIST *new;
  
  new = xmalloc(sizeof(CLIST));
  new->class = class;
  new->next = list;
  return(new);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Destroy a list and free up memory. */

void delete(CLIST *list)
{
  CLIST *next;

  while(list) {
    next = list->next;
    free(list);
    list = next;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Encode the ZCS's environment into a string. */

void environment(char *env)
{
  int x;

  /* The environment consists of the cells to our left and right,
   * the colision sensors, and the value of the register.
   */
  x = (me_w + width - 1) % width;
  env[0] = NUM2BIN(world[me_h][x]);
  x = (me_w + width + 1) % width;
  env[1] = NUM2BIN(world[me_h][x]);
  env[2] = col_l + '0';
  env[3] = col_r + '0';
  env[4] = reg1 + '0';
  env[CLEN] = 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Checks if two strings are "equal" considering while card characters. */

int condeq(char *cond, char *env)
{
  int i;

  for(i = 0; i < CLEN; i++)
    if(cond[i] != '#' && cond[i] != env[i]) return(0);
  return(1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Steps through the conditions of all classifiers and forms a match
   list of all classifiers that match the environment. */

CLIST *matchlist(char *env)
{
  int i;
  CLIST *mlist = NULL;

  for(i = 0; i < size; i++)
    if(condeq(pop[i].cond, env))
      mlist = cons(&pop[i], mlist);
  return(mlist);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Performs random roulette selection based on the normalized strengths
   of the classifiers. */

int picklarge(int skip)
{
  int i;
  double x, sum, runsum;

  /* Calculate the sum of strengths of everything but the
   * skip'th classifier.
   */
  sum = 0;
  for(i = 0; i < size; i++)
    if(i != skip) sum += pop[i].str;
  runsum = 0;
  x = random_range(0, 1);
  for(i = 0; i < size; i++) {
    if(i == skip) continue;
    runsum += pop[i].str / sum;

    /* Accept a choice based on cumulative sum of strengths (which
     * should be equal to 1 if done over all strings).
     */
    if(x <= runsum)
      return(i);
  }
  /* Just in case there was a subtle numerical error. */
  return(size - 1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Performs random roulette selection based on the normalized inverse
   strengths of the classifiers. */

int picksmall(int skip)
{
  int i;
  double x, sum, runsum;

  /* Calculate the sum of inverse strengths of everything but the
   * skip'th classifier.
   */
  sum = 0;
  for(i = 0; i < size; i++)
    if(i != skip) sum += 1 / pop[i].str;
  runsum = 0;
  x = random_range(0, 1);
  for(i = 0; i < size; i++) {
    if(i == skip) continue;
    runsum += (1 / pop[i].str) / sum;

    /* Accept a choice based on cumulative sum of inverse strengths
     * (which should be equal to 1 if done over all strings).
     */
    if(x <= runsum)
      return(i);
  }
  /* Just in case there was a subtle numerical error. */
  return(size - 1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Optionally perform covering, which entails building a new
   classifier if none match the current environment. */

CLIST *covering(CLIST *mlist, char *env)
{
  CLIST *l;
  int i, replace;
  double str = 0, mean = 0;

  /* Get the total strength of the matchlist.  Note that this will
   * be zero if the match list is empty.
   */
  for(l = mlist; l != NULL; l = l->next)
    str += l->class->str;

  /* Get the average strength of all classifiers. */
  for(i = 0; i < size; i++)
    mean += pop[i].str;
  mean /= size;

  /* Check first bailout condition: total strength is greater
   * than mean of math list times cover constant.
   */
  if(str > (mean * cover)) return(mlist);

  /* Pick something very weak from all classifiers. */
  replace = picksmall(-1);
  /* Copy in the current environment. */
  strcpy(pop[replace].cond, env);
  /* Sprinkle in some wildcards. */
  for(i = 0; i < CLEN; i++)
    if(random_range(0, 1) < wild)
      pop[replace].cond[i] = '#';

  /* Set the action of the new classifier to some random string. */
  for(i = 0; i < ALEN; i++)
    pop[replace].act[i] = '0' + (random() % 2);

  /* Give it the mean fitness. */
  pop[replace].str = mean;
  return(cons(&pop[replace], mlist));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Compute an action list from a match list. */

CLIST *actlist(CLIST *mlist)
{
  CLIST *l, *alist = NULL;
  CLASSIFIER *pick = NULL;
  double sum, runsum, x;
  
  /* Get the sum of strengths of the match list. */
  sum = 0;
  for(l = mlist; l != NULL; l = l->next)
    sum += l->class->str;

  /* Do random roulette selection based on strengths. */
  runsum = 0;
  x = random_range(0, 1);
  for(l = mlist; l != NULL; l = l->next) {
    runsum += l->class->str / sum;

    /* Accept a choice based on cumulative sum of strengths (which
     * should be equal to 1 if done over all strings).
     */
    if(x <= runsum) {
      pick = l->class;
      break;
    }
  }

  /* If (l == NULL), then the above loop hit a strange statistical
   * burp.  Pick the first thing in the match list to fix. 
   */
  if(l == NULL) pick = mlist->class;

  /* Form a list of every member of the match list that advocates
   * the same action and put them into an action list.
   */
  for(l = mlist; l != NULL; l = l->next)
    if(strcmp(pick->act, l->class->act) == 0)
      alist = cons(l->class, alist);
  return(alist);  
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Move based on the action string: 00 = nothing, 01 = right,
   10 = left, 11 = pick.  Also update the colision and register
   bits. */

int move(char *act)
{
  col_l = col_r = 0;
  reg1 = act[2] - '0';

  /* Move right. */
  if(act[0] == '0' && act[1] == '1') {
    /* Move right okay. */
    if(world[me_h][me_w + 1] != WALL) {
      /* Land on a cup. */
      if(world[me_h][me_w + 1] == CUP) {
        world[me_h][me_w + 1] = MECUP;
        world[me_h][me_w] = EMPTY;
        plot_point(me_w, me_h, EMPTY);
        plot_point(me_w + 1, me_h, MECUP);
      }
      else {
        world[me_h][me_w + 1] = ME;
        plot_point(me_w + 1, me_h, ME);
        /* Redraw a cup that was not picked. */
        if(world[me_h][me_w] == MECUP) {
          world[me_h][me_w] = CUP;
          plot_point(me_w, me_h, CUP);
        }
        else {
          world[me_h][me_w] = EMPTY;
          plot_point(me_w, me_h, EMPTY);
        }
      }
      me_w++;
    }
    /* Hit a wall. */
    else
      col_r = 1;
  }
  /* Move left. */
  else if(act[0] == '1' && act[1] == '0') {
    /* Move left okay. */
    if(world[me_h][me_w - 1] != WALL) {
      /* Land on a cup. */
      if(world[me_h][me_w - 1] == CUP) {
        world[me_h][me_w - 1] = MECUP;
        world[me_h][me_w] = EMPTY;
        plot_point(me_w, me_h, EMPTY);
        plot_point(me_w - 1, me_h, MECUP);
      }      
      else {
        world[me_h][me_w - 1] = ME;
        plot_point(me_w - 1, me_h, ME);
        /* Redraw a cup that was not picked. */
        if(world[me_h][me_w] == MECUP) {
          world[me_h][me_w] = CUP;
          plot_point(me_w, me_h, CUP);
        }
        else {
          world[me_h][me_w] = EMPTY;
          plot_point(me_w, me_h, EMPTY);
        }
      }
      me_w--;
    }
    /* Hit a wall. */
    else
      col_l = 1;
  }
  /* Pickup. */
  else if(act[0] == '1' && act[1] == '1') {
    /* Cup is there, so get it. */
    if(world[me_h][me_w] == MECUP) {
      cups++;
      world[me_h][me_w] = ME;
      plot_point(me_w, me_h, ME);
    }
  }

  /* Only reward when both cups are found. */
  return((cups == 2) ? REWARD : 0);
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Perform one step of the implicit bucket brigade. */

void update(int reward, CLIST *mlist, CLIST *alist, CLIST *alistold)
{
  int sz = 0;
  double hold = 0;
  CLIST *l;

  /* Sum of the hold amount, decay the strengths of the
   * classifiers in the action list, and count the size.
   */
  for(l = alist; l != NULL; l = l->next) {
    hold += lrate * l->class->str;
    l->class->str -= lrate * l->class->str;
    sz++;
  }

  /* Pass out rewards to the action list. */
  for(l = alist; l != NULL; l = l->next)
    l->class->str += lrate * reward / sz;

  /* Share the wealth with the previous action list. */
  if(alistold) {
    sz = 0;
    for(l = alistold; l != NULL; l = l->next)
      sz++;
    for(l = alistold; l != NULL; l = l->next)
      l->class->str += drate * hold / sz;
  }

  /* Tax all classifiers in the match list that advocated a
   * different action.
   */
  for(l = mlist; l != NULL; l = l->next)
    if(strcmp(l->class->act, alist->class->act) != 0) {
      l->class->str -= trate * l->class->str;
    }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Do one step of the GA to weed out the weaklings. */

void ga(void)
{
  int pa, pb, oa, ob;
  int i, cindex, t;
  double ave;

  /* Pick two parents by strength that are guaranteed to be different. */
  pa = picklarge(-1);
  pb = picklarge(pa);

  /* Pick two classifiers by inverse strength that are guaranteed to be
   * different.
   */
  oa = picksmall(-1);
  ob = picksmall(oa);

  /* Halve the strength of the parents and pass on to
   * the children.
   */
  pop[pa].str /= 2;
  pop[oa] = pop[pa];
  pop[pb].str /= 2;
  pop[ob] = pop[pb];

  /* Optionally cross the two children. */
  if(random_range(0, 1) < crate) {
    /* Do crossover on the condition. */
    cindex = (random() % CLEN) + 1;
    for(i = 0; i < cindex; i++) {
      t = pop[oa].cond[i];
      pop[oa].cond[i] = pop[ob].cond[i];
      pop[ob].cond[i] = t;
    }
    /* Do crossover on the action. */
    cindex = (random() % ALEN) + 1;
    for(i = 0; i < cindex; i++) {
      t = pop[oa].act[i];
      pop[oa].act[i] = pop[ob].act[i];
      pop[ob].act[i] = t;
    }
    /* Blur the strengths. */
    ave = (pop[oa].str + pop[ob].str) / 2;
    pop[oa].str = pop[ob].str = ave;
  }

  /* Optionally mutate condition. */
  for(i = 0; i < CLEN; i++) {
    if(random_range(0, 1) < mrate) {
      pop[oa].cond[i] = '0' + (random() % 3);
      pop[oa].cond[i] = (pop[oa].cond[i] == '2') ? '#' : pop[oa].cond[i];
    }
    if(random_range(0, 1) < mrate) {
      pop[ob].cond[i] = '0' + (random() % 3);
      pop[ob].cond[i] = (pop[ob].cond[i] == '2') ? '#' : pop[ob].cond[i];
    }
  }
  /* Optionally mutate action. */
  for(i = 0; i < ALEN; i++) {
    if(random_range(0, 1) < mrate)
      pop[oa].act[i] = '0' + (random() % 2);
    if(random_range(0, 1) < mrate)
      pop[ob].act[i] = '0' + (random() % 2);
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  FILE *fp;
  extern int plot_mag;
  extern int plot_inverse;
  CLIST *mlist, *alist, *alistold;
  int t, reward, cnt, *counts, i;
  double ave = 0, totcount = 0;
  char env[CLEN + 1];

  get_options(argc, argv, options, help_string);
  srandom(seed);
  initialize();

  counts = xmalloc(sizeof(int) * avelen);

  plot_inverse = invert;
  plot_mag = mag;
  plot_init(width, height, 5, term);
  plot_set_all(0);

  mlist = alist = alistold = NULL;

  /* For each time step... */
  for(t = 0; t < steps; t++) {
    reward = cnt = 0;
    draw_world();
    /* Keep going until some a reward is earned. */
    while(reward == 0) {
      /* Build an environment string. */
      environment(env);
      /* Form the match list. */
      mlist = matchlist(env);
      /* Do covering. */
      mlist = covering(mlist, env);
      /* Make the action list. */
      alist = actlist(mlist);
      /* Move the little guy. */
      reward = move(alist->class->act);
      /* Do implicit BB. */
      update(reward, mlist, alist, alistold);
      /* Optionally perform GA step. */ 
      if(random_range(0, 1) < grate && t > 0) ga();
      /* Clean up excess baggage. */
      delete(mlist);
      delete(alistold);
      alistold = alist;
      cnt++;
    }
    /* Clean up excess baggage. */
    delete(alistold); alistold = NULL;

    /* Take a windowed moving average of the number of steps
     * needed to complete the previous trials.
     */
    if(t >= avelen)
      ave = ((ave * avelen) - counts[t % avelen] + cnt) / avelen;

    counts[t % avelen] = cnt;
    totcount += cnt;

    /* If we've completed enough trials, calculate the average. */
    if(t == avelen - 1) {
      for(i = 0; i < avelen; i++)
        ave += counts[i];
      ave /= avelen;
    }

    /* Dump out stats. */
    if(t >= avelen)
      printf("%d\t%f\t%f\n", cnt, ave, totcount / (t + 1));

    /* Restart in a new position. */
    restart();
  }

  /* Simulation is complete, so print out classifiers to log file. */
  if((fp = fopen("zcscup.log", "w")) != NULL) {
    qsort(pop, size, sizeof(CLASSIFIER), classcomp);
    for(i = 0; i < size; i++)
      fprintf(fp, "%s : %s : %.5f\n", pop[i].cond, pop[i].act, pop[i].str);
  }

  plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
