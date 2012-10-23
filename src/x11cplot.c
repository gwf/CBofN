
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <math.h>

#include "misc.h"

#define NUM_LEVELS      256
#define BORDER_WIDTH    2

#define C_WHITE         0
#define C_BLACK         1

static int x_backgroundcolor = C_WHITE, x_foregroundcolor = C_BLACK;
static int x_screen, x_depth, x11plot_levels, x11plot_width, x11plot_height;
static unsigned long x_blackpixel, x_whitepixel;
static unsigned long x_colors[NUM_LEVELS];
static Colormap x_colormap;
static Display  *x_display;
static XGCValues x_gcvalues;
static Window x_window;
static GC x_gc;

extern int plot_mag;
extern int x11_force_flush;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static int create_gc(Window x_newwindow, GC *x_newgc)
{
  *x_newgc = XCreateGC(x_display, x_newwindow, (unsigned long)0, &x_gcvalues);
  if(*x_newgc == 0)                     /* unable to create a GC */
    return(0);
  XSetForeground(x_display, *x_newgc, x_whitepixel);
  XSetBackground(x_display, *x_newgc, x_blackpixel);
  x_backgroundcolor = C_BLACK;
  x_foregroundcolor = C_WHITE;
  return(1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static Window open_window(int width, int height)
{
   XSetWindowAttributes x_windowattributes;
   unsigned long        x_windowmask;
   Window               x_newwindow;

   /* Set up the attributes for the window. */

   x_windowattributes.backing_store = Always;
   x_windowattributes.event_mask = ButtonPressMask | ButtonReleaseMask;
   x_windowattributes.border_pixel  = x_whitepixel;
   x_windowattributes.background_pixel = x_blackpixel;
   x_backgroundcolor = C_BLACK;
   x_foregroundcolor = C_WHITE;
   
   x_windowmask = CWBackingStore | CWEventMask | CWBackPixel | CWBorderPixel;
   x_windowattributes.override_redirect = False;
   x_windowmask |= CWOverrideRedirect;

   /* Open a window on the display */
   x_newwindow = XCreateWindow(x_display, XRootWindow(x_display, x_screen),
                               0,0, width,height, BORDER_WIDTH, x_depth,
                               InputOutput, CopyFromParent, x_windowmask,
                               &x_windowattributes);

   XSetStandardProperties(x_display, x_newwindow, "X11plot", "X11plot",
                          None, 0, 0, 0);

   /* Create a graphic context (GC) for the window, only for the first time */
   if(create_gc(x_newwindow, &x_gc) == 0) {
     XDestroyWindow(x_display, x_newwindow);
     return((Window)0);
   }
 
   /* Ask X to place the window visibly on the screen */
   XMapWindow(x_display, x_newwindow);
   XFlush(x_display);
   return(x_newwindow);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static void int_to_hexstr(int i, char *str)
{
   sprintf(str, "%2x", i);
   for(i = 0; i < 2; i++)
     if(str[i] == ' ') str[i] = '0';
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static inline double hue_shift(double x)
{
  return(x - floor(x));
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define H2V(x) ((x)<1.0/6 ? 6*(x) : (x)<0.5 ? 1.0 : (x)<4.0/6 ? 4-6*(x) : 0.0)

/* hue should go from 0 to 1. */

static void hsb_to_rgb(double hue, int *R, int *G, int *B)
{
  double hueshift;

  hueshift = hue_shift(hue + 2.0/6);
  *R = 255 * H2V(hueshift) + 0.5;
  hueshift = hue_shift(hue);
  *G = 255 * H2V(hueshift) + 0.5;
  hueshift = hue_shift(hue - 2.0/6);
  *B = 255 * H2V(hueshift) + 0.5;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void X11plot_init(int width, int height, int levels)
{
  XColor x_hardwarecolor;
  char hexR[8], hexG[8], hexB[8], name[16], *display;
  int status, i, R, G, B;
  double hue;

  display = getenv("DISPLAY");
  if((x_display = XOpenDisplay(display)) == NULL) {
    fprintf(stderr, "X11plot_init: could not open display %s.\n",
            XDisplayName(display));
    exit(1);
  }
  x_screen     = XDefaultScreen(x_display);
  x_depth      = XDefaultDepth(x_display, x_screen);
  x_whitepixel = XWhitePixel(x_display, x_screen);
  x_blackpixel = XBlackPixel(x_display, x_screen);
  x_colormap   = XDefaultColormap(x_display, x_screen);

  /* Initialize grey map */
  if(x_depth == 1 || levels == 2) {  /* monochrome system */
    for(i = 0; i < NUM_LEVELS / 2; i++) x_colors[i] = x_blackpixel;
    for(i = NUM_LEVELS / 2; i < NUM_LEVELS; i++) x_colors[i] = x_whitepixel;
  }
  else {
    for(i = 0; i < NUM_LEVELS; i++) {
      if(i > 0) {
        hue = i / (NUM_LEVELS - 1.0);
        hsb_to_rgb(hue, &R, &G, &B);
        int_to_hexstr(R, hexR);
        int_to_hexstr(G, hexG);
        int_to_hexstr(B, hexB);
        sprintf(name,"#%s%s%s", hexR, hexG, hexB);
        status = XParseColor(x_display, x_colormap, name, &x_hardwarecolor);
        if (status != 0) {
          status = XAllocColor(x_display, x_colormap, &x_hardwarecolor);
          x_colors[i] = x_hardwarecolor.pixel;
        }
        else
          fprintf(stderr, "No such color %s\n", name);
      }
      else
        x_colors[i] = x_blackpixel;
    }
  }

  x11plot_width = width;
  x11plot_height = height;
  x11plot_levels = levels;
  x_window = open_window(width * plot_mag, height * plot_mag);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void X11plot_point(int i, int j, int val)
{
  int cval, ii, jj;

  val = (val < 0) ? 0 : (val >= x11plot_levels) ? x11plot_levels - 1: val;
  cval = ((double)val / (x11plot_levels - 1)) * (NUM_LEVELS - 1) + 0.5;
  XSetForeground(x_display, x_gc, x_colors[cval]);
  for(ii = 0; ii < plot_mag; ii++)
    for(jj = 0; jj < plot_mag; jj++)
      XDrawPoint(x_display, x_window, x_gc, i * plot_mag + ii,
                 j * plot_mag + jj);
  if(x11_force_flush) XFlush(x_display);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void X11plot_line(int i, int j, int k, int l, int val)
{
  int cval, ii;

  val = (val < 0) ? 0 : (val >= x11plot_levels) ? x11plot_levels : val;
  cval = ((double)val / x11plot_levels) * (NUM_LEVELS - 1) + 0.5;
  XSetForeground(x_display, x_gc, x_colors[cval]);
  if(plot_mag == 1)
    XDrawLine(x_display, x_window, x_gc, i, j, k, l);
  else {
    double tx, ty, t, dt;
    int len;
  
    if(i == k && j == l)
      X11plot_point(i, j, val);
    else {
      len = MAX(ABS(i - k), ABS(j - l));
      dt = 1.0 / len;
      for(ii = 0, t = 0.0; ii < len + 1; ii++, t += dt) {
        tx = t * i + (1.0 - t) * k + 0.5;
        ty = t * j + (1.0 - t) * l + 0.5;
        X11plot_point(tx, ty, val);
      }
    }
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static Bool button_click(XEvent *ev)
{
  return(1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void X11plot_finish(void)
{
  XEvent ev;

  XFlush(x_display);
  fprintf(stderr, ">> Done. Click mouse on window to end program. <<\n");
  XIfEvent(x_display, &ev, (Bool (*)()) button_click, 0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
