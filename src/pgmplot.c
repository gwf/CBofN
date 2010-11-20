
/* Copyright (c) 1995 Gary W. Flake -- Permission granted for any use
 * provied that the author's comments are neither modified nor removed.
 * No warranty is given or implied.
 *
 * NAME
 *   pgmplot.c
 * PURPOSE
 *   Plot routines to emit pgm files to stdout.  Data is stored in a
 *   buffer and is not emmited until the pgm_pgmplot_finish() call.
 */

#include "misc.h"


/* Space to store the data. */

static unsigned char **pgmplot_data;


/* Number of gray levels, width, and height. */

int pgmplot_levels, pgmplot_width, pgmplot_height;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void pgmplot_init(int width, int height, int levels)
{
  int i, j;

  levels = (levels > 256) ? 256 : levels;
  printf("P5\n");
  printf("%d %d\n", width, height);
  printf("%d\n", levels - 1);

  pgmplot_data = malloc(width * sizeof(unsigned char *));
  for(i = 0; i < width; i++) {
    pgmplot_data[i] = malloc(height * sizeof(unsigned char));
    for(j = 0; j < height; j++)
      pgmplot_data[i][j] = 0;
  }
  pgmplot_levels = levels;
  pgmplot_width = width;
  pgmplot_height = height;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void pgmplot_point(int i, int j, int val)
{
  if(i < 0 || i >= pgmplot_width || j < 0 || j > pgmplot_height)
    return;
  pgmplot_data[i][j] = val;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void pgmplot_finish(void)
{
  int i, j;

/*
  for(i = 0; i < pgmplot_width; i++)
    fwrite(pgmplot_data[i], sizeof(unsigned char), pgmplot_height, stdout);
*/

  for(j = 0; j < pgmplot_height; j++)
    for(i = 0; i < pgmplot_width; i++)
      fputc(pgmplot_data[i][j], stdout);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */



