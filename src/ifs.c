
/* NAME
 *   ifs - generate fractals with an IFS
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

#define MAXRULES 256

double a[MAXRULES], b[MAXRULES], c[MAXRULES];
double d[MAXRULES], e[MAXRULES], f[MAXRULES], p[MAXRULES];
int border = 10, width = 640, height = 480, skip = 50, its = 1000;
int xoff, yoff, rules, boxwidth, invert = 0, mag = 1;
char *term = NULL, *infile = "-";

char help_string[] =  "\
An affine fractal is computed via an Iterated Functional System. \
The rules must be supplied from a file with each line \
consisting of six values (A-F) such that the values of A-D describe \
a 2x2 matrix (A, B; C, D), while E and F describes a 2x1 column vector \
(E; F).\
";


OPTION options[] = {
  { "-infile", OPT_STRING,  &infile, "Data input file."                  },
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels."      },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels."     },
  { "-border", OPT_INT,     &border, "Number of pixels in border."       },
  { "-its",    OPT_INT,     &its,    "Number of iterations."             },
  { "-skip",   OPT_INT,     &skip,   "Number of iteration to skip."      },
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
  double sum;
  int check, i;
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

  /* Make the probabilities for selecting a rule proportional
   * to the determininant of the rule's matrix.
   */
  sum = 0.0;
  for(i = 0; i < rules; i++) {
    p[i] = a[i] * d[i] - b[i] * c[i];
    p[i] = ABS(p[i]);
    /* Give all rules at least a small chance. */
    if(p[i] < 0.01)
      p[i] = 0.01;
    sum += p[i];
  }
  for(i = 0; i < rules; i++)
    p[i] /= sum;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{ 
  extern int plot_mag;
  extern int plot_inverse;
  double x, y, t, r, s;
  int i, j, ax, ay;

  get_options(argc, argv, options, help_string);

  load_rules(infile);

  plot_mag = mag;
  plot_inverse = invert;
  plot_init(width, height, 2, term);
  plot_set_all(0);

  boxwidth = MIN(width, height) - 2 * border;
  xoff = (width - boxwidth) / 2;
  yoff = (height - boxwidth) / 2;

  x = random_range(0.0, 1.0);
  y = random_range(0.0, 1.0);
  i = 0;

  /* Loop for the total iterations plus the skip count ... */
  while(i < (its + skip)) {
    
    /* Pick a rule with a roulette-like choice. */
    j = 0;
    s = p[j];
    r = random_range(0.0, 1.0);
    while(s < r) {
      j++;
      s += p[j];
    }

    /* Tranform the point via the affine transformation. */
    t = a[j] * x + b[j] * y + e[j];
    y = c[j] * x + d[j] * y + f[j];
    x = t;

    /* Plot the point if we've skipped enough. */
    if(i >= skip) {
      ax = x * (boxwidth - 1) + xoff + 0.5;
      ay = height - (int)(y * (boxwidth - 1) + yoff + 0.5);
      plot_point(ax, ay, 1);
    }
    i++;
  }

  plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
