
/* NAME
 *   mrcm - generate fractals with the MRCM algorithm
 * NOTES
 *   This code was written before the plotting interface had
 *   auto-scaling builtin to it.  Thus, the implementation could be
 *   made a bit simpler by exploiting it, but I decided not to
 *   bother fixing what already worked.
 * MISCELLANY
 *   There is a shell script called 'ifscma' supplied with the source
 *   code that has a simple interface that allows you to reference
 *   sets of predefined rules by name.  You may also wish to see the
 *   author's book, "The Computational Beauty of Nature," for
 *   some more examples.
 * BUGS
 *   No sanity checks are performed to make sure that any of the
 *   options make sense.
 *   
 *   The maximum number of affine rules is determined by the
 *   constant MAXRULES, which you may need to adjust under exceptional
 *   circumstances.
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

#define POINTS   7
#define MAXRULES 256

double a[MAXRULES], b[MAXRULES], c[MAXRULES];
double d[MAXRULES], e[MAXRULES], f[MAXRULES];
double **x, **y, bw = 1.0, bh = 1.0;
int depth = 5, border = 10, width = 640, height = 480, L = 0;
int boxwidth, xoff, yoff, rules, invert = 0, mag = 1;
char *term = NULL, *infile = "-";

char help_string[] =  "\
An affine fractal is computed via the Multiple Reduction Copy Machine \
Algorithm.  The rules must be supplied from a file with each line \
consisting of six values (A-F) such that the values of A-D describe \
a 2x2 matrix (A, B; C, D), while E and F describes a 2x1 column vector \
(E; F).\
";

OPTION options[] = {
  { "-infile", OPT_STRING,  &infile, "Data input file."                  },
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels."      },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels."     },
  { "-depth",  OPT_INT,     &depth,  "Depth of recursive calls."         },
  { "-border", OPT_INT,     &border, "Number of pixels in border."       },
  { "-bw",     OPT_DOUBLE,  &bw,     "Width of the seed box."            },
  { "-bh",     OPT_DOUBLE,  &bh,     "Height of the seed box."           },
  { "-L",      OPT_SWITCH,  &L,      "Draw an 'L' in each box?"          },
  { "-term",   OPT_STRING,  &term,   "How to plot points."               },
  { "-inv",    OPT_SWITCH,  &invert, "Invert colors?" },
  { "-mag",    OPT_INT,     &mag,    "Magnification factor." },
  { NULL,      OPT_NULL,    NULL,    NULL                                }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Load the affines linear rules from the named file.  If fname is
   equal to "-", then read from stdin. */

void load_rules(char *fname)
{
  SCANNER *scan;
  int check;
  char *s;
  FILE *fp;

  /* Open the appropriate file. */
  if(strcmp(fname, "-") == 0)
    fp = stdin;
  else {
    if((fp = fopen(fname, "r")) == NULL) {
      fprintf(stderr, "Unable to open input file \"%s\".\n", fname);
      exit(1);
    }
  }

  /* Create a scanner that skips white space and treats '#' as a 
   * comment character.
   */
  scan = scan_init(fp, "", " \t", "#");
  s = scan_peek(scan);
  rules = check = 0;

  /* Read numbers until EOF. */
  while(*s) {
    if(rules >= MAXRULES) {
      fprintf(stderr, "Maximum number of rules (%d) exceeded.\n", MAXRULES);
      exit(1);
    }
    check++;
    s = scan_get(scan); if(!*s) break; a[rules] = atof(s);
    s = scan_get(scan); if(!*s) break; b[rules] = atof(s);
    s = scan_get(scan); if(!*s) break; c[rules] = atof(s);
    s = scan_get(scan); if(!*s) break; d[rules] = atof(s);
    s = scan_get(scan); if(!*s) break; e[rules] = atof(s);
    s = scan_get(scan); if(!*s) break; f[rules] = atof(s);
    s = scan_peek(scan);
    rules++;
  }
  if(rules == 0)
    exit(0);

  /* This checks to make sure that a multiple of six numbers has
   * been read in (six to define a 2x2 matrix and a 2x1 vector).
   */
  if(rules != check) {
    fprintf(stderr, "Problem with input data.\n");
    exit(1);
  }
  if(fp != stdin) fclose(fp);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void compute_figure(int level)
{
  int i, j, ax, ay, bx, by;

  /* If this is not the bottom-most recursive level... */
  if(level < depth + 1) {
    /* For each rule... */
    for(i = 0; i < rules; i++) {
      /* For each point in the seed image ... */
      for(j = 0; j < POINTS; j++) {
        /* ... transform the point in the seed image by the proper
         * affine rule.
         */
        x[level][j] = a[i] * x[level-1][j] + b[i] * y[level-1][j] + e[i];
        y[level][j] = c[i] * x[level-1][j] + d[i] * y[level-1][j] + f[i];
      }
      /* Descend to the next recursion level. */
      compute_figure(level + 1);
    }
  }
  /* This is the bottom-most recursion level, so plot the points. */
  else {
    /* For all points in the seed image except the 'L' ... */
    for(i = 0; i < POINTS - 3; i++) {
      /* ... calculate two endpoints and draw the line. */
      ax = x[level-1][i] * (boxwidth - 1) + xoff + 0.5;
      ay = height - (int)(y[level-1][i] * (boxwidth - 1) + yoff + 0.5);
      bx = x[level-1][(i + 1) % (POINTS - 3)] * (boxwidth - 1) + xoff + 0.5;
      by = height - (int)(y[level-1][(i + 1) % (POINTS - 3)] *
                          (boxwidth - 1) + yoff + 0.5);
      plot_line(ax, ay, bx, by, 1);
    }
    if(L)
      /* If we should plot the 'L' as well, then do it. */
      for(i = 4; i < POINTS - 1; i++) {
        /* ... calculate two endpoints and draw the line. */
        ax = x[level-1][i] * (boxwidth - 1) + xoff + 0.5;
        ay = height - (int)(y[level-1][i] * (boxwidth - 1) + yoff + 0.5);
        bx = x[level-1][i + 1] * (boxwidth - 1) + xoff + 0.5;
        by = height - (int)(y[level-1][i + 1] * (boxwidth - 1) + yoff + 0.5);
        plot_line(ax, ay, bx, by, 1);
      }
  } 
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{ 
  extern int plot_mag;
  extern int plot_inverse;
  int i;

  get_options(argc, argv, options, help_string);

  load_rules(infile);

  plot_mag = mag;
  plot_inverse = invert;
  plot_init(width, height, 2, term);
  plot_set_all(0);
  
  /* Get space to hold the tranformations to the requested depth. */
  x = xmalloc((depth + 1) * sizeof(double *));
  y = xmalloc((depth + 1) * sizeof(double *));
  for(i = 0; i < depth + 1; i++) {
    x[i] = xmalloc(POINTS * sizeof(double));
    y[i] = xmalloc(POINTS * sizeof(double));
  }
  
  boxwidth = MIN(width, height) - 2 * border;
  xoff = (width - boxwidth) / 2;
  yoff = (height - boxwidth) / 2;

  /* Define the points for a simple box. */
  x[0][0] = (1 + bw) / 2; y[0][0] = (1 - bh) / 2;
  x[0][1] = (1 - bw) / 2; y[0][1] = (1 - bh) / 2;
  x[0][2] = (1 - bw) / 2; y[0][2] = (1 + bh) / 2;
  x[0][3] = (1 + bw) / 2; y[0][3] = (1 + bh) / 2;

  /* Define the points for an 'L' in the box. */
  x[0][4] = 0.5; y[0][4] = y[0][0] + bh * 0.1;
  x[0][5] = x[0][1] + bh * 0.1; y[0][5] = y[0][4];
  x[0][6] = x[0][5]; y[0][6] = y[0][2] - bh * 0.1;

  /* Shazam. */
  compute_figure(1);
  plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
