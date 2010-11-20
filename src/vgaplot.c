
/* Copyright (c) 1995 Gary W. Flake -- Permission granted for any use
 * provied that the author's comments are neither modified nor removed.
 * No warranty is given or implied.
 *
 * NAME
 *   vgaplot.c
 * PURPOSE
 *   Plot routines for linux vga.
 */

#ifndef linux
#error The code only works under Linux.
#endif

#include <vga.h>
#include "misc.h"


/* Number of gray levels, width, and height. */

int vgaplot_levels = 16, vgaplot_width = 640, vgaplot_height = 480;

/* Scale factors. */

static double ax, bx, ay, by, al;

/* Keep track of the current color so that we change the color
   only when necessary. */

static int current_color = -1;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* We take whatever the requested size is and linearly map it into
   the real 640x480 range as best we can.  If the requested size is
   smaller then we obey the size but center the points.  Grey Levels
   are also linearly mapped.  The aspect ratio is always maintained. */

void vgaplot_init(int width, int height, int levels)
{
  ax = ay = MIN(479.0 / (height - 1), 639.0 / (width - 1));
  bx = (639 - ax * (width - 1)) / 2 + 0.5;
  by = (479 - ay * (height - 1)) / 2 + 0.5;
  if(levels > 16)
    al = 15.0 / (levels - 1);
  else
    al = 1.0;
  vga_setmode(G640x480x16);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void vgaplot_point(int i, int j, int val)
{
  /* Only change the color if it is different.  I am also inverting the
   * colors because I like it that way.
   */
  if(val != current_color) {
    current_color = val;
    vga_setcolor((int) (val * al));
  }

  /* Notice that we are being reckless in not checking the bounds. */
  vga_drawpixel(ax * i + bx, ay * j + by);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void vgaplot_line(int i, int j, int k, int l, int val)
{
  /* Only change the color if it is different.  I am also inverting the
   * colors because I like it that way.
   */
  if(val != current_color) {
    current_color = val;
    vga_setcolor((int) (val * al));
  }

  /* Notice that we are being reckless in not checking the bounds. */
  vga_drawline(ax * i + bx, ay * j + by, ax * k + bx, ay * l + by);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void vgaplot_finish(void)
{
  vga_getch();
  vga_setmode(TEXT);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

