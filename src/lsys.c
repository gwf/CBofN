
/* NAME
 *   lsys - builds an L-system fractal from multiple rules
 * NOTES
 *   None.
 * RULES
 *   The L-system rules should always take the form "x=..." where
 *   "x" is a letter and the remaining portion is a sequence of
 *   turtle graphics commands, which operate as described below.
 *   
 *   \'F\' is the draw forward command, which moves the plotter
 *   forward by a fixed length, drawing a line from the old
 *   position to the new position.
 *   
 *   \'G\' (Go Forward) is similar to \'F\', but only moves the
 *   plotter and does not draw a line.
 *   
 *   \'+\' and \'-\' turn the plotter to the right (or left) by a
 *   fixed angle determined by the -da switch.  If an integer
 *   precedes the \'+\' or \'-\' symbol, then the plotter
 *   effectively makes that number of right-hand turns.
 *   
 *   \'[\' saves the current position and angle of the plotter
 *   for later use, which is restored by the \']\' command.
 *   
 *   \'|\' move the turtle forward by a length computed from
 *   the execution depth, drawing a line from the old position
 *   to the new.  Thus, this command is similar to \'F\' but it
 *   is not recursively expanded.  If DS is the value supplied
 *   by the \fB-ds\fP switch, then the step size will be proportional
 *   to DS^depth.  This allows for certain types of figures to
 *   be drawn that would otherwise be difficult to express
 *   without this command.
 * EXAMPLES
 *   .IP A\ weed:
 *   \fBlsys -da 20 -rule "F=|[-F]|[+F][-F]F"\fP
 *   .IP Kock\ curve:
 *   \fBlsys -da 60 -rule F=F-F++F-F\fP
 *   .IP Penrose\ Snowflake:
 *   \fBlsys -da 18 -axiom F4-F4-F4-F4-F -rule F=F4-F4-F10-F++F4-F\fP
 *   .IP Sierpinski\ Arrowhead:
 *   \fBlsys -da 60 -rule "F=[-G+++F][-G+F][GG--F]" -rule G=GG\fP
 *   .LP
 *   See the author's book, "The Computational Beauty of Nature," for
 *   more examples.
 * MISCELLANY
 *   If you are using a UNIX command shell, be sure to quote any rules
 *   with brackets so that the shell passes them to the program correctly.
 * BUGS
 *   No sanity checks are performed to make sure that any of the
 *   options make sense.
 *   
 *   The border size is only approximately calculated because the
 *   author felt a bit lazy at the time of writing this program.
 *   However, it's pretty close for most values.
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

char help_string[] =  "\
An L-system is computed according to the specified axiom, rules, \
and step angle.  The L-system axiom is recursively expanded for the \
specified depth, with 'F', 'G', and '|' actions resulting in movement \
of the virtual plotter, and '+' and '-' commands resulting in rotation \
of the virtual plotter.  See the RULES section of the manual page for \
more information on the format for legal rules. \
";

double aa0 = 90.0, da = 90.0, ds = 1, unoise = 0.0;
int width = 480, height = 480, border = 10, depth = 5, mag = 1, invert = 0;
char *rules['z' - 'a' + 1], stubs[('z' - 'a' + 1) * 2], *axiom = "f";
char *term = NULL;

int rparse(char **argv, int argc, OPTION *options, int *cargc, int opt);

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels." },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels." },
  { "-border", OPT_INT,     &border, "Approximate number of border pixels." },
  { "-depth",  OPT_INT,     &depth,  "Recursion depth to use." },
  { "-a0",     OPT_DOUBLE,  &aa0,    "Initial angle." },
  { "-da",     OPT_DOUBLE,  &da,     "Delta angle for '+' and '-' commands." },
  { "-ds",     OPT_DOUBLE,  &ds,
    "Delta step size.  Only affects '|' commands and should be less than 1." },
  { "-unoise", OPT_DOUBLE,  &unoise,
    "Amount of uniform noise to add to step angle." },
  { "-rule",   OPT_OTHER ,  rparse, 
    "Specify a production rule.  See RULES section of manual page.", },
  { "-axiom",  OPT_STRING,  &axiom,  "Starting axiom.", },
  { "-inv",    OPT_SWITCH,  &invert, "Invert all colors?" },
  { "-mag",    OPT_INT,     &mag,    "Magnification factor." },
  { "-term",   OPT_STRING,  &term,   "How to plot points." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* This is a function to parse command line options that take an L-system
   rule.  It is called whenever the -rule switch is given. */

int rparse(char **argv, int argc, OPTION *options, int *cargc, int opt)
{
  int i, len;
  char letter;

  /* No remaining arguments, so return an error. */
  if(argc == *cargc) return(1);

  /* Step through the rule string and convert upper-case letters to
   * lower-case letters.
   */
  len = strlen(argv[*cargc + 1]);
  for(i = 0; i < len; i++)
    if(argv[*cargc + 1][i] >= 'A' && argv[*cargc + 1][i] <= 'Z')
      argv[*cargc + 1][i] = argv[*cargc + 1][i] - 'A' + 'a';

  /* Check that the rule looks like "x=..." where "x" is any letter.
   * If it is not, return an error.
   */
  letter = argv[*cargc + 1][0];
  if(letter < 'a' || letter > 'z' || argv[*cargc + 1][1] != '=')
    return(1);
  
  /* Set the rule table entry to everything after the "=" character. */
  rules[letter - 'a'] = &argv[*cargc + 1][2];

  /* Increment this counter to signify that two arguments were gobbled. */
  *cargc += 2;
  return(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Some global variables that make life easier. X, y, a, s, represent the
   current (x, y) position, angle, and scale.  The min and max variables
   are to determing the plotting boundaries of the final fractal. */

double x, y, a, s;
int maxx = -10000, maxy = -10000, minx = 10000, miny = 10000;

/* This function is essentially a turtle graphics interpreter.  If 
   calcbounds is set, then the plotting bounds are computed.  Otherwise,
   the figure is plotted. */

void compute_figure(char *rule, int d, int calcbounds)
{
  double sx, sy, sa, ss;  /* Used to save states between calls. */
  int len, i, ax, ay, bx, by, num; /* Mostly temporary variables. */

  num = 0;
  len = (rule != NULL) ? strlen(rule) : 0;
  
  /* For each character in the rule ... */
  for(i = 0; i < len; i++) {

    /* If it is a letter or a '|' ... */
    if((rule[i] >= 'a' && rule[i] <= 'z') || rule[i] == '|') {

      /* For any letter, reduce the scale and recursively expand things
       * by expanding the letter's rule.  Restore the scale afterwards.
       */
      if(d > 0 && rule[i] != '|') {
        ss = s; s *= ds;
        compute_figure(rules[rule[i] - 'a'], d - 1, calcbounds);
        s = ss;
      }

      /* It is a command that requires movement of some form. */
      else if(rule[i] == 'f' || rule[i] == 'g' || rule[i] == '|') {

        /* Calculate where we should step. */
        sx = x + sin(a + unoise * random_range(-1.0, 1.0)) * s;
        sy = y + cos(a + unoise * random_range(-1.0, 1.0)) * s;

        /* We need to plot any 'f' or '|' commands ... */
        if(rule[i] == 'f' || rule[i] == '|') {
          /* Calculate the line segments two endpoints properly scaled. */
          ax = width * x + 0.5;
          ay = height * y + 0.5;
          bx = width * sx + 0.5;
          by = height * sy + 0.5;

          /* Save the boundaries if appropriate ... */
          if(calcbounds) {
            maxx = MAX(maxx, MAX(ax, bx));
            maxy = MAX(maxy, MAX(height - ay, height - by));
            minx = MIN(minx, MIN(ax, bx));
            miny = MIN(miny, MIN(height - ay, height - by));
          }
          /* ... or plot the line. */
          else
            plot_line(ax, height - ay, bx, height - by, 1);
        }
        /* Set the current position to the new position calculated. */
        x = sx; y = sy;
      }
    }
    /* If it is a number digit, then calculate the num for future use. */
    else if(rule[i] >= '0' && rule[i] <= '9')
      num = num * 10 + (rule[i] - '0');

    /* If it is a turn request ... */
    else if(rule[i] == '+' || rule[i] == '-') {
      /* ... then use any saved num and modify the angle. */
      num = (num == 0) ? 1 : num;
      a += (rule[i] == '+') ? num * da : num * -da;
      num = 0;
    }

    /* If it is a state save request ... */
    else if(rule[i] == '[') {
      /* ... save the state on the stack, ... */
      sx = x; sy = y; sa = a, ss = s;
      /* ... recursively call on the next characters, ... */
      compute_figure(rule + i + 1, d, calcbounds);
      /* ... and restore the state. */
      x = sx; y = sy; a = sa; s = ss; num = 1;

      /* Everything between the '[' and ']' characters has been interpreted,
       * so gobble up everything until we see a properly nested pair of
       * brackets.
       */
      do {
        i++;
        num = num + ((rule[i] == '[') ? 1 : (rule[i] == ']') ? -1 : 0);
      } while(num != 0);
    }

    /* If it is a state restore request, then just return.  The
     * previous state will be restored on the caller's side.
     */
    else if(rule[i] == ']') return;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{ 
  extern int plot_mag;
  extern int plot_inverse;
  double xs, ys, xo, yo;
  int i, len;

  /* Set each rule to initially be defined as itself. */
  for(i = 0; i < ('z' - 'a' + 1); i++) {
    stubs[i * 2] = i + 'a';
    stubs[i * 2 + 1] = 0;
    rules[i] = &stubs[i * 2];
  }
  get_options(argc, argv, options, help_string);
  plot_mag = mag;
  plot_inverse = invert;

  /* Convert the axiom to lower-case, just to be consistent. */
  len = strlen(axiom);
  for(i = 0; i < len; i++)
    if(axiom[i] >= 'A' && axiom[i] <= 'Z')
      axiom[i] = axiom[i] - 'A' + 'a';

  /* Convert the angular arguments from degrees to radians. */
  aa0 = aa0 * M_PI / 180.0;
  da = da * M_PI / 180.0;
  
  plot_init(width, height, 2, term);
  plot_set_all(0);

  x = 0; y = 0; a = aa0; s = 1;
  srandom(0);
  /* Calculate the bounding box size. */
  compute_figure(axiom, depth, 1);

  /* Calculate two possible scalings, but pick the one that is biggest
   * for both so that the scaling is identical.
   */
  xs = (maxx - minx) / (double) width;
  ys = (maxy - miny) / (double) height;
  if(ys > xs) xs = ys; else ys = xs;

  /* Calculate offsets to maintain a correct aspect ratio with respect
   * to the screen width, height, and desired border.
   */
  if((maxx - minx) > (maxy - miny)) {
    yo = (maxx - minx - maxy + miny) - 2 * border * ys;
    xo = -2 * border * xs;
  }
  else {
    xo = (maxy - miny - maxx + minx) - 2 * border * xs;
    yo = -2 * border * ys;
  }

  /* Set the plotting ranges appropriately. */
  plot_set_range(xs * width + minx - xo / 2 - 0.5,
                 maxx - xs * width + xo / 2 + 0.5,
                 ys * height + miny - yo / 2 - 0.5,
                 maxy - ys * height + yo / 2 + 0.5);

  x = 0; y = 0; a = aa0; s = 1;
  srandom(0);
  /* Finally, plot the fractal. */
  compute_figure(axiom, depth, 0);

  plot_finish();
  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
