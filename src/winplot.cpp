
/* Copyright (c) 1995 Gary W. Flake -- Permission granted for any use
 * provied that the author's comments are neither modified nor removed.
 * No warranty is given or implied.
 *
 * NAME
 *   winplot.c
 * PURPOSE
 *   Simple wrapper for the plot.h package.
 */

#include "misc.h"

#define COLORS 256

#include "win98.cpp"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

extern "C" main(int argc, char *argv[ ]);
void win98_init(int width, int height);

#include <io.h>

void MainThread ()
{
  int argc, fd;
  FILE *fp;
  char *p, *argv[1024];
  
  AllocConsole();
  fd = _open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), 0);
  fp = _fdopen(fd, "w");
  *stdout = *fp;
  setvbuf(stdout, NULL, _IONBF, 0);
  
  fd = _open_osfhandle((long)GetStdHandle(STD_ERROR_HANDLE), 0);
  fp = _fdopen(fd, "w");
  *stderr = *fp;
  setvbuf(stderr, NULL, _IONBF, 0);
  
  fd = _open_osfhandle((long)GetStdHandle(STD_INPUT_HANDLE), 0);
  fp = _fdopen(fd, "r");
  *stdin = *fp;
  setvbuf(stdin, NULL, _IONBF, 0);
  
  argv[0] = "CBNdemo";
  argc = 1;
  p = cmdline;
  while (*p) {
    if (*p != 0 && *p != ' ')
      argv[argc++] = p;
    while (*p != 0 && *p != ' ')
      p++;
    if (*p) {
      *p = 0;
      p++;
    }
    while (*p == ' ')
      p++;
  }
  main(argc, argv);
  while (MouseLp == FALSE)
    ;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Number of color levels, width, and height actually requested. */

int winplot_levels, winplot_width, winplot_height;

static double ax, bx, ay, by, al;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void winplot_init(int width, int height, int levels)
{
  levels = (levels > COLORS) ? COLORS : levels;
  winplot_levels = levels;
  winplot_width = width;
  winplot_height = height;
  ax = (win98width - 1.0) / (width * plot_mag - 1);
  ay = (win98height - 1.0) / (height * plot_mag - 1);
  ax = ay = MIN(ax, ay);           
  if (ax > 1) ax = ay = 1;
  bx = ((win98width - 1) - ax * (width * plot_mag - 1)) / 2 + 0.5;
  by = ((win98height - 1) - ay * (height * plot_mag - 1)) / 2 + 0.5;
  al = ((double) COLORS) / levels;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void winplot_point(int i, int j, int val)
{
  int ii;

  if(plot_mag == 1)
    Plot(ax * i + bx, ay * (winplot_height - j) + by,
         al * val, al * val, al * val);
  else {
    for(ii = 0; ii < plot_mag; ii++)
      Line(ax * (i * plot_mag + ii) + bx,
           ay * ((winplot_height - j) * plot_mag) + by,
           ax * (i * plot_mag + ii) + bx,
           ay * ((winplot_height - j + 1) * plot_mag - 1) + by,
           al * val, al * val, al * val);
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void winplot_line(int i1, int j1, int i2, int j2, int val)
{
  if(plot_mag == 1)
    Line(ax * i1 + bx, ay * (winplot_height - j1) + by,
         ax * i2 + bx, ay * (winplot_height - j2) + by,
         al * val, al * val, al * val);
  else {
    double tx, ty, t, dt;
    int len, i;
  
    if(i1 == i2 && j1 == j2)
      winplot_point(i1, j1, val);
    else {
      len = MAX(ABS(i1 - i2), ABS(j1 - j2));
      dt = 1.0 / len;
      for(i = 0, t = 0.0; i < len + 1; i++, t += dt) {
        tx = t * i1 + (1.0 - t) * i2 + 0.5;
        ty = t * j1 + (1.0 - t) * j2 + 0.5;
        winplot_point(tx, ty, val);
      }
    }
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void winplot_finish(void)
{
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static int colors[COLORS][3];

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static double hue_shift(double x)
{
  return(x - floor(x));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define H2V(x) ((x)<1.0/6?6*(x):(x)<0.5?1.0:(x)<4.0/6?4-6*(x):0.0)

/* hue should go from 0 to 1. */

static void hsb_to_rgb(double hue, int *R, int *G, int *B)
{
  double hueshift;

  hueshift = hue_shift(hue + 2.0/6);
  *R = (COLORS - 1) * H2V(hueshift) + 0.5;
  hueshift = hue_shift(hue);
  *G = (COLORS - 1) * H2V(hueshift) + 0.5;
  hueshift = hue_shift(hue - 2.0/6);
  *B = (COLORS - 1) * H2V(hueshift) + 0.5;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void Winplot_init(int width, int height, int levels)
{
  int i, R, G, B;
  double hue;

  levels = (levels > COLORS) ? COLORS : levels;
  winplot_levels = levels;
  winplot_width = width;
  winplot_height = height;
  ax = (win98width - 1.0) / (width * plot_mag - 1);
  ay = (win98height - 1.0) / (height * plot_mag - 1);
  ax = ay = MIN(ax, ay);           
  if (ax > 1) ax = ay = 1;
  bx = ((win98width - 1) - ax * (width * plot_mag - 1)) / 2 + 0.5;
  by = ((win98height - 1) - ay * (height * plot_mag - 1)) / 2 + 0.5;
  al = ((double) COLORS) / levels;
  if(levels == 2) {
    for(i = 0; i < COLORS / 2; i++)
      colors[i][0] = colors[i][1] = colors[i][2] = 0;
    for(i = COLORS / 2; i < COLORS; i++)
      colors[i][0] = colors[i][1] = colors[i][2] = 255;
  }
  else {
    for(i = 0; i < COLORS; i++) {
      if(i > 0) {
              hue = i / (COLORS - 1.0);
              hsb_to_rgb(hue, &R, &G, &B);
              colors[i][0] = R;
              colors[i][1] = G;
              colors[i][2] = B;
      }
      else if(levels >= 32)
              colors[i][0] = colors[i][1] = colors[i][2] = 0;
      else if(levels < 32 && i == 0)
              colors[i][0] = colors[i][1] = colors[i][2] = 0;
      else if(levels < 32 && i == COLORS - 1)
              colors[i][0] = colors[i][1] = colors[i][2] = COLORS - 1;
    }
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void Winplot_point(int i, int j, int val)
{
  int ii;

  if(plot_mag == 1)
    Plot(ax * i + bx, ay * (winplot_height - j) + by,
         colors[(int)(al * val)][0],
         colors[(int)(al * val)][1],
         colors[(int)(al * val)][2]);
  else {
    for(ii = 0; ii < plot_mag; ii++)
      Line(ax * (i * plot_mag + ii) + bx,
           ay * ((winplot_height - j) * plot_mag) + by,
           ax * (i * plot_mag + ii) + bx,
           ay * ((winplot_height - j + 1) * plot_mag - 1) + by,
           colors[(int)(al * val)][0],
           colors[(int)(al * val)][1],
           colors[(int)(al * val)][2]);
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void Winplot_line(int i1, int j1, int i2, int j2, int val)
{
  if(plot_mag == 1)
    Line(ax * i1 + bx, ay * (winplot_height - j1) + by,
         ax * i2 + bx, ay * (winplot_height - j2) + by,
         colors[(int)(al * val)][0],
         colors[(int)(al * val)][1],
         colors[(int)(al * val)][2]);
  else {
    double tx, ty, t, dt;
    int len, i;
  
    if(i1 == i2 && j1 == j2)
      Winplot_point(i1, j1, val);
    else {
      len = MAX(ABS(i1 - i2), ABS(j1 - j2));
      dt = 1.0 / len;
      for(i = 0, t = 0.0; i < len + 1; i++, t += dt) {
        tx = t * i1 + (1.0 - t) * i2 + 0.5;
        ty = t * j1 + (1.0 - t) * j2 + 0.5;
        Winplot_point(tx, ty, val);
      }
    }
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void Winplot_finish(void)
{
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


