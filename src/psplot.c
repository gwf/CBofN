
/* Copyright (c) 1995 Gary W. Flake -- Permission granted for any use
 * provied that the author's comments are neither modified nor removed.
 * No warranty is given or implied.
 *
 * NAME
 *   psplot.c
 * PURPOSE
 *   Plot routines for postscript.
 */


#include "misc.h"

int psplot_levels = 2, psplot_width = 640, psplot_height = 480;
int oldx = -1, oldy = -1;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void psplot_init(int width, int height, int levels)
{
  char *psheader = 
    "%%!PS-Adobe-2.0 EPSF-2.0\n\
%%%%Creator: psplot\n\
%%%%DocumentFonts: \n\
%%%%BoundingBox: 0 0 %d %d\n\
%%%%EndComments\n\
/gnudict 40 dict def\n\
gnudict begin\n\
/gnulinewidth 1.000 def\n\
/M {moveto} bind def\n\
/L {lineto} bind def\n\
/V {rlineto} bind def\n\
/P { stroke [] 0 setdash\n\
  currentlinewidth 2 div sub M\n\
  0 currentlinewidth V stroke } def\n\
/dl {10 mul} def\n\
/AL { stroke gnulinewidth 2 div setlinewidth } def\n\
end\n\
%%%%EndProlog\n\
gnudict begin\n\
gsave\n\
newpath\n";

  psplot_levels = levels;
  psplot_width = width;
  psplot_height = height;
  printf(psheader, width, height);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void psplot_point(int i, int j, int val)
{
  printf("%d %d P\n", i, psplot_height - j);
  oldx = oldy = -1;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void psplot_line(int i, int j, int k, int l, int val)
{
  if(oldx != i || oldy != j)
    printf("%d %d M\n", i, psplot_height - j);
  printf("%d %d L\n", k, psplot_height - l);
  oldx = k; oldy = l;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void psplot_finish(void)
{
  printf("stroke\ngrestore\nend\nshowpage\n");
  printf("%%%%Trailer\n");
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

