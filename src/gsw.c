
/* NAME
 *   gsw - simulate a three species individual-based ecosystem
 * NOTES
 *   None.
 * MISCELLANY
 *   An interesting change to this code would involve making the
 *   the updates asynchronous, which would avoid some subtle deadlock
 *   conditions that can occur in how the critters move.  Moreover,
 *   the motion of the overall system would probably be more "lifelike".
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

int width = 100, height = 100, seed = 0, invert = 0, steps = 1000000;
int plants = 5000, herbs = 500, carns = 50, Ep = 15, Eh = 50, Ec = 200;
int pmin = 3, pmax = 8, Cc = 5, Ch = 5, Pt = 0, stats = 1, samp = -1;
int noext = 1, mag = 1, pfreq = 1;

#define EMPTY 0
#define PLANT 1
#define HERB  2
#define CARN  3
#define NEWPLANT 4

typedef struct CELL {
  unsigned type : 4;
  unsigned mark : 4;
  int energy;
} CELL;
CELL **world;

/* Array of offsets for neighbors in NW, N, NE, W, E, SW, S, SE order */
int neighs[8][2] = {
  {-1, -1 }, {-1,  0 }, {-1,  1 },
  { 0, -1 },            { 0,  1 },
  { 1, -1 }, { 1,  0 }, { 1,  1 }
};

char *term = NULL;
char help_string[] = "\
The time evolution of an individual-based three species predator-prey \
ecosystem is simulated according to the specified parameters.  The three \
species consist of plants, herbivores, and carnivores (grass, sheep, and \
wolves; hence the name GSW).  Updates are done synchronously, and each \
species has several parameters which can control the life cycle, from \
the ability to give birth, to the likelihood of starvation.  Population \
statistics of the three species can be calculated over a subset of the \
entire grid.\
";

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels." },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels." },
  { "-steps",  OPT_INT,     &steps,  "Number of simulated steps." },
  { "-seed",   OPT_INT,     &seed,   "Random seed for initial state." },
  { "-plants", OPT_INT,     &plants, "Initial number of plants." },
  { "-herbs",  OPT_INT,     &herbs,  "Initial number of herbivores." },
  { "-carns",  OPT_INT,     &carns,  "Initial number of carnivores." },
  { "-pmin",   OPT_INT,     &pmin,
    "Minimum plants in vicinity to make new plant." },
  { "-pmax",   OPT_INT,     &pmax,
    "Maximum allowed plants in vicinity to make new plant." },
  { "-Ep",     OPT_INT,     &Ep,     "Energy of plant." },
  { "-Eh",     OPT_INT,     &Eh,     "Energy of herbivore." },
  { "-Ec",     OPT_INT,     &Ec,     "Energy of carnivore." },
  { "-Ch",     OPT_INT,     &Ch,     "Step energy cost for herbivores." },
  { "-Cc",     OPT_INT,     &Cc,     "Step energy cost for carnivores." },
  { "-Pt",     OPT_INT,     &Pt,     "Number of steps to grow plant." },
  { "-samp",   OPT_INT,     &samp,   "Size of subsample statistaics." },
  { "-stats",  OPT_SWITCH,  &stats,  "Show statistics?" },
  { "-pfreq",  OPT_INT,     &pfreq,  "Plot frequency." },
  { "-noext",  OPT_SWITCH,  &noext,  "Prevent extinction?" },
  { "-inv",    OPT_SWITCH,  &invert, "Invert colors?" },
  { "-mag",    OPT_INT,     &mag,    "Magnification factor." },
  { "-term",   OPT_STRING,  &term,   "How to plot points." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void initialize_world(void)
{
  int i, j, k;

  /* Make an empty world. */
  world = xmalloc(sizeof(CELL *) * height);
  for(i = 0; i < height; i++) {
    world[i] = xmalloc(sizeof(CELL) * width);
    for(j = 0; j < width; j++) {
      world[i][j].type = EMPTY;
      world[i][j].mark = 0;
      world[i][j].energy = 0;
    }
  }

  /* Add a bunch of plants. */
  for(i = 0; i < plants; i ++) {
    do {
      j = random_range(0, height);
      k = random_range(0, width);
    } while(world[j][k].type != EMPTY);
    world[j][k].type = PLANT;
    world[j][k].energy = Pt;
  }

  /* Add some herbivores. */
  for(i = 0; i < herbs; i ++) {
    do {
      j = random_range(0, height);
      k = random_range(0, width);
    } while(world[j][k].type != EMPTY);
    world[j][k].type = HERB;
    world[j][k].energy = Eh;
  }

  /* Add some carnivores. */
  for(i = 0; i < carns; i ++) {
    do {
      j = random_range(0, height);
      k = random_range(0, width);
    } while(world[j][k].type != EMPTY);
    world[j][k].type = CARN;
    world[j][k].energy = Ec;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* FOR each empty cell, count the number of plants that surround it.
   if the count is greater than or equal to pmin, then make a new plant. */

void update_plants(void)
{
  int i, j, ni, nj, n;

  /* For every grid cell... */
  for(i = 0; i < height; i++)
    for(j = 0; j < width; j++) {

      /* Skip non-empty cells. */
      if(world[i][j].type != EMPTY) continue;
      n = 0;

      /* For all cells in the 3x3 neighborhood... */
      for(ni = -1; ni <= 1; ni++)
        for(nj = -1; nj <= 1; nj++)
          if(world[(i + ni + height) % height]
                  [(j + nj + width) % width].type == PLANT) n++;

      if(n >= pmin && n <= pmax && world[i][j].energy >= Pt) {
        /* Make sure that new plants are not counted. */
        world[i][j].type = NEWPLANT;
      }
    }

  /* For every cell... */
  for(i = 0; i < height; i++)
    for(j = 0; j < width; j++) {
      /* Reset new plants to plants. */
      if(world[i][j].type == NEWPLANT)
        world[i][j].type = PLANT;

      /* Use the energy field to count how long this empty cell
       * has been empty.
       */
      else if(world[i][j].type == EMPTY)
        world[i][j].energy++;
    }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* For each herbivore, decrement energy cost, and kill if empty.  Else, 
   take a random step towrd plant if possible (or to empty space if
   need be).  If energy greater than Eh, then make a baby. */

void update_herbs(void)
{
  int i, j, ni, nj, k, r, found;

  /* For every cell... */
  for(i = 0; i < height; i++)
    for(j = 0; j < width; j++) {
      /* Only continue for herbivores. */
      if(world[i][j].type != HERB || world[i][j].mark == 1) continue;
      world[i][j].mark = 1;

      /* Decrement energy cost and check for rigor mortis. */
      world[i][j].energy -= Ch;
      if(world[i][j].energy <= 0) {
        world[i][j].type = EMPTY;
        world[i][j].energy = 0;
        break;
      }

      /* Search for nearby plant. */
      found = 0; r = random_range(0, 8);
      for(k = 0; k < 8; k++, r = (r + 1) % 8) {
        ni = (i + neighs[r][0] + height) % height;
        nj = (j + neighs[r][1] + width) % width;
        if(world[ni][nj].type == PLANT) {
          found = 1; break;
        }
      }
      /* Move, and eat plant if found */
      if(found) {
        world[ni][nj].type = HERB;
        world[ni][nj].energy = world[i][j].energy + Ep;
        world[ni][nj].mark = 1;
        /* Make a baby if possible. */
        if(world[i][j].energy > Eh) {
          world[ni][nj].energy /= 2;
          world[i][j] = world[ni][nj];
        }
        else {
          world[i][j].type = EMPTY;
          world[i][j].energy = 0;
        }
      }

      /* No plant found, so just move. */
      else {
        r = random_range(0, 8);
        for(k = 0; k < 8; k++, r = (r + 1) % 8) {
          ni = (i + neighs[r][0] + height) % height;
          nj = (j + neighs[r][1] + width) % width;
          if(world[ni][nj].type == EMPTY) {
            world[ni][nj] = world[i][j];
            world[i][j].type = EMPTY;
            world[i][j].energy = 0;
            break;
          }
        }
      }
    }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* For each carnivore, decrement energy cost, and kill if empty.  Else, 
   take a random step towrd herbivore if possible (or to empty space if
   need be).  If energy greater than Ec, then make a baby. */

void update_carns(void)
{
  int i, j, ni, nj, k, r, found;

  /* For every cell... */
  for(i = 0; i < height; i++)
    for(j = 0; j < width; j++) {
      /* Only continue for carnivores. */
      if(world[i][j].type != CARN || world[i][j].mark == 1) continue;
      world[i][j].mark = 1;

      /* Decrement energy cost and check for rigor mortis. */
      world[i][j].energy -= Cc;
      if(world[i][j].energy <= 0) {
        world[i][j].type = EMPTY;
        world[i][j].energy = 0;
        break;
      }

      /* Search for nearby herbivore. */
      found = 0; r = random_range(0, 8);
      for(k = 0; k < 8; k++, r = (r + 1) % 8) {
        ni = (i + neighs[r][0] + height) % height;
        nj = (j + neighs[r][1] + width) % width;
        if(world[ni][nj].type == HERB) {
          found = 1; break;
        }
      }
      /* Move, and herbivore plant if found */
      if(found) {
        world[ni][nj].type = CARN;
        world[ni][nj].energy = world[i][j].energy + Eh;
        world[ni][nj].mark = 1;
        /* Make a baby if possible. */
        if(world[ni][nj].energy > Ec) {
          world[ni][nj].energy /= 2;
          world[i][j] = world[ni][nj];
        }
        else {
          world[i][j].type = EMPTY;
          world[i][j].energy = 0;
        }
      }

      /* No herbivore found, so just move. */
      else {
        r = random_range(0, 8);
        for(k = 0; k < 8; k++, r = (r + 1) % 8) {
          ni = (i + neighs[r][0] + height) % height;
          nj = (j + neighs[r][1] + width) % width;
          if(world[ni][nj].type == EMPTY || world[ni][nj].type == PLANT) {
            found = world[ni][nj].type;
            world[ni][nj] = world[i][j];
            world[i][j].type = found;
            if(found == EMPTY)
              world[i][j].energy = 0;
            break;
          }
        }
      }
    }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  extern int plot_mag;
  extern int plot_inverse;
  int t, i, j;

  get_options(argc, argv, options, help_string);

  plot_inverse = invert;
    plot_mag = mag;
  plot_init(width, height, 4, term);
  plot_set_all(0);

  srandom(seed);
  initialize_world();
  /* Make the sub sample size sane. */
  if(samp <= 0) samp = MIN(width, height);

  /* For each time step... */
  for(t = 0; t < steps; t++) {
    plants = herbs = carns = 0;

    /* For evey cell. */
    for(i = 0; i < height; i++)
      for(j = 0; j < width; j++) {
        world[i][j].mark = 0;
        if(t % pfreq == 0) plot_point(i, j, world[i][j].type);

        /* Count the life forms in the subsample grid. */
        if(i < samp && j < samp) {
          if(world[i][j].type == PLANT) plants++;
          else if(world[i][j].type == HERB) herbs++;
          else if(world[i][j].type == CARN) carns++;
        }
      }

    /* Do not allow extinctions, if appropriate. */
    if(noext) {
      if(herbs == 0) {
        i = random_range(0, height);
        j = random_range(0, width);
        world[i][j].type = HERB;
        world[i][j].energy = Eh;
        herbs = 1;
      }
      if(carns == 0) {
        i = random_range(0, height);
        j = random_range(0, width);
        world[i][j].type = CARN;
        world[i][j].energy = Ec;
        carns = 1;
      }
    }

    if(stats) fprintf(stderr, "%d\t%d\t%d\n", plants, herbs, carns);

    update_plants();
    update_herbs();
    update_carns();
  }

  plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

