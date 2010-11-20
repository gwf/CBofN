
/* Copyright (c) 1995 Gary W. Flake -- Permission granted for any use
 * provied that the author's comments are neither modified nor removed.
 * No warranty is given or implied.
 *
 * NAME
 *   rawplot.c
 * PURPOSE
 *   Plot routines to emit raw files to stdout.  Data is stored in a
 *   buffer and is not emmited until the raw_rawplot_finish() call.
 */

#include "misc.h"


/* Space to store the data. */

static unsigned char **rawplot_data;


/* Number of gray levels, width, and height. */

int rawplot_levels, rawplot_width, rawplot_height;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void rawplot_init(int width, int height, int levels)
{
  int i, j;

  levels = (levels > 256) ? 256 : levels;

  rawplot_data = malloc(width * sizeof(unsigned char *));
  for(i = 0; i < width; i++) {
    rawplot_data[i] = malloc(height * sizeof(unsigned char));
    for(j = 0; j < height; j++)
      rawplot_data[i][j] = 0;
  }
  rawplot_levels = levels;
  rawplot_width = width;
  rawplot_height = height;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void rawplot_point(int i, int j, int val)
{
  if(i < 0 || i >= rawplot_width || j < 0 || j > rawplot_height)
    return;
  rawplot_data[i][j] = val;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void rawplot_finish(void)
{
  int i, j;

  for(j = 0; j < rawplot_height; j++)
    for(i = 0; i < rawplot_width; i++)
      printf("%d %d %d\n", i, j, rawplot_data[i][j]);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
