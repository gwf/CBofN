
#include <stddef.h>
#include <string.h>
#include <math.h>
#include "misc.h"

#define PLOTPROTOS(base) \
  extern void base ## plot_init(int width, int height, int levels); \
  extern void base ## plot_point(int x, int y, int val); \
  extern void base ## plot_line(int x1, int y1, int x2, int y2, int val); \
  extern void base ## plot_finish(void); \

#ifdef  __cplusplus
extern "C" {
#endif
PLOTPROTOS(pgm)
PLOTPROTOS(raw)
PLOTPROTOS(ps)
#ifdef  __cplusplus
}
#endif

#ifdef WIN32
PLOTPROTOS(win)
PLOTPROTOS(Win)
static char *term_default = "Win";
#endif

#ifdef PLOTX11
static char *term_default = "X11";
PLOTPROTOS(x11)
PLOTPROTOS(X11)
#endif

#ifdef PLOTVGA
PLOTPROTOS(vga)
#ifndef PLOTX11
static char *term_default = "vga";
#endif
#endif

/*MRM begin*/
#if __dest_os == __mac_os
PLOTPROTOS(mac)
PLOTPROTOS(Mac)
static char *term_default = "Mac";
#endif
/*MRM end*/

#ifndef PLOTX11
#ifndef PLOTVGA
#ifndef WIN32
/*MRM begin*/
#if __dest_os != __mac_os
/*MRM end*/
static char *term_default = "pgm";
/*MRM begin*/
#endif
/*MRM end*/
#endif
#endif
#endif


static void (*_plot_init)(int width, int height, int levels);
static void (*_plot_point)(int x, int y, int val);
static void (*_plot_line)(int x1, int y1, int x2, int y2, int val);
static void (*_plot_finish)(void);
static void plot_line_internal(int x1, int y1, int x2, int y2, int val);


static void none_init(int width, int height, int levels);
static void none_point(int i, int j, int val);
static void none_line(int ax, int ay, int bx, int by, int val);
static void none_finish(void);

int plot_levels, plot_width, plot_height, plot_inverse = 0, plot_mag = 1;
double plot_xmin, plot_xmax, plot_ymin, plot_ymax;

#define NORMX(x) \
  ((int) (plot_xmax == plot_xmin) ? plot_xmin : \
   ((((x) - plot_xmin) / (plot_xmax - plot_xmin)) * plot_width))

#define NORMY(y) \
  ((int) (plot_ymax == plot_ymin) ? plot_ymin : \
   (((plot_ymin - (y)) / (plot_ymax - plot_ymin) + 1.0) * plot_height))

#define LIMX(x) (((x) == plot_width) ? ((x) - 1) : (x))
#define LIMY(y) (((y) == plot_height) ? ((y) - 1) : (y))

#define COLOR(val) (plot_inverse ? ((plot_levels - 1) - (val)) : (val))

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void plot_init(int width, int height, int levels, char *term)
{
  if(!term) term = term_default;

  if(0) ;
#ifdef PLOTX11
  else if(strcmp(term, "x11") == 0) {
    _plot_init = x11plot_init;
    _plot_point = x11plot_point;
    _plot_line = x11plot_line;
    _plot_finish = x11plot_finish;
  }
  else if(strcmp(term, "X11") == 0) {
    _plot_init = X11plot_init;
    _plot_point = X11plot_point;
    _plot_line = X11plot_line;
    _plot_finish = X11plot_finish;
  }
#endif
#ifdef WIN32
  else if(strcmp(term, "win") == 0) {
    _plot_init = winplot_init;
    _plot_point = winplot_point;
    _plot_line = winplot_line;
    _plot_finish = winplot_finish;
  }
  else if(strcmp(term, "Win") == 0) {
    _plot_init = Winplot_init;
    _plot_point = Winplot_point;
    _plot_line = Winplot_line;
    _plot_finish = Winplot_finish;
  }
#endif
/*MRM begin*/
#if __dest_os == __mac_os
  else if(strcmp(term, "mac") == 0) {
    _plot_init = macplot_init;
    _plot_point = Macplot_point;
    _plot_line = Macplot_line;
    _plot_finish = Macplot_finish;
  }
  else if(strcmp(term, "Mac") == 0) {
    _plot_init = Macplot_init;
    _plot_point = Macplot_point;
    _plot_line = Macplot_line;
    _plot_finish = Macplot_finish;
  }
#endif
/*MRM end*/
#ifdef PLOTVGA
  else if(strcmp(term, "vga") == 0) {
    _plot_init = vgaplot_init;
    _plot_point = vgaplot_point;
    _plot_line = vgaplot_line;
    _plot_finish = vgaplot_finish;
  }
#endif
  else if(strcmp(term, "ps") == 0) {
    _plot_init = psplot_init;
    _plot_point = psplot_point;
    _plot_line = psplot_line;
    _plot_finish = psplot_finish;
  }
  else if(strcmp(term, "pgm") == 0) {
    _plot_init = pgmplot_init;
    _plot_point = pgmplot_point;
    _plot_line = plot_line_internal;
    _plot_finish = pgmplot_finish;
  }
  else if(strcmp(term, "raw") == 0) {
    _plot_init = rawplot_init;
    _plot_point = rawplot_point;
    _plot_line = plot_line_internal;
    _plot_finish = rawplot_finish;
  }
  else if(strcmp(term, "none") == 0) {
    _plot_init = none_init;
    _plot_point = none_point;
    _plot_line = none_line;
    _plot_finish = none_finish;
  }
  else {
    plot_init(width, height, levels, term_default);
    return;
  }
  _plot_init(width, height, levels);
  plot_levels = levels;
  plot_width = width;
  plot_height = height;
  plot_xmin = 0.0;
  plot_xmax = width - 1;
  plot_ymin = height - 1;
  plot_ymax = 0.0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void plot_set_range(double xmin, double xmax, double ymin, double ymax)
{
  plot_xmin = xmin;
  plot_xmax = xmax;
  plot_ymin = ymin;
  plot_ymax = ymax;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void plot_set_all(int val)
{
  int i;

  for(i = 0; i < plot_width; i++)
    _plot_line(i, 0, i, plot_height - 1, COLOR(val));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void plot_point(double x, double y, int val)
{
  int xi, yi;
  
  xi = NORMX(x); xi = LIMX(xi);
  yi = NORMY(y); yi = LIMY(yi);
  if(!(xi < 0 || xi >= plot_width || yi < 0 || yi >= plot_height))
    _plot_point(xi, yi, COLOR(val));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void plot_line(double x1, double y1, double x2, double y2, int val)
{
  int ax, ay, bx, by;
  
  ax = NORMX(x1); ax = LIMX(ax);
  ay = NORMY(y1); ay = LIMY(ay);
  bx = NORMX(x2); bx = LIMX(bx);
  by = NORMY(y2); by = LIMY(by);
  _plot_line(ax, ay, bx, by, COLOR(val));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void plot_finish(void)
{
  _plot_finish();
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void plot_line_internal(int ax, int ay, int bx, int by, int val)
{
  double tx, ty, t, dt;
  int len, i;
  
  if(ax == bx && ay == by)
    _plot_point(ax, ay, val);
  else {
    len = MAX(ABS(ax - bx), ABS(ay - by));
    dt = 1.0 / len;
    for(i = 0, t = 0.0; i < len + 1; i++, t += dt) {
      tx = t * ax + (1.0 - t) * bx + 0.5;
      ty = t * ay + (1.0 - t) * by + 0.5;
      _plot_point(tx, ty, val);
    }
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void plot_box(double ulx, double uly, double lrx, double lry, int lwidth)
{
  int i, ulxi, ulyi, lrxi, lryi;

  ulxi = NORMX(ulx); ulxi = LIMX(ulxi);
  ulyi = NORMY(uly); ulyi = LIMY(ulyi);
  lrxi = NORMX(lrx); lrxi = LIMX(lrxi);
  lryi = NORMY(lry); lryi = LIMY(lryi);

  _plot_line(ulxi, ulyi, lrxi, ulyi, COLOR(plot_levels - 1));
  _plot_line(lrxi, ulyi, lrxi, lryi, COLOR(plot_levels - 1));
  _plot_line(lrxi, lryi, ulxi, lryi, COLOR(plot_levels - 1));
  _plot_line(ulxi, lryi, ulxi, ulyi, COLOR(plot_levels - 1));

  for(i = 1; i < lwidth + 1; i++) {
    _plot_line(ulxi - i, ulyi - i, lrxi + i, ulyi - i, COLOR(0));
    _plot_line(lrxi + i, ulyi - i, lrxi + i, lryi + i, COLOR(0));
    _plot_line(lrxi + i, lryi + i, ulxi - i, lryi + i, COLOR(0));
    _plot_line(ulxi - i, lryi + i, ulxi - i, ulyi - i, COLOR(0));
  }

  _plot_line(ulxi - i, ulyi - i, lrxi + i, ulyi - i, COLOR(plot_levels - 1));
  _plot_line(lrxi + i, ulyi - i, lrxi + i, lryi + i, COLOR(plot_levels - 1));
  _plot_line(lrxi + i, lryi + i, ulxi - i, lryi + i, COLOR(plot_levels - 1));
  _plot_line(ulxi - i, lryi + i, ulxi - i, ulyi - i, COLOR(plot_levels - 1));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void none_init(int width, int height, int levels)
{
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void none_point(int i, int j, int val)
{
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void none_line(int ax, int ay, int bx, int by, int val)
{
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void none_finish(void)
{
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
