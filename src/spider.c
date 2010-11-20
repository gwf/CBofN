
/* Copyright (c) 1995 Gary W. Flake -- Permission granted for any use
   provided that this comment remains unaltered. No warranty is given */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "misc.h"


int width = 640, height = 480, maxit = 160;
int levels = 16, rev = 0, box = 0, idiv = 1;
double ulx = -2.4, uly = 1.4, lly = -1.4, bulx, buly, blly, bail = 16.0;
char *term = NULL;

char help_string[] = "\
\n";

OPTION options[] = {
  { "-width",  OPT_INT,     &width,  "width of the plot in pixels"    },
  { "-height", OPT_INT,     &height, "height of the plot in pixels"   },
  { "-maxit",  OPT_INT,     &maxit,  "maximum number of iterations"  },
  { "-levels", OPT_INT,     &levels, "number of plot (gray) levels"   },
  { "-bail",   OPT_DOUBLE,  &bail,   "value of |z| to end iteration"  },
  { "-ulx",    OPT_DOUBLE,  &ulx,    "upper-left corner x-coordinate" },
  { "-uly",    OPT_DOUBLE,  &uly,    "upper-left corner y-coordinate" },
  { "-lly",    OPT_DOUBLE,  &lly,    "lower-left corner y-coordinate" },
  { "-box",    OPT_INT,     &box,    "line width for a box"           },
  { "-bulx",   OPT_DOUBLE,  &bulx,   "box's upper-left x-coordinate"  },
  { "-buly",   OPT_DOUBLE,  &buly,   "box's upper-left y-coordinate"  },
  { "-blly",   OPT_DOUBLE,  &blly,   "box's lower-left y-coordinate"  },
  { "-idiv",   OPT_INT,     &idiv,   "iteration divisor"              },
  { "-rev",    OPT_SWITCH,  &rev,    "reverse all colors but first"   },
  { "-term",   OPT_STRING,  &term,   "how to plot points"             },
  { NULL,      OPT_NULL,    NULL,    NULL                             }
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int main(int argc, char **argv)
{
  int i, j, k;
  double a, b, inc, binc, u, v, w, x, y, ca, cb;

  get_options(argc, argv, options, help_string);
  plot_init(width, height, levels, term);

/*
        c(0) = z(0) = pixel;
        z(n+1) = z(n)^2 + c(n);
        c(n+1) = c(n)/2 + z(n+1);
*/

  inc = (uly - lly) / (height - 1);
  for(j = 0, b = uly; j < height; j++, b -= inc) {
    for(i = 0, a = ulx; i < width; i++, a += inc) {
      x = ca = a;
      y = cb = b;
      for(k = 1; k <= maxit; k++) {
        u = x * x;
        v = y * y;
        w = 2.0 * x * y;
        x = u - v + ca;
        y = w + cb;
        ca = 0.5 * ca + x;
        cb = 0.5 * cb + y;
        if(u + v > bail) {
          if(rev)
            plot_point(i, j, -((k / idiv + (k % idiv) * (levels / idiv)) %
                               levels) + levels - 1);
          else
            plot_point(i, j, (k / idiv + (k % idiv) *
                              (levels / idiv)) % levels);
          break;
        }
      }
  } }

  if(box > 0) {
    binc = (buly - blly) / (height - 1);
    plot_box((bulx - ulx) / inc, (uly - buly) / inc,
             (bulx + width * binc - ulx) / inc,
             (uly + height * binc - buly) / inc, box);
  }

  plot_finish();

  exit(0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
