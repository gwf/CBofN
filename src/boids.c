
/* NAME
 *   boids - simulate a flock of android birds from Brooklyn
 * NOTES
 *   As noted below, this program makes heavy use of some global
 *   variables.
 *   
 *   The compute_new_heading() function is somewhat monolithic,
 *   but it is well documented at least.  Change it with some care.
 * RULES
 *   All of the rules have a weight option and a radius option.
 *   The radius option specifies how close a boid need to be to
 *   another in order for the rule to be acted upon.  The weight
 *   is used when combining all of the rules actions into a single
 *   new velocity vector.
 *   
 *   The four rules can be simply described as follows:
 *   
 *      Centering: move towards the center of any boids in my
 *                 viewing area.
 *   
 *      Copying: attempt to move in the average direction of
 *               that all boids that can be seen are moving
 *               in.
 *   
 *      Avoidance: ``Please don't stand so close to me.''
                  Move away from any close flyers.
 *   
 *      Visual: move in such a way that the bonehead
 *              obstructing your view no longer interferes.
 *   
 *   The four rules are then normalized and added together to make
 *   the next velocity vector of the boid.  All radii in the rules
 *   are in terms of pixels.
 *   
 *   You may wish to try turning on and off different combinations
 *   of the rules to see how the boids' behaviors change.  For example,
 *   if you turn off the avoidance rule, increase the centering radius
 *   and weight, and increase the viewing angle to nearly 360 degrees,
 *   then the boids will behave like a pack of wolves fighting over
 *   the center.  Other changes can yield similar surprises.
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

int width = 640, height = 480, num = 20, len = 20, mag = 1;
int seed = 0, invert = 0, steps = 100000000, psdump = 0;

double angle = 270.0, vangle = 90, minv = 0.5, ddt = 0.95, dt = 3.0;
double rcopy = 80, rcent = 30, rviso = 40, rvoid = 15;
double wcopy = 0.2, wcent = 0.4, wviso = 0.8, wvoid = 1.0, wrand = 0.0;

char *term = NULL;

char help_string[] = "\
Simulate a flock of boids according to rules that determine their \
individual behaviors as well as the ``physics'' of their universe. \
A boid greedily attempts to apply four rules with respect to its \
neighbors: it wants to fly in the same direction, be in the center \
of the local cluster of boids, avoid collisions with boids too close, \
and maintain a clear view ahead by skirting around others that block \
its view.  Changing these rules can make the boids behave like birds, \
gnats, bees, fish, or magnetic particles.  See the RULES section of \
the manual pages for more details.\
";

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "Width of the plot in pixels." },
  { "-height", OPT_INT,     &height, "Height of the plot in pixels." },
  { "-num",    OPT_INT,     &num,    "Number of boids." },
  { "-steps",  OPT_INT,     &steps,  "Number of simulated steps." },
  { "-seed",   OPT_INT,     &seed,   "Random seed for initial state." },
  { "-angle",  OPT_DOUBLE,  &angle,  "Number of viewing degrees." },
  { "-vangle", OPT_DOUBLE,  &vangle, "Visual avoidance angle." },
  { "-rcopy",  OPT_DOUBLE,  &rcopy,  "Radius for copy vector." },
  { "-rcent",  OPT_DOUBLE,  &rcent,  "Radius for centroid vector." },
  { "-rvoid",  OPT_DOUBLE,  &rvoid,  "Radius for avoidance vector." },
  { "-rviso",  OPT_DOUBLE,  &rviso,  "Radius for visual avoidance vector." },
  { "-wcopy",  OPT_DOUBLE,  &wcopy,  "Weight for copy vector." },
  { "-wcent",  OPT_DOUBLE,  &wcent,  "Weight for centroid vector." },
  { "-wvoid",  OPT_DOUBLE,  &wvoid,  "Weight for avoidance vector." },
  { "-wviso",  OPT_DOUBLE,  &wviso,  "Weight for visual avoidance vector." },
  { "-wrand",  OPT_DOUBLE,  &wrand,  "Weight for random vector." },
  { "-dt",     OPT_DOUBLE,  &dt,     "Time-step increment." },
  { "-ddt",    OPT_DOUBLE,  &ddt,    "Momentum factor (0 < ddt < 1)." },
  { "-minv",   OPT_DOUBLE,  &minv,   "Minimum velocity." },
  { "-len",    OPT_INT,     &len,    "Tail length." },
  { "-psdump", OPT_SWITCH,  &psdump, "Dump PS at the very end?" },
  { "-inv",    OPT_SWITCH,  &invert, "Invert all colors?" },
  { "-mag",    OPT_INT,     &mag,    "Magnification factor." },
  { "-term",   OPT_STRING,  &term,   "How to plot points." },
  { NULL,      OPT_NULL,    NULL,    NULL }
};


/* These are global to avoid passing them around all of time.  They
   represent the boids (x, y) positions, velocity vectors, and new
   velocity vectors. */

double *xp, *yp, *xv, *yv, *xnv, *ynv;


/* Some handy macros ... */

#define LEN(x, y) sqrt(SQR(x) + SQR(y))
#define DIST(x1, y1, x2, y2) LEN(((x1)-(x2)),((y1)-(y2)))
#define DOT(x1, y1, x2, y2) ((x1)*(x2)+(y1)*(y2))

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Destructively normalize a vector. */

void norm(double *x, double *y)
{
  double len;

  len = LEN(*x, *y);
  if(len != 0.0) {
    *x /= len;
    *y /= len;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Compute the heading for a particular boid based on its current
   environment. */

void compute_new_heading(int which)
{
  int i, j, k, numcent = 0;
  double xa, ya, xb, yb, xc, yc, xd, yd, xt, yt;
  double mindist, mx = 0, my = 0, d;
  double cosangle, cosvangle, costemp;
  double xtemp, ytemp, maxr, u, v;

  /* This is the maximum distance in which any rule is activated. */
  maxr = MAX(rviso, MAX(rcopy, MAX(rcent, rvoid)));

  /* These two values are used to see if a boid can "see" another
   * boid in various ways.
   */
  cosangle = cos(angle / 2);
  cosvangle = cos(vangle / 2);

  /* These are the accumulated change vectors for the four rules. */
  xa = ya = xb = yb = xc = yc = xd = yd = 0;

  /* For every boid... */
  for(i = 0; i < num; i++) {

    /* Don't include self for computing new heading. */
    if(i == which) continue;

    /* Since we want boids to "see" each other around the borders of
     * the screen, we need to check if a boid on the left edge is
     * actually "close" to a boid on the right edge, etc.  We do this
     * by searching over nine relative displacements of boid(i) and
     * pick the one that is closest to boid(which).  These coordinates
     * are then used for all remaining calculations.
     */
    mindist = 10e10;
    for(j = -width; j <= width; j += width)
      for(k = -height; k <= height; k += height) {
        d = DIST(xp[i] + j, yp[i] + k, xp[which], yp[which]);
        if(d < mindist) {
          mindist = d;
          mx = xp[i] + j;
          my = yp[i] + k;
        }
      }

    /* If that distance is farther than any of the rule radii,
     * then skip.
     */
    if(mindist > maxr) continue;

    /* Make a vector from boid(which) to boid(i). */
    xtemp = mx - xp[which]; ytemp = my - yp[which];
    
    /* Calculate the cosine of the velocity vector of boid(which)
     * and the vector from boid(which) to boid(i).
     */
    costemp = DOT(xv[which], yv[which], xtemp, ytemp) /
      (LEN(xv[which], yv[which]) * LEN(xtemp, ytemp));

    /* If this cosine is less than the cosine of one half
     * of the boid's eyesight, i.e., boid(which) cannot see
     * boid(i), then skip.
     */
    if(costemp < cosangle) continue;

    /* If the distance between the two boids is within the radius
     * of the centering rule, but outside of the radius of the
     * avoidance rule, then attempt to center in on boid(i).
     */
    if(mindist <= rcent && mindist > rvoid) {
      xa += mx - xp[which];
      ya += my - yp[which]; 
      numcent++;
    }

    /* If we are close enough to copy, but far enough to avoid,
     * then copy boid(i)'s velocity.
     */
    if(mindist <= rcopy && mindist > rvoid) {
      xb += xv[i];
      yb += yv[i];
    }

    /* If we are within collision range, then try to avoid boid(i). */
    if(mindist <= rvoid) {

      /* Calculate the vector which moves boid(which) away from boid(i). */
      xtemp = xp[which] - mx;
      ytemp = yp[which] - my;

      /* Make the length of the avoidance vector inversely proportional
       * to the distance between the two boids.
       */
      d = 1 / LEN(xtemp, ytemp);
      xtemp *= d; ytemp *= d; 
      xc += xtemp; yc += ytemp;
    }

    /* If boid(i) is within rviso distance and the angle between this boid's
     * velocity vector and the boid(i)'s position relative to this boid is
     * less than vangle, then try to move so that vision is restored.
     */
    if(mindist <= rviso && cosvangle < costemp) {

      /* Calculate the vector which moves boid(which) away from boid(i). */
      xtemp = xp[which] - mx;
      ytemp = yp[which] - my;

      /* Calculate another vector that is orthogonal to the previous,
       * But try to make it in the same general direction of boid(which)'s
       * direction of movement.
       */
      u = v = 0;
      if(xtemp != 0 && ytemp != 0) {
        u = sqrt(SQR(ytemp / xtemp) / (1 + SQR(ytemp / xtemp)));
        v = -xtemp * u / ytemp;
      }
      else if(xtemp != 0)
        u = 1;
      else if(ytemp != 0)
        v = 1;
      if((xv[which] * u + yv[which] * v) < 0) {
        u = -u; v = -v;
      }

      /* Add the vector that moves away from boid(i). */
      u = xp[which] - mx + u;
      v = yp[which] - my + v;

      /* Make this vector's length inversely proportional to the
       * distance between the two boids.
       */
      d = LEN(xtemp, ytemp);
      if(d != 0) {
        u /= d; v /= d; 
      }
      xd += u; yd += v;
    }
  }

  /* Avoid centering on only one other boid;
   * it makes you look aggressive!
   */
  if(numcent < 2)
    xa = ya = 0;
  
  /* Normalize all big vectors. */
  if(LEN(xa, ya) > 1.0) norm(&xa, &ya);
  if(LEN(xb, yb) > 1.0) norm(&xb, &yb);
  if(LEN(xc, yc) > 1.0) norm(&xc, &yc);
  if(LEN(xd, yd) > 1.0) norm(&xd, &yd);

  /* Compute the composite trajectory based on all of the rules. */
  xt = xa * wcent + xb * wcopy + xc * wvoid + xd * wviso;
  yt = ya * wcent + yb * wcopy + yc * wvoid + yd * wviso;

  /* Optionally add some noise. */
  if(wrand > 0) {
    xt += random_range(-1, 1) * wrand;
    yt += random_range(-1, 1) * wrand;
  }

  /* Update the velocity and renormalize if it is too small. */
  xnv[which] = xv[which] * ddt + xt * (1 - ddt);
  ynv[which] = yv[which] * ddt + yt * (1 - ddt);
  d = LEN(xnv[which], ynv[which]);
  if(d < minv) {
    xnv[which] *= minv / d;
    ynv[which] *= minv / d;
  }    
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void draw_boid(int which, int color)
{
  double x1, x2, x3, y1, y2, y3, a, t;

  /* Plot a line in the direction that it is heading. */
  x3 = xv[which]; y3 = yv[which];
  norm(&x3, &y3);
  x1 = xp[which]; y1 = yp[which];
  x2 = x1 - x3 * len;
  y2 = y1 - y3 * len;
  plot_line(x1, y1, x2, y2, color);

  /* Plot the head of the boid, with the angle of the arrow head
   * indicating its viewing angle.
   */
  t = (x1 - x2) / len;
  t = (t < -1) ? -1 : (t > 1) ? 1 : t;
  a = acos(t);
  a = (y1 - y2) < 0 ? -a : a;

  /* This is for the right portion of the head. */
  x3 = x1 + cos(a + angle / 2) * len / 3.0;
  y3 = y1 + sin(a + angle / 2) * len / 3.0;
  plot_line(x1, y1, x3, y3, color);

  /* This is for the left portion of the head. */
  x3 = x1 + cos(a - angle / 2) * len / 3.0;
  y3 = y1 + sin(a - angle / 2) * len / 3.0;
  plot_line(x1, y1, x3, y3, color);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  extern int plot_mag;
  extern int plot_inverse;
  int i, j;

  get_options(argc, argv, options, help_string);
  if(!psdump) {
    plot_mag = mag;
    plot_inverse = invert;
    plot_init(width, height, 2, term);
    plot_set_all(0);
  }
  srandom(seed);

  /* Convert angles to radians. */
  angle = angle * M_PI / 180.0;
  vangle = vangle * M_PI / 180.0;

  /* Make space for the positions, velocities, and new velocities. */
  xp  = xmalloc(sizeof(double) * num);
  yp  = xmalloc(sizeof(double) * num);
  xv  = xmalloc(sizeof(double) * num);
  yv  = xmalloc(sizeof(double) * num);
  xnv = xmalloc(sizeof(double) * num);
  ynv = xmalloc(sizeof(double) * num);

  /* Set to random initial conditions. */
  for(i = 0; i < num; i++) {
    xp[i] = random() % width;
    yp[i] = random() % height;
    xv[i] = random_range(-1.0, 1.0);
    yv[i] = random_range(-1.0, 1.0);
    norm(&xv[i], &yv[i]);
  }

  /* For each time step... */
  for(i = 0; i < steps; i++) {

    /* For each boid, compute its new heading. */
    for(j = 0; j < num; j++)
      compute_new_heading(j);

    /* For each boid again... */
    for(j = 0; j < num; j++) {

      /* Undraw the boid. */
      if(!psdump) draw_boid(j, 0);

      /* Update the velocity and position. */
      xv[j] = xnv[j];
      yv[j] = ynv[j];
      xp[j] += xv[j] * dt;
      yp[j] += yv[j] * dt;

      /* Wrap around the screen coordinates. */
      if(xp[j] < 0) xp[j] += width;
      else if(xp[j] >= width) xp[j] -= width;
      if(yp[j] < 0) yp[j] += height;
      else if(yp[j] >= height - 1) yp[j] -= height;

      /* Redraw the boid. */
      if(!psdump) draw_boid(j, 1);
    }
  }
  if(!psdump) plot_finish();

  /* If we want a PS dump of the final configuration, do it. */
  if(psdump) {
    plot_inverse = 0;
    plot_init(width, height, 2, "ps");
    for(i = 0; i < num; i++) {
      draw_boid(i, 0);
    }
    plot_finish();
  }

  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

